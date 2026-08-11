#include "SyncManager.h"


void SyncManager::publishThermState(const ThermostatState& s) {
    // TODO: Implementation for publishing thermostat state
    // This could involve sending the state over MQTT, ESP-NOW, etc.
    Serial.println("Publishing Thermostat State...");
}

void SyncManager::publishCommand(Command c) {
    // TODO: Implementation for publishing a command
    // This could involve sending the command over MQTT, ESP-NOW, etc.
    Serial.println("Publishing Command...");
}