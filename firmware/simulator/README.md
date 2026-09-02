# 850C desktop firmware simulation

This Windows application provides an interactive 320 x 480 preview of the
planned 850C instrumentation firmware. It models ride telemetry and calculates
SOC, power, trip energy, efficiency, range, peaks and communication age live.

## Run from VS Code

Open the `firmware` folder, then run **Terminal > Run Task > Run 850C UI preview**.
The first run builds the app. Saving `mainscreen-850.c` automatically reloads
the preview.

## Controls

| Preview control | Keyboard | Simulated 850C action |
| --- | --- | --- |
| Previous page | Left arrow | Cycle backward through dashboard pages |
| Power / next page | `P` or right arrow | Cycle through Riding, Battery, Diagnostics and Graphs |
| Up | Up arrow | Increase assist or move up in configuration |
| Down | Down arrow | Decrease assist or move down in configuration |
| Enter | `Enter` | Open/confirm the selected setting; same as short Power in a menu |
| Back | `Esc` | Stop editing or leave configuration; similar to long Power |
| Config | `C` | Simulate the Power + Up + Down long-press configuration shortcut |

The right-side controls change speed, voltage, current, temperature, throttle,
brake and battery type. The target default is the confirmed 48 V / 17.5 Ah /
840 Wh pack; its full and empty voltage settings remain editable because the
actual pack endpoints have not been verified. Freeze UART to watch live fields become STALE after
0.5 seconds and LOST after 2 seconds. Error injection maintains a small
occurrence-counted history. Reset Trip clears energy, distance and all peaks.

The buttons at the top of the preview perform the same actions.

## Run from a terminal

```powershell
dotnet run --project simulator/ColorLcdSimulator.csproj
```

## Scope

This is a behavioral UI prototype, not an MCU emulator. It does not execute the
GD32/STM32 firmware, LCD driver, bootloader or UART byte parser. Bafang fields
whose availability is not verified can be switched to N/A. Simulator behavior
must be ported to and separately validated in the embedded firmware before use.
