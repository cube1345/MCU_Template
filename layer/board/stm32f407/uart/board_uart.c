/**
 * @file    board_uart.c
 * @brief   STM32F407 板载 USART1 的对象组合和接线绑定。
 * @author  weixin
 */
#include "board_uart.h"

#include "stm32f4.h"
#include "stm32f4_uart.h"
#include "platform/inferface/uart/stdio_uart.h"
#include "result.h"

#include <stddef.h>

#define BOARD_UART1_MODE_NONE INT32_C(0)
#define BOARD_UART1_MODE_BLOCKING INT32_C(1)
#define BOARD_UART1_MODE_INTERRUPT INT32_C(2)

#define BOARD_UART1_RECEIVE_BLOCKING INT32_C(0)
#define BOARD_UART1_RECEIVE_INTERRUPT INT32_C(1)
#define BOARD_UART1_RECEIVE_MODE BOARD_UART1_RECEIVE_INTERRUPT

#define BOARD_UART1_BAUD_RATE UINT32_C(115200)
#define BOARD_UART1_TIMEOUT_MS UINT32_C(1000)
#define BOARD_UART1_IRQ_PRIORITY UINT32_C(5)

/* EMM V5闭环步进驱动器使用的USART6通信参数。 */
#define BOARD_UART6_BAUD_RATE UINT32_C(115200)
#define BOARD_UART6_TIMEOUT_MS UINT32_C(100)
#define BOARD_UART6_IRQ_PRIORITY UINT32_C(5)

static const struct stm32f4_uart_config g_uart1_config = {
  .id = STM32F4_UART1,
  .name = "usart1",
  .tx_pin = STM32F4_PIN_PA9,
  .rx_pin = STM32F4_PIN_PA10,
  .baud_rate = BOARD_UART1_BAUD_RATE,
  .timeout_ms = BOARD_UART1_TIMEOUT_MS,
  .irq_priority = BOARD_UART1_IRQ_PRIORITY,
};

/* USART6固定接到PC6(TX)与PC7(RX)，不参与printf重定向。 */
static const struct stm32f4_uart_config g_uart6_config = {
  .id = STM32F4_UART6,
  .name = "usart6",
  .tx_pin = STM32F4_PIN_PC6,
  .rx_pin = STM32F4_PIN_PC7,
  .baud_rate = BOARD_UART6_BAUD_RATE,
  .timeout_ms = BOARD_UART6_TIMEOUT_MS,
  .irq_priority = BOARD_UART6_IRQ_PRIORITY,
};

static struct uart *g_uart1;
static int32_t g_uart1_mode;
/* USART6仅供外部电机驱动器使用，采用阻塞收发实现。 */
static struct uart *g_uart6;

/**
 * @brief  初始化 STM32F407 板级 USART1。
 * @return PLATFORM_OK 表示初始化成功或已经初始化。
 * @return 其他 PLATFORM_* 表示 MCU、UART 或 printf 重定向初始化失败。
 * @note   当前 USART1 使用 PA9/PA10、115200 baud 和中断接收能力。
 */
int board_uart1_init(void)
{
  int result;

  if (g_uart1_mode != BOARD_UART1_MODE_NONE) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  if (result == PLATFORM_OK) {
    #if BOARD_UART1_RECEIVE_MODE == BOARD_UART1_RECEIVE_BLOCKING
    g_uart1 = stm32f4_uart_blocking_create(&g_uart1_config);
    #elif BOARD_UART1_RECEIVE_MODE == BOARD_UART1_RECEIVE_INTERRUPT
    g_uart1 = stm32f4_uart_interrupt_create(&g_uart1_config);
    #else
    #error "Unsupported BOARD_UART1_RECEIVE_MODE"
    #endif
    result = (g_uart1 == NULL) ?
             PLATFORM_ERR_INVALID_CONFIGURATION :
             uart_start(g_uart1);
  }
  if (result != PLATFORM_OK) {
    g_uart1 = NULL;
    return result;
  }
  result = stdio_uart_bind(g_uart1);
  if (result != PLATFORM_OK) {
    g_uart1 = NULL;
    return result;
  }

  #if BOARD_UART1_RECEIVE_MODE == BOARD_UART1_RECEIVE_BLOCKING
  g_uart1_mode = BOARD_UART1_MODE_BLOCKING;
  #else
  g_uart1_mode = BOARD_UART1_MODE_INTERRUPT;
  #endif
  return PLATFORM_OK;
}

/**
 * @brief  获取板级 USART1 抽象 UART 对象。
 * @return 初始化成功后返回 struct uart *，否则返回 NULL。
 * @note   应用层只依赖 UART 基础接口和能力接口，不依赖 STM32F4 子类。
 */
struct uart *board_uart1_get(void)
{
  return (g_uart1_mode != BOARD_UART1_MODE_NONE) ? g_uart1 : NULL;
}

/**
 * @brief  初始化连接EMM V5驱动器的STM32F407板级USART6。
 * @retval PLATFORM_OK表示初始化成功或已经初始化，其他PLATFORM_*表示失败。
 * @note   USART6使用PC6/PC7、115200 baud和阻塞收发，不会绑定到printf。
 */
int board_uart6_init(void)
{
  int result;

  if (g_uart6 != NULL) {
    return PLATFORM_OK;
  }
  result = stm32f4_init();
  if (result != PLATFORM_OK) {
    return result;
  }
  g_uart6 = stm32f4_uart_blocking_create(&g_uart6_config);
  if (g_uart6 == NULL) {
    return PLATFORM_ERR_INVALID_CONFIGURATION;
  }
  result = uart_start(g_uart6);
  if (result != PLATFORM_OK) {
    g_uart6 = NULL;
  }
  return result;
}

/**
 * @brief  获取EMM V5连接使用的板级USART6抽象对象。
 * @retval 初始化成功后返回struct uart *，否则返回NULL。
 * @note   返回对象仅供EMM V5板级组合和驱动层使用，应用层不应访问HAL句柄。
 */
struct uart *board_uart6_get(void)
{
  return g_uart6;
}
