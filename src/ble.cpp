#include "config.hpp"

#ifdef BLE_INTERFACE_ENABLE

#pragma region BLE Services + Characteristics
// https://btprodspecificationrefs.blob.core.windows.net/assigned-numbers/Assigned%20Number%20Types/Assigned_Numbers.pdf
static BLEUUID bleService_Battery((uint16_t)0x180F /* Battery service */);
static BLEUUID bleChar_Battery_Percentage((uint16_t)0x2A19 /* Battery Level */); // R , 0 - 100
static BLEUUID bleChar_Battery_Critical((uint16_t)0x2BE9 /* Battery Critical Status  */); // R/N , bit

static BLEUUID bleService_LedStrip(         "00000000-ed6e-4e04-a2c4-da0cabffbfd5");
static BLEUUID bleChar_LedStrip_Mode(       "00000001-ed6e-4e04-a2c4-da0cabffbfd5"); // R/W , ( 0 = off, 1 = color, 2 = per-LED data)
static BLEUUID bleChar_LedStrip_LedCount(   "00000002-ed6e-4e04-a2c4-da0cabffbfd5"); // R
static BLEUUID bleChar_LedStrip_AllLedColor("00000010-ed6e-4e04-a2c4-da0cabffbfd5"); // R/W
static BLEUUID bleChar_LedStrip_PerLed(     "00000011-ed6e-4e04-a2c4-da0cabffbfd5"); // R/W

static BLEUUID bleService_LedFilament(        "00000000-d3c5-44ea-a00b-602adb9cb0b2");
static BLEUUID bleChar_LedFilament_Brightness("00000001-d3c5-44ea-a00b-602adb9cb0b2"); // R/W

static BLEUUID bleService_Buttons(  "00000000-5030-46c4-b37c-e63723a38b49");
static BLEUUID bleChar_Buttons_Boot("00000001-5030-46c4-b37c-e63723a38b49"); // R/N
static BLEUUID bleChar_Buttons_Mode("00000002-5030-46c4-b37c-e63723a38b49"); // R/N

static BLEUUID bleService_ESP(        "00000000-adc1-4d51-8018-77ded18308ce");
static BLEUUID bleChar_ESP_CoreTemp(  "00000001-adc1-4d51-8018-77ded18308ce"); // R , temperatureRead()
static BLEUUID bleChar_ESP_Frequency( "00000002-adc1-4d51-8018-77ded18308ce"); // R , getCpuFrequencyMhz()
#pragma endregion

#pragma region BLE variables + callbacks
BLECharacteristic* bleCharBatteryPercentage;
BLECharacteristic* bleCharBatteryCritical;

BLECharacteristic* bleCharLedStripMode;
BLECharacteristic* bleCharLedStripAllLedColor;
BLECharacteristic* bleCharLedStripPerLed;
void LedStripAllLedColor_Update(const NimBLEAttValue& value)
{
    switch(value.size())
    {
        case 0: // No data -> all black
        {
            strip.setAllLedsColor(0);

            DEBUG_PRINT(F("LED Strip turned off "));
            DEBUG_PRINT(static_cast<uint32_t>(value[0]));
            DEBUG_PRINTLN(F(" using BLE"));
            break;
        }
        case 3: // 3 bytes -> RGB 8-bits per component
        {
            strip.setAllLedsColor(value[0], value[1], value[2]);

            DEBUG_PRINT(F("LED Strip changed to "));
            DEBUG_PRINT(static_cast<uint32_t>(value[0]));
            DEBUG_PRINT(F(", "));
            DEBUG_PRINT(static_cast<uint32_t>(value[1]));
            DEBUG_PRINT(F(", "));
            DEBUG_PRINT(static_cast<uint32_t>(value[2]));
            DEBUG_PRINTLN(F(" using BLE"));
            break;
        }
        case 4: // 4 bytes -> 32-bit RGB color
        {
            strip.setAllLedsColor(
                //(static_cast<uint32_t>(allLedVal[0]) << (8 * 3)) + // This would be an alpha channel
                (static_cast<uint32_t>(value[1]) << (8 * 2)) +
                (static_cast<uint32_t>(value[2]) << (8 * 1)) +
                (static_cast<uint32_t>(value[3]) << (8 * 0))
            );

            DEBUG_PRINT(F("LED Strip changed to "));
            DEBUG_PRINT(static_cast<uint32_t>(value[0]));
            DEBUG_PRINT(F(", "));
            DEBUG_PRINT(static_cast<uint32_t>(value[1]));
            DEBUG_PRINT(F(", "));
            DEBUG_PRINT(static_cast<uint32_t>(value[2]));
            DEBUG_PRINTLN(F(" using BLE"));
            break;
        }
    }
}
void LedStripPerLed_Update(const NimBLEAttValue& value)
{
    strip.setAllLedsColorData(0);
    for(
        int ledIndex = 0, valueLedIndex = 0;
        ledIndex < STRIP_LED_COUNT && valueLedIndex + 2 < value.size();
        ledIndex++, valueLedIndex += 3
    )
    {
        strip.setLedColorData(ledIndex, value[valueLedIndex + 0], value[valueLedIndex + 1], value[valueLedIndex + 2]);
    }
    strip.show();

    DEBUG_PRINTLN(F("LED Strip changed per-LED color using BLE"));
}
class LedStripCallback : public NimBLECharacteristicCallbacks
{
public:
    void onWrite(NimBLECharacteristic* pCharacteristic) override
    {
        NimBLECharacteristicCallbacks::onWrite(pCharacteristic);

        auto val = bleCharLedStripMode->getValue();
        uint8_t value255 = val.size() == 0 ? 0 : val[0];
        switch(value255)
        {
            default:
            case 0:
                break;
            case 1: // All LED Color
            {
                LedStripAllLedColor_Update(bleCharLedStripAllLedColor->getValue());
                break;
            }
            case 2: // Per-LED Color
            {
                LedStripPerLed_Update(bleCharLedStripPerLed->getValue());
                break;
            }
        }
    }
};

BLECharacteristic* bleCharLedFilamentBrightness;
class LedFilamentCallback : public NimBLECharacteristicCallbacks
{
public:
    void onWrite(NimBLECharacteristic* pCharacteristic) override
    {
        NimBLECharacteristicCallbacks::onWrite(pCharacteristic);

        auto val = pCharacteristic->getValue();
        uint8_t value255 = val.size() == 0 ? 0 : val[0];
        setFilamentBrightness(value255);

        DEBUG_PRINT(F("Filament brightness changed to "));
        DEBUG_PRINT((int)value255);
        DEBUG_PRINTLN(F(" using BLE"));
    }
};

BLECharacteristic* bleCharButtonsBoot;
BLECharacteristic* bleCharButtonsMode;
class ButtonCallback : public NimBLECharacteristicCallbacks
{
public:
    const int PinNumber;

public:
    explicit ButtonCallback(int pinNumber) : PinNumber(pinNumber) {}

public:
    void onRead(NimBLECharacteristic* pCharacteristic) override
    {
        NimBLECharacteristicCallbacks::onRead(pCharacteristic);

        pCharacteristic->setValue(static_cast<uint8_t>(digitalRead(PinNumber) ? 1 : 0));
    }
};

BLECharacteristic* bleCharEspCoreTemp;

std::vector<uint8_t> perLedVec_DefaultRgbLeds{};
class ServerCallback : public NimBLEServerCallbacks
{
public:
    void onConnect(NimBLEServer* pServer) override
    {
        NimBLEServerCallbacks::onConnect(pServer);

        DEBUG_PRINTLN(F("Bluetooth device connected"));

        // Turn off "shutdown timer"
        turnBluetoothOffAfterTime = 0;

        // Reset values
        bleCharLedStripMode->setValue(0 /* Off */);
        bleCharLedStripAllLedColor->setValue(static_cast<uint32_t>(0));
        bleCharLedStripPerLed->setValue(perLedVec_DefaultRgbLeds);
        bleCharLedFilamentBrightness->setValue(static_cast<uint8_t>(0 /* 0% = Off */));

        // Filament + Strip off, CPU sleep timers
        //filamentOffStartTime = 0;
        //stripOffStartTime = 0;
    }
    void onDisconnect(NimBLEServer* pServer) override
    {
        NimBLEServerCallbacks::onDisconnect(pServer);

        DEBUG_PRINTLN(F("Bluetooth device disconnected"));

        // Filament + Strip off, CPU sleep timers
        if(filamentOffStartTime != 0)
            filamentOffStartTime = millis();
        if(stripOffStartTime != 0)
            stripOffStartTime = millis();
    }
};
#pragma endregion

unsigned long turnBluetoothOffAfterTime = 0;

void initBluetooth()
{
    if(isBluetoothOn())
        return;

    // Initialize LED Strip testing pattern
    {
        perLedVec_DefaultRgbLeds.resize(STRIP_LED_COUNT * 3);
        for(int i = 0; i < STRIP_LED_COUNT; i++)
            perLedVec_DefaultRgbLeds[i * 3 + (i % 3)] = 0xFF; // Red, Green, Blue, Red, Green...
    }

    BLEDevice::init(BLE_NAME);
    DEBUG_PRINT(F("BLE Name: "));
    DEBUG_PRINTLN(BLE_NAME);

    NimBLEDevice::setPower(ESP_PWR_LVL_N3); // -3db

#if defined(BLE_PASSKEY) && BLE_PASSKEY != 0
#   define BLE_SECURED 1
    BLEDevice::setSecurityAuth(true, true, true);
    BLEDevice::setSecurityPasskey(BLE_PASSKEY);
    BLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    DEBUG_PRINT(F("BLE Passkey: "));
    DEBUG_PRINTLN(BLE_PASSKEY);
#endif

    esp_bt_sleep_enable();

    BLEServer* bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallback);
    { // Services
        // Battery
        {
            BLEService* bleServiceBattery = bleServer->createService(bleService_Battery);
            {
                auto batteryVoltage = getBatteryVoltage();
                auto batteryPercentage = guessBatteryPercentage(batteryVoltage);

                bleCharBatteryPercentage = bleServiceBattery->createCharacteristic(
                    bleChar_Battery_Percentage,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ
                );
                bleCharBatteryPercentage->setValue(static_cast<uint8_t>(batteryPercentage));

                bleCharBatteryCritical = bleServiceBattery->createCharacteristic(
                    bleChar_Battery_Critical,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::NOTIFY
                );
                bleCharBatteryCritical->setValue(static_cast<uint8_t>(batteryPercentage <= BATTERY_CRITICAL_PERCENTAGE ? 1 : 0));
            }
            bleServiceBattery->start();
        }
        // LED Strip
        {
            BLEService* bleServiceLedStrip = bleServer->createService(bleService_LedFilament);
            {
                LedStripCallback* ledStripCallback = new LedStripCallback();

                bleCharLedStripMode = bleServiceLedStrip->createCharacteristic(
                    bleChar_LedStrip_Mode,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
                    NIMBLE_PROPERTY::WRITE_ENC |
                    NIMBLE_PROPERTY::WRITE_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE
                );
                bleCharLedStripMode->setCallbacks(ledStripCallback);

                BLECharacteristic* bleCharLedStripLedCount = bleServiceLedStrip->createCharacteristic(
                    bleChar_LedStrip_LedCount,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ
                );
                bleCharLedStripLedCount->setValue(static_cast<uint8_t>(STRIP_LED_COUNT));

                bleCharLedStripAllLedColor = bleServiceLedStrip->createCharacteristic(
                    bleChar_LedStrip_AllLedColor,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
                    NIMBLE_PROPERTY::WRITE_ENC |
                    NIMBLE_PROPERTY::WRITE_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE
                );
                bleCharLedStripAllLedColor->setCallbacks(ledStripCallback);

                bleCharLedStripPerLed = bleServiceLedStrip->createCharacteristic(
                    bleChar_LedStrip_PerLed,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
                    NIMBLE_PROPERTY::WRITE_ENC |
                    NIMBLE_PROPERTY::WRITE_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE
                );
                bleCharLedStripPerLed->setCallbacks(ledStripCallback);
            }
            bleServiceLedStrip->start();
        }
        // LED Filament
        {
            BLEService* bleServiceLedFilament = bleServer->createService(bleService_LedFilament);
            {
                bleCharLedFilamentBrightness = bleServiceLedFilament->createCharacteristic(
                    bleChar_LedFilament_Brightness,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
                    NIMBLE_PROPERTY::WRITE_ENC |
                    NIMBLE_PROPERTY::WRITE_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::WRITE
                );
                bleCharLedFilamentBrightness->setCallbacks(new LedFilamentCallback());
            }
            bleServiceLedFilament->start();
        }
        // Buttons
        {
            BLEService* bleServiceButtons = bleServer->createService(bleService_Buttons);
            {
                bleCharButtonsBoot = bleServiceButtons->createCharacteristic(
                    bleChar_Buttons_Boot,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::NOTIFY
                );
                bleCharButtonsBoot->setCallbacks(new ButtonCallback(PIN_BUTTON_BOOT));

                bleCharButtonsMode = bleServiceButtons->createCharacteristic(
                    bleChar_Buttons_Mode,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ |
                    NIMBLE_PROPERTY::NOTIFY
                );
                bleCharButtonsMode->setCallbacks(new ButtonCallback(PIN_BUTTON_MODE));
            }
            bleServiceButtons->start();
        }
        // ESP
        {
            BLEService* bleServiceEsp = bleServer->createService(bleService_ESP);
            {
                bleCharEspCoreTemp = bleServiceEsp->createCharacteristic(
                    bleChar_ESP_CoreTemp,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ //THINK Add notify
                );
                bleCharEspCoreTemp->setValue(static_cast<uint8_t>(temperatureRead()));

                BLECharacteristic* bleCharEspFrequency = bleServiceEsp->createCharacteristic(
                    bleChar_ESP_Frequency,
#ifdef BLE_SECURED
                    NIMBLE_PROPERTY::READ_ENC |
                    NIMBLE_PROPERTY::READ_AUTHEN |
#endif
                    NIMBLE_PROPERTY::READ
                );
                bleCharEspFrequency->setValue(static_cast<uint8_t>(getCpuFrequencyMhz()));
            }
            bleServiceEsp->start();
        }
    }

    BLEAdvertising* bleAdvertising = BLEDevice::getAdvertising();
    {
        bleAdvertising->addServiceUUID(bleService_Battery);
        bleAdvertising->addServiceUUID(bleService_LedStrip);
        bleAdvertising->addServiceUUID(bleService_LedFilament);
        bleAdvertising->addServiceUUID(bleService_ESP);

        bleAdvertising->setScanResponse(true);
        bleAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
        bleAdvertising->setMaxPreferred(0x12);
    }

    BLEDevice::startAdvertising();

    DEBUG_PRINTLN(F("Bluetooth initialized"));
}
void deinitBluetooth()
{
    if(!isBluetoothOn())
        return;

    turnBluetoothOffAfterTime = 0;

    // Characteristics pointers
    {
        bleCharBatteryPercentage = nullptr;
        bleCharBatteryCritical = nullptr;

        bleCharLedStripMode = nullptr;
        bleCharLedStripAllLedColor = nullptr;
        bleCharLedStripPerLed = nullptr;

        bleCharLedFilamentBrightness = nullptr;

        bleCharButtonsBoot = nullptr;
        bleCharButtonsMode = nullptr;

        bleCharEspCoreTemp = nullptr;
    }

    BLEDevice::deinit(true);

    DEBUG_PRINTLN(F("Bluetooth shutdown"));
}

#endif
