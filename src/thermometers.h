#ifndef THERMOMETERS_H
#define THERMOMETERS_H

#include <NimBLEDevice.h>
#include <Wire.h>

// Forward declaration
void onNewTempReading(float temp);

struct SENSOR {
    char name[32];
    float temp;
    int rssi;
    unsigned long lastUpdateTime;
};

const int numSensors = 4;
SENSOR sensors[numSensors] = {
    {"ATC_8A7083", -1.0, -10000, 0},
    {"ATC_E52940", -1.0, -10000, 0},
    {"ATC_3EE5A2", -1.0, -10000, 0}
};

float avgTemp = -1.0;


void recaculateTemp() {
    // Update average temperature, only include sensors updated
    //      within last 3 minutes and with valid temp

    float sum = 0;
    int count = 0;
    unsigned long now = millis();
    const unsigned long maxAge = 3 * 60 * 1000; // 3 minutes in ms

    for (int i = 0; i < numSensors; ++i) {
        if (sensors[i].temp > -1.0 && (now - sensors[i].lastUpdateTime <= maxAge)) {
            sum += sensors[i].temp;
            count++;
        }
    }
    
    if (count > 0) {
        avgTemp = sum / count;
        onNewTempReading(avgTemp);
    }
}


class SimpleAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        bool newValue = false;

        // Match by device name
        for (int i = 0; i < numSensors; ++i) {
            if (advertisedDevice->haveName() &&
                strcmp(advertisedDevice->getName().c_str(), sensors[i].name) == 0) {

                Serial.printf("Found sensor: %s, RSSI: %d\n", sensors[i].name, advertisedDevice->getRSSI());

                newValue = true;
                sensors[i].rssi = advertisedDevice->getRSSI();

                
                // Example: parse temperature from service data (customize as needed)
                if (advertisedDevice->haveServiceData()) {
                    std::string data = advertisedDevice->getServiceData();
                    if (data.length() >= 8) {
                        int16_t tempRaw = (data[7] | (data[6] << 8));
                        float celciusRaw = tempRaw / 10.0;
                        sensors[i].temp = celciusRaw * 1.8 + 32;  // Convert to Fahrenheit
                        sensors[i].lastUpdateTime = millis();
                    }
                }
            }
        }

        if(newValue) recaculateTemp();
    }
};

void bleScanTask(void* pvParameters) {
    NimBLEDevice::init("");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(new SimpleAdvertisedDeviceCallbacks(), false);
    scan->setActiveScan(true);
    scan->setInterval(97);
    scan->setWindow(37);
    scan->setMaxResults(0);

    while (true) {
        Serial.println("Starting BLE scan...");
        scan->start(7000, false, true); // Scan for 7 seconds
        vTaskDelay(pdMS_TO_TICKS(6000)); // Wait 6 seconds before next scan
    }
}

void startBLESensorScan() {
    xTaskCreatePinnedToCore(
        bleScanTask,         // Task function
        "BLEScanTask",       // Name
        4096,                // Stack size
        nullptr,             // Parameters
        1,                   // Priority
        nullptr,             // Task handle
        0                    // Core (0 or 1)
    );
}

#endif //THERMOMETERS_H