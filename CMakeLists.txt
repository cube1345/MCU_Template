cmake_minimum_required(VERSION 3.20)

# 本文件只用于 VSCode/CMake Tools 生成准确的 compile_commands.json，
# 实际固件仍由 MDK-ARM/empty.uvprojx 使用 Keil ARMCC 构建和下载。
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(stm32f407_oop_index LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# 与 Keil 工程中实际启用的源文件保持一致，避免索引未参与工程的 HAL 模块。
set(PROJECT_SOURCES
    Core/Src/freertos.c
    layer/application/core/main.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/stm32f4xx_hal_timebase_tim.c
    Core/Src/stm32f4xx_it.c
    Core/Src/system_stm32f4xx.c

    layer/platform/inferface/pin/pin.c
    layer/platform/inferface/pwm/pwm.c
    layer/platform/inferface/encoder_counter/encoder_counter.c
    layer/platform/inferface/timebase/timebase.c
    layer/platform/inferface/uart/stdio_uart.c
    layer/platform/inferface/i2c/i2c.c
    layer/platform/inferface/i2c/i2c_blocking.c
    layer/platform/inferface/i2c/i2c_interrupt.c
    layer/platform/inferface/uart/uart.c
    layer/platform/inferface/uart/uart_blocking.c
    layer/platform/inferface/uart/uart_interrupt.c
    layer/platform/mcu/stm32f4/core/stm32f4.c
    layer/platform/mcu/stm32f4/pin/stm32f4_pin.c
    layer/platform/mcu/stm32f4/pwm/stm32f4_pwm.c
    layer/platform/mcu/stm32f4/encoder_counter/stm32f4_encoder_counter.c
    layer/platform/mcu/stm32f4/i2c/stm32f4_i2c.c
    layer/platform/mcu/stm32f4/i2c/stm32f4_i2c_sw.c
    layer/platform/mcu/stm32f4/uart/stm32f4_uart.c

    layer/device/led/led.c
    layer/device/key/key.c
    layer/device/imu/imu.c
    layer/device/display/display.c
    layer/device/encoder/encoder.c
    layer/device/motor/motor.c
    layer/driver/led/led_gpio.c
    layer/driver/led/led_pwm.c
    layer/driver/key/key_gpio.c
    layer/driver/icm42688/icm42688.c
    layer/driver/ssd1306/ssd1306.c
    layer/driver/encoder/quadrature_encoder.c
    layer/driver/motor/at8236_motor.c
    layer/driver/motor/emm_v5/emm_v5.c
    layer/component/key/key_event.c
    layer/component/emm_v5_soft_limit/emm_v5_soft_limit.c
    layer/component/motor_axis/motor_axis.c
    layer/component/tracking/tracking_inputs.c
    layer/component/weixin_protocol/weixin_protocol.c
    layer/library/ahrs/mahony.c
    layer/library/filter/low_pass_filter.c
    layer/library/math/math_utils.c
    layer/library/control/pid.c
    layer/library/graphics/mono_canvas.c
    layer/library/graphics/mono_shape.c
    layer/library/graphics/mono_text.c
    layer/library/graphics/mono_text_ascii.c
    layer/library/graphics/mono_text_format.c
    layer/library/graphics/mono_text_multibyte.c
    layer/library/graphics/font/mono_font_ascii.c
    layer/application/core/app_rtos.c
    layer/application/core/app_led.c
    layer/application/core/app_key.c
    layer/application/core/app_mission.c
    layer/application/core/app_oled.c
    layer/application/core/app_motor.c
    layer/application/core/app_encoder.c
    layer/application/core/app_emm_v5.c
    layer/application/core/app_serial_monitor.c
    layer/application/core/app_tracking.c
    layer/application/core/app_vision.c
    layer/application/core/app_ball_control.c
    layer/board/stm32f407/led/board_led.c
    layer/board/stm32f407/key/board_key.c
    layer/board/stm32f407/uart/board_uart.c
    layer/board/stm32f407/i2c/board_i2c.c
    layer/board/stm32f407/oled/board_oled.c
    layer/board/stm32f407/drive/board_drive.c
    layer/board/stm32f407/emm_v5/board_emm_v5.c
    layer/board/stm32f407/tracking/board_tracking.c

    layer/platform/RTOS/monitor/my_freetros_lib.c
    layer/platform/RTOS/portable/heap_4.c
    layer/platform/RTOS/portable/port.c
    layer/platform/RTOS/source/croutine.c
    layer/platform/RTOS/source/event_groups.c
    layer/platform/RTOS/source/list.c
    layer/platform/RTOS/source/queue.c
    layer/platform/RTOS/source/stream_buffer.c
    layer/platform/RTOS/source/tasks.c
    layer/platform/RTOS/source/timers.c

    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
)

# 静态库目标不会执行固件链接，因此无需在索引工程中维护链接脚本。
add_library(stm32f407_oop_index STATIC ${PROJECT_SOURCES})

target_include_directories(stm32f407_oop_index PRIVATE
    Core/Inc
    layer
    layer/platform
    layer/platform/inferface/uart
    layer/platform/inferface/i2c
    layer/platform/inferface/pin
    layer/platform/inferface/pwm
    layer/platform/inferface/encoder_counter
    layer/platform/inferface/timebase
    layer/platform/mcu/stm32f4
    layer/platform/mcu/stm32f4/core
    layer/platform/mcu/stm32f4/uart
    layer/platform/mcu/stm32f4/i2c
    layer/platform/mcu/stm32f4/pin
    layer/platform/mcu/stm32f4/pwm
    layer/platform/mcu/stm32f4/encoder_counter
    layer/device
    layer/device/key
    layer/device/led
    layer/device/encoder
    layer/device/motor
    layer/driver
    layer/driver/key
    layer/driver/led
    layer/driver/encoder
    layer/driver/motor
    layer/driver/motor/emm_v5
    layer/component
    layer/component/key
    layer/component/emm_v5_soft_limit
    layer/component/motor_axis
    layer/component/tracking
    layer/component/weixin_protocol
    layer/library
    layer/library/ahrs
    layer/library/filter
    layer/library/math
    layer/library/control
    layer/application/core
    layer/board/stm32f407
    layer/board/stm32f407/i2c
    layer/board/stm32f407/imu
    layer/board/stm32f407/key
    layer/board/stm32f407/led
    layer/board/stm32f407/oled
    layer/board/stm32f407/uart
    layer/board/stm32f407/drive
    layer/board/stm32f407/emm_v5
    layer/board/stm32f407/tracking
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    layer/platform/RTOS
    layer/platform/RTOS/monitor
    layer/platform/RTOS/include
    layer/platform/RTOS/portable
    MDK-ARM/RTE/_empty
)

target_compile_definitions(stm32f407_oop_index PRIVATE
    USE_HAL_DRIVER
    STM32F407xx
)

# 这些参数描述 STM32F407 的真实编译环境，供 IntelliSense 正确解析 CMSIS/HAL。
target_compile_options(stm32f407_oop_index PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -ffreestanding
    -ffunction-sections
    -fdata-sections
)
