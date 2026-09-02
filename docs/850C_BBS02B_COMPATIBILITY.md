# Green Pedel APT 850C E2.3 / BBS02B compatibility

> **STATUS: DEVELOPMENT ONLY — NOT SAFE TO FLASH**

## Confirmed target

| Item | Confirmed value | Evidence |
| --- | --- | --- |
| Display | APT TFT 850C, Green Pedel branding | Physical display and startup screen |
| Display hardware | E2.3 | Product Information screen |
| Installed software | 2.0B-V2 | Product Information screen |
| Motor/controller family | Bafang BBS02B UART | Motor marking and installed system |
| Motor rating | 48 V, 750 W | `BBS02B 48V 750W`, serial `2107190733` |
| Battery | Li-ion, 48 V, 17.5 Ah, 840 Wh | Battery label |
| MCU | GigaDevice GD32F303RET6, marking `CKST3A / JJ2026`, U2 | Direct PCB inspection |
| MCU resources | Cortex-M4, LQFP64, 512 KiB flash, 64 KiB SRAM, 120 MHz maximum | GigaDevice device specification plus exact RET6 marking |
| Display geometry | 3.2 inch, 320 x 480, three physical buttons | Original OpenSourceEBike 850C hardware documentation |
| RTC/USB | Battery-backed timekeeping hardware and USB charger present | Original 850C hardware documentation and matching source facilities |

840 Wh is the default configurable energy capacity. Full and empty voltage
remain configurable. A 54.6 V endpoint is plausible for 13S but is not treated
as confirmed for this pack.

## Local source map

| Concern | Source | Finding |
| --- | --- | --- |
| Earlier 850C hardware work | `Bafang_color_LCD_850C/Bafang_LCD_850C_firmware` | Board research, APT bootloader material and hardware artifacts; not proof of E2.3 identity |
| Combined application | `firmware/860C_850C` | Active 850C/860C firmware and UI base |
| CPU flags | `firmware/860C_850C/src/Makefile` | `STM32F10X_MD`, Cortex-M3, Thumb, GNU Arm toolchain |
| Startup | `firmware/860C_850C/src/startup_stm32f10x_md.s` | Medium-density STM32F10x vector/startup file |
| Linker | `firmware/860C_850C/src/stm32_flash.ld` | Declares 512 KiB flash and 64 KiB RAM despite stale 128/20 KiB header comment |
| Bootloader relocation | `firmware/860C_850C/src/main.c` | `NVIC_SetVectorTable(..., 0x4000)` for bootloader builds, matching the captured official updater target |
| GPIO | `firmware/860C_850C/src/pins.h` | 850C system power PC1; buttons PC11/PC12/PA15; USART1 PA9/PA10 |
| UART | `firmware/860C_850C/src/usart1.c` | Existing TSDZ2 transport is 19200 baud; Bafang candidate is 1200 baud and must be selected as a distinct backend |
| LCD | `firmware/860C_850C/src/lcd.c` and `ugui_driver/ugui_display_8x0c.c` | 16-bit GPIO bus and runtime ILI9481/ST7796 paths |
| Protocol boundary | `firmware/common/protocol` | Common fixed-point telemetry, TSDZ2 backend and new read-only Bafang parser |
| Local voltage ADC | `firmware/860C_850C/src/adc.c` | PA4 / ADC channel 4, 12-bit conversion; source assumes a 200 kOhm / 10 kOhm divider but E2.3 population and calibration remain unverified |

## Confirmed GD32F303RET6 target

The primary build identity is `APT_850C_GD32F303RET6`; it must not be called a
GD32F103RET6 target. Physical memory is now resolved:

- flash `0x08000000`, length 512 KiB;
- SRAM `0x20000000`, length 64 KiB;
- stack top `0x20010000`;
- expected memory-density word `0x00400200`.

The board marking is `311107 KC-A / 20190011 / 94V-0 2045`, MCU designator U2,
and B1 is the visible RTC coin cell. A read-only SWD check of `0xE0042000` and
`0x1FFFF7E0` remains desirable corroboration but no longer gates the capacity
selection.

### Historical compatibility-layer audit

The current source is not yet a native F303 platform layer:

| Area | Current implementation | F303 disposition |
| --- | --- | --- |
| Compiler/assembly CPU | `-mcpu=cortex-m3`; startup `.cpu cortex-m3` | Replace/validate with Cortex-M4, without enabling an FPU ABI |
| CMSIS device | STM32F10x medium-density headers and vector names | Compare every used IRQ/register against official GD32F303 definitions |
| Clock | Explicit 8 MHz x16 = 128 MHz | Invalid for 120 MHz-rated F303; replace with verified 120 MHz configuration |
| `SystemCoreClock` | Hardcoded 128 MHz | Must match the verified F303 clock tree |
| Timers | TIM3/TIM4 prescalers assume 128 MHz timer clock | Recalculate after the F303 clock tree is selected |
| Flash driver | STM32F10x SPL interface and latency model | Replace/validate against GD32F30x FMC and wait-state requirements |
| Startup/vector table | STM32F10x medium-density table | Replace/compare with official GD32F303xE startup vector table |
| Peripherals | STM32F10x SPL GPIO/DMA/USART/ADC/RTC | Audit register and interrupt compatibility; do not assume pin compatibility proves register compatibility |

No DSP/FPU instruction or hard-float ABI will be enabled merely because the
core is Cortex-M4. The existing 128 MHz configuration must not be used on the
physical display.

## Datasheet-established GD32 identification

For a GD32F10x device, the factory-programmed read-only memory-density word is
at `0x1FFFF7E0`: SRAM KiB occupies bits 31:16 and flash KiB occupies bits 15:0.

| Candidate | Expected word | SRAM | Flash | Package |
| --- | --- | ---: | ---: | --- |
| GD32F103RCT6 | `0x00300100` | 48 KiB | 256 KiB | LQFP64 |
| GD32F103RET6 | `0x00400200` | 64 KiB | 512 KiB | LQFP64 |

This provides a read-only corroboration of the identified RET6 memory target.
PA13 is SWDIO, PA14 is SWCLK, PA9 is USART0 TX and PA10 is USART0 RX at the
candidate MCU. PCB test pads must still be traced to those pins.

The confirmed RET6 capacity matches the linker's 512 KiB/64 KiB sizes. It does
not validate the historical CPU/startup/clock layer, E2.3 pin mapping or the APT
bootloader boundary.

The GD32 immutable system-memory ROM loader and the APT flash-resident loader
are separate mechanisms. The ROM loader must not be entered until BOOT pin and
display power behavior are understood. The APT loader is the code presumed to
occupy the reserved main-flash prefix.

Original 850C boards document the five SWD pads from left to right as GND, NRST,
SWCLK, SWDIO and 3.3 V. This is a candidate map for E2.3, not permission to wire
by position alone: trace SWCLK/SWDIO to PA14/PA13 and verify ground/reference
voltage first. The initial session is limited to debug ID, `0x1FFFF7E0` and,
optionally, read-only protection status.

### Actual E2.3 SWD result

The physical E2.3 connection has now superseded the legacy candidate order:

| E2.3 pad | Verified function |
| ---: | --- |
| 1 | 3.3 V target reference; not used to power the target |
| 2 | SWCLK |
| 3 | SWDIO |
| 4 | NRST |
| 5 | GND |

With the display powered normally, an XDS110 over CMSIS-DAP/SWD established a
stable 10 kHz link and returned DPIDR `0x2BA01477`, DP CTRL/STAT `0xF0000040`,
AHB-AP IDR `0x24770011`, and AP CSW `0x23000052`. The AP rejected the TAR/CSW
operation required to read target memory. Consequently CPUID, memory density,
vectors, peripheral registers and flash could not be read. No unlock, erase,
programming or option-byte operation was attempted.

This behavior is consistent with debug/AHB protection. Removing GD32 security
protection may mass-erase main flash, so SWD is no longer considered a viable
factory-backup path for this unit. The exact report and command log are stored
at `C:/PJ/Bike/850C_Backup/READ_ONLY_IDENTIFICATION_REPORT.md`.

## External connector and APT updater

Original OpenSourceEBike 850C documentation uses a 3.3 V USB-to-UART adapter on
the normal five-pin connector to reach the APT flash-resident bootloader. The
documented functions are Battery+, switched power/Vin, GND, controller TX and
controller RX. This establishes classic-board precedent, but E2.3 voltage and
pin identity must be measured before connecting PC hardware. Battery+ and
switched power must never be connected to a USB UART logic pin.

## LCD controller evidence

ILI9481 and ST7796S both natively support 320 x 480 and an 8080-style 16-bit MCU
interface. Existing detection first reads ILI9481 command `0xBF` and accepts ID
bytes `94 81`; it then checks the ST7796 path and retains an Unknown result when
neither matches. Both paths remain enabled.

The legacy `ugui_SSD1963.c` filename is historical. SSD1963 is an external
framebuffer/controller for a separate RGB panel and is not treated as evidence
of the physical APT 850C controller.

## Battery sag compensation

The existing firmware already estimates open-circuit voltage as loaded voltage
plus `I * R`, using filtered current and configurable/estimated pack resistance.
It separately adds `I^2 * R` pack/cable loss to energy accounting. This is the
correct sign when reconstructing resting voltage from a discharging loaded
measurement; subtracting `I * R` would estimate the loaded voltage from a known
open-circuit value.

The improved SOC estimator should retain three separate inputs:

1. load-compensated voltage for the voltage/SOC curve;
2. configurable or conservatively learned pack resistance; and
3. cumulative Wh integration for consumption-based SOC and range confidence.

PA4/ADC channel 4 and the source's 200 kOhm/10 kOhm divider are still board
assumptions until verified on E2.3. No TSDZ2 motor-configuration field may be
carried into the BBS02B backend merely because the battery estimator is shared.

## Link and bootloader analysis

### Green Pedel recovery candidate

Green Pedel directly supplied and confirmed for this HMI
`C:/PJ/Bike/850C-Jialuo-Revbike-28inch48V99k-IAP.bin`. Read-only inspection
found:

- length 485,188 bytes (`0x76744`), which fits the 507,904-byte region after a
  16 KiB bootloader;
- SHA-256
  `3a5393174ba87481fc34aa00a23767a86add4ef580ea90cb438841eb552cfa63`;
- a Cortex-M-style vector table at file offset zero;
- initial MSP `0x200005C8` and reset vector `0x0801EDC5`;
- a completed APT Burn Tools virtual transfer writes byte zero to `0x08004000`
  and advances in fixed 2 KiB blocks, directly proving the updater origin and
  raw-image format without accessing physical hardware.

Green Pedel's confirmation resolves the recovery-image identity for this HMI.
The virtual updater test proves the official tool's image framing and address
contract, but not yet the protected physical E2.3 bootloader's electrical or
write behavior. The factory file must remain unmodified.

- Direct image flash origin and vector table: `0x08000000`.
- Bootloader-compatible application origin and vector table: `0x08004000`.
- Reserved prefix: `0x4000`, or 16,384 bytes.
- Linker-declared bootloader application region: `0x08004000` through the end
  of a presumed 512 KiB bank (`0x08080000` exclusive).
- A raw `main.bin` has no leading 16 KiB padding; the bootloader-target binary's
  byte zero corresponds to linked address `0x08004000`.
- The build changes the linker symbol `USE_WITH_BOOTLOADER` and runtime vector
  relocation. It does not prove that E2.3 uses this boundary or accepts the
  resulting binary.
- The linker header says STM32F103VB/128 KiB/20 KiB while the actual MEMORY
  block says 512 KiB/64 KiB. The latter governs the build. This contradiction is
  release-critical.

No linker address, option byte or protection setting was changed.

## Public protocol evidence

Primary implementation references:

- `danielnilsson9/bbs-fw`, Bafang Display Protocol wiki
- `Youmayu/BBS-Open-Firmware`, `wiki/Bafang-Display-Protocol.md` and
  `src/firmware/extcom.c`

These implement the controller side: the display sends a request and the
controller responds. They establish packet shapes useful to a display client.
The Youmayu firmware intentionally repurposes some standard display fields;
those repurposed meanings are not assumed for a stock BBS02B.

| Command | Request | Response | Validation / scaling | Direction | Status |
| --- | --- | --- | --- | --- | --- |
| Status | `11 08` | `SS` | Exactly 1 byte; stock capture held `01` while stopped and pedaling, so bit meanings remain unknown | read-only | Implemented as raw diagnostic only |
| Current | `11 0A` | `AA CC` | `CC=AA`; current is `AA/2` A; reject over 100 A | read-only | Implemented |
| Nominal battery field | `11 11` | `PP CC` | `CC=PP`; stock returns `63/64` even at about 50.4 V, so it must not drive SOC | read-only | Implemented as raw diagnostic only |
| Speed | `11 20` | `HH LL CC` | Stock capture confirms `CC=HH+LL+0x20`; converted using configured circumference | read-only | Implemented, response validated |
| Unknown 1 | `11 21` plus unresolved request byte | implementation returns zeros | Request length/stock meaning unresolved | read-only | Not sent |
| Range | `11 22 33` | `HH LL CC` | Request is capture-confirmed; stock response meaning remains unverified and is not published | read-only | Polled, value quarantined |
| Calories | `11 24` plus unresolved request byte | `HH LL CC` | Open firmware repurposes this as voltage x10; not assumed for stock BBS02B | read-only | Not sent |
| Unknown 3 | `11 25` plus unresolved request byte | implementation returns zeros | Meaning unresolved | read-only | Not sent |
| Moving | `11 31` | `MM CC` | Appears in public documentation but did not occur in the factory capture | read-only | Not sent |
| PAS | `16 0B ...` | controller write | Changes assist | write | Deliberately absent |
| Work mode | `16 0C ...` | controller write | Changes/requests mode | write | Deliberately absent |
| Lights | `16 1A ...` | controller write | Changes lights/mode depending firmware | write | Deliberately absent |
| Speed limit | `16 1F ...` | controller write | Controller setting | write | Deliberately absent |

### Passive stock-link capture

A receive-only capture of the controller-to-HMI wire on the intact factory
system confirmed 1200 baud, 8N1 and the recurring stock response sequence. The
XDS110 TX connection was physically removed; RX was tapped through 4.7 kOhm.
No data was transmitted.

The captured request block is speed (`11 20`), current (`11 0A`) and status
(`11 08`), followed by one rotating slow slot: lights (`16 1A F1`), PAS
(`16 0B 0B 2C`), speed limit (`16 1F 02 E0 17`), nominal battery (`11 11`) or
range (`11 22 33`). The safe backend preserves that cadence but suppresses all
three `0x16` slots, so it can only emit `0x11` reads. Dynamic
speed frames including `00 14 34`, `00 A0 C0` and `00 BE DE` all satisfy the
protocol's `HH+LL+0x20` checksum. Current responses `00 00`, `01 01` and
`02 02`, and battery responses `63 63` and `64 64`, confirm the duplicated
one-byte additive checksums used by those fields.

A stationary capture taken at approximately 50.4 V returned `64 64`
continuously. The field therefore numerically says 100 but is not established
as an accurate voltage-derived state of charge on this system. No 50.4 V value
appears in the captured controller responses, supporting use of the display's
local voltage measurement for voltage/SOC work.

The status byte remained `01` during both long zero-speed, zero-current
intervals and a separately labeled pedaling capture. It therefore is not a
usable pedaling flag on this stock controller. Brake status remains to be
captured. The request opcodes and the optional moving response are not proven
by this one-direction capture.
Detailed evidence is in
`C:/PJ/Bike/850C_Backup/PASSIVE_BAFANG_CAPTURE_ANALYSIS.md`.

A later passive capture of HMI TX confirmed the factory request side. Every
poll block contains `11 20` (speed), `11 0A` (current), and `11 08` (status).
The fourth frame rotates through `16 1A F1`, `16 0B 0B 2C`,
`16 1F 02 E0 17`, `11 11`, and `11 22 33`; each occurs about once per 2.8
seconds. The optional `11 31` moving request does not appear. Repeated throttle
operation produced no change in the HMI transmissions, consistent with a
throttle connected directly to the controller.

Not exposed by the verified read set: throttle percentage, PAS level, light
state, motor/controller temperature, true range, and direct battery voltage.
These fields remain invalid (`--`) unless exact E2.3+BBS02B traffic establishes
a standard command/response. Status can expose brake and a controller status or
fault code, but it is not a throttle percentage or temperature channel.

## Read-only parser behavior

`bafang_protocol.c` cycles through status, current, battery percent, speed and
moving requests. It correlates each untagged response with the outstanding
opcode, checks exact response size, checksum and plausible range, and publishes
fixed-point values through `ebike_telemetry_t`.

Communication states are NOT_INITIALIZED, WAITING_FOR_RESPONSE, ACTIVE, TIMEOUT
and LOST. A request times out after 250 ms; three consecutive timeouts produce
LOST. Timeout invalidates current, power, temperature, brake and controller
status so stale values are not silently presented. The module contains no
`0x16` request builder.

The existing embedded runtime still selects its legacy TSDZ2 transport. Enabling
the Bafang backend requires a separate 1200-baud transport scheduler and real-bike
electrical/baud validation. It must not be achieved by mixing Bafang parsing into
the 19200-baud TSDZ2 state machine.

## Evidence still required

| Risk | Exact evidence needed |
| --- | --- |
| MCU and memory | Resolved by direct marking as GD32F303RET6/512 KiB/64 KiB; SWD corroboration blocked by protected AHB access |
| PCB compatibility | E2.3 board photos or continuity measurements confirming every power, LCD, button, UART, EEPROM and backlight pin |
| LCD variant | Runtime LCD ID or visible controller marking proving ILI9481/ST7796 and correct orientation |
| Boot boundary | Resolved offline: the confirmed image and complete official-tool virtual transfer prove raw image base `0x08004000`; physical bootloader behavior remains untested |
| Recovery | Exact Green Pedel E2.3 Bafang 2.0B-V2 factory image plus tested APT recovery procedure; SWD readback is blocked and must not be unlocked |
| UART electrical layer | Oscilloscope/logic-analyzer measurement of voltage, idle polarity and baud before attaching USB UART hardware |
| Protocol behavior | Passive, bidirectional captures at startup, PAS changes, pedaling, throttle, brake, lights, pages/settings and safe errors |

Sniffer wiring must share ground and observe Green (controller to display) and
Yellow (display to controller) through suitable logic-level inputs. Battery Red
and controller-power Blue must never be connected to a USB UART logic pin.

Until every release-critical row is resolved, all binaries are **NOT SAFE TO
FLASH**. Do not mass erase, unlock read protection, change option bytes, overwrite
the bootloader, modify the motor controller, or flash the display.

## Local build result

Built with GNU Arm Embedded GCC 7.3.1 and GNU Make. No new warning originates in
the Bafang protocol module. The direct build still reports inherited warnings in
legacy code: missing `RTC_GetCounter` declarations, incomplete enum switches and
one mixed `&&`/`||` precedence suggestion.

| Development artifact | Linked origin | text | data | bss | Binary bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `850C_E2.3_BBS02B_DEV-NOT-SAFE-TO-FLASH-direct.bin` | `0x08000000` | 282,916 | 5,728 | 16,076 | 292,993 |
| `850C_E2.3_BBS02B_DEV-NOT-SAFE-TO-FLASH-bootloader.bin` | `0x08005000` | 283,012 | 5,728 | 16,076 | 293,089 |

For the linker-assumed 512 KiB/64 KiB device, the bootloader image consumes
288,740 bytes of loadable text+data (57.3% of the 492 KiB application region)
and 21,804 bytes of data+BSS (33.3% of 64 KiB RAM). This is only a linker-model
calculation, not proof that E2.3 contains those resources.
