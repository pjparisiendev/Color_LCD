# APT 850C E2.3 flash-readiness checklist

> **CURRENT VERDICT: FAIL / DO NOT FLASH**

This checklist targets the physically identified Green Pedel APT 850C E2.3
with GD32F303RET6 and factory software 2.0B-V2. A passing offline build is not
permission to program hardware.

| Gate | Status | Evidence / remaining work |
| --- | --- | --- |
| Exact MCU and memory | Pass | GD32F303RET6 marking; 512 KiB flash and 64 KiB SRAM |
| Non-destructive SWD identification | Partial | DP/AP identification succeeded; protected AHB memory reads failed safely |
| Factory recovery image | Pass | Green Pedel supplied and confirmed `850C-Jialuo-Revbike-28inch48V99k-IAP.bin` for this HMI; SHA-256 is recorded |
| APT bootloader boundary | Pass (offline) | A complete virtual APT Burn Tools transfer of the confirmed factory image starts at `0x08004000` and advances in 2 KiB blocks; physical E2.3 programming remains prohibited |
| Factory recovery procedure | **Fail** | Exact E2.3 updater, wiring, acceptance checks and recovery behavior have not been safely verified |
| Bootloader update format/acceptance | **Fail** | Not verified on E2.3; no updater transmission is authorized |
| Native GD32F303 startup/CMSIS layer | **Fail** | Firmware still uses the historical STM32F10x compatibility layer |
| Clock configuration | **Fail** | Historical code requests 128 MHz, above the confirmed 120 MHz MCU maximum |
| Interrupt/vector audit | **Fail** | Historical STM32F10x vector table not compared against official GD32F303xE startup |
| Peripheral/flash-controller audit | **Fail** | GPIO, DMA, USART, ADC, RTC and FMC compatibility not fully verified |
| LCD controller and board pinout | Partial | ILI9481/ST7796 paths exist; E2.3 LCD identity and all pins are not runtime-confirmed |
| Bafang UART electrical/protocol | Partial | 1200 8N1 and passive traffic confirmed; loaded current scaling and status meanings remain open |
| Safe default traffic | Pass (unit level) | Scheduler only emits captured `0x11` reads; all observed `0x16` writes are suppressed |
| Host protocol tests | Pass | Capture vectors, malformed data, timeouts, recovery and no-write invariant tested |
| Simulator build | Pass | Desktop simulator builds without warnings or errors |
| Hardware-in-loop validation | **Blocked** | Requires recovery first; no firmware is to be loaded yet |

## Release rule

Do not produce or label any artifact as flashable until every **Fail** is
resolved with reproducible evidence, the factory firmware can be restored, and
the direct and bootloader-offset linker layouts are independently verified.
Physical programming, erase, unlock, option-byte changes and unverified
controller commands remain prohibited.
