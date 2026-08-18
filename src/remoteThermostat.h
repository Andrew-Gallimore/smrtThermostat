#ifndef REMOTE_THERMOSTAT_H
#define REMOTE_THERMOSTAT_H

#include "core/core-structs.h"
#include "core/ThermostatModel.h"
#include <WiFi.h>
#include <ArduinoHA.h>
#include "storage.h"

extern ThermostatModel* model;

// Forward declarations
// void onRemoteTemp(float newTemp);
// void onRemoteTempGoal(float newGoalTemp);
// void onRemoteMode(MODE newMode);
// void onHARemoteMode(MODE newMode);
// void onRemoteState(STATE newState);
// void onHARemoteState(STATE newState);

// Regular declarations
void updateSharedTemp(float temp);
void updateSharedTempGoal(float goalTemp);
void updateSharedMode(MODE mode);
void updateSharedState(STATE state);

struct WiFiNetwork {
    char ssid[33];
    int rssi;
};

void scanAvailableNetworks();
void getAvailableNetworks(WiFiNetwork* networks, int& networkCount);
void setWiFiCredentials(char* ssid, char* password);

// void requestRemoteTemp(float temp);
// void requestRemoteMode(MODE mode);
// void requestRemoteState(MODE mode, STATE state);

// void onNewRemoteTemp(float newTemp);
// void onNewRemoteGoalTemp(float newGoalTemp);
// void onRemoteMode(MODE newMode);
// void onNewRemoteState(STATE newState);

void setupMQTT();
void loopMQTT();

void publishSyncState(const ThermostatState& state);
void sendSyncCommand(const Command& cmd);
void publishHAState(const ThermostatState& state);

#endif // REMOTE_THERMOSTAT_H