#include "config.hpp"

int filamentMode = -1;
int stripMode = -1;

#ifdef BLE_INTERFACE_ENABLE
extern BLECharacteristic* bleCharButtonsMode;
#endif

void nextFilamentMode()
{
    filamentMode++;
    if(filamentMode >= FILAMENT_BRIGHTNESSES)
    {
        filamentMode = -1 /* off */;
        setFilamentBrightness(static_cast<uint8_t>(0));

        DEBUG_PRINTLN(F("Switched filament OFF"));
    }
    else // filament has valid predefined brightness
    {
        setFilamentBrightness(filamentBrightnesses[filamentMode]);

        DEBUG_PRINT(F("Switched filament to mode "));
        DEBUG_PRINTLN(static_cast<uint32_t>(filamentMode));
    }
}
void onButtonPress(bool longPress)
{
#ifdef DEBUG
    if(longPress)
    {
        DEBUG_PRINTLN(F("Short button press"));
    }
    else
    {
        DEBUG_PRINTLN(F("Long button press"));
    }
#endif


#ifdef BLE_INTERFACE_ENABLE
    if(isBluetoothConnected())
    {
        if(longPress)
        {
            deinitBluetooth();
        }
        else
        {
            bleCharButtonsMode->notify();
        }
    }
    else
#endif
    {
        if(longPress)
        {
#ifdef BLE_INTERFACE_ENABLE
            // Turn bluetooth ON
            initBluetooth();

            // Set a timer to turn bluetooth off after X seconds to save battery
            turnBluetoothOffAfterTime = millis() + (1000 * BLUETOOTH_ON_NO_CLIENT_WAIT);
#else
            nextFilamentMode();
#endif
        }
        else // short press
        {
            stripMode++;
            if(stripMode >= STRIP_COLORS)
            {
                stripMode = -1;
                strip.setAllLedsColor(0);

                if(stripOffStartTime == 0 /* Not already off */)
                    stripOffStartTime = millis();

                DEBUG_PRINTLN(F("Switched LED Strip OFF"));

#ifdef BLE_INTERFACE_ENABLE
                nextFilamentMode();
#endif
            }
            else // strip has valid predefined color
            {
                strip.setAllLedsColor(stripColors[stripMode]);
                stripOffStartTime = 0;

                DEBUG_PRINT(F("Switched LED Strip to mode "));
                DEBUG_PRINTLN(static_cast<uint32_t>(stripMode));
            }
        }
    }
}

unsigned long buttonPressStart = 0;

void IRAM_ATTR buttonPressInterrupt()
{
    buttonPressStart = millis();
}
void IRAM_ATTR buttonReleaseInterrupt()
{
    if(buttonPressStart == 0)
        return;
    /// How long was the button held.
    /// 1000 = 1s
    auto buttonPressTime = millis() - buttonPressStart;
    if(buttonPressTime < 50) // 0.05s
    {
        // Nothing
        // Too short, probably a button bounce
    }
    else if(buttonPressTime < 600) // 0.6s
    {
        onButtonPress(false /* short */);
    }
    else if(buttonPressTime < 10000) // 10s
    {
        onButtonPress(true /* long */);
    }
    else
    {
        // Nothing
        // The press was too long, probably a mistake or held by some fabric.
    }
}

