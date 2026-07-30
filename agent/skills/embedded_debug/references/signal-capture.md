# Signal Capture

Chinese version: [signal-capture_zh.md](signal-capture_zh.md).

Use a logic analyzer for digital protocol timing and an oscilloscope for analog
integrity, voltage levels, ringing, rise/fall time, noise, or power behavior.

## Request a useful capture

Specify:

- Signals and physical reference/ground.
- Expected idle levels and protocol mode.
- Trigger condition tied to the symptom.
- Sample rate sufficient to resolve the shortest event of interest.
- Capture window before and after the trigger.
- Whether reset, chip select, interrupt, enable, or power-good should be included.
- Expected behavior and the observed deviation.

Suggested digital channels:

- SPI: SCK, MOSI, MISO, CS, and an optional IRQ/ready signal.
- I2C: SCL, SDA, plus reset/interrupt if relevant.
- UART: TX, RX, optional RTS/CTS and reset.
- PWM: PWM output, enable, fault, and feedback/trigger.
- Reset/startup: NRST, power-good, boot strap, clock, and a firmware marker GPIO.

Choose a sampling rate based on the fastest clock or shortest pulse, commonly
well above that rate (often at least 10x for useful digital inspection). Increase
it when edge placement or narrow glitches matter. A logic analyzer cannot prove
analog signal integrity.

## Correlate software and hardware

Add a nonintrusive marker GPIO or timestamp only when scoped and safe. Correlate
the waveform with serial logs, interrupts, DMA state, or debugger snapshots.
Remove temporary instrumentation after diagnosis unless it becomes a documented
diagnostic feature.

## Stop guessing

Request a capture when the open questions are physical timing, bus ownership,
missing edges, reset pulse width, chip-select sequencing, contention, or voltage
behavior. Source code describes intent; the waveform shows what the board did.
