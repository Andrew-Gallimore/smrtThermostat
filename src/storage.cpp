#include "storage.h"
#include <cstring>
#include <esp_system.h>
#include <SD.h>
#include <SPI.h>

#define DO_SD_CARD true

uint8_t myMac[6];
bool myMacFound = false;
bool sdCardInitialized = false;
bool variablesLoaded = false;
bool networkConfigLoaded = false;
bool thermometersConfigLoaded = false;
TaskHandle_t storeVariablesTaskHandle = NULL;
TaskHandle_t sdRecoveryTaskHandle = NULL;

// State stored variables
unsigned long lastStorageTime   = 0;
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

// Thermometers stored variables
bool updatedThermometers = false;
std::vector<String> storage_thermometers = {};

SemaphoreHandle_t sdMutex;


PEERTYPE whoAmI() {
  // Getting my mac address
  if(!myMacFound) {
      esp_read_mac(myMac, ESP_MAC_WIFI_STA);
      myMacFound = true;
  }

  if(JUST_TREAT_ME_AS_PARENT) {
    return PEERTYPE::PARENT;
  }

  // Telling if we are a parent or child device
  if(memcmp(myMac, PARENT_ADDR, sizeof(myMac)) == 0) {
    return PEERTYPE::PARENT;
  } else {
    return PEERTYPE::CHILD;
  }
}

// In the format of: "dcb4d9049024"
char macStr[13]; // 12 hex chars + null terminator
char* getParentMac() {
  snprintf(macStr, sizeof(macStr),
          "%02x%02x%02x%02x%02x%02x",
          PARENT_ADDR[0], PARENT_ADDR[1], PARENT_ADDR[2],
          PARENT_ADDR[3], PARENT_ADDR[4], PARENT_ADDR[5]);

  return macStr;
}

char myMacStr[13];
char* getMyMac() {
  uint8_t Imac[6];
  esp_read_mac(Imac, ESP_MAC_WIFI_STA);
  snprintf(myMacStr, sizeof(myMacStr),
          "%02x%02x%02x%02x%02x%02x",
          Imac[0], Imac[1], Imac[2],
          Imac[3], Imac[4], Imac[5]);

  return myMacStr;
}

#define SD_CS     42   // Chip Select for the SD card
#define SPI_SCK   48   // SPI Clock
#define SPI_MOSI  47   // SPI Data (MOSI)
#define SPI_MISO  41   // SPI Data (MISO)

SPIClass sdSPI(FSPI);


void writeFile(const char* filename, const char* data) {
  if (!DO_SD_CARD || !sdCardInitialized) {
    return;
  }

  if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {

    SD.remove(filename);
    File dataFile = SD.open(filename, FILE_WRITE);

    if (dataFile) {
      dataFile.seek(0);
      dataFile.print(data);
      dataFile.flush();
      dataFile.close();

      Serial.printf("Data written to %s\n", filename);
    } else {
      Serial.printf("Error opening %s for writing\n", filename);
    }

    xSemaphoreGive(sdMutex);
  }
}

void readFile(const char* filename, void (*parserCallback)(const char*)) {
  if (!DO_SD_CARD) {
    return;
  }

  if (!sdCardInitialized) {
    Serial.printf("SD Card not available, cannot read from %s\n", filename);
    return;
  }

  if (!SD.exists(filename)) {
    Serial.printf("%s does not exist\n", filename);
    return;
  }

  if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {

    File dataFile = SD.open(filename, FILE_READ);

    if (!dataFile) {
      Serial.printf("Failed to open %s\n", filename);
      xSemaphoreGive(sdMutex);
      return;
    }

    char buffer[256] = {0};
    while (dataFile.available()) {
      size_t len = dataFile.readBytesUntil(
        '\n',
        buffer,
        sizeof(buffer) - 1
      );

      buffer[len] = '\0';
    }
    dataFile.close();

    xSemaphoreGive(sdMutex);

    parserCallback(buffer);
  }
}

void storeVariablesTask(void* parameter) {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  while (true) {
    // Writing to temp.txt
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

    // Writing to network.txt
    if (updatedNetwork) {
      char dataBuffer[256*2];
      snprintf(dataBuffer, sizeof(dataBuffer), "%s,%s",
                storage_networkSSID,
                storage_networkPWD);
      writeFile("/network.txt", dataBuffer);
      updatedNetwork = false;
    }

    // Writing to thermometers.txt
    if (updatedThermometers) {
      // 10 means we can store up to 10 thermometers, adjust as needed
      char dataBuffer[10*32];
      dataBuffer[0] = '\0';

      for(size_t i = 0; i < storage_thermometers.size() && i < 10; ++i) {
        strncat(dataBuffer, storage_thermometers[i].c_str(), sizeof(dataBuffer) - strlen(dataBuffer) - 1);
        strncat(dataBuffer, "\n", sizeof(dataBuffer) - strlen(dataBuffer) - 1);
      }
      writeFile("/thermometers.txt", dataBuffer);
      updatedThermometers = false;
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

void readThermometerConfig() {
  storage_thermometers.clear();

  if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    return;
  }

  File file = SD.open("/thermometers.txt");
  if (!file) {
    xSemaphoreGive(sdMutex);
    return;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      storage_thermometers.push_back(line);
    }
  }
  file.close();

  xSemaphoreGive(sdMutex);

  thermometersConfigLoaded = true;
}




void runSDreading() {
  if(sdCardInitialized) {
      Serial.println("Reading stored variables...");
      readStates();
      readNetworkConfig();
      readThermometerConfig();
  } else {
      Serial.println("SD Card not initialized, skipping reading stored variables");
  }

  // Create task to store variables periodically
  if (storeVariablesTaskHandle == NULL) {
    xTaskCreatePinnedToCore(
        storeVariablesTask,
        "StoreVariablesTask",
        4096,
        NULL,
        1,
        &storeVariablesTaskHandle,
        1
    );
  }
}

bool initSDCardOnce() {

    if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        return false;
    }

    SD.end();
    sdSPI.end();

    delay(50);

    sdSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    delay(50);

    bool ok = SD.begin(SD_CS, sdSPI, 4000000);

    if (!ok) {
        xSemaphoreGive(sdMutex);
        return false;
    }

    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        SD.end();
        xSemaphoreGive(sdMutex);
        return false;
    }

    xSemaphoreGive(sdMutex);

    return true;
}

bool initSDCardWithRetry(int maxRetries = 5) {
    for (int attempt = 1; attempt <= maxRetries; attempt++) {

        Serial.printf(
            "Initializing SD card (attempt %d/%d)\n",
            attempt,
            maxRetries
        );

        if (initSDCardOnce()) {
            Serial.println("SD card initialized");
            sdCardInitialized = true;
            return true;
        }

        Serial.println("SD init failed");

        delay(500 * attempt); // progressive backoff
    }

    sdCardInitialized = false;
    return false;
}

void sdRecoveryTask(void* parameter) {
  while(true) {
    if(!sdCardInitialized) {
      Serial.println("Attempting SD recovery...");

      if(initSDCardWithRetry(3)) {
        Serial.println("SD recovered!");

        runSDreading();

        sdRecoveryTaskHandle = NULL;
        vTaskDelete(NULL);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

void initializeStorage() {
    if(!DO_SD_CARD) {
        return;
    }
    
    sdMutex = xSemaphoreCreateMutex();

    if(!initSDCardWithRetry(5)) {
      if (sdRecoveryTaskHandle == NULL) {
        // Try to recover...
        xTaskCreatePinnedToCore(
          sdRecoveryTask,
          "SDRecovery",
          4096,
          NULL,
          1,
          &sdRecoveryTaskHandle,
          1
        );
      }
    }else {
      // It worked!
      runSDreading();
    }
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

void storeThermometerList(const std::vector<String>& thermometerNames) {
  storage_thermometers = thermometerNames;
  updatedThermometers = true;
}



// Getters
int getMinTemp() {
  return MIN_GOAL_TEMP;
}
int getMaxTemp() {
  return MAX_GOAL_TEMP;
}

unsigned long getStoredTimestamp() {
  return lastStorageTime;
}

float getStoredTemp() {
  return storage_temp;
}
float getStoredTempGoal() {
  return storage_tempGoal;
}

STATE getStoredState() {
  return storage_state;
}
STATE getStoredLastState() {
  return storage_lastState;
}
STATE getStoredLastHeavyState() {
  return storage_lastHeavyState;
}

MODE getStoredMode() {
  return storage_mode;
}
MODE getStoredLastMode() {
  return storage_lastMode;
}

void getStoredNetworkSSID(char* SSIDBuffer, size_t bufSize) {
  if (SSIDBuffer == nullptr || bufSize == 0) return;

  if (storage_networkSSID[0] == '\0') {
    SSIDBuffer[0] = '\0';
    return;
  }
  strncpy(SSIDBuffer, storage_networkSSID, bufSize - 1);
  SSIDBuffer[bufSize - 1] = '\0';
}
void getStoredNetworkPWD(char* PWDBuffer, size_t bufSize) {
  if (PWDBuffer == nullptr || bufSize == 0) return;

  if (storage_networkPWD[0] == '\0') {
    PWDBuffer[0] = '\0';
    return;
  }
  strncpy(PWDBuffer, storage_networkPWD, bufSize - 1);
  PWDBuffer[bufSize - 1] = '\0';
}

void getStoredThermometerList(std::vector<String>& thermometerNames) {
    thermometerNames = storage_thermometers;
}