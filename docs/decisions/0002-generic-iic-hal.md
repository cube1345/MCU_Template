# ADR-0002：IIC HAL 使用实例注入，隔离 SysConfig

## 状态

Accepted

## 日期

2026-08-03

## 背景

原 `0_hal/hal_iic.c` 通过 `I2C_0_INST`、`I2C_1_INST`、总线速率宏和 IRQ 宏
直接选择硬件实例。这样 HAL 只能在当前 SysConfig 配置下编译；关闭某个 I2C
实例会因为生成宏消失而失败，也让可复用的 HAL 与当前板级资源耦合。

## 决策

- IIC HAL 的公共 API 接收 `I2C_Regs *`、显式总线速率和可选时间回调，不再接收逻辑端口号，
  也不包含 `ti_msp_dl_config.h`。
- HAL 内部仅使用可覆盖的通用实例槽位容量 `HAL_IIC_MAX_INSTANCES` 保存运行时状态，
  该容量不对应任何具体外设编号或 SysConfig 符号。
- `3_board/iic_board.c` 负责把 `I2C_0_INST`、`I2C_1_INST`、IRQ 号和 SysConfig
  生成的速率宏绑定到 HAL，并在实例不存在时跳过对应代码。
- IIC IRQ 入口保留在 Board 层，再通过 `hal_iic_irq_handler()` 转发到 HAL 状态机。
- 未注册或未配置的实例由 HAL 返回 `HAL_IIC_STATUS_ERROR_PARAM`，不引入空的
  SysConfig 符号。
- `1_core/device_iic.h` 定义 `device_iic_t`：设备以**组合**方式保存 transport、
  context 和 7 位地址；C 没有适用于该场景的结构体继承。OLED、ICM42688 和
  MPU6050 均通过同一 endpoint 契约访问总线。
- `3_board/iic_board.c` 提供硬件 IIC transport；`3_board/oled_board.c` 提供软件
  IIC transport。两者实现相同的 `device_iic_ops_t`，Device 层不区分总线实现。

## 备选方案

### 在 HAL 内保留 `I2C_0_INST` 查表

实现改动小，但继续把 HAL 锁定到当前板子的 SysConfig 输出，关闭外设时仍会产生
编译错误；不接受。

### 在 HAL 内使用动态分配的总线对象

可以消除静态槽位，但会把动态内存带入 RTOS/ISR 相关路径，增加生命周期和实时性
风险；不接受。当前实现使用固定的实例注册槽位，硬件句柄仍由 Board 显式传入。

## 后果

- HAL 可在没有 I2C SysConfig 实例宏的情况下独立编译。
- Board 文件承担少量板级绑定与 IRQ 配置，并向设备注入 IIC endpoint。
- 新增 I2C 控制器时只需增加 Board 绑定，不需要修改 HAL 状态机。
- 新增 IIC 设备时只需在 Board 层构造 `device_iic_t`，Device 层不应重新实现
  IIC 端口、地址或 GPIO 位操作。
- 当前 Keil 工程和 CMake 源清单必须同时包含新增的 `iic_board.c`。
