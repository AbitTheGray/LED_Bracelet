#include "config.hpp"

unsigned long filamentOffStartTime = 0;

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(STRIP_LED_COUNT, PIN_STRIP_DIGITAL, 0, TYPE_GRB);
unsigned long stripOffStartTime = 0;

#ifdef BLE_INTERFACE_ENABLE
extern BLECharacteristic* bleCharLedFilamentBrightness;
#endif

float getBatteryVoltage()
{
    // 256 = 2^8 steps/resolution
    // 3.3 to convert it to voltage
    // 4/3 to revert voltage divider
    //return static_cast<float>(analogRead(PIN_BATTERY_VOLTAGE)) / 255.0f * 3.3f * 4 / 3.0f;
    return static_cast<float>(analogRead(PIN_BATTERY_VOLTAGE)) * 4 * 3.3f / (3.0f * 255u);
}
uint8_t guessBatteryPercentage(float voltage)
{
    if(voltage >= BATTERY_VOLTAGE_MAX)
        return 100;
    if(voltage <= BATTERY_VOLTAGE_MIN)
        return 0;
    return static_cast<uint8_t>((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN) * 100); // map BATTERY_VOLTAGE_MIN - BATTERY_VOLTAGE_MAX to 0 - 100
}
void setFilamentBrightness(uint8_t value255)
{
    analogWrite(PIN_FILAMENT_PWM, value255);
    if(value255 == 0 /* Off */)
    {
        if(filamentOffStartTime == 0 /* Not already off */)
            filamentOffStartTime = millis();
    }
    else
        filamentOffStartTime = 0;

#ifdef BLE_INTERFACE_ENABLE
    // Set value of BLE characteristic
    bleCharLedFilamentBrightness->setValue(value255);
#endif
}
bool IRAM_ATTR canEnterDeepSleep()
{
    if(filamentMode != -1)
        return false;
    if(filamentOffStartTime + OUTPUT_OFF_SLEEP_TIME <= millis())
        return false;

    if(stripMode != -1) // We could, in theory, ignore this as the color is controlled by LED's IC.
        return false;
    if(stripOffStartTime + OUTPUT_OFF_SLEEP_TIME <= millis())
        return false;

#ifdef BLE_INTERFACE_ENABLE
    if(isBluetoothOn())
        return false;
#endif
    return true;
}
