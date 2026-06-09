#include "protocol_probe_app.h"

#include "hal_iic.h"
#include "hal_spi.h"
#include "stm32f1xx_hal.h"

#if defined(HAL_UART_MODULE_ENABLED) && defined(__has_include)
#if __has_include("usart.h")
#include "usart.h"
#define PROTOCOL_PROBE_HAS_UART4        (1U)
#endif
#endif

#ifndef PROTOCOL_PROBE_HAS_UART4
#define PROTOCOL_PROBE_HAS_UART4        (0U)
#endif

#ifndef PROTOCOL_PROBE_SPI_USE_SOFT_CS
#define PROTOCOL_PROBE_SPI_USE_SOFT_CS  (1U)
#endif

#if (PROTOCOL_PROBE_SPI_USE_SOFT_CS == 1U)
#ifndef PROTOCOL_PROBE_SPI_CS_PORT
#define PROTOCOL_PROBE_SPI_CS_PORT      GPIOB
#endif

#ifndef PROTOCOL_PROBE_SPI_CS_PIN
#define PROTOCOL_PROBE_SPI_CS_PIN       GPIO_PIN_12
#endif
#endif

static uint8_t g_protocol_probe_sequence;

#if (PROTOCOL_PROBE_SPI_USE_SOFT_CS == 1U)
static void ProtocolProbe_InitSpiCs(void)
{
#if defined(HAL_GPIO_MODULE_ENABLED)
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin = PROTOCOL_PROBE_SPI_CS_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PROTOCOL_PROBE_SPI_CS_PORT, &gpio_init);
    HAL_GPIO_WritePin(PROTOCOL_PROBE_SPI_CS_PORT, PROTOCOL_PROBE_SPI_CS_PIN, GPIO_PIN_SET);
#endif
}

static void ProtocolProbe_SetSpiCs(uint8_t active)
{
#if defined(HAL_GPIO_MODULE_ENABLED)
    HAL_GPIO_WritePin(PROTOCOL_PROBE_SPI_CS_PORT, PROTOCOL_PROBE_SPI_CS_PIN,
                      (active != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    (void)active;
#endif
}
#endif

static void ProtocolProbe_SendUart(uint8_t sequence)
{
#if (PROTOCOL_PROBE_HAS_UART4 == 1U)
    uint8_t frame[] = {
        0x55U, 0xAAU, 0x10U, sequence,
        (uint8_t)(sequence ^ 0xFFU), 0x0DU, 0x0AU
    };

    (void)HAL_UART_Transmit(&huart4, frame, (uint16_t)sizeof(frame), 100U);
#else
    (void)sequence;
#endif
}

static void ProtocolProbe_SendI2c(uint8_t sequence)
{
    uint8_t frame[] = {
        0xA5U, 0x5AU, 0x20U, sequence,
        (uint8_t)(sequence + 1U)
    };

    (void)hal_iic_write_bytes(HAL_IIC_PORT_0,
                              PROTOCOL_PROBE_I2C_ADDR,
                              frame,
                              (uint16_t)sizeof(frame));
}

static void ProtocolProbe_SendSpi(uint8_t sequence)
{
    uint8_t frame[] = {
        0x9AU, 0xBCU, 0x30U, sequence,
        (uint8_t)(sequence ^ 0xA5U)
    };

#if (PROTOCOL_PROBE_SPI_USE_SOFT_CS == 1U)
    ProtocolProbe_SetSpiCs(1U);
#endif
    (void)hal_spi_transmit(HAL_SPI_PORT_0, frame, (uint16_t)sizeof(frame));
#if (PROTOCOL_PROBE_SPI_USE_SOFT_CS == 1U)
    ProtocolProbe_SetSpiCs(0U);
#endif
}

void ProtocolProbe_AppInit(void)
{
    g_protocol_probe_sequence = 0U;

    hal_iic_init();
    hal_spi_init();

#if (PROTOCOL_PROBE_SPI_USE_SOFT_CS == 1U)
    ProtocolProbe_InitSpiCs();
#endif
}

void ProtocolProbe_AppTask(void)
{
    uint8_t sequence = g_protocol_probe_sequence++;

    ProtocolProbe_SendUart(sequence);
    HAL_Delay(10U);

    ProtocolProbe_SendI2c(sequence);
    HAL_Delay(10U);

    ProtocolProbe_SendSpi(sequence);
    HAL_Delay(PROTOCOL_PROBE_PERIOD_MS);
}
