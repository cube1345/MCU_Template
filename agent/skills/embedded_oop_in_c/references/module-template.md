# Production Module Template

Use this template only after the design requires multiple implementations.
Adapt names, error types, allocation policy, formatting, and synchronization to
the host repository.

## File Layout

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

## Base Header

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

## Base Initialization

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

Implement wrapper APIs with state checks and direct error propagation.

## Concrete Backend

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

The concrete init validates backend dependencies, calls `module_init` once, then
stores concrete configuration. Keep its type private unless another board-level
composition module must instantiate it.

## Target Verification

Compile with the project's MCU toolchain and strict warnings. Verify board
initialization, required-ops validation, dispatch, error propagation, and the
actual peripheral behavior after flashing. Keep target-only observations in the
project's test or debug documentation rather than embedding demo code in
drivers.
