# 850C compatibility contract

This file is the acceptance checklist for the Green Pedel APT 850C E2.3
firmware and desktop simulator. A feature is not considered preserved until its
embedded behavior, persistence, simulator behavior and hardware dependency are
all accounted for.

Status values:

- **implemented** - present in the embedded application and build-tested
- **partial** - some behavior exists but does not yet match the APT contract
- **planned-local** - safe to implement entirely within the display
- **gated** - must remain read-only/unavailable until hardware or protocol proof
- **not-applicable** - optional hardware is confirmed absent (none confirmed yet)

## Factory user interface

| Capability | Status | Compatibility requirement |
| --- | --- | --- |
| Power on/off by long Power | implemented | Preserve system-power GPIO behavior |
| PAS up/down including level 0 | implemented | Clamp to configured controller-supported levels |
| Trip/ODO/range/time/max/average cycling | partial | Preserve all modes; range requires valid estimator/controller data |
| Headlight long-Up and day/night scene | partial | Existing light command/backlight exists; verify Bafang command |
| Up+Down temporary-data reset | partial | Reset trip, riding time, average and maximum values together |
| Settings by double-Power | partial | Existing entry gesture differs; add compatibility gesture |
| Menu blocked while moving | planned-local | Require zero/fresh speed before controller-affecting settings |
| Menu 30-second timeout/exit on movement | planned-local | Never leave controller edit pending |
| Metric/imperial | implemented | Apply consistently to every distance/speed/range field |
| Day and night brightness | implemented | Existing on/off brightness values; clarify labels |
| Auto power-off 1-9/off | partial | Existing 0-255 minute value; constrain compatibility UI |
| Clock/date and 12/24-hour format | partial | Hours/minutes exist; date/seconds/format missing |
| Battery display voltage/percent/off | implemented | Percent must identify estimated versus BMS value |
| Power display watts/current | partial | Values exist; factory selection missing |
| Startup password/PIN | planned-local | Store salted verifier or protected PIN; preserve recovery path |
| Wheel 14-30 inch | implemented | Firmware stores circumference, compatibility UI may expose diameter |
| Battery 24/36/48/52 V | partial | Target default is 48 V / 840 Wh; voltage endpoints remain configurable; 24 V preset still required |
| USB output on/off | gated | Verify a controllable GPIO and board circuit first |
| Light sensor on/off/sensitivity | gated | Verify sensor population, ADC/GPIO and polarity first |
| Factory reset | implemented | Must reset only owned application settings, not bootloader data |
| Product information | partial | Firmware version exists; hardware/date/serial acquisition missing |
| Battery/BMS information | gated | Show N/A unless verified BMS frames supply each field |
| Error display and documented meanings | partial | Current error exists; mapping/history still incomplete |

## APT advanced/controller settings

These settings are controller commands, not ordinary display preferences. They
must be visible as **Unavailable - protocol not verified** until the exact Bafang
controller protocol supplies a confirmed read and write transaction. No value
may be guessed from TSDZ2 fields.

| Setting | APT documented range | Status |
| --- | --- | --- |
| Advanced-menu PIN | default 1919 | planned-local |
| Speed limit | 15-60 km/h | gated |
| Current limit | 6-50 A | gated |
| Motor pole count | 0-10 | gated |
| Start-after-poles | 1-3 | gated |
| Speed-sensor direction | yes/no | gated |
| Throttle 6 km/h mode | yes/no | gated |
| Throttle follows assist level | yes/no | gated |
| Start mode | Power/ECO/Standard | gated |
| Assist-level scheme | 3/5/9/controller default | gated |
| Key anti-jam | yes/no | gated |

## Enhanced instrumentation

| Capability | Status |
| --- | --- |
| Riding, Battery/Efficiency and Diagnostics pages | simulator implemented; embedded partial |
| Fixed-point common telemetry and validity flags | implemented |
| UART OK/STALE/LOST | implemented in telemetry layer and simulator |
| Voltage SOC presets/custom curve/BMS override | simulator partial; embedded planned |
| Trip Wh, stable Wh/km and remaining-energy range | simulator implemented; embedded partial |
| Peak speed/power/current/temperature | simulator implemented; embedded partial |
| Recent error history without frequent flash writes | simulator implemented; embedded planned |
| Configurable dashboard fields | embedded legacy partial; new pages planned |

## Truthfulness rules for the simulator

1. Every simulator setting must map to an embedded setting or be labelled
   `Prototype only`.
2. Every hardware/protocol-gated setting must render `Unavailable` by default.
3. Invalid, stale or unsupported telemetry renders `--` or `N/A`.
4. The simulator may replay verified UART captures but may not invent packets.
5. Embedded defaults, ranges, units, reset behavior and menu gestures are test
   cases shared with the simulator implementation.

## Release gates

- Identify the exact MCU marking, flash/RAM, LCD ID and populated optional parts.
- Validate the public Bafang UART read implementation with bidirectional E2.3
  captures; require exact proof for every controller-writing option.
- Preserve the capture-verified 16 KiB bootloader application offset and prove recovery workflow.
- Build direct and bootloader targets with recorded map/stack headroom.
- Pass a feature-by-feature simulator/embedded conformance checklist.
- Do not flash until the user separately approves a verified recovery procedure.
