# 生产模块模板

仅当设计确实需要多个实现时使用本模板。请按目标仓库调整名称、错误类型、分配策略、格式和同步方式。

## 文件布局

```text
drivers/<module>/
    <module>.h
    <module>.c
    <module>_<backend>.h
    <module>_<backend>.c
board/
    <module>_board.c
platform/
    platform_<service>.h
    arch/<target>/<service>_board.c
tests/
    <module>_fake.c
    test_<module>.c
```

## 基类头文件

```c
struct module;

struct module_ops {
    int (*start)(struct module *self);
    int (*stop)(struct module *self);
    int (*process)(struct module *self, const void *input,
                   uint32_t input_size);
};

struct module {
    const struct module_ops *ops;
    const char *name;
    bool initialized;
};

int module_init(struct module *self, const char *name,
                const struct module_ops *ops);
int module_start(struct module *self);
int module_stop(struct module *self);
int module_process(struct module *self, const void *input,
                   uint32_t input_size);
```

## 基类初始化

```c
int module_init(struct module *self, const char *name,
                const struct module_ops *ops)
{
    if ((self == NULL) || (name == NULL) || (ops == NULL)) {
        return ERR_INVALID_ARG;
    }
    if ((ops->start == NULL) || (ops->stop == NULL) ||
        (ops->process == NULL)) {
        return ERR_INVALID_CONFIG;
    }

    self->ops = ops;
    self->name = name;
    self->initialized = true;
    return 0;
}
```

实现包含状态检查并直接传播错误的包装 API。

## 具体后端

```c
struct module_backend {
    struct module base;
    struct platform_device *device;
    uint32_t configuration;
};

static int backend_start(struct module *base)
{
    struct module_backend *self =
        container_of(base, struct module_backend, base);

    return platform_device_start(self->device, self->configuration);
}

static const struct module_ops backend_ops = {
    .start = backend_start,
    .stop = backend_stop,
    .process = backend_process,
};
```

具体初始化函数应校验后端依赖项，仅调用一次 `module_init`，然后保存具体配置。除非另一个板级组合模块必须实例化它，否则将其类型保持为私有。

## 目标验证

使用项目的 MCU 工具链和严格告警选项进行编译。验证板级初始化、必需操作校验、分派、错误传播，以及烧录后的真实外设行为。将仅目标端可观察到的结果记录到项目测试或调试文档中，而不要将演示代码嵌入驱动。
