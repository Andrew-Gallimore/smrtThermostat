#include "./thermometers.h"

std::vector<SENSOR> initSensors() {
    std::vector<SENSOR> v;
    SENSOR s;

    strncpy(s.name, "ATC_8A7083", sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    s.temp = -1.0;
    s.lastUpdateTime = -10000;
    s.rssi = 0;
    v.push_back(s);

    strncpy(s.name, "ATC_E52940", sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    s.temp = -1.0;
    s.lastUpdateTime = -10000;
    s.rssi = 0;
    v.push_back(s);

    strncpy(s.name, "ATC_3EE5A2", sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    s.temp = -1.0;
    s.lastUpdateTime = -10000;
    s.rssi = 0;
    v.push_back(s);

    return v;
}

std::vector<SENSOR> sensors = initSensors();

float avgTemp = -1.0;

std::vector<SENSOR> getSensorList() {
    return sensors;
}

void removeSensor(char* name) {
    for (size_t i = 0; i < sensors.size(); ++i) {
        if (strcmp(sensors[i].name, name) == 0) {
            sensors.erase(sensors.begin() + i);
            break;
        }
    }
}
void addSensor(const char* name) {
    SENSOR newSensor;
    strncpy(newSensor.name, name, sizeof(newSensor.name) - 1);
    newSensor.name[sizeof(newSensor.name) - 1] = '\0';
    newSensor.temp = -1.0;
    newSensor.lastUpdateTime = -10000;
    newSensor.rssi = 0;
    sensors.push_back(newSensor);
}


void recaculateTemp() {
    // Update average temperature, only include sensors updated
    //      within last 3 minutes and with valid temp

    float sum = 0;
    int count = 0;
    unsigned long now = millis();
    const unsigned long maxAge = 3 * 60 * 1000; // 3 minutes in ms

    for (size_t i = 0; i < sensors.size(); ++i) {
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
        for (size_t i = 0; i < sensors.size(); ++i) {
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