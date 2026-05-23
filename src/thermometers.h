#ifndef THERMOMETERS_H
#define THERMOMETERS_H

#include <NimBLEDevice.h>
#include <Wire.h>
#include <vector>

// Forward declaration
void onNewTempReading(float temp);

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