# Confirmed Green Pedel 850C target

This is the authoritative target configuration for local firmware and simulator
work. It supersedes assumptions inferred from case labels or related 860C
hardware.

## Confirmed from the equipment

- HMI: Green Pedel-branded APT TFT 850C
- HMI hardware version: E2.3
- Installed HMI software: 2.0B-V2
- Controller link: Bafang UART
- Motor: Bafang BBS02B, 48 V, 750 W
- Battery: Li-ion, 48 V, 17.5 Ah, 840 Wh
- Existing startup branding: Green Pedel
- MCU: GD32F303RET6 (`CKST3A / JJ2026`), U2
- MCU resources: Cortex-M4, LQFP64, 512 KiB flash, 64 KiB SRAM, 120 MHz maximum
- Display class: 3.2 inch, 320 x 480, three physical buttons
- Original 850C hardware includes persistent RTC support and a USB charger

## Application defaults

- Nominal energy: 840 Wh (`8400` in the existing Wh x10 setting)
- Simulator profile: 48 V, 840 Wh
- Full and empty voltage endpoints remain user-configurable. The simulator's
  54.6 V and 39.0 V endpoints are provisional 13S defaults, not verified pack
  specifications.

## Unverified release-critical facts

- Remaining GD32F303 peripheral audit: EEPROM/flash and final
  board-pin confirmation
- LCD controller ID, panel timings, pinout and backlight polarity on E2.3
- Physical bootloader behavior and a tested recovery procedure
- Exact byte-level behavior and electrical parameters of this E2.3/BBS02B pair
- Battery voltage-to-SOC curve and actual charged/empty endpoints

The GD32 target now bypasses the inherited TSDZ2 transmitter. Its Bafang
runtime can emit only the five read requests observed in the passive factory
capture, at the confirmed 1200-baud 8N1 link settings. Responses are strictly
validated; ambiguous fields are quarantined. Controller-writing settings remain
unavailable.

## Safety status

Local compilation and simulator testing are allowed. Do not flash, alter linker
or bootloader addresses, or claim E2.3 compatibility until all release-critical
hardware and recovery gates above are resolved.

The confirmed physical memory is flash `0x08000000`/512 KiB and SRAM
`0x20000000`/64 KiB with stack top `0x20010000`. When SWD becomes available,
read `0x1FFFF7E0` before any erase, unlock or write; the corroborating expected
value is `0x00400200`.

The exact target name is `APT_850C_GD32F303RET6`. It uses the GD32F30x startup,
CMSIS device layer, Cortex-M4 soft ABI and a 120 MHz system clock. The historical
Cortex-M3/STM32F10x path remains isolated for legacy 850C/860C builds.

Classic OpenSourceEBike 850C documentation describes a five-pad SWD row ordered
GND, NRST, SWCLK, SWDIO, 3.3 V and a 3.3 V USB-UART connection to the normal
five-pin display connector for the APT bootloader. Treat both as candidate E2.3
information: trace/measure before connecting. The initial SWD session remains
read-only.

Actual E2.3 tracing/testing supersedes that SWD order: pad 1 is 3.3 V reference,
pad 2 SWCLK, pad 3 SWDIO, pad 4 NRST and pad 5 GND. SWD DPIDR and AHB-AP ID were
stable, but protected AHB access rejected target-memory addressing. Do not
unlock: protection removal may mass-erase the factory firmware. Recovery must
come from an exact vendor image or a proven non-destructive APT UART path.
# Controller transport safety

The target still defines `CONTROLLER_TX_LOCKED` to disable the inherited TSDZ2
transmitter. A separate hard whitelist permits only captured read frames:
`11 20`, `11 0A`, `11 08`, `11 11`, and `11 31`. The three observed `0x16`
factory control frames are deliberately suppressed.

`11 11` is parsed as battery percentage after checksum and range validation.
`11 20` remains validated raw speed only; no km/h conversion is made. `11 31`
publishes a checksummed moving flag. `11 22` and `11 24` are disabled. Every
field has an independent stale timer. The captured `11 08` response remains a
single status byte because the real Green Pedel/BBS02B capture is stronger
evidence than the generic status-plus-checksum description. The inherited
TSDZ2 publisher is excluded so it cannot overwrite Bafang telemetry.

# Offline verification status

- Official-tool virtual upload accepts the custom application at `0x08004000`.
- Current ELF: `.isr_vector=0x08004000`, `_estack=0x20010000`, Cortex-M4.
- GD32 target and legacy 850C target compile successfully.
- Windows UI simulator builds with no warnings or errors.
- GD32 settings persistence is intentionally disabled: the target cannot erase
  or program its emulated-EEPROM flash pages until the native FMC path and
  reserved address range are verified.
- The linker now excludes `0x0807F000-0x0807FFFF` from the GD32 application,
  and the STM32 flash driver is absent from that target. The confirmed Green
  Pedel application ends at `0x0807A744`, which is consistent with—but does not
  conclusively prove—those final two pages being available for settings.
- `tools/verify_gd32_image.ps1` automatically checks the application vector,
  stack top, Bafang runtime, read-only TX whitelist and maximum binary size.
- The LCD is driven by a 16-bit GPIO/8080 bus rather than EXMC memory mapping.
  The GD32 target now uses an isolated native GPIO implementation and preserves
  SWD while releasing JTAG pins PB3/PB4. The inherited 850C pin routing and
  actual E2.3 LCD controller still require physical confirmation.
- The GD32 target now has an isolated native RTC/backup-domain path. It reuses
  an existing factory RTC source without resetting the backup domain, only
  selects LXTAL when no source exists, uses bounded startup waits and configures
  a 32,767 divider for a one-second tick. Legacy targets retain their old RTC.
- GD32 early startup now sets VTOR natively, preserves PA13/PA14 for SWD and
  releases the window-watchdog reset through the GD32 RCU layer. Compile-time
  checks reject overlapping input/output assignments or use of SWD pins.
- The PC1 display-power latch is driven high before its output mode is enabled,
  and power retention now occurs before ADC initialization. A requested 0%
  backlight correctly produces a zero PWM compare instead of being clamped to
  5%.
- This remains a development build and is **not safe to flash** until the
  remaining physical LCD/pin/boot/recovery checks are completed.
