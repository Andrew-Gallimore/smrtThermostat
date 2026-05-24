#ifndef THERMOMETERS_H
#define THERMOMETERS_H

#include <NimBLEDevice.h>
#include <Wire.h>
#include <vector>
#include <string>

// Forward declaration for main.cpp
void onNewTempReading(float temp);

// Forward declaration for storage.cpp
void getStoredThermometerList(std::vector<String>& thermometerNames);
void storeThermometerList(const std::vector<String>& thermometerNames);

struct SENSOR {
    char name[32];
    float temp;
    int rssi;
    unsigned long lastUpdateTime;
};

std::vector<SENSOR> getSensorList();

void removeSensor(char* name);
void addSensor(const char* name);

void startBLESensorScan();

#endif //THERMOMETERS_H