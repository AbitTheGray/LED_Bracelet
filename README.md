# LED Bracelet

Single-cell LED Bracelet controlled by ESP32-C3.

While the bracelet/collar can work in "standalone" mode, it is recommended to connect it to other device using [BLE](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy).

![PCB Preview](Preview.png)

**This board is untested.**

Designed to be manufactured by [JLCPCB](https://jlcpcb.com/) as 4layer PCB with both inner layers acting as GND (you can probably use just the outer layers).

Requires [Espressif's KiCad Libraries](https://github.com/espressif/kicad-libraries).

## Features

- 5V LED strip
  - Designed for `WS2812` LEDs (=digital signal)
- 5V LED filament (outline)
  - Most of those filaments are 3V so use two of them, 2.5V should be enough
  - Can use PWM for brightness control
- USB-C charging
  - 5V 1.5A max (5.1k resistors)
  - `TP4056` charger
    - Status LEDs
      - Red = Charging
      - Blue = Standby
    - 1S 1A charging (1 cell in series, unlimited in parallel)
  - 1.5A discharge (`DW01A`)
  - Voltage measurement
    - 3/4 of voltage
    - Max 4.4V (=don't connect to 5V / USB)
- ESP32-C3 controller
  - BLE controls
  - Mode button to cycle colors
  - BOOT jumper pads (instead of a button)

### Warnings

- There are no reverse-polarity protections
- There are no fuses, you are responsible for maximum current on 5V output(s)
  - 1.2A shared between LED Strip and LED Filament connectors
    - max 20 RGB LEDs as `20*3*20mA = 1.2A` (`3` as the current is for each LED of RGB)
- Highlighted traces on silkscreen show voltages to measure (for testing)
- Some components are not in component placement
  - Use any `1206` LEDs (recommended colors are on silkscreen)
  - Mode button may need an uncommon component
  - Any THT connectors (battery + pin headers)
- Some components may need rotation tweak for fabrication
  - All `U` and `Q` components may need a rotation change (align 1st pin with long line on silkscreen)
  - `L1` needs 90deg rotation

### Possible future features

- Gyro+accelerometer to measure hand movement (available through BLE)
- Dedicated BLE button (BOOT button?) + status LED

## TODO

- Implement logic for `BLUETOOTH_ON_NO_CLIENT_WAIT`
  - Wait X seconds after turning the BLE ON, then turn it OFF if no client connected in the meantime
- Turn bluetooth OFF on critical battery level (and prevent it from turning on)
  - Compile define to disable this behavior
  - Add "What happens on Battery Critical?" section to [FAQ](FAQ.md)
- Compile define to independently disable Deep Sleep
