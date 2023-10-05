#pragma once
#include <Arduino.h>

#define SERIAL_SPEED 115200
#ifdef DEBUG
#   define DEBUG_PRINT(a_msg) Serial.print(a_msg)
#   define DEBUG_PRINTLN(a_msg) Serial.println(a_msg)
#else
#   define DEBUG_PRINT(a_msg)
#   define DEBUG_PRINTLN(a_msg)
#endif

/// Checks prerequisites to enter deep sleep.
/// In addition to this make sure the IC is idle for some time so it does not fall asleep between button presses.
bool canEnterDeepSleep();
/// Number of milliseconds to wait before going into deep sleep when all outputs are OFF.
#define OUTPUT_OFF_SLEEP_TIME (1000 * 60) /* 1 minute */
/// Frequency of ticks tor repeated logic.
#define TICK_FREQUENCY_MS (1000 * 1) /* 1s */

#define PIN_FILAMENT_PWM 4
#define FILAMENT_BRIGHTNESSES 2
extern const uint8_t filamentBrightnesses[FILAMENT_BRIGHTNESSES];
/// Index into `filamentBrightnesses`.
/// Can be `-1` for OFF.
extern int filamentMode;
/// Start time when LED Filament was turned OFF.
/// Ignored when Bluetooth is connected.
/// Add OUTPUT_OFF_SLEEP_TIME to this value when comparing.
extern unsigned long filamentOffStartTime;
/// Set filament brightness.
/// 0 = OFF
/// 255 = ON
void setFilamentBrightness(uint8_t value255);
/// Set filament brightness.
/// Use value in range <0,1>.
inline void setFilamentBrightness(float value01) { setFilamentBrightness(static_cast<uint8_t>(value01 * 255u)); }

#include "Freenove_WS2812_Lib_for_ESP32.h"
#define STRIP_LED_COUNT 20
#define STRIP_LED_BRIGHTNESS 0xFF /* 100% (values in `stripColors` represent actual colors) */
#define PIN_STRIP_DIGITAL 7
extern Freenove_ESP32_WS2812 strip;
#define STRIP_COLORS 8
/// Colors to cycle through using Mode button
extern const uint32_t stripColors[STRIP_COLORS];
/// Index into `stripColors`.
/// Can be `-1` for OFF.
extern int stripMode;
/// Start time when LED Strip was fully turned OFF.
/// Ignored when Bluetooth is connected.
/// Add OUTPUT_OFF_SLEEP_TIME to this value when comparing.
extern unsigned long stripOffStartTime;

#define PIN_BATTERY_VOLTAGE 5 /* 3/4 of real voltage */
/// Reads analog battery voltage.
/// Returns value between 0 and 4.4V.
float getBatteryVoltage();
/// Rough guess of battery percentage from its voltage.
/// Not accurate at all but enough for this purpose.
uint8_t guessBatteryPercentage(float voltage);
/// Minimum battery voltage, equivalent to 0%.
#define BATTERY_VOLTAGE_MIN 3.5f
/// Maximum battery voltage, equivalent to 100%.
#define BATTERY_VOLTAGE_MAX 4.2f
/// Battery will be considered at "Critical Low" when percentage reaches below this value.
#define BATTERY_CRITICAL_PERCENTAGE 15

#define PIN_BUTTON_BOOT 9 /* Ground = Pressed */

#define PIN_BUTTON_MODE 3 /* Ground = Pressed */


#ifdef BLE_INTERFACE_ENABLE
#   include "NimBLEDevice.h"
/// Initialize Bluetooth and allow devices to connect.
void initBluetooth();
/// Disconnect all devices and disable Bluetooth.
void deinitBluetooth();
/// Retruns true if Bluetooth is currently enabled and initialized.
inline bool isBluetoothOn() { return BLEDevice::getInitialized(); }
/// Retruns true if Bluetooth is currently enabled, initialized and there is any device connected to it.
inline bool isBluetoothConnected() { return BLEDevice::getInitialized() && BLEDevice::getClientListSize() > 0; }
/// Bluetooth Device name.
#   define BLE_NAME "LED Bracelet"
/// Passkey to connect using Bluetooth.
/// Change to 0 to disable.
#   define BLE_PASSKEY 8018
/// How long to wait for clients before turning Bluetooth OFF.
#   define BLUETOOTH_ON_NO_CLIENT_WAIT 60 /* seconds */
/// Time in milliseconds.
/// If not 0 the nBluetooth should be turned off after this time is reached.
extern unsigned long turnBluetoothOffAfterTime;
#else
inline void initBluetooth() {}
inline void deinitBluetooth() {}
inline bool isBluetoothConnected() { return false; }
#endif

/// Internal function.
/// Called when Mode button is pressed.
/// Don't put any logic there, use `onButtonPress` which identifies short and long press (and discards invalid presses).
void buttonPressInterrupt();
/// Internal function.
/// Called when Mode button is pressed.
/// Don't put any logic there, use `onButtonPress` which identifies short and long press (and discards invalid presses).
void buttonReleaseInterrupt();
/// Mode button function:
/// - Bluetooth connected or ON:
///   - Short press: Notify BLE characteristic
///   - Long press: Disconnect and turn OFF Bluetooth
/// - Bluetooth off:
///   - Short press: Cycle between LED strip colors with 3 sets of strip brightness (first all modes with strip off, then all modes 100%, then all modes at 100%)
///     - For colors see `stripColors`, for filament values see `filamentBrightnesses`
///   - Long press: Turn Bluetooth ON
void onButtonPress(bool longPress);
