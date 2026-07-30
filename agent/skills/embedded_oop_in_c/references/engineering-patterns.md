# Engineering Patterns

## Select the Smallest Pattern

| Need | Pattern |
| --- | --- |
| One stable stateful implementation | Prefixed API plus explicit lifecycle |
| Interchangeable implementations | Embedded base plus const ops |
| Application must ignore hardware choice | Base handles plus board binding |
| Driver must cross MCU/vendor SDKs | Platform interface plus arch backend |
| Multiple buses/controllers | Per-device object carrying its own ops |
| MCU verification | Target build, flash, and board-level smoke test |
| Startup discovery | Explicit registry first; linker section only when justified |

Do not create an ops table merely to make a single fixed module look object
oriented.

## Module Boundary and Lifecycle

- Pair `<module>.h` and `<module>.c`; prefix public symbols with `<module>_`.
- Use `struct <module>` for state and caller-owned storage.
- Define `init`/`deinit` behavior, partial-init handling, retry policy, and
  valid call states.
- Keep file-private helpers and data `static`; keep queries `const`.

## Base, Ops, and Dispatch

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

The base contains only common state. Backend-specific pins, handles, and
registers belong in the concrete type. Expose wrapper APIs such as
`sensor_read()` rather than letting application code call `ops` directly;
wrappers centralize state validation, locking, tracing, and error policy.

Validate required operations during init. Keep implementation metadata immutable:

```c
static const struct sensor_ops sensor_spi_ops = {
    .start = sensor_spi_start,
    .stop = sensor_spi_stop,
    .read = sensor_spi_read,
};
```

For optional behavior, return the project's explicit unsupported error or use a
documented default. Do not silently return success.

## Embedding and Casting

```c
struct sensor_spi {
    struct sensor base;
    struct platform_spi_device *device;
    uint8_t data_reg;
};
```

Keep `base` first by default; upcast with `&spi_sensor.base`. Downcast only in
the matching concrete operation:

```c
static int sensor_spi_read(struct sensor *base, int32_t *value)
{
    struct sensor_spi *self =
        container_of(base, struct sensor_spi, base);

    return platform_spi_read_reg(self->device, self->data_reg, value);
}
```

Use the repository's checked `container_of` macro. Do not use raw casts from a
base pointer to a child pointer, and do not pass side-effect expressions to the
macro.

## Board Binding

Keep concrete instances and hardware wiring in a composition module:

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

Application modules depend only on the base API and the exposed base handle.
For larger systems, prefer a composition/context object instead of many globals.

## Platform Boundaries

Use this direction when a driver must cross MCU or vendor-SDK boundaries:

```text
application -> device driver -> platform interface -> arch/vendor backend -> HAL/registers
```

The platform layer expresses domain operations rather than copying a whole
vendor HAL. In a single-MCU project, a concrete driver may call the generated
HAL directly; HAL types must still remain out of base and application APIs. A
single global dispatcher is acceptable only for a true singleton; for multiple
devices, bind ops and context to each device object.

## Registration and Initcall

Prefer explicit startup calls when order matters. Use registration tables when
modules are discoverable. Use linker-section initcalls only with controlled
toolchains/linker scripts and verified behavior under LTO and section GC.

## Concurrency, Ownership, and Errors

Document who owns each object/buffer, callback lifetime/context, permitted
thread/ISR callers, blocking/allocation behavior, synchronization, and cleanup
order. Use project synchronization primitives, not `volatile` alone. Preserve
the first meaningful error and unwind incrementally acquired resources in
reverse order.
