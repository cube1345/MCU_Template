/**
 * @file    stm32f4_uart.c
 * @brief   STM32F4 HAL UART 固定接收能力实现。
 * @author  weixin
 *
 * uart 是应用层唯一可见的基础对象。阻塞读取接口内部使用中断环形缓冲
 * 防止任务抢占造成丢字节；应用层不接触任何 STM32F4 具体类型。
 *
 * HAL 句柄、引脚复用表、GPIO/USART 时钟和 NVIC 都封装在本文件内。Board
 * 层只选择串口控制器及 TX/RX 引脚，上层只能看到 struct uart。
 */
#include "stm32f4_uart.h"

#include "container_of.h"
#include "stm32f4.h"
#include "result.h"

#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stddef.h>

#define STM32F4_UART_MODE_NONE INT32_C(0)
#define STM32F4_UART_MODE_BLOCKING INT32_C(1)
#define STM32F4_UART_MODE_INTERRUPT INT32_C(2)

#define STM32F4_UART_SIGNAL_TX INT32_C(0)
#define STM32F4_UART_SIGNAL_RX INT32_C(1)

/* 阻塞接口内部使用的中断接收环形缓冲区容量。 */
#define STM32F4_UART_RX_RING_CAPACITY UINT16_C(128)
/* HAL UART 中与接收链相关的错误位。 */
#define STM32F4_UART_RX_ERROR_MASK \
  (HAL_UART_ERROR_PE | HAL_UART_ERROR_NE | \
   HAL_UART_ERROR_FE | HAL_UART_ERROR_ORE)

/*
 * 一条记录描述一个 UART 信号能够复用到哪个 MCU 引脚。
 * Board 层只保存稳定的整数引脚编号；GPIO 端口、HAL 引脚掩码和 AF 编号
 * 均属于 STM32F4 后端细节，不向通用 UART 父类泄漏。
 */
struct stm32f4_uart_pin_map {
  int32_t uart_id;
  int32_t signal;
  int32_t pin_id;
  GPIO_TypeDef *port;
  uint16_t pin_mask;
  uint32_t alternate;
};

struct stm32f4_uart_blocking {
  struct uart base;
  UART_HandleTypeDef handle;
  /* USART6 发送专用 DMA 句柄，USART1 对象不使用该成员。 */
  DMA_HandleTypeDef tx_dma;
  const struct stm32f4_uart_config *config;
  /* USART6 DMA 发送尚未完成时置位，由发送完成或错误回调清除。 */
  volatile bool tx_dma_active;
  /* USART6 DMA 或 UART 发送异常时置位，由任务侧发送函数读取。 */
  volatile bool tx_dma_error;
  /* 中断每次接收一个字节时使用的临时存储。 */
  uint8_t rx_byte;
  /* 中断生产、任务消费的接收环形缓冲区。 */
  uint8_t rx_ring[STM32F4_UART_RX_RING_CAPACITY];
  /* 中断写入位置，只由接收完成回调推进。 */
  volatile uint16_t rx_head;
  /* 任务读取位置，只由阻塞读取函数推进。 */
  volatile uint16_t rx_tail;
  /* 接收错误或缓冲区溢出后置位，由任务读取并清除。 */
  volatile bool rx_error;
  /* 文本协议错误后丢弃当前残缺命令，直到收到行结束符。 */
  volatile bool rx_discarding;
  /* true 表示连续中断接收链已经启动。 */
  volatile bool rx_irq_active;
};

struct stm32f4_uart_interrupt {
  struct uart base;
  UART_HandleTypeDef handle;
  const struct stm32f4_uart_config *config;
  uart_interrupt_callback_t rx_callback;
  void *rx_context;
  uint8_t *rx_data;
  uint16_t rx_size;
  /* 任务启动、ISR 完成的跨上下文状态，true 表示存在一笔挂起接收。 */
  volatile bool rx_pending;
};

static struct stm32f4_uart_blocking g_uart1_blocking;
static struct stm32f4_uart_blocking g_uart6_blocking;
static struct stm32f4_uart_interrupt g_uart1_interrupt;
static struct stm32f4_uart_interrupt g_uart3_interrupt;
static int32_t g_uart1_mode;
static int32_t g_uart3_mode;
static int32_t g_uart6_mode;

/* STM32F407 USART1 的合法 TX/RX 复用引脚，数据来自芯片复用功能表。 */
static const struct stm32f4_uart_pin_map g_uart_pin_maps[] = {
  {STM32F4_UART1, STM32F4_UART_SIGNAL_TX, STM32F4_PIN_PA9,
   GPIOA, GPIO_PIN_9, GPIO_AF7_USART1},
  {STM32F4_UART1, STM32F4_UART_SIGNAL_RX, STM32F4_PIN_PA10,
   GPIOA, GPIO_PIN_10, GPIO_AF7_USART1},
  {STM32F4_UART1, STM32F4_UART_SIGNAL_TX, STM32F4_PIN_PB6,
   GPIOB, GPIO_PIN_6, GPIO_AF7_USART1},
  {STM32F4_UART1, STM32F4_UART_SIGNAL_RX, STM32F4_PIN_PB7,
   GPIOB, GPIO_PIN_7, GPIO_AF7_USART1},
  {STM32F4_UART3, STM32F4_UART_SIGNAL_TX, STM32F4_PIN_PB10,
   GPIOB, GPIO_PIN_10, GPIO_AF7_USART3},
  {STM32F4_UART3, STM32F4_UART_SIGNAL_RX, STM32F4_PIN_PB11,
   GPIOB, GPIO_PIN_11, GPIO_AF7_USART3},
  {STM32F4_UART6, STM32F4_UART_SIGNAL_TX, STM32F4_PIN_PC6,
   GPIOC, GPIO_PIN_6, GPIO_AF8_USART6},
  {STM32F4_UART6, STM32F4_UART_SIGNAL_RX, STM32F4_PIN_PC7,
   GPIOC, GPIO_PIN_7, GPIO_AF8_USART6},
};

/**
 * @brief  查找 UART 信号对应的 STM32F4 引脚复用记录。
 * @param  uart_id UART 控制器编号。
 * @param  signal UART 信号类型，取 STM32F4_UART_SIGNAL_TX/RX。
 * @param  pin_id Board 层传入的整数引脚编号。
 * @return 匹配的引脚复用记录；未找到时返回 NULL。
 * @note   该函数只查询静态映射表，不访问硬件，也不修改任何状态。
 */
static const struct stm32f4_uart_pin_map *stm32f4_uart_pin_find(
    int32_t uart_id,
    int32_t signal,
    int32_t pin_id)
{
  size_t index;
  size_t map_count = sizeof(g_uart_pin_maps) / sizeof(g_uart_pin_maps[0]);

  for (index = 0U; index < map_count; ++index) {
    const struct stm32f4_uart_pin_map *map = &g_uart_pin_maps[index];

    if ((map->uart_id == uart_id) && (map->signal == signal) &&
        (map->pin_id == pin_id)) {
      return map;
    }
  }
  return NULL;
}

/**
 * @brief  判断当前代码是否运行在中断上下文。
 * @param  无。
 * @return true 表示当前位于 ISR；false 表示当前位于线程上下文。
 * @note   阻塞 UART 操作不允许在中断中调用，因此统一通过该函数检查。
 */
static bool stm32f4_uart_is_in_isr(void)
{
  return __get_IPSR() != 0U;
}

/**
 * @brief  将 HAL UART 返回值转换为平台层返回码。
 * @param  status HAL_UART_* 函数返回的状态。
 * @return PLATFORM_OK 或对应的 PLATFORM_ERR_* 错误码。
 * @note   平台接口不向上层暴露 HAL_StatusTypeDef，避免 HAL 类型泄漏。
 */
static int stm32f4_uart_result(HAL_StatusTypeDef status)
{
  if (status == HAL_OK) {
    return PLATFORM_OK;
  }
  if (status == HAL_TIMEOUT) {
    return PLATFORM_ERR_TIMEOUT;
  }
  if (status == HAL_BUSY) {
    return PLATFORM_ERR_BUSY;
  }
  return PLATFORM_ERR_IO;
}

/**
 * @brief  校验 STM32F4 UART 配置是否有效。
 * @param  config Board 层传入的 UART 配置。
 * @return true 表示配置可用；false 表示配置无效。
 * @note   这里会校验 UART 编号、名称、波特率、超时时间以及 TX/RX
 *         引脚是否能复用为对应 UART 信号。
 */
static bool stm32f4_uart_config_is_valid(
    const struct stm32f4_uart_config *config)
{
  if ((config == NULL) || (config->name == NULL) ||
      ((config->id != STM32F4_UART1) &&
       (config->id != STM32F4_UART3) &&
       (config->id != STM32F4_UART6)) ||
      (config->baud_rate == 0U) ||
      (config->timeout_ms == 0U)) {
    return false;
  }

  return (stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_TX,
                                config->tx_pin) != NULL) &&
         (stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_RX,
                                config->rx_pin) != NULL);
}

/**
 * @brief  申请 USART1 硬件资源的使用模式。
 * @param  mode 需要申请的 UART 使用模式。
 * @return PLATFORM_OK 表示申请成功；被其他模式占用时返回错误码。
 * @note   阻塞实现和中断实现共享 USART1，不能同时启动硬件。
 */
static int stm32f4_uart_claim(int32_t id, int32_t mode)
{
  int32_t *current_mode;

  if (id == STM32F4_UART1) {
    current_mode = &g_uart1_mode;
  } else if (id == STM32F4_UART3) {
    current_mode = &g_uart3_mode;
  } else {
    current_mode = &g_uart6_mode;
  }
  if ((*current_mode != STM32F4_UART_MODE_NONE) &&
      (*current_mode != mode)) {
    return PLATFORM_ERR_INVALID_STATE;
  }
  *current_mode = mode;
  return PLATFORM_OK;
}

/**
 * @brief  释放 USART1 硬件资源的使用模式。
 * @param  mode 当前对象持有的 UART 使用模式。
 * @return 无。
 * @note   只有释放者与当前持有模式一致时，才会把资源状态恢复为空闲。
 */
static void stm32f4_uart_release(int32_t id, int32_t mode)
{
  int32_t *current_mode;

  if (id == STM32F4_UART1) {
    current_mode = &g_uart1_mode;
  } else if (id == STM32F4_UART3) {
    current_mode = &g_uart3_mode;
  } else {
    current_mode = &g_uart6_mode;
  }
  if (*current_mode == mode) {
    *current_mode = STM32F4_UART_MODE_NONE;
  }
}

/**
 * @brief  打开指定 GPIO 端口的外设时钟。
 * @param  port GPIO 端口指针。
 * @return 无。
 * @note   当前 USART1 复用表只覆盖 GPIOA/GPIOB，其他端口不会被处理。
 */
static void stm32f4_uart_gpio_clock_enable(GPIO_TypeDef *port)
{
  if (port == GPIOA) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port == GPIOB) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port == GPIOC) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
  }
}

/**
 * @brief  初始化单个 UART 复用 GPIO 引脚。
 * @param  map 已校验的 UART 引脚复用记录。
 * @return 无。
 * @note   调用者需要先开启对应 GPIO 端口时钟。
 */
static void stm32f4_uart_gpio_pin_init(
    const struct stm32f4_uart_pin_map *map)
{
  GPIO_InitTypeDef pin = {0};

  pin.Pin = map->pin_mask;
  pin.Mode = GPIO_MODE_AF_PP;
  pin.Pull = GPIO_NOPULL;
  pin.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  pin.Alternate = map->alternate;
  HAL_GPIO_Init(map->port, &pin);
}

/**
 * @brief  初始化 UART TX/RX GPIO 复用引脚。
 * @param  tx_map TX 引脚复用记录。
 * @param  rx_map RX 引脚复用记录。
 * @return 无。
 * @note   该函数会自动打开 TX/RX 所在 GPIO 端口时钟。
 */
static void stm32f4_uart_gpio_init(
    const struct stm32f4_uart_pin_map *tx_map,
    const struct stm32f4_uart_pin_map *rx_map)
{
  stm32f4_uart_gpio_clock_enable(tx_map->port);
  stm32f4_uart_gpio_clock_enable(rx_map->port);
  stm32f4_uart_gpio_pin_init(tx_map);
  stm32f4_uart_gpio_pin_init(rx_map);
}

/**
 * @brief  反初始化 UART TX/RX GPIO 引脚。
 * @param  tx_map TX 引脚复用记录。
 * @param  rx_map RX 引脚复用记录。
 * @return 无。
 * @note   仅释放 GPIO 配置，不关闭 GPIO 端口时钟，避免影响同端口其他外设。
 */
static void stm32f4_uart_gpio_deinit(
    const struct stm32f4_uart_pin_map *tx_map,
    const struct stm32f4_uart_pin_map *rx_map)
{
  HAL_GPIO_DeInit(tx_map->port, tx_map->pin_mask);
  HAL_GPIO_DeInit(rx_map->port, rx_map->pin_mask);
}

/**
 * @brief  启动 USART6 发送使用的 DMA2 Stream6 硬件资源。
 * @param  self：已完成 USART6 初始化的阻塞 UART 具体对象。
 * @retval PLATFORM_OK：DMA 初始化成功。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：对象不是 USART6。
 * @retval PLATFORM_ERR_IO：HAL DMA 初始化失败。
 */
static int stm32f4_uart6_tx_dma_start(
    struct stm32f4_uart_blocking *self)
{
  if ((self == NULL) || (self->config == NULL) ||
      (self->config->id != STM32F4_UART6)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }

  __HAL_RCC_DMA2_CLK_ENABLE();
  self->tx_dma.Instance = DMA2_Stream6;
  self->tx_dma.Init.Channel = DMA_CHANNEL_5;
  self->tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  self->tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
  self->tx_dma.Init.MemInc = DMA_MINC_ENABLE;
  self->tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  self->tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  self->tx_dma.Init.Mode = DMA_NORMAL;
  self->tx_dma.Init.Priority = DMA_PRIORITY_LOW;
  self->tx_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&self->tx_dma) != HAL_OK) {
    return PLATFORM_ERR_IO;
  }

  /* HAL UART 通过 hdmatx 找到 DMA，并由 DMA 的 Parent 回到 UART。 */
  __HAL_LINKDMA(&self->handle, hdmatx, self->tx_dma);
  self->tx_dma_active = false;
  self->tx_dma_error = false;
  HAL_NVIC_ClearPendingIRQ(DMA2_Stream6_IRQn);
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn,
                       self->config->irq_priority, 0U);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
  return PLATFORM_OK;
}

/**
 * @brief  停止 USART6 发送 DMA 并释放其中断和 HAL 句柄。
 * @param  self：持有 USART6 发送 DMA 的阻塞 UART 具体对象。
 * @retval PLATFORM_OK：DMA 已停止并成功反初始化。
 * @retval PLATFORM_ERR_IO：中止发送或反初始化 DMA 失败。
 */
static int stm32f4_uart6_tx_dma_stop(
    struct stm32f4_uart_blocking *self)
{
  HAL_StatusTypeDef status;

  if (self->tx_dma_active) {
    status = HAL_UART_AbortTransmit(&self->handle);
    if (status != HAL_OK) {
      return stm32f4_uart_result(status);
    }
    self->tx_dma_active = false;
  }

  HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
  HAL_NVIC_ClearPendingIRQ(DMA2_Stream6_IRQn);
  if (HAL_DMA_DeInit(&self->tx_dma) != HAL_OK) {
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
    return PLATFORM_ERR_IO;
  }
  self->handle.hdmatx = NULL;
  self->tx_dma_error = false;
  return PLATFORM_OK;
}

static IRQn_Type stm32f4_uart_irq_get(int32_t id)
{
  if (id == STM32F4_UART1) {
    return USART1_IRQn;
  }
  if (id == STM32F4_UART3) {
    return USART3_IRQn;
  }
  return USART6_IRQn;
}

static void stm32f4_uart_clock_enable(UART_HandleTypeDef *handle,
                                      int32_t id)
{
  if (id == STM32F4_UART1) {
    __HAL_RCC_USART1_CLK_ENABLE();
    handle->Instance = USART1;
  } else if (id == STM32F4_UART3) {
    __HAL_RCC_USART3_CLK_ENABLE();
    handle->Instance = USART3;
  } else {
    __HAL_RCC_USART6_CLK_ENABLE();
    handle->Instance = USART6;
  }
}

static void stm32f4_uart_clock_disable(int32_t id)
{
  if (id == STM32F4_UART1) {
    __HAL_RCC_USART1_CLK_DISABLE();
  } else if (id == STM32F4_UART3) {
    __HAL_RCC_USART3_CLK_DISABLE();
  } else {
    __HAL_RCC_USART6_CLK_DISABLE();
  }
}

/**
 * @brief  启动指定 UART 控制器的公共硬件资源。
 * @param  handle STM32 HAL UART 句柄。
 * @param  config Board 层传入的 UART 配置。
 * @param  enable_irq 是否同时开启对应 UART 的全局中断。
 * @retval PLATFORM_OK：启动成功。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：UART 配置无效。
 * @retval PLATFORM_ERR_IO：HAL UART 初始化失败。
 * @note   该函数配置 GPIO 复用、外设时钟、HAL UART 参数和可选 NVIC。
 */
static int stm32f4_uart_hardware_start(
    UART_HandleTypeDef *handle,
    const struct stm32f4_uart_config *config,
    bool enable_irq)
{
  const struct stm32f4_uart_pin_map *tx_map;
  const struct stm32f4_uart_pin_map *rx_map;
  IRQn_Type irq;

  if ((handle == NULL) || !stm32f4_uart_config_is_valid(config)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  tx_map = stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_TX,
                                 config->tx_pin);
  rx_map = stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_RX,
                                 config->rx_pin);
  stm32f4_uart_gpio_init(tx_map, rx_map);

  stm32f4_uart_clock_enable(handle, config->id);
  handle->Init.BaudRate = config->baud_rate;
  handle->Init.WordLength = UART_WORDLENGTH_8B;
  handle->Init.StopBits = UART_STOPBITS_1;
  handle->Init.Parity = UART_PARITY_NONE;
  handle->Init.Mode = UART_MODE_TX_RX;
  handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  handle->Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(handle) != HAL_OK) {
    stm32f4_uart_clock_disable(config->id);
    stm32f4_uart_gpio_deinit(tx_map, rx_map);
    return PLATFORM_ERR_IO;
  }
  if (enable_irq) {
    irq = stm32f4_uart_irq_get(config->id);
    HAL_NVIC_SetPriority(irq, config->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(irq);
  }
  return PLATFORM_OK;
}

/**
 * @brief  停止指定 UART 控制器并释放公共硬件资源。
 * @param  handle STM32 HAL UART 句柄。
 * @param  config Board 层传入的 UART 配置。
 * @param  irq_enabled 当前对象是否启用了对应 UART 的全局中断。
 * @retval PLATFORM_OK：停止成功。
 * @retval PLATFORM_ERR_INVALID_CONFIGURATION：UART 配置无效。
 * @retval PLATFORM_ERR_IO：HAL UART 反初始化失败。
 * @note   停止成功后关闭对应外设时钟并释放 TX/RX GPIO 配置。
 */
static int stm32f4_uart_hardware_stop(
    UART_HandleTypeDef *handle,
    const struct stm32f4_uart_config *config,
    bool irq_enabled)
{
  const struct stm32f4_uart_pin_map *tx_map;
  const struct stm32f4_uart_pin_map *rx_map;
  IRQn_Type irq;

  if ((handle == NULL) || !stm32f4_uart_config_is_valid(config)) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  tx_map = stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_TX,
                                 config->tx_pin);
  rx_map = stm32f4_uart_pin_find(config->id, STM32F4_UART_SIGNAL_RX,
                                 config->rx_pin);
  irq = stm32f4_uart_irq_get(config->id);
  if (irq_enabled) {
    HAL_NVIC_DisableIRQ(irq);
  }
  if (HAL_UART_DeInit(handle) != HAL_OK) {
    if (irq_enabled) {
      HAL_NVIC_EnableIRQ(irq);
    }
    return PLATFORM_ERR_IO;
  }
  stm32f4_uart_clock_disable(config->id);
  stm32f4_uart_gpio_deinit(tx_map, rx_map);
  return PLATFORM_OK;
}

/**
 * @brief  启动阻塞对象内部的连续中断接收链。
 * @param  self STM32F4 阻塞 UART 具体对象。
 * @return PLATFORM_OK 表示接收链启动成功，其他 PLATFORM_* 表示失败。
 * @note   每次接收一个字节，并由完成回调持续续接下一次接收。
 */
static int stm32f4_uart_blocking_rx_start(
    struct stm32f4_uart_blocking *self)
{
  self->rx_head = 0U;
  self->rx_tail = 0U;
  self->rx_error = false;
  self->rx_discarding = false;
  self->rx_irq_active = true;
  if (HAL_UART_Receive_IT(&self->handle,
                          &self->rx_byte,
                          UINT16_C(1)) != HAL_OK) {
    self->rx_irq_active = false;
    return PLATFORM_ERR_IO;
  }

  return PLATFORM_OK;
}

/**
 * @brief  从中断接收环形缓冲区取出一个字节。
 * @param  self STM32F4 阻塞 UART 具体对象。
 * @param  byte 用于接收字节的变量。
 * @return true 表示成功取出字节，false 表示缓冲区为空。
 * @note   短临界区保护任务侧读取与中断侧写入的索引快照。
 */
static bool stm32f4_uart_blocking_rx_pop(
    struct stm32f4_uart_blocking *self,
    uint8_t *byte)
{
  uint32_t primask;
  bool available = false;

  primask = __get_PRIMASK();
  __disable_irq();
  if (self->rx_tail != self->rx_head) {
    *byte = self->rx_ring[self->rx_tail];
    self->rx_tail++;
    if (self->rx_tail >= STM32F4_UART_RX_RING_CAPACITY) {
      self->rx_tail = 0U;
    }
    available = true;
  }
  if (primask == 0U) {
    __enable_irq();
  }
  return available;
}

/**
 * @brief  从中断环形缓冲区阻塞读取指定数量字节。
 * @param  self STM32F4 阻塞 UART 具体对象。
 * @param  data 接收缓冲区。
 * @param  size 期望读取的字节数。
 * @param  timeout_ms 最大等待时间，单位为毫秒。
 * @return PLATFORM_OK 表示成功，超时或接收异常返回对应错误码。
 */
static int stm32f4_uart_blocking_ring_read(
    struct stm32f4_uart_blocking *self,
    uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
  uint32_t start_tick;
  uint16_t received = 0U;

  start_tick = HAL_GetTick();
  while (received < size) {
    if (self->rx_error) {
      self->rx_error = false;
      return PLATFORM_ERR_IO;
    }
    if (stm32f4_uart_blocking_rx_pop(self, &data[received])) {
      received++;
      continue;
    }
    if ((timeout_ms != HAL_MAX_DELAY) &&
        ((HAL_GetTick() - start_tick) >= timeout_ms)) {
      return PLATFORM_ERR_TIMEOUT;
    }
  }

  return PLATFORM_OK;
}

/**
 * @brief  启动阻塞 UART 对象。
 * @param  base UART 基础对象。
 * @retval PLATFORM_OK：启动成功。
 * @retval 其他 PLATFORM_ERR_*：参数、上下文、资源或硬件启动失败。
 * @note   两个串口均启动中断接收环；USART6 另外启动发送 DMA。
 */
static int stm32f4_uart_blocking_start(struct uart *base)
{
  struct stm32f4_uart_blocking *self;
  bool hardware_started = false;
  bool tx_dma_started = false;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_blocking, base);
  result = stm32f4_uart_claim(self->config->id,
                               STM32F4_UART_MODE_BLOCKING);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = stm32f4_uart_hardware_start(&self->handle, self->config,
                                        true);
  if (result == PLATFORM_OK) {
    hardware_started = true;
    if (self->config->id == STM32F4_UART6) {
      result = stm32f4_uart6_tx_dma_start(self);
      tx_dma_started = (result == PLATFORM_OK);
    }
    if (result == PLATFORM_OK) {
      result = stm32f4_uart_blocking_rx_start(self);
    }
  }
  if (result != PLATFORM_OK) {
    if (tx_dma_started) {
      (void)stm32f4_uart6_tx_dma_stop(self);
    }
    if (hardware_started) {
      (void)stm32f4_uart_hardware_stop(&self->handle,
                                       self->config,
                                       true);
    }
    stm32f4_uart_release(self->config->id,
                          STM32F4_UART_MODE_BLOCKING);
  }
  return result;
}

/**
 * @brief  停止阻塞 UART 对象。
 * @param  base UART 基础对象。
 * @retval PLATFORM_OK：释放成功。
 * @retval 其他 PLATFORM_ERR_*：参数、上下文或 HAL 操作失败。
 * @note   USART6 先释放发送 DMA，再释放 UART 公共硬件资源。
 */
static int stm32f4_uart_blocking_stop(struct uart *base)
{
  struct stm32f4_uart_blocking *self;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_blocking, base);
  self->rx_irq_active = false;
  if (self->config->id == STM32F4_UART6) {
    result = stm32f4_uart6_tx_dma_stop(self);
    if (result != PLATFORM_OK) {
      return result;
    }
  }
  result = stm32f4_uart_hardware_stop(&self->handle, self->config, true);
  if (result == PLATFORM_OK) {
    stm32f4_uart_release(self->config->id,
                          STM32F4_UART_MODE_BLOCKING);
  }
  return result;
}

/**
 * @brief  通过阻塞 UART 对象发送数据。
 * @param  base UART 基础对象。
 * @param  data 待发送数据缓冲区。
 * @param  size 待发送字节数。
 * @param  timeout_ms 阻塞发送超时时间，单位 ms。
 * @retval PLATFORM_OK：发送成功。
 * @retval 其他 PLATFORM_ERR_*：参数、上下文、超时或 HAL 操作失败。
 * @note   USART6 使用 DMA 连续发送并等待完成中断；USART1 使用 HAL
 *         轮询发送。两种路径均不允许在 ISR 上下文调用。
 */
static int stm32f4_uart_blocking_write(
    struct uart *base,
    const uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
  struct stm32f4_uart_blocking *self;
  HAL_StatusTypeDef status;
  uint32_t start_tick;

  if ((base == NULL) || (data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_blocking, base);
  if (self->config->id == STM32F4_UART6) {
    self->tx_dma_error = false;
    self->tx_dma_active = true;
    status = HAL_UART_Transmit_DMA(&self->handle, data, size);
    if (status != HAL_OK) {
      self->tx_dma_active = false;
      return stm32f4_uart_result(status);
    }

    /* DMA 保证帧内连续发送；任务仅等待完成中断更新传输状态。 */
    start_tick = HAL_GetTick();
    while (self->tx_dma_active) {
      if ((timeout_ms != PLATFORM_WAIT_FOREVER) &&
          ((HAL_GetTick() - start_tick) >= timeout_ms)) {
        status = HAL_UART_AbortTransmit(&self->handle);
        self->tx_dma_active = false;
        return (status == HAL_OK) ? PLATFORM_ERR_TIMEOUT :
               stm32f4_uart_result(status);
      }
    }
    return self->tx_dma_error ? PLATFORM_ERR_IO : PLATFORM_OK;
  }
  return stm32f4_uart_result(
      HAL_UART_Transmit(&self->handle, (uint8_t *)data, size, timeout_ms));
}

/**
 * @brief  通过阻塞 UART 对象接收数据。
 * @param  base UART 基础对象。
 * @param  data 接收数据缓冲区。
 * @param  size 期望接收字节数。
 * @param  timeout_ms 阻塞接收超时时间，单位 ms。
 * @return PLATFORM_OK 表示接收成功；其他值表示参数、上下文或 HAL 错误。
 * @note   数据由中断写入环形缓冲，本函数等待并消费固定长度数据。
 */
static int stm32f4_uart_blocking_read(
    struct uart *base,
    uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
  struct stm32f4_uart_blocking *self;

  if ((base == NULL) || (data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_blocking, base);
  return stm32f4_uart_blocking_ring_read(self, data, size,
                                         timeout_ms);
}

/**
 * @brief  启动中断 UART 对象。
 * @param  base UART 基础对象。
 * @return PLATFORM_OK 表示启动成功；其他值表示参数、上下文或资源错误。
 * @note   启动成功后 USART1_IRQn 会被开启，用于驱动中断接收完成回调。
 */
static int stm32f4_uart_interrupt_start(struct uart *base)
{
  struct stm32f4_uart_interrupt *self;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_interrupt, base);
  result = stm32f4_uart_claim(self->config->id,
                               STM32F4_UART_MODE_INTERRUPT);
  if (result != PLATFORM_OK) {
    return result;
  }
  result = stm32f4_uart_hardware_start(&self->handle, self->config,
                                        true);
  if (result != PLATFORM_OK) {
    stm32f4_uart_release(self->config->id,
                          STM32F4_UART_MODE_INTERRUPT);
  }
  return result;
}

/**
 * @brief  停止中断 UART 对象。
 * @param  base UART 基础对象。
 * @return PLATFORM_OK 表示释放成功；其他值表示参数、忙状态或 HAL 错误。
 * @note   如果仍有接收请求挂起，会返回 PLATFORM_ERR_BUSY。
 */
static int stm32f4_uart_interrupt_stop(struct uart *base)
{
  struct stm32f4_uart_interrupt *self;
  int result;

  if (base == NULL) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_interrupt, base);
  if (self->rx_pending) {
    return PLATFORM_ERR_BUSY;
  }
  result = stm32f4_uart_hardware_stop(&self->handle, self->config,
                                       true);
  if (result == PLATFORM_OK) {
    stm32f4_uart_release(self->config->id,
                          STM32F4_UART_MODE_INTERRUPT);
  }
  return result;
}

/**
 * @brief  通过中断 UART 对象发送数据。
 * @param  base UART 基础对象。
 * @param  data 待发送数据缓冲区。
 * @param  size 待发送字节数。
 * @param  timeout_ms 阻塞发送超时时间，单位 ms。
 * @return PLATFORM_OK 表示发送成功；其他值表示参数、上下文或 HAL 错误。
 * @note   当前发送仍使用 HAL 阻塞发送；中断能力仅用于接收。
 */
static int stm32f4_uart_interrupt_write(
    struct uart *base,
    const uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
  struct stm32f4_uart_interrupt *self;

  if ((base == NULL) || (data == NULL) || (size == 0U)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  if (stm32f4_uart_is_in_isr()) {
    return PLATFORM_ERR_ISR_CONTEXT;
  }
  self = container_of(base, struct stm32f4_uart_interrupt, base);
  return stm32f4_uart_result(
      HAL_UART_Transmit(&self->handle, (uint8_t *)data, size, timeout_ms));
}

/**
 * @brief  启动一次 UART 中断接收。
 * @param  base UART 基础对象。
 * @param  data 接收数据缓冲区。
 * @param  size 期望接收字节数。
 * @param  callback 接收完成或错误时调用的回调函数。
 * @param  context 传递给回调函数的用户上下文。
 * @return PLATFORM_OK 表示启动成功；其他值表示参数、上下文或忙状态错误。
 * @note   data 在回调发生前必须保持有效；允许在接收完成回调中立即续接下一笔接收。
 */
static int stm32f4_uart_interrupt_receive(
    struct uart *base,
    uint8_t *data,
    uint16_t size,
    uart_interrupt_callback_t callback,
    void *context)
{
  struct stm32f4_uart_interrupt *self;
  HAL_StatusTypeDef status;

  if ((base == NULL) || (data == NULL) || (size == 0U) ||
      (callback == NULL)) {
    return PLATFORM_ERR_INVALID_ARGUMENT;
  }
  self = container_of(base, struct stm32f4_uart_interrupt, base);
  if (self->rx_pending) {
    return PLATFORM_ERR_BUSY;
  }
  self->rx_callback = callback;
  self->rx_context = context;
  self->rx_data = data;
  self->rx_size = size;
  self->rx_pending = true;
  status = HAL_UART_Receive_IT(&self->handle, data, size);
  if (status != HAL_OK) {
    self->rx_pending = false;
    self->rx_callback = NULL;
    self->rx_context = NULL;
    self->rx_data = NULL;
    self->rx_size = 0U;
    return stm32f4_uart_result(status);
  }
  return PLATFORM_OK;
}

static const struct uart_ops g_blocking_base_ops = {
  .start = stm32f4_uart_blocking_start,
  .stop = stm32f4_uart_blocking_stop,
  .write = stm32f4_uart_blocking_write,
};

static const struct uart_blocking_ops g_blocking_ops = {
  .read = stm32f4_uart_blocking_read,
};

static const struct uart_ops g_interrupt_base_ops = {
  .start = stm32f4_uart_interrupt_start,
  .stop = stm32f4_uart_interrupt_stop,
  .write = stm32f4_uart_interrupt_write,
};

static const struct uart_interrupt_ops g_interrupt_ops = {
  .receive = stm32f4_uart_interrupt_receive,
};

/**
 * @brief  创建 USART1 阻塞 UART 对象。
 * @param  config Board 层传入的 UART 配置。
 * @return UART 基础对象；配置无效时返回 NULL。
 * @note   返回对象使用静态存储，调用者不拥有释放权。
 */
struct uart *stm32f4_uart_blocking_create(
    const struct stm32f4_uart_config *config)
{
  struct stm32f4_uart_blocking *object;

  if (!stm32f4_uart_config_is_valid(config)) {
    return NULL;
  }
  if (config->id == STM32F4_UART3) {
    return NULL;
  }
  object = (config->id == STM32F4_UART1) ?
           &g_uart1_blocking : &g_uart6_blocking;
  object->base.ops = &g_blocking_base_ops;
  object->base.blocking_ops = &g_blocking_ops;
  object->base.interrupt_ops = NULL;
  object->base.name = config->name;
  object->config = config;
  return &object->base;
}

/**
 * @brief  创建 USART1 中断 UART 对象。
 * @param  config Board 层传入的 UART 配置。
 * @return UART 基础对象；配置无效时返回 NULL。
 * @note   返回对象使用静态存储，调用者不拥有释放权。
 */
struct uart *stm32f4_uart_interrupt_create(
    const struct stm32f4_uart_config *config)
{
  struct stm32f4_uart_interrupt *object;

  if (!stm32f4_uart_config_is_valid(config) ||
      (config->id == STM32F4_UART6)) {
    return NULL;
  }
  object = (config->id == STM32F4_UART1) ?
           &g_uart1_interrupt : &g_uart3_interrupt;
  object->base.ops = &g_interrupt_base_ops;
  object->base.blocking_ops = NULL;
  object->base.interrupt_ops = &g_interrupt_ops;
  object->base.name = config->name;
  object->config = config;
  return &object->base;
}

/**
 * @brief  保存阻塞对象收到的一个字节并继续下一字节接收。
 * @param  self STM32F4 阻塞 UART 具体对象。
 * @return 无返回值。
 * @note   USART1 按文本帧恢复，USART6 按二进制帧立即清空恢复。
 */
static void stm32f4_uart_blocking_rx_complete(
    struct stm32f4_uart_blocking *self)
{
  uint16_t next_head;

  if (!self->rx_irq_active) {
    return;
  }

  if (self->rx_discarding) {
    if ((self->rx_byte == '\r') || (self->rx_byte == '\n')) {
      self->rx_discarding = false;
    }
  } else {
    next_head = self->rx_head + 1U;
    if (next_head >= STM32F4_UART_RX_RING_CAPACITY) {
      next_head = 0U;
    }
    if (next_head == self->rx_tail) {
      /* 溢出后清空残帧；文本串口继续丢弃到下一行结束符。 */
      self->rx_head = 0U;
      self->rx_tail = 0U;
      self->rx_error = true;
      self->rx_discarding =
          (self->config->id == STM32F4_UART1);
    } else {
      self->rx_ring[self->rx_head] = self->rx_byte;
      self->rx_head = next_head;
    }
  }

  if (HAL_UART_Receive_IT(&self->handle,
                          &self->rx_byte,
                          UINT16_C(1)) != HAL_OK) {
    self->rx_error = true;
    self->rx_irq_active = false;
  }
}

/**
 * @brief  从阻塞对象接收错误中恢复连续中断接收。
 * @param  self STM32F4 阻塞 UART 具体对象。
 * @return 无返回值。
 * @note   USART1 丢弃残行；USART6 清空二进制残帧后立即接收新帧。
 */
static void stm32f4_uart_blocking_rx_recover(
    struct stm32f4_uart_blocking *self)
{
  self->rx_head = 0U;
  self->rx_tail = 0U;
  self->rx_error = true;
  self->rx_discarding =
      (self->config->id == STM32F4_UART1);
  self->rx_irq_active = false;
  if (HAL_UART_AbortReceive(&self->handle) != HAL_OK) {
    return;
  }
  self->rx_irq_active = true;
  if (HAL_UART_Receive_IT(&self->handle,
                          &self->rx_byte,
                          UINT16_C(1)) != HAL_OK) {
    self->rx_irq_active = false;
  }
}

/**
 * @brief  分发 USART1 中断给 HAL UART 驱动。
 * @param  无。
 * @return 无。
 * @note   仅当中断 UART 对象已经启动时才会调用 HAL_UART_IRQHandler()。
 */
void stm32f4_usart1_irq_handler(void)
{
  if ((g_uart1_mode == STM32F4_UART_MODE_BLOCKING) &&
      g_uart1_blocking.rx_irq_active) {
    HAL_UART_IRQHandler(&g_uart1_blocking.handle);
  } else if ((g_uart1_mode == STM32F4_UART_MODE_INTERRUPT) &&
      g_uart1_interrupt.base.initialized) {
    HAL_UART_IRQHandler(&g_uart1_interrupt.handle);
  }
}

void stm32f4_usart3_irq_handler(void)
{
  if ((g_uart3_mode == STM32F4_UART_MODE_INTERRUPT) &&
      g_uart3_interrupt.base.initialized) {
    HAL_UART_IRQHandler(&g_uart3_interrupt.handle);
  }
}

/**
 * @brief  将 USART6 全局中断分发给 HAL UART 驱动。
 * @param  None。
 * @retval None。
 */
void stm32f4_usart6_irq_handler(void)
{
  if ((g_uart6_mode == STM32F4_UART_MODE_BLOCKING) &&
      g_uart6_blocking.base.initialized) {
    HAL_UART_IRQHandler(&g_uart6_blocking.handle);
  }
}

/**
 * @brief  将 USART6 发送 DMA 中断分发给 HAL DMA 驱动。
 * @param  None。
 * @retval None。
 */
void stm32f4_usart6_tx_dma_irq_handler(void)
{
  if ((g_uart6_mode == STM32F4_UART_MODE_BLOCKING) &&
      g_uart6_blocking.tx_dma_active) {
    HAL_DMA_IRQHandler(&g_uart6_blocking.tx_dma);
  }
}

/**
 * @brief  完成一次中断接收并通知上层回调。
 * @param  result 本次接收结果，通常为 PLATFORM_OK 或 PLATFORM_ERR_IO。
 * @return 无。
 * @note   该函数会先清理挂起状态，再调用用户回调，避免回调中重入时
 *         看到旧的接收上下文。
 */
static void stm32f4_uart_interrupt_rx_finish(
    struct stm32f4_uart_interrupt *self,
    int result)
{
  uart_interrupt_callback_t callback;
  void *context;
  uint8_t *data;
  uint16_t size;

  if ((self == NULL) || !self->rx_pending) {
    return;
  }
  callback = self->rx_callback;
  context = self->rx_context;
  data = self->rx_data;
  size = self->rx_size;
  self->rx_pending = false;
  self->rx_callback = NULL;
  self->rx_context = NULL;
  self->rx_data = NULL;
  self->rx_size = 0U;
  callback(&self->base, data, size, result, context);
}

/**
 * @brief  HAL UART 接收完成回调。
 * @param  handle HAL 传入的 UART 句柄。
 * @return 无。
 * @note   由 HAL 中断流程调用，并续接阻塞对象的连续接收链。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle == &g_uart1_blocking.handle) {
    stm32f4_uart_blocking_rx_complete(&g_uart1_blocking);
  } else if (handle == &g_uart1_interrupt.handle) {
    stm32f4_uart_interrupt_rx_finish(&g_uart1_interrupt,
                                     PLATFORM_OK);
  } else if (handle == &g_uart3_interrupt.handle) {
    stm32f4_uart_interrupt_rx_finish(&g_uart3_interrupt,
                                     PLATFORM_OK);
  } else if (handle == &g_uart6_blocking.handle) {
    stm32f4_uart_blocking_rx_complete(&g_uart6_blocking);
  }
}

/**
 * @brief  处理 USART6 DMA 发送完成事件。
 * @param  handle：HAL 传入的 UART 句柄。
 * @retval None。
 * @note   本函数在 USART6 中断上下文执行，只清除任务侧等待标志。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle == &g_uart6_blocking.handle) {
    g_uart6_blocking.tx_dma_active = false;
  }
}

/**
 * @brief  HAL UART 错误回调。
 * @param  handle HAL 传入的 UART 句柄。
 * @return 无。
 * @note   该函数由 HAL 中断处理流程调用，错误会转换为 PLATFORM_ERR_IO。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
  uint32_t error_code;

  if (handle == &g_uart1_blocking.handle) {
    stm32f4_uart_blocking_rx_recover(&g_uart1_blocking);
  } else if (handle == &g_uart1_interrupt.handle) {
    stm32f4_uart_interrupt_rx_finish(&g_uart1_interrupt,
                                     PLATFORM_ERR_IO);
  } else if (handle == &g_uart3_interrupt.handle) {
    stm32f4_uart_interrupt_rx_finish(&g_uart3_interrupt,
                                     PLATFORM_ERR_IO);
  } else if (handle == &g_uart6_blocking.handle) {
    error_code = handle->ErrorCode;
    if (g_uart6_blocking.tx_dma_active &&
        ((error_code & HAL_UART_ERROR_DMA) != 0U)) {
      g_uart6_blocking.tx_dma_error = true;
      g_uart6_blocking.tx_dma_active = false;
    }
    if ((error_code & STM32F4_UART_RX_ERROR_MASK) != 0U) {
      stm32f4_uart_blocking_rx_recover(&g_uart6_blocking);
    }
  }
}
