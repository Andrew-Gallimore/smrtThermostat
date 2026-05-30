#ifndef STORAGE_H
#define STORAGE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include "stateMachine.h"
// #include "thermometers.h"

#define GPIO_RELAY1  40
#define GPIO_RELAY2  2
#define GPIO_RELAY3  1

// const int MIN_GOAL_TEMP = 65;
// const int MAX_GOAL_TEMP = 85;
const int MIN_GOAL_TEMP = 40;
const int MAX_GOAL_TEMP = 110;


enum PEERTYPE {
  PARENT,
  CHILD
};

// 8C:BF:EA:0D:B7:E4
// AC:27:6E:A4:A9:70
// 28:84:85:85:3B:88
// const uint8_t CHILD_ADDR[] = {0x8C, 0xBF, 0xEA, 0x0D, 0xB7, 0xE4};
// const uint8_t PARENT_ADDR[] = {0x28, 0x84, 0x85, 0x85, 0x3B, 0x88};
const uint8_t CHILD_ADDR[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t PARENT_ADDR[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const bool JUST_TREAT_ME_AS_PARENT = true; // If set, bypasses mac address check
// const uint8_t PARENT_ADDR[] = {0x8C, 0xBF, 0xEA, 0x0E, 0xD0, 0xD4};
// DC:B4:D9:04:90:24

PEERTYPE whoAmI();

char* getParentMac();

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

void storeThermometerList(const std::vector<std::string>& thermometerNames);

// Getters
int getMinTemp();
int getMaxTemp();
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

void getStoredThermometerList(std::vector<std::string>& thermometerNames);


#endif // STORAGE_H