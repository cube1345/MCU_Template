/**
 * @file    app_rtos.h
 * @brief   应用层 FreeRTOS 任务创建与调度器启动接口。
 * @author  weixin
 *
 * 本模块位于 application/core，集中完成本工程默认应用任务的创建与
 * FreeRTOS 调度器启动。具体任务逻辑不应放入 platform、device、driver
 * 或 board 层。
 */
#ifndef APP_RTOS_H
#define APP_RTOS_H

/**
 * @brief  初始化任务依赖并创建工程默认应用任务。
 * @return PLATFORM_OK 表示 UART1、驱动板对象及默认任务均已准备完成。
 * @return 其他 PLATFORM_* 表示依赖初始化或任务创建失败。
 * @note   任务通过 FreeRTOS heap_4 动态分配 TCB 和任务栈。
 * @note   串口监视器和各设备均由对应任务负责运行或启动。
 * @note   必须在调度器启动前调用；重复调用不会重复创建任务。
 */
int tasks_create(void);

/**
 * @brief  启动 FreeRTOS 调度器。
 * @return PLATFORM_ERR_INVALID_STATE 表示调度器意外返回。
 * @note   成功启动后此函数不会返回。
 * @note   必须在 tasks_create() 成功后调用，只能调用一次。
 */
int rtos_start(void);

#endif /* APP_RTOS_H */
