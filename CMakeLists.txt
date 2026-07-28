cmake_minimum_required(VERSION 3.20)
project(MSPM0G3507_Template C)

# Keil is the authoritative firmware build.  This target gives editors and
# static tools the same, deliberately bounded source inventory.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB DRIVERLIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ti/driverlib/*.c")
file(GLOB FREERTOS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/source/*.c")
file(GLOB LAYER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/0_hal/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/2_device/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/3_board/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/4_facade/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/5_libs/*.c")

set(FREERTOS_PORT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/portable/heap_4.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/portable/port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/FreeRTOS_demo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/my_freetros_lib.c")

# imu_app.c is intentionally excluded until its removed imu_int boundary is
# restored.  The remaining app sources are the same examples compiled by Keil.
set(APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/6_apps/car_app.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/6_apps/usart_app.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/6_apps/xunji_app.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/6_apps/yuntai_app.c")

add_executable(${PROJECT_NAME}
    empty.c
    ti_msp_dl_config.c
    ${DRIVERLIB_SOURCES}
    ${FREERTOS_SOURCES}
    ${FREERTOS_PORT_SOURCES}
    ${LAYER_SOURCES}
    ${APP_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    __MSPM0G3507__
    MSPM0G3507)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/third_party/CMSIS/Core/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    "${CMAKE_CURRENT_SOURCE_DIR}/0_hal"
    "${CMAKE_CURRENT_SOURCE_DIR}/1_core"
    "${CMAKE_CURRENT_SOURCE_DIR}/2_device"
    "${CMAKE_CURRENT_SOURCE_DIR}/3_board"
    "${CMAKE_CURRENT_SOURCE_DIR}/4_facade"
    "${CMAKE_CURRENT_SOURCE_DIR}/5_libs"
    "${CMAKE_CURRENT_SOURCE_DIR}/6_apps"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/portable"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mid/source")

include(CTest)

if(BUILD_TESTING)
    add_executable(motor_device_test
        tests/motor_device_test.c
        2_device/motor_driver.c)
    target_include_directories(motor_device_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/1_core"
        "${CMAKE_CURRENT_SOURCE_DIR}/2_device")
    add_test(NAME motor_device_test COMMAND motor_device_test)
endif()
