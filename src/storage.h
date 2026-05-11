#ifndef STORAGE_H
#define STORAGE_H

#include <cstdint>
#include <cstddef>
#include "stateMachine.h"

#define GPIO_RELAY1  40
#define GPIO_RELAY2  2
#define GPIO_RELAY3  1

const int MIN_GOAL_TEMP = 40;
const int MAX_GOAL_TEMP = 110;


enum PEERTYPE {
  PARENT,
  CHILD
};

const uint8_t CHILD_ADDR[] = {0x8C, 0xBF, 0xEA, 0x0D, 0xB7, 0xE4};
const uint8_t PARENT_ADDR[] = {0xDC, 0xB4, 0xD9, 0x04, 0x90, 0x24};
// const uint8_t PARENT_ADDR[] = {0x8C, 0xBF, 0xEA, 0x0E, 0xD0, 0xD4};
// DC:B4:D9:04:90:24

PEERTYPE whoAmI();

// void updateStorageMode(MODE newMode);
// void updateStorageTempGoal(float newTempGoal);
// void updateStorageHeavyState(STATE newHeavyState);
// void updateStorageState(STATE newState);

// MODE getStorageMode();
// float getStorageTempGoal();
// STATE getStorageState();
// STATE getStorageHeavyState();

void initializeStorage();
void initSDCard();
void storeVariablesTask(void *parameter);

// Storage functions
void storeTemp(float newTemp);
void storeTempGoal(float newTempGoal);

void storeState(STATE newState);
void storeLastState(STATE newLastState);
void storeLastHeavyState(STATE newLastHeavyState);

void storeMode(MODE newMode);
void storeLastMode(MODE newLastMode);

void storeNetworkSSID(char* SSID);
void storeNetworkPWD(char* PWD);

// Getters
unsigned long getStoredTimestamp();
float getStoredTemp();
float getStoredTempGoal();

STATE getStoredState();
STATE getStoredLastState();
STATE getStoredLastHeavyState();

MODE getStoredMode();
MODE getStoredLastMode();

void getStoredNetworkSSID(char* SSIDBuffer, size_t bufSize);
void getStoredNetworkPWD(char* PWDBuffer, size_t bufSize);


#endif // STORAGE_H