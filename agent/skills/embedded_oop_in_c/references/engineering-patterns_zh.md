# 工程模式

## 选择最小的模式

| 需求 | 模式 |
| --- | --- |
| 单个稳定且有状态的实现 | 带前缀的 API 加显式生命周期 |
| 可互换的实现 | 嵌入式基类加 const 操作表 |
| 应用必须忽略硬件选择 | 基类句柄加板级绑定 |
| 驱动必须跨 MCU/厂商 SDK | 平台接口加架构后端 |
| 多条总线/多个控制器 | 每个设备对象携带自身操作表 |
| MCU 验证 | 目标构建、烧录和板级冒烟测试 |
| 启动期发现 | 先使用显式注册表；只有合理时才用链接段 |

不要只为了让单个固定模块“看起来面向对象”而创建操作表。

## 模块边界与生命周期

- 成对使用 `<module>.h` 与 `<module>.c`；公共符号以 `<module>_` 为前缀。
- 用 `struct <module>` 保存状态，存储由调用方持有。
- 定义 `init`/`deinit` 行为、部分初始化处理、重试策略和有效调用状态。
- 文件私有的辅助函数和数据使用 `static`；查询接口使用 `const`。

## 基类、操作表与分派

```c
struct sensor;

struct sensor_ops {
    int (*start)(struct sensor *self);
    int (*stop)(struct sensor *self);
    int (*read)(struct sensor *self, int32_t *value);
};

struct sensor {
    const struct sensor_ops *ops;
    const char *name;
    bool initialized;
};
```

基类只包含公共状态。后端专有的引脚、句柄和寄存器属于具体类型。应暴露 `sensor_read()` 这类包装 API，而不是让应用代码直接调用 `ops`；包装层集中执行状态校验、加锁、跟踪和错误策略。

初始化时校验必需操作。实现元数据应保持不可变：

```c
static const struct sensor_ops sensor_spi_ops = {
    .start = sensor_spi_start,
    .stop = sensor_spi_stop,
    .read = sensor_spi_read,
};
```

对可选行为，返回项目明确规定的“不支持”错误，或采用有文档说明的默认行为。不要静默返回成功。

## 嵌入与转型

```c
struct sensor_spi {
    struct sensor base;
    struct platform_spi_device *device;
    uint8_t data_reg;
};
```

默认将 `base` 放在首成员；用 `&spi_sensor.base` 向上转型。仅在对应的具体操作中向下转型：

```c
static int sensor_spi_read(struct sensor *base, int32_t *value)
{
    struct sensor_spi *self =
        container_of(base, struct sensor_spi, base);

    return platform_spi_read_reg(self->device, self->data_reg, value);
}
```

使用仓库中带检查的 `container_of` 宏。不要从基类指针向子类指针做裸强制转换，也不要向该宏传入带副作用的表达式。

## 板级绑定

将具体实例和硬件连线保存在组合模块中：

```c
static struct sensor_spi board_temp_sensor;
struct sensor *g_temperature_sensor;

int sensor_board_init(void)
{
    int rc;

    rc = sensor_spi_init(&board_temp_sensor, "temp0",
                         platform_spi_get("spi1"), 0x20U);
    if (rc != 0) {
        return rc;
    }
    g_temperature_sensor = &board_temp_sensor.base;
    return 0;
}
```

应用模块仅依赖基类 API 和暴露的基类句柄。较大的系统应优先使用组合/上下文对象，而不是许多全局变量。

## 平台边界

当驱动必须跨越 MCU 或厂商 SDK 边界时，采用此方向：

```text
application -> device driver -> platform interface -> arch/vendor backend -> HAL/registers
```

平台层应表达领域操作，而不是复制整套厂商 HAL。在单 MCU 项目中，具体驱动可以直接调用生成的 HAL；但 HAL 类型仍不得出现在基类和应用 API 中。单一全局分派器仅适用于真正的单例；存在多个设备时，应将操作表和上下文绑定到每个设备对象。

## 注册与初始化调用

顺序重要时优先使用显式启动调用。模块需要被发现时使用注册表。只有在工具链/链接脚本受控，且 LTO 与段垃圾回收行为已验证时，才使用链接段初始化调用。

## 并发、所有权与错误

记录每个对象/缓冲区的所有者、回调生命周期/上下文、允许调用的线程/ISR、阻塞/分配行为、同步方式及清理顺序。使用项目的同步原语，而不是只依赖 `volatile`。保留第一个有意义的错误，并以资源获取的逆序回滚。
