# APT 850C source audit and implementation plan

## Scope and safety status

This audit covers the repository's `firmware/860C_850C` application target. The
target builds, but the resulting image is **not yet qualified for installation**
on the confirmed Green Pedel-branded APT 850C hardware revision E2.3. The V5.2
label used in earlier notes is superseded by the display's Product Information
screen. No display was flashed.

The supplied UI requirement describes 320 x 240. This source target defines a
320 x 480 portrait framebuffer (`DISPLAY_WIDTH` 320, `DISPLAY_HEIGHT` 480), so
new layouts must use 320 x 480 unless physical identification proves otherwise.

## Architecture

| Area | Principal modules | Responsibility |
| --- | --- | --- |
| Startup/link | `startup_stm32f10x_md.s`, `stm32_flash.ld`, `main.c` | vectors, memory layout, optional bootloader offset |
| Board support | `pins.c`, `adc.c`, `timers.c`, `rtc.c`, `uart.c`, `usart1.c`, `eeprom-hw.c` | GPIO, timers, UART, RTC and external EEPROM |
| LCD | `lcd.c`, `ugui_driver/ugui_display_8x0c.c` | controller setup and 16-bit parallel LCD writes |
| Graphics/UI | `common/src/ugui.c`, `fonts.c`, `screen.c`, `mainscreen*.c`, `configscreen.c` | drawing, fonts, dashboard and settings |
| Application state | `common/src/state.c`, `common/include/state.h` | controller exchange, decoded state, trips and energy |
| Protocol boundary | `common/protocol/*` | allocation-free common telemetry and protocol adapters |

The legacy `state.c` still owns TSDZ2 packet sequencing and parsing. The first
abstraction stage publishes its decoded state through `ebike_telemetry_t`; moving
packet decoding behind protocol-specific interfaces is a later, deliberately
small refactor.

## Hardware facts encoded by the source

- CPU/toolchain target: Cortex-M3 using STM32F10x medium-density SPL interfaces;
  comments also refer to GD32F103 hardware. The exact fitted E2.3 MCU is unknown.
- Linker memory declaration: 512 KiB flash at `0x08000000`, 64 KiB RAM at
  `0x20000000`. Linker comments instead claim STM32F103VB with 128 KiB flash and
  20 KiB RAM. This contradiction must be resolved from the physical MCU marking.
- Application-with-bootloader origin: `0x08004000` (16 KiB offset), proven by
  a complete APT Burn Tools virtual transfer of the confirmed factory image. The new work
  does not alter this address or vector relocation.
- LCD: 320 x 480, 16-bit GPIO data bus. Runtime identification includes ILI9481
  and ST7796 initialization paths. The V5.2 panel/controller ID is unverified.
- UART: USART1, PA9 TX / PA10 RX, 19200 baud, 8 data bits, no parity, 1 stop bit.
  Legacy buffers are 88-byte TX and 29-byte RX.
- Buttons: up PC11, power PC12, down PA15.
- Other 850C pins: system power PC1; backlight PA7; LCD reset PC6, command PC3,
  chip select PC4, write PC5, read PC7, 16-bit data bus on GPIOB; EEPROM interface
  PC8/PC9; USB detect/control PA3.

## Protocol and telemetry findings

**2026-08-31 correction:** the rejecting-stub conclusion in the historical
paragraphs below is superseded. Public controller-side implementations document
the Bafang display request/response shapes. The repository now has a strict,
read-only client for status, current, battery percent, speed and moving. It does
not contain controller writes and is not yet selected by the embedded UART
transport. See `../../docs/850C_BBS02B_COMPATIBILITY.md` for the current audit.

The target motor is now confirmed as a Bafang BBS02B, 48 V, 750 W. The
implemented controller protocol is still TSDZ2-specific. It uses `0x59` framed
ALIVE, STATUS, PERIODIC, CONFIGURATION and firmware-version messages. Repository
names containing “Bafang” describe display hardware and are not evidence of a
Bafang motor-controller packet decoder.

Existing state already supplies speed, voltage, current, calculated power, Wh,
Wh/km, trip data, assist level, brake, error and conditionally motor temperature
or throttle. Existing battery percentage is consumption/capacity based rather
than a configurable voltage curve. There was no stale/lost UART state.

The new common telemetry structure uses fixed-point integer units and validity
bits, avoiding floats and allocation. Its link state becomes STALE after 0.5 s
and LOST after 2.0 s without a valid receive. The TSDZ2 adapter is active. The
Bafang decoder is now a strict, read-only client for the publicly documented
status, current, battery-percent, speed and moving response formats.

To validate and activate Bafang support, obtain:

1. Timestamped UART captures for power-on, idle, wheel rotation, each PAS level,
   braking, throttle, and a known error condition, including voltage levels,
   baud, polarity and wire direction.

The public sources support read-parser development but do not establish every
stock BBS02B field or the E2.3 electrical layer. Unknown fields must remain
invalid and render as `--`; they must not be inferred
from unrelated bytes.

## Reproducible build baseline

Validated with GNU Arm Embedded GCC 7.3.1 and GNU Make on Windows. From
`firmware/860C_850C/src`:

```text
make clean
make -j4
make clean
make -j4 DISPLAY_VERSION=850C_BOOTLOADER
```

After the telemetry abstraction:

| Image | text | data | bss (includes reserved stack) | total RAM | binary size |
| --- | ---: | ---: | ---: | ---: | ---: |
| 850C direct | 274,860 | 5,716 | 59,280 | 64,996 | 284,257 bytes |
| 850C bootloader | 274,964 | 5,716 | 59,280 | 64,996 | 284,361 bytes |

The abstraction costs 480 bytes of text and 24 bytes of RAM versus the audited
baseline. With a 64 KiB linker region, only 540 bytes remain. Stack safety and
the unusually large existing BSS require map/stack analysis before adding pages
or history. If the device truly has the 20 KiB RAM stated in the linker comments,
this image is incompatible regardless of the new changes.

Current inherited warnings include an implicit declaration of `RTC_GetCounter`
and incomplete enum handling in switch statements. They should be removed before
a release build. No new protocol-module warnings were observed.

## Incremental implementation plan

Each stage must build both direct and bootloader-offset 850C variants and record
text/data/BSS deltas.

1. **Compatibility gate:** identify MCU marking and flash/RAM, read LCD ID using a
   non-destructive method, preserve a recoverable stock image, identify the exact
   controller, and capture/verify its UART protocol.
2. **Protocol boundary:** move TSDZ2 frame parsing behind its adapter without
   changing behavior; feed UI/application code only common telemetry; add parser
   tests on host-recorded frames.
3. **Bafang protocol:** implement only verified frames, checksum and timeouts;
   retain TSDZ2 as a build/runtime option.
4. **Battery/energy services:** configurable 36/48/52 V piecewise SOC curves,
   optional BMS override, fixed-point Wh integration, minimum-distance Wh/km,
   remaining-energy/range validity, and peak/reset behavior.
5. **Dashboard pages:** implement 320 x 480 Riding, Battery/Efficiency and
   Diagnostics pages using dirty-field updates and the existing button/state
   conventions. Render unavailable/stale data explicitly.
6. **Warnings/history/configuration:** temperature threshold where valid, small
   RAM error history (no frequent flash writes), and configurable data fields.
7. **Release validation:** warning-free clean builds, binary/map report, simulator
   navigation tests, captured-frame replay, power-cycle/settings tests, then a
   separately approved hardware recovery/install procedure.

## Compatibility risks still open

- Exact V5.2 MCU, flash and RAM are unknown and conflict with repository metadata.
- LCD controller, panel orientation and board pinout are not proven for V5.2.
- APT bootloader version, accepted image framing/checksum and recovery path are
  unknown; application offset alone is not sufficient proof of compatibility.
- The connected Bafang controller model and UART dialect are unknown.
- Electrical levels/direction must be measured before attaching capture hardware.
- Existing RAM headroom is inadequate for careless buffers or full-screen state.
