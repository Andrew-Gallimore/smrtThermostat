#include "storage.h"
#include <cstring>
#include <esp_system.h>
#include <SD.h>
#include <SPI.h>

#define DO_SD_CARD false

uint8_t myMac[6];
bool myMacFound = false;
bool sdCardInitialized = false;
bool variablesLoaded = false;
bool networkConfigLoaded = false;
TaskHandle_t storeVariablesTaskHandle = NULL;

// State stored variables
unsigned long lastStorageTime   = -1;
bool  updatedStates             = false;
float storage_temp              = 70;
float storage_tempGoal          = 70;

STATE storage_state            = STATE::Idle;
STATE storage_lastState        = STATE::Idle;
STATE storage_lastHeavyState   = STATE::Idle;

MODE storage_mode        = MODE::Off;
MODE storage_lastMode    = MODE::Auto;

// Network stored variables
bool updatedNetwork           = false;
char storage_networkSSID[256] = "";
char storage_networkPWD[256]  = "";


PEERTYPE whoAmI() {
    if(!myMacFound) {
        esp_read_mac(myMac, ESP_MAC_WIFI_STA);
        myMacFound = true;
    }
  return PEERTYPE::PARENT;
  // if(memcmp(myMac, PARENT_ADDR, sizeof(myMac)) == 0) {
  // } else {
  //   return PEERTYPE::CHILD;
  // }
}

#define SD_CS     42   // Chip Select for the SD card
#define SPI_SCK   48   // SPI Clock
#define SPI_MOSI  47   // SPI Data (MOSI)
#define SPI_MISO  41   // SPI Data (MISO)

void initializeStorage() {
    if(!DO_SD_CARD) {
        return;
    }

    // // Create task to store variables periodically
    // xTaskCreatePinnedToCore(
    //     storeVariablesTask,       // Task function
    //     "StoreVariablesTask",     // Name of the task
    //     4096,                     // Stack size (in bytes)
    //     NULL,                     // Task input parameter
    //     1,                        // Priority
    //     &storeVariablesTaskHandle,// Task handle
    //     1                         // Core 1
    // );

    // initSDCard();
}

void initSDCard() {
    if(!DO_SD_CARD) {
        sdCardInitialized = false;
        return;
    }

    // SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    // if (!SD.begin(SD_CS)) {
    //     Serial.println("SD Card failed or not present");
    //     sdCardInitialized = false;
    //     return;
    // }
    // sdCardInitialized = true;
    // Serial.println("SD Card initialized");
}

void writeFile(const char* filename, const char* data) {
    if(!DO_SD_CARD) {
        return;
    }

    // if (!sdCardInitialized) {
    //     // Try to reinitialize in case card was plugged in
    //     initSDCard();
    //     if (!sdCardInitialized) {
    //         Serial.printf("SD Card not available, cannot write to %s\n", filename);
    //         return;
    //     }
    // }

    // File dataFile = SD.open(filename, FILE_WRITE);
    // if (dataFile) {
    //     dataFile.println(data);
    //     dataFile.close();
    //     Serial.printf("Data written to %s\n", filename);
    // } else {
    //     Serial.printf("Error opening %s for writing\n", filename);
    //     sdCardInitialized = false;  // Mark as failed in case card was removed
    // }
}

void readFile(const char* filename, void (*parserCallback)(const char*)) {
    if(!DO_SD_CARD) {
        return;
    }

    // if (!sdCardInitialized) {
    //     // Try to reinitialize in case card was plugged in
    //     initSDCard();
    //     if (!sdCardInitialized) {
    //         Serial.printf("SD Card not available, cannot read from %s\n", filename);
    //         return;
    //     }
    // }

    // char buffer[256];
    // buffer[0] = '\0';

    // File dataFile = SD.open(filename);
    // if (dataFile) {
    //     while (dataFile.available()) {
    //         size_t len = dataFile.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
    //         buffer[len] = '\0'; // Null-terminate the string
    //     }
    //     dataFile.close();
    // } else {
    //     Serial.printf("Error opening %s for reading\n", filename);
    //     sdCardInitialized = false;  // Mark as failed in case card was removed
    // }

    // parserCallback(buffer);

    // // // Parse CSV line
    // // // Expected format: timestamp,temp,tempGoal,state,lastState,lastHeavyState,mode,lastMode
    // // unsigned long timestamp;
    // // int parsed = sscanf(buffer, "%lu,%f,%f,%d,%d,%d,%d,%d",
    // //                     &timestamp,
    // //                     &storage_temp,
    // //                     &storage_tempGoal,
    // //                     &storage_state,
    // //                     &storage_lastState,
    // //                     &storage_lastHeavyState,
    // //                     &storage_mode,
    // //                     &storage_lastMode);
}

void storeVariablesTask(void* parameter) {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  while (true) {
    if (updatedStates) {
      lastStorageTime = millis();

      // Make variables char* in csv format + current timestamp
      char dataBuffer[256];
      snprintf(dataBuffer, sizeof(dataBuffer), "%lu,%.2f,%.2f,%d,%d,%d,%d,%d,%d",
                lastStorageTime,
                storage_temp,
                storage_tempGoal,
                static_cast<int>(storage_state),
                static_cast<int>(storage_lastState),
                static_cast<int>(storage_lastHeavyState),
                static_cast<int>(storage_mode),
                static_cast<int>(storage_lastMode));
      writeFile("/temp.txt", dataBuffer);
      updatedStates = false;
    }

    if (updatedNetwork) {
      char dataBuffer[256*2];
      snprintf(dataBuffer, sizeof(dataBuffer), "%s,%s",
                storage_networkSSID,
                storage_networkPWD);
      writeFile("/network.txt", dataBuffer);
      updatedNetwork = false;
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}

void readStates() {
  readFile("/temp.txt", [](const char* data) {
      // Parse CSV line
      // Expected format: timestamp,temp,tempGoal,state,lastState,lastHeavyState,mode,lastMode
      unsigned long timestamp;
      int stateInt, lastStateInt, lastHeavyStateInt, modeInt, lastModeInt;
      int parsed = sscanf(data, "%lu,%f,%f,%d,%d,%d,%d,%d",
                          &timestamp,
                          &storage_temp,
                          &storage_tempGoal,
                          &stateInt,
                          &lastStateInt,
                          &lastHeavyStateInt,
                          &modeInt,
                          &lastModeInt);
      if (parsed == 8) {
          lastStorageTime = timestamp;
          storage_state = static_cast<STATE>(stateInt);
          storage_lastState = static_cast<STATE>(lastStateInt);
          storage_lastHeavyState = static_cast<STATE>(lastHeavyStateInt);
          storage_mode = static_cast<MODE>(modeInt);
          storage_lastMode = static_cast<MODE>(lastModeInt);
          variablesLoaded = true;
      }
  });
}

void readNetworkConfig() {
  readFile("/network.txt", [](const char* data) {
      // Parse CSV line
      // Expected format: SSID,password
      char ssid[256];
      char password[256];
      int parsed = sscanf(data, "%255[^,],%255s", ssid, password);
      if (parsed == 2) {
          strncpy(storage_networkSSID, ssid, sizeof(storage_networkSSID) - 1);
          storage_networkSSID[sizeof(storage_networkSSID) - 1] = '\0';
          strncpy(storage_networkPWD, password, sizeof(storage_networkPWD) - 1);
          storage_networkPWD[sizeof(storage_networkPWD) - 1] = '\0';
          networkConfigLoaded = true;
      }
  });
}

// Setters
void storeTemp(float newTemp) {
  if(newTemp == storage_temp) {
    return;
  }
  storage_temp = newTemp;
  updatedStates = true;
}
void storeTempGoal(float newTempGoal) {
  if(newTempGoal == storage_tempGoal) {
    return;
  }
  storage_tempGoal = newTempGoal;
  updatedStates = true;
}

void storeState(STATE newState) {
  if(newState == storage_state) {
    return;
  }
  storage_state = newState;
  updatedStates = true;
}
void storeLastState(STATE newLastState) {
  if(newLastState == storage_lastState) {
    return;
  }
  storage_lastState = newLastState;
  updatedStates = true;
}
void storeLastHeavyState(STATE newLastHeavyState) {
  if(newLastHeavyState == storage_lastHeavyState) {
    return;
  }
  storage_lastHeavyState = newLastHeavyState;
  updatedStates = true;
}

void storeMode(MODE newMode) {
  if(newMode == storage_mode) {
    return;
  }
  storage_mode = newMode;
  updatedStates = true;
}
void storeLastMode(MODE newLastMode) {
  if(newLastMode == storage_lastMode) {
    return;
  }
  storage_lastMode = newLastMode;
  updatedStates = true;
}

void storeNetworkSSID(char* SSID) {
  if (SSID == nullptr) SSID = (char*)"";

  strncpy(storage_networkSSID, SSID, sizeof(storage_networkSSID) - 1);
  storage_networkSSID[sizeof(storage_networkSSID) - 1] = '\0';
  updatedNetwork = true;
}

void storeNetworkPWD(char* password) {
  if (password == nullptr) password = (char*)"";

  strncpy(storage_networkPWD, password, sizeof(storage_networkPWD) - 1);
  storage_networkPWD[sizeof(storage_networkPWD) - 1] = '\0';
  updatedNetwork = true;
}

// Getters
unsigned long getStoredTimestamp() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return lastStorageTime;
}

float getStoredTemp() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_temp;
}
float getStoredTempGoal() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_tempGoal;
}

STATE getStoredState() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_state;
}
STATE getStoredLastState() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_lastState;
}
STATE getStoredLastHeavyState() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_lastHeavyState;
}

MODE getStoredMode() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_mode;
}
MODE getStoredLastMode() {
  if(!variablesLoaded) {
    // Try to read from file
    readStates();
  }
  return storage_lastMode;
}

void getStoredNetworkSSID(char* SSIDBuffer, size_t bufSize) {
  if (SSIDBuffer == nullptr || bufSize == 0) return;
  if(!networkConfigLoaded) {
    // Try to read from file
    readNetworkConfig();
  }
  if (storage_networkSSID[0] == '\0') {
      SSIDBuffer[0] = '\0';
      return;
  }
  strncpy(SSIDBuffer, storage_networkSSID, bufSize - 1);
  SSIDBuffer[bufSize - 1] = '\0';
}
void getStoredNetworkPWD(char* PWDBuffer, size_t bufSize) {
  if (PWDBuffer == nullptr || bufSize == 0) return;
  if(!networkConfigLoaded) {
    // Try to read from file
    readNetworkConfig();
  }
  if (storage_networkPWD[0] == '\0') {
      PWDBuffer[0] = '\0';
      return;
  }
  strncpy(PWDBuffer, storage_networkPWD, bufSize - 1);
  PWDBuffer[bufSize - 1] = '\0';
}


// void updateStorageMode(MODE newMode) {
//   Serial.print("Updating storage mode to: ");
//   Serial.println((int)newMode);

//   Preferences preferences;
//   preferences.begin("thermostat", false); // namespace "thermostat", RW mode
//   preferences.putInt("mode", static_cast<int>(newMode));
//   preferences.end();
// }

// void updateStorageTempGoal(float newTempGoal) {
//   Preferences preferences;
//   preferences.begin("thermostat", false); // namespace "thermostat", RW mode
//   preferences.putFloat("tempGoal", newTempGoal);
//   preferences.end();
// }

// void updateStorageState(STATE newState) {
//   Preferences preferences;
//   preferences.begin("thermostat", false); // namespace "thermostat", RW mode
//   preferences.putInt("state", static_cast<int>(newState));
//   preferences.end();
// }

// void updateStorageHeavyState(STATE newHeavyState) {
//   Preferences preferences;
//   preferences.begin("thermostat", false); // namespace "thermostat", RW mode
//   preferences.putInt("heavyState", static_cast<int>(newHeavyState));
//   preferences.end();
// }


// MODE getStorageMode() {
//   Preferences preferences;
//   preferences.begin("thermostat", true); // namespace "thermostat", RO mode
//   int storedMode = preferences.getInt("mode", static_cast<int>(MODE::Manual));
//   preferences.end();

//   Serial.print("Retrieved stored mode: ");
//   Serial.println((int)storedMode);
//   return static_cast<MODE>(storedMode);
// }

// float getStorageTempGoal() {
//   Preferences preferences;
//   preferences.begin("thermostat", true); // namespace "thermostat", RO mode

//   // Default if not already set: 70
//   float storedTempGoal = preferences.getFloat("tempGoal", 70.0);
//   preferences.end();
//   return storedTempGoal;
// }

// STATE getStorageState() {
//   Preferences preferences;
//   preferences.begin("thermostat", true); // namespace "thermostat", RO mode

//   int storedState = preferences.getInt("state", static_cast<int>(STATE::Idle));
//   preferences.end();
//   return static_cast<STATE>(storedState);
// }

// STATE getStorageHeavyState() {
//   Preferences preferences;
//   preferences.begin("thermostat", true); // namespace "thermostat", RO mode

//   int storedHeavyState = preferences.getInt("heavyState", static_cast<int>(STATE::Heat));
//   preferences.end();
//   return static_cast<STATE>(storedHeavyState);
// }