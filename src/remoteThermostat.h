#ifndef REMOTE_THERMOSTAT_H
#define REMOTE_THERMOSTAT_H

#include "stateMachine.h"
#include <WiFi.h>
#include <ArduinoHA.h>
#include "storage.h"

// Forward declarations for main
extern volatile bool flag_offButton;
extern volatile bool flag_manualButton;
extern volatile bool flag_autoButton;
extern volatile bool flag_manualHeatButton;
extern volatile bool flag_manualCoolButton;
extern volatile bool flag_manualFanButton;

void parentOnTempGoal(float newTempGoal);
void childOnTempGoal(float newTempGoal);
void childOnRemoteTemp(float newTemp);
void parentOnRemoteMode(MODE newMode);
void childOnRemoteMode(MODE newMode);
void parentOnRemoteState(STATE newState);
void childOnRemoteState(STATE newState);

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
// void parentOnRemoteMode(MODE newMode);
// void onNewRemoteState(STATE newState);

void sendAutoButtonClick();
void sendManualButtonClick();
void sendOffButtonClick();
void sendFanButtonClick();
void sendCoolButtonClick();
void sendHeatButtonClick();

void setupMQTT();
void loopMQTT();

#endif // REMOTE_THERMOSTAT_H