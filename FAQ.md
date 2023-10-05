# Frequently Asked Questions

## How many LEDs can I connect as an LED Strip?

There is 1.2A limit on 5V boost converter which is shared between LED Strip and LED Filament connectors.
If you leave LED Filament empty and have 60mA total RGB LED (like `WS2812`), you can have only 20 LEDs running at full brightness white color at the same time.

## Which LED Strips can I use?

The LED Strip connector (3pin) is designed for indexable 5V LED strips of `WS2812` but any other 1-wire 5V LEDs should work.
Don't use LED Strips of other voltages (like 12V).

If you have fixed-color LED Strips, you can use LED Filament connector (2pin) and control them using a PWM signal.

Always check maximum current!

## Why are Mode and BOOT buttons separated?

This is due to Mode button being used to wake up from sleep which is possible only on `GPIO0` - `GPIO5` but BOOT button must be on `GPIO9`.
It could also be a problem with Boot Mode switching on waking up from sleep (maybe, not tested).

## Why isn't BOOT a real button?

There is not enough space.
Even with shifted components, there is not enough space for 2 buttons.
They cannot be close to each other to prevent users from mixing them up.
BOOT button cannot be "hidden" or have different orientation as it can cause hard-to-figure-out problems when pressed by its environment.

## Can I enable/disable Bluetooth support?

Yes.

In fact, there are several options you can easily tweak.
In `platformio.ini` there are `build_flags` which you can comment-out using `;` at the beginning of the line.

| Flag                   | Recommended | Effect                                                                                                 |
|------------------------|:-----------:|--------------------------------------------------------------------------------------------------------|
| `DEBUG`                |     OFF     | Enables `Serial` output (default speed `115200`, `SERIAL_SPEED`).                                      |
| `TIMER_LOOP`           |     ON      | Uses timer for checking several things (like battery voltage, possibility of sleep...).                |
| `BLE_INTERFACE_ENABLE` |     ON      | Enables Bluetooth support. Mode button will act slightly different when Bluetooth is disabled by this. |
| `CONFIG_PM_ENABLE`     |     ON      | Enables ESP Power Management (variable CPU frequency) and allows it to enter (deep) sleep.             |
| `CONFIG_CPU_MIN_FREQ`  |     40      | Minimum dynamic frequency, requires `CONFIG_PM_ENABLE` to work.                                        |
| `CONFIG_CPU_MAX_FREQ`  |     160     | Maximum dynamic frequency, requires `CONFIG_PM_ENABLE` to work.                                        |

Do not comment out `CONFIG_FREERTOS_USE_TICKLESS_IDLE` and `CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP`, those do not have an effect without `CONFIG_PM_ENABLE` if you really need them OFF.

## How do I change Bluetooth name/passkey?

Both name and passkey (or password) are hard-coded in the software.
To change them you need to alter `BLE_NAME` and `BLE_PASSKEY` values in `config.hpp`.
If you do not want to have any passkey (not recommended) use value `0`.

You need to recompile the software.
Make sure you have Bluetooth enabled when compiling.

## Can I change BLE Service/Characteristic UUID?

Yes.

Look at the beginning of `ble.cpp` file.
Battery service+characteristics are from official Bluetooth GATT, the rest is random v4 UUIDs (per-service) first manually defined first part (4 bytes) of the UUID.

## Can I use any ESP microprocessor?

No.

Both the PCB and software are made for `ESP32-C3`.
`ESP32-C3-WROOM-02` for the PCB to be more specific (and its `ESP32-C3-WROOM-02U` variant with an external antenna).

In case you would like to port this project, you would need to:
- Change PCB or create WROOM-compatible daughter board
- Change `PIN_`* defines in `config.hpp`
- Check Mode Button initiation (`setup()` in `main.cpp`) as you need a button there which can wake up the ESP from deep sleep (there is ESP32-S3 specific check)

## Which battery should I use?

The PCB is designed for any Li-Ion battery (up to 4.25V, chargeable by `TP4056`) which can provide at least 2A (lower in theory if you have less LEDs).
See "Maximum discharge current" in your battery's specification.

## Which LED strips are supported on the connector?

The PCB and software are designed for `WS2812` / `WS2812B` LEDs.
Other protocol-compatible LEDs should work.

Maximum supported LEDs is 20 (due to current limit, constant `STRIP_LED_COUNT` in `config.hpp`).
Decreasing this count may allow you to change colors a little bit faster (not needed at all).

Do not increase `STRIP_LED_COUNT` unless you really know what you are doing as you will go over maximum current.
If you must, you should either use only one RGB component (red, green or blue) or modify `STRIP_LED_BRIGHTNESS` to limit maximum current.

## How do I change colors/brightness modes?

Filament brightnesses are defined in `filamentBrightnesses` array with `FILAMENT_BRIGHTNESSES` entries.
LED Strip colors are defined in `stripColors` array with `STRIP_COLORS` entries.

Both arrays have values defined in `config.cpp`.
There is no "black"/OFF value in those arrays, index `-1` is used instead (and hard-coded).
