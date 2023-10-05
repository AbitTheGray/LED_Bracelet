#include <esp_pm.h>
#include "config.hpp"

#ifdef TIMER_LOOP
#   include "ESP32_C3_TimerInterrupt.h"
#endif

#ifdef BLE_INTERFACE_ENABLE
extern BLECharacteristic* bleCharBatteryPercentage;
extern BLECharacteristic* bleCharBatteryCritical;

extern BLECharacteristic* bleCharButtonsBoot;

extern BLECharacteristic* bleCharEspCoreTemp;
#endif

#ifdef TIMER_LOOP
ESP32Timer Timer0(0);
bool IRAM_ATTR TimerHandler0(void* timerNo);
#endif

void setup()
{
#ifdef DEBUG
    Serial.begin(SERIAL_SPEED);
    do
    {
        delay(100);
    }
    while(!Serial && millis() < 5000);
#endif

    // LED Filament (PWM)
    {
        analogWriteResolution(8); // 256 values
        pinMode(PIN_FILAMENT_PWM, OUTPUT);

        // Turn OFF
        setFilamentBrightness(static_cast<uint8_t>(0));
    }

    // LED Strip
    {
        strip.setBrightness(STRIP_LED_BRIGHTNESS);

        // Turn OFF
        strip.setAllLedsColor(0);
        stripOffStartTime = millis();
    }

    // Battery Voltage (analog)
    {
        analogReadResolution(8);
        pinMode(PIN_BATTERY_VOLTAGE, ANALOG /* INPUT */);
    }

    // Mode Button
    {
        pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
        attachInterrupt(PIN_BUTTON_MODE, buttonPressInterrupt, FALLING);
        attachInterrupt(PIN_BUTTON_MODE, buttonReleaseInterrupt, RISING);

#ifdef CONFIG_PM_ENABLE
        // Sleep wakeup on button press
        gpio_wakeup_enable(static_cast<gpio_num_t>(PIN_BUTTON_MODE), GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
#   if PIN_BUTTON_MODE <= 5 // Required for Deep sleep
        esp_deep_sleep_enable_gpio_wakeup(1u << PIN_BUTTON_MODE, ESP_GPIO_WAKEUP_GPIO_LOW);
#   else
#       warning "Mode button won't wakeup the microcontroller from deep sleep"
#   endif
        esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);//TODO Is needed?
#endif
    }

    // Boot button
    {
        pinMode(PIN_BUTTON_BOOT, INPUT_PULLUP);
#ifdef BLE_INTERFACE_ENABLE
        attachInterrupt(
            PIN_BUTTON_BOOT,
            []()
            {
                if(bleCharButtonsBoot)
                {
                    bleCharButtonsBoot->setValue(static_cast<uint8_t>(1));
                    bleCharButtonsBoot->notify();
                }
            },
            FALLING
        );
        attachInterrupt(
            PIN_BUTTON_BOOT,
            []()
            {
                if(bleCharButtonsBoot)
                {
                    bleCharButtonsBoot->setValue(static_cast<uint8_t>(0));
                    bleCharButtonsBoot->notify(); //THINK Do we want 2nd notify?
                }
            },
            RISING
        );
#endif
    }

    // Sleep
    {
#ifdef CONFIG_PM_ENABLE
        esp_pm_config_esp32c3_t pm_config = {
#   ifdef CONFIG_CPU_MAX_FREQ
            .max_freq_mhz = CONFIG_CPU_MAX_FREQ,
#   else
            .max_freq_mhz = 160,
#   endif
#   ifdef CONFIG_CPU_MIN_FREQ
            .min_freq_mhz = CONFIG_CPU_MIN_FREQ,
#   else
            .min_freq_mhz = 10,
#   endif
#   if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = true
#   endif
        };
        esp_pm_configure(&pm_config);
#endif

#ifdef TIMER_LOOP
        if (Timer0.attachInterruptInterval(TICK_FREQUENCY_MS * 1000, TimerHandler0))
        {
            DEBUG_PRINT(F("Starting  ITimer0 OK, millis() = "));
            DEBUG_PRINTLN(millis());
        }
        else
        {
            DEBUG_PRINTLN(F("Can't set ITimer0. Select another freq. or timer"));
        }
#endif
    }

    DEBUG_PRINTLN(F("setup() finished"));
}
void IRAM_ATTR tick()
{
    auto currentTime = millis();

    // Bluetooth turn off
    {
#ifdef BLE_INTERFACE_ENABLE
        if(turnBluetoothOffAfterTime >= currentTime && isBluetoothOn())
        {
            deinitBluetooth();
        }
#endif
    }

    // Battery
    {
#ifdef BLE_INTERFACE_ENABLE
        auto batteryVoltage = getBatteryVoltage();
        auto batteryPercentage = guessBatteryPercentage(batteryVoltage);

        if(bleCharBatteryPercentage)
        {
            bleCharBatteryPercentage->setValue(static_cast<uint8_t>(batteryPercentage));
        }
        if(bleCharBatteryCritical)
        {
            uint8_t batteryCritical = batteryPercentage <= BATTERY_CRITICAL_PERCENTAGE ? 1 : 0;
            auto wasBatteryCriticalValue = bleCharBatteryCritical->getValue();
            uint8_t wasBatteryCritical = wasBatteryCriticalValue.size() == 0 ? 0 : wasBatteryCriticalValue[0];

            bleCharBatteryCritical->setValue(static_cast<uint8_t>(batteryCritical));
            if(wasBatteryCritical != batteryCritical)
            {
                bleCharBatteryCritical->notify();

#ifdef DEBUG
                if(batteryCritical)
                {
                    DEBUG_PRINTLN(F("Battery became critical"));
                }
                else
                {
                    DEBUG_PRINTLN(F("Battery no longer critical"));
                }
#endif
            }
        }
#endif
    }

    // Core temperature
    {
#ifdef BLE_INTERFACE_ENABLE
        if(bleCharEspCoreTemp)
        {
            bleCharEspCoreTemp->setValue(temperatureRead());
        }
#endif
    }

    // Sleep
    {
#ifdef CONFIG_PM_ENABLE
        if(canEnterDeepSleep())
        {
            DEBUG_PRINTLN(F("Entering deep sleep..."));
            esp_deep_sleep_start();
        }
#endif
    }
}
#ifdef TIMER_LOOP
bool IRAM_ATTR TimerHandler0(void* timerNo)
{
    tick();

    return true;
}
#endif
void loop()
{
#ifndef TIMER_LOOP
    tick();
    delay(LOOP_CHECK_DELAY_MS);
#endif
}
