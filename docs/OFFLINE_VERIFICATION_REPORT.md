# Offline verification report — 2026-09-01

All work in this report was local and offline. No display/controller traffic,
flash operation, erase, unlock, option-byte change or ROM-loader entry occurred.

## Results

| Check | Result |
| --- | --- |
| Bafang protocol host tests | Pass |
| Simulator `.NET` build | Pass, 0 warnings and 0 errors |
| Historical direct-origin firmware link | Pass, vector at `0x08000000` |
| Quarantined legacy-offset target link | Pass, vector at `0x08005000` |
| Linker physical-end assertion | Pass, both layouts end at `0x08080000` |
| Serial/JTAG programming targets | Pass (confirmed disabled/fail-closed) |

The host tests cover the capture-derived polling sequence, speed checksum
vectors, current/status/nominal-battery parsing, malformed and truncated
responses, timeouts, link loss/recovery, and the invariant that every generated
request begins with read prefix `0x11`. Captured `0x16` controller-control
frames are retained only as disabled fixtures.

## Quarantined build artifacts

These artifacts prove only that the historical compatibility source compiles
and fits. **They are not safe or authorized to flash.**

| Artifact | Vector origin | `text` | `data` | `bss` | SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `apt_850c_historical-direct.bin` | `0x08000000` | 282916 | 5728 | 16076 | `AC4D0E64B64A26FA5A32AE0AF83AF8F81EB5B224F5143C9676F899B5CE6C20A8` |
| `apt_850c_gd32f303ret6_legacy-offset.bin` | `0x08005000` | 283012 | 5728 | 16076 | `7BF5C187FA8E6BD94728C63FD6DE1657C95C52A7AEA67B94983236607BB6295C` |

The size difference reflects target conditionals and relocation. Neither
layout establishes the E2.3 bootloader contract. The exact-target build still
uses the historical Cortex-M3/STM32F10x platform layer and unsafe 128 MHz clock
code, so it remains a compile-audit artifact only.

## Open release blockers

The authoritative list is in `FLASH_READINESS_CHECKLIST.md`. The critical
blockers are a verified factory recovery path, E2.3 bootloader layout/update
format, a native official GD32F303 startup/CMSIS/clock/peripheral port, and
controlled validation of still-unknown Bafang telemetry meanings.
