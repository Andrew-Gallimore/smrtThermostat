#include "remoteThermostat.h"

TaskHandle_t wifiMqttTaskHandle = NULL;

SemaphoreHandle_t mqttMutex;

// Making wifi using classes
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

bool wifiCredentialsChanged = false;

// char deviceName[28] = "Community_Hall_1";
// char deviceName[17] = "Community_Hall_2";
char deviceName[10] = "Sanctuary";
// char deviceName[19] = "Testing_Thermostat";
char HAaddr[12] = "10.1.10.132";
char Version[8] = "2.0.1";

// Intializing HVAC object 
HAHVAC hvac(
  getMyMac(),
  HAHVAC::TargetTemperatureFeature | HAHVAC::PowerFeature | HAHVAC::ModesFeature | HAHVAC::ActionFeature
);

// For listening to home assistant (populated in setupMQTT())
char tempStatTopic[128];
char tempCmdTopic[128];
char modeCmdTopic[128];
char actTopic[128];

// For listening to peer therm. (populated in setupMQTT())
char toChildGoalTempTopic[64];
char toChildTempTopic[64];
char toChildModeTopic[64];
char toChildStateTopic[64];
char toChildUnlockedTopic[64];

char toParentGoalTempTopic[64];
char toParentTempTopic[64];
char toParentModeTopic[64];
char toParentStateTopic[64];
char toParentUnlockedTopic[64];

char syncStateTopic[64];
char syncCommandTopic[64];
char remoteStateTopic[64];
char remoteCommandTopic[64];


// Helpers for sending state and commands between parent and child thermostats

String serializeThermostatState(const ThermostatState& state) {
    return String("role=") + String((int)state.role)
         + ";mode=" + String((int)state.mode)
         + ";lastMode=" + String((int)state.lastMode)
         + ";state=" + String((int)state.state)
         + ";goalState=" + String((int)state.goalState)
         + ";temp=" + String(state.temp, 1)
         + ";goalTemp=" + String(state.goalTemp, 1)
         + ";unlocked=" + String(state.unlocked ? 1 : 0);
}

bool deserializeThermostatState(const char* payload, ThermostatState& outState) {
    int role = 0;
    int mode = 0;
    int lastMode = 0;
    int state = 0;
    int goalState = 0;
    float temp = 0.0f;
    float goalTemp = 0.0f;
    int unlocked = 0;

    int matched = sscanf(payload,
        "role=%d;mode=%d;lastMode=%d;state=%d;goalState=%d;temp=%f;goalTemp=%f;unlocked=%d",
        &role,
        &mode,
        &lastMode,
        &state,
        &goalState,
        &temp,
        &goalTemp,
        &unlocked
    );

    if (matched != 8) {
        return false;
    }

    outState.role = static_cast<ROLE>(role);
    outState.mode = static_cast<MODE>(mode);
    outState.lastMode = static_cast<MODE>(lastMode);
    outState.state = static_cast<STATE>(state);
    outState.goalState = static_cast<GOAL_STATE>(goalState);
    outState.temp = temp;
    outState.goalTemp = goalTemp;
    outState.unlocked = (unlocked != 0);
    return true;
}

String serializeCommand(const Command& cmd) {
    return String("type=") + String((int)cmd.type)
         + ";mode=" + String((int)cmd.mode)
         + ";tempGoal=" + String(cmd.tempGoal, 1)
         + ";temp=" + String(cmd.temp, 1)
         + ";state=" + String((int)cmd.state);
}

bool deserializeCommand(const char* payload, Command& outCommand) {
    int type = 0;
    int mode = 0;
    float tempGoal = 0.0f;
    float temp = 0.0f;
    int state = 0;

    int matched = sscanf(payload,
        "type=%d;mode=%d;tempGoal=%f;temp=%f;state=%d",
        &type,
        &mode,
        &tempGoal,
        &temp,
        &state
    );

    if (matched != 5) {
        return false;
    }

    outCommand.type = static_cast<COMMAND_TYPE>(type);
    outCommand.mode = static_cast<MODE>(mode);
    outCommand.tempGoal = tempGoal;
    outCommand.temp = temp;
    outCommand.state = static_cast<STATE>(state);
    return true;
}


// Old ones for sending to home assistant and remote thermostat

// void updateSharedTemp(float temp) {
//     // Sending to homeassistant or remote thermostat
//     if(whoAmI() == ROLE::PARENT) {
//         xSemaphoreTake(mqttMutex, portMAX_DELAY);
//         mqtt.publish(toChildTempTopic, String(temp).c_str());
//         xSemaphoreGive(mqttMutex);
//         hvac.setCurrentTemperature(temp);
//     }else {
//         // mqtt.publish(toParentTempTopic, String(temp).c_str());
//     }
    
//     Serial.print("Updating shared temperature to: ");
//     Serial.println(temp);
// }

// void updateSharedTempGoal(float goalTemp) {
//     // Sending to homeassistant or remote thermostat
//     if(whoAmI() == ROLE::PARENT) {
//         xSemaphoreTake(mqttMutex, portMAX_DELAY);
//         mqtt.publish(toChildGoalTempTopic, String(goalTemp).c_str());
//         xSemaphoreGive(mqttMutex);
//         hvac.setTargetTemperature(goalTemp);
//     }else {
//         // mqtt.publish(toParentGoalTempTopic, String(goalTemp).c_str());
//     }

//     Serial.print("Updating shared goal temperature to: ");
//     Serial.println(goalTemp);
// }

// void updateSharedMode(MODE mode) {
//     // Sending to homeassistant or remote thermostat
//     if(whoAmI() == ROLE::PARENT) {
//         xSemaphoreTake(mqttMutex, portMAX_DELAY);
//         mqtt.publish(toChildModeTopic, String((int)mode).c_str());
//         xSemaphoreGive(mqttMutex);
        
//         if(mode == MODE::Off) {
//             hvac.setMode(HAHVAC::OffMode);
//         } else if (mode == MODE::Auto) {
//             hvac.setMode(HAHVAC::AutoMode);
//         } else if (mode == MODE::Manual) {
//             STATE currentState = model->getCurrentState();
//             if(currentState == STATE::Cool) {
//                 hvac.setMode(HAHVAC::CoolMode);
//             } else if (currentState == STATE::Heat) {
//                 hvac.setMode(HAHVAC::HeatMode);
//             } else if (currentState == STATE::Fan) {
//                 hvac.setMode(HAHVAC::FanOnlyMode);
//             } else if (currentState == STATE::Idle) {
//                 hvac.setMode(HAHVAC::DryMode); // Assuming Manual is equivalent to DryMode mode
//             }
//         }
//     }else {
//         // mqtt.publish(toParentModeTopic, String((int)mode).c_str());
//     }
    
//     Serial.print("Updating shared mode to: ");
//     Serial.println(mode);
// }

// void updateSharedState(STATE state) {
//     MODE mode = model->getMode();

//     if(whoAmI() == ROLE::PARENT) {
//         xSemaphoreTake(mqttMutex, portMAX_DELAY);
//         mqtt.publish(toChildStateTopic, String((int)state).c_str());
//         xSemaphoreGive(mqttMutex);

//         // =========================
//         // HVAC MODE
//         // =========================

//         if(mode == MODE::Off) {
//             Serial.println("Sending off mode...");
//             hvac.setMode(HAHVAC::OffMode);
//         } else if(mode == MODE::Auto) {
//             hvac.setMode(HAHVAC::AutoMode);
//         } else if(mode == MODE::Manual) {
//             switch(state) {
//                 case STATE::Heat:
//                     hvac.setMode(HAHVAC::HeatMode);
//                     break;

//                 case STATE::Cool:
//                     hvac.setMode(HAHVAC::CoolMode);
//                     break;

//                 case STATE::Fan:
//                     hvac.setMode(HAHVAC::FanOnlyMode);
//                     break;

//                 case STATE::Idle:
//                     hvac.setMode(HAHVAC::DryMode);
//                     break;
//             }
//         }

//         // =========================
//         // HVAC ACTION
//         // =========================

//         if(mode == MODE::Off) {
//             Serial.println("Sending off state...");
//             hvac.setAction(HAHVAC::OffAction);
//         } else {
//             switch(state) {
//                 case STATE::Heat:
//                     hvac.setAction(HAHVAC::HeatingAction);
//                     break;

//                 case STATE::Cool:
//                     hvac.setAction(HAHVAC::CoolingAction);
//                     break;

//                 case STATE::Fan:
//                     hvac.setAction(HAHVAC::FanAction);
//                     break;

//                 case STATE::Idle:
//                     hvac.setAction(HAHVAC::IdleAction);
//                     break;
//             }
//         }
//     }

//     Serial.print("Updating shared state to: ");
//     Serial.println(state);
// }


void publishHAState(const ThermostatState& state) {
    if (whoAmI() != ROLE::PARENT) {
        return;
    }

    if (!mqttMutex) {
        return;
    }

    // Sending to Home Assistant
    // Sending mode
    if(state.mode == MODE::Off) {
        hvac.setMode(HAHVAC::OffMode);
    } else if (state.mode == MODE::Auto) {
        hvac.setMode(HAHVAC::AutoMode);
    } else if (state.mode == MODE::Manual) {
        switch(state.state) {
            case STATE::Heat:
                hvac.setMode(HAHVAC::HeatMode);
                break;

            case STATE::Cool:
                hvac.setMode(HAHVAC::CoolMode);
                break;

            case STATE::Fan:
                hvac.setMode(HAHVAC::FanOnlyMode);
                break;

            case STATE::Idle:
                hvac.setMode(HAHVAC::DryMode);
                break;
        }
    }

    // Sending state
    if(state.mode == MODE::Off) {
        hvac.setAction(HAHVAC::OffAction);
    } else {
        switch(state.state) {
            case STATE::Heat:
                hvac.setAction(HAHVAC::HeatingAction);
                break;

            case STATE::Cool:
                hvac.setAction(HAHVAC::CoolingAction);
                break;

            case STATE::Fan:
                hvac.setAction(HAHVAC::FanAction);
                break;

            case STATE::Idle:
                hvac.setAction(HAHVAC::IdleAction);
                break;
        }
    }

    // Sending current temperature
    hvac.setCurrentTemperature(state.temp);

    // Sending goal temperature
    hvac.setTargetTemperature(state.goalTemp);
}


// Sending state and commands between parent and child thermostats

void publishSyncState(const ThermostatState& state) {
    if (whoAmI() != ROLE::PARENT) {
        return;
    }

    if (!mqttMutex) {
        return;
    }

    // Sending to child thermostat
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(syncStateTopic, serializeThermostatState(state).c_str());
    xSemaphoreGive(mqttMutex);
}

void sendSyncCommand(const Command& cmd) {
    if (whoAmI() != ROLE::CHILD) {
        return;
    }

    if (!mqttMutex) {
        return;
    }

    // Sending to parent thermostat
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(syncCommandTopic, serializeCommand(cmd).c_str());
    xSemaphoreGive(mqttMutex);
}






// Callbacks for Home Assistant commands

void onGoalTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
    // This is from Home Assistant, so we are parent
    float temperatureFloat = temperature.toFloat();
    
    Serial.print("Target (goal) temperature: ");
    Serial.println(temperatureFloat);

    // Update model
    model->setGoalTemp(temperatureFloat);
    
    // Send to child thermostat
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toChildGoalTempTopic, String(temperatureFloat).c_str());
    xSemaphoreGive(mqttMutex);
}

void onModeCommand(HAHVAC::Mode mode, HAHVAC* sender) {
    Serial.print("Mode: ");
    if (mode == HAHVAC::OffMode) {
        Serial.println("off");

        // Update model
        model->setMode(MODE::Off);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Off).c_str());
        xSemaphoreGive(mqttMutex);
        
    }else if (mode == HAHVAC::AutoMode) {
        Serial.println("auto");

        // Update model
        model->setMode(MODE::Auto);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Auto).c_str());
        xSemaphoreGive(mqttMutex);
    } else if (mode == HAHVAC::CoolMode) {
        Serial.println("cool");

        // Update model
        model->setMode(MODE::Manual);
        model->requestManualState(STATE::Cool);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Manual).c_str());
        xSemaphoreGive(mqttMutex);

        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildStateTopic, String((int)STATE::Cool).c_str());
        xSemaphoreGive(mqttMutex);
    } else if (mode == HAHVAC::HeatMode) {
        Serial.println("heat");

        // Update model
        model->setMode(MODE::Manual);
        model->requestManualState(STATE::Heat);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Manual).c_str());
        xSemaphoreGive(mqttMutex);

        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildStateTopic, String((int)STATE::Heat).c_str());
        xSemaphoreGive(mqttMutex);
    }else if (mode == HAHVAC::FanOnlyMode) {
        Serial.println("fan");

        // Update model
        model->setMode(MODE::Manual);
        model->requestManualState(STATE::Fan);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Manual).c_str());
        xSemaphoreGive(mqttMutex);

        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildStateTopic, String((int)STATE::Fan).c_str());
        xSemaphoreGive(mqttMutex);
    } else if (mode == HAHVAC::DryMode) {
        Serial.println("(dry) manual");

        // Update model
        model->setMode(MODE::Manual);

        // Sending to child thermostat
        xSemaphoreTake(mqttMutex, portMAX_DELAY);
        mqtt.publish(toChildModeTopic, String((int)MODE::Manual).c_str());
        xSemaphoreGive(mqttMutex);
    } else {
        Serial.print("Wasn't planned for... FREAK OUT!!!! Mode:");
        Serial.println(mode);
    } 

    // sender->setMode(mode); // report mode back to the HA panel
}



// Callbacks for MQTT messages from parent/child and Home Assistant

void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
    // This callback is called when message from MQTT broker is received.
    char msg[128];
    if(length >= sizeof(msg)) {
        length = sizeof(msg) - 1;
    }

    memcpy(msg, payload, length);
    msg[length] = '\0';

    Serial.print("New message on topic: ");
    Serial.println(topic);

    Serial.print("Data: ");
    Serial.println(msg);

    if(whoAmI() == ROLE::CHILD && strcmp(topic, remoteStateTopic) == 0) {
        Serial.println("Received parent sync state update");
        ThermostatState remoteState;
        if(deserializeThermostatState(msg, remoteState)) {
            model->applyRemoteState(remoteState);
        } else {
            Serial.println("Failed to deserialize sync state payload");
        }
    } else if(whoAmI() == ROLE::PARENT && strcmp(topic, syncCommandTopic) == 0) {
        Serial.println("Received sync command from child");
        Command cmd;
        if(deserializeCommand(msg, cmd)) {
            switch(cmd.type) {
                case COMMAND_TYPE::SetMode:
                    model->setMode(cmd.mode);
                    break;
                case COMMAND_TYPE::SetTempGoal:
                    model->setGoalTemp(cmd.tempGoal);
                    break;
                case COMMAND_TYPE::SetState:
                    model->requestManualState(cmd.state);
                    break;
                case COMMAND_TYPE::SetTemp:
                    model->setTemp(cmd.temp);
                    break;
            }
        } else {
            Serial.println("Failed to deserialize sync command payload");
        }
    }

    // Regardless of if we are a parent of child, show the set temp from home assistant
    if (strcmp(topic, tempCmdTopic) == 0) {
        Serial.println("Received temperature update from Home Assistant");
        float newGoalTemp = atof(msg);

        if(newGoalTemp < MAX_GOAL_TEMP && newGoalTemp > MIN_GOAL_TEMP) {
            // childOnTempGoal(newGoalTemp);
            // TODO: Replace with model method
        };
    }
}

void onMqttConnected() {
    Serial.println("Connected to the broker!");

    if(whoAmI() == ROLE::PARENT) {
        // To receive command requests from children
        mqtt.subscribe(syncCommandTopic);
    } else {
        // To receive parent state updates
        mqtt.subscribe(remoteStateTopic);
    }
}

void onMqttDisconnected() {
    Serial.println("Disconnected from the broker!");
}


// Function for starting an asyncronous network scan
void scanAvailableNetworks() {
    Serial.println("Starting network scan...");
    WiFi.scanDelete(); // Clear previous scan results
    
    WiFi.scanNetworks(true); // Starting an async scan
}

// Getter function to access the available networks
void getAvailableNetworks(WiFiNetwork* networks, int& networkCount) {
    Serial.println("Getting available networks...");
    int scanResult = WiFi.scanComplete();
    
    if (scanResult == WIFI_SCAN_FAILED || scanResult <= 0) {
        networkCount = 0;
        return;
    }

    networkCount = (scanResult < 16) ? scanResult : 16;

    for (int i = 0; i < networkCount; i++) {
        strncpy(networks[i].ssid, WiFi.SSID(i).c_str(), 32);
        networks[i].ssid[32] = '\0';
        networks[i].rssi = WiFi.RSSI(i);
    }
    Serial.printf("Found %d networks.\n", networkCount);
}

// Function to set WiFi credentials and signal the task to reconnect
void setWiFiCredentials(char* ssid, char* password) {
    storeNetworkSSID(ssid);
    storeNetworkPWD(password);
    wifiCredentialsChanged = true;  // Signal the task to reconnect
    Serial.printf("WiFi credentials updated for SSID: %s\n", ssid);
}


// Task for handling WiFi and MQTT connections

void wifiMqttTask(void* parameter) {
    // Wait for ESP-NOW setup to complete
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay to reduce task load

    // Generating hostname for the network to see
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char hostname[64];
    snprintf(
        hostname,
        sizeof(hostname),
        "%s-%02X%02X%02X",
        deviceName,
        mac[0],
        mac[2],
        mac[4]
    );

    WiFi.setHostname(hostname);
    Serial.println("Starting WiFi STA connection to router...");

    int attempts = 0;

    while (true) {
        // Check if credentials changed
        if (wifiCredentialsChanged) {
            Serial.println("Credentials changed, reconnecting...");
            WiFi.disconnect(true);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            wifiCredentialsChanged = false;
            attempts = 0;
        }

        char currentWifiSSID[33];
        getStoredNetworkSSID(currentWifiSSID, sizeof(currentWifiSSID));
        char currentWifiPassword[65];
        getStoredNetworkPWD(currentWifiPassword, sizeof(currentWifiPassword));

        // (Re)start WiFi connection if not connected
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect(true); // Disconnect and clear config
            vTaskDelay(500 / portTICK_PERIOD_MS);
            // Serial.printf("++ Connecting to SSID: %s\n", currentWifiSSID);
            // Serial.printf("++ Using Password: %s\n", currentWifiPassword);
            WiFi.begin(currentWifiSSID, currentWifiPassword);
            attempts = 0;

            while (WiFi.status() != WL_CONNECTED && attempts < 30) {
                Serial.printf("WiFi connection attempt %d/30...\n", attempts + 1);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                attempts++;

                if (wifiCredentialsChanged) {
                    Serial.println("Credentials changed during connection attempts, restarting...");
                    break; // Break to outer loop to handle credential change
                }
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFi STA connected to router!");
                Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());

                // Start MQTT only after successful WiFi connection
                // mqtt.begin("10.1.10.132", 1883, "thermostat", "thermostat");
                mqtt.begin(HAaddr, 1883, "thermostat_user", "thermostat");
                mqtt.onMessage(onMqttMessage);
                mqtt.onConnected(onMqttConnected);
                mqtt.onDisconnected(onMqttDisconnected);
            } else {
                Serial.println("Failed to connect WiFi STA to router, restarting WiFi...");
                WiFi.disconnect(true); // Reset WiFi
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue; // Try again
            }
        }

        // Handle MQTT loop
        if (WiFi.status() == WL_CONNECTED) {
            mqtt.loop(); // Handle MQTT operations only when WiFi is connected
        } else {
            // Try to reconnect WiFi if disconnected
            attempts++;
            if (attempts >= 30) {
                Serial.println("WiFi disconnected, restarting WiFi after 30 failed attempts...");
                WiFi.disconnect(true); // Reset WiFi
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue; // Restart connection attempts
            } else {
                Serial.println("WiFi disconnected, attempting reconnection...");
                WiFi.begin(currentWifiSSID, currentWifiPassword);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);   // Delay to reduce task load
    }
}

void setupMQTT() {
    Serial.println("Setting up MQTT...");

    mqttMutex = xSemaphoreCreateMutex();

    // Print out the MAC address of the device
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    WiFi.macAddress(mac);

    // Populating MQTT topics
    char* parentMac = getParentMac();
    Serial.printf(parentMac);
    snprintf(tempStatTopic, sizeof(tempStatTopic),
            "aha/%s/%s/temp_stat_t",
            parentMac, deviceName);
    snprintf(tempCmdTopic, sizeof(tempCmdTopic),
            "aha/%s/%s/temp_cmd_t",
            parentMac, deviceName);
    snprintf(modeCmdTopic, sizeof(modeCmdTopic),
            "aha/%s/%s/mode_cmd_t",
            parentMac, deviceName);
    snprintf(actTopic, sizeof(actTopic),
            "aha/%s/%s/act_t",
            parentMac, deviceName);

    snprintf(toChildGoalTempTopic, sizeof(toChildGoalTempTopic),
            "privSync/%s/toChild/goal_temp", deviceName);
    snprintf(toChildTempTopic, sizeof(toChildGoalTempTopic),
            "privSync/%s/toChild/temp", deviceName);
    snprintf(toChildModeTopic, sizeof(toChildGoalTempTopic),
            "privSync/%s/toChild/mode", deviceName);
    snprintf(toChildStateTopic, sizeof(toChildGoalTempTopic),
            "privSync/%s/toChild/state", deviceName);
    snprintf(toChildUnlockedTopic, sizeof(toChildGoalTempTopic),
            "privSync/%s/toChild/unlocked", deviceName);

    snprintf(toParentGoalTempTopic, sizeof(toParentGoalTempTopic),
            "privSync/%s/toParent/goal_temp", deviceName);
    snprintf(toParentTempTopic, sizeof(toParentTempTopic),
            "privSync/%s/toParent/temp", deviceName);
    snprintf(toParentModeTopic, sizeof(toParentModeTopic),
            "privSync/%s/toParent/mode", deviceName);
    snprintf(toParentStateTopic, sizeof(toParentStateTopic),
            "privSync/%s/toParent/state", deviceName);
    snprintf(toParentUnlockedTopic, sizeof(toParentUnlockedTopic),
            "privSync/%s/toParent/unlocked", deviceName);

    char parentSyncId[32];
    snprintf(parentSyncId, sizeof(parentSyncId), "%s", getParentMac());
    snprintf(syncStateTopic, sizeof(syncStateTopic), "sync/%s/state", parentSyncId);
    snprintf(syncCommandTopic, sizeof(syncCommandTopic), "sync/%s/command", parentSyncId);
    snprintf(remoteStateTopic, sizeof(remoteStateTopic), "%s", syncStateTopic);
    snprintf(remoteCommandTopic, sizeof(remoteCommandTopic), "%s", syncCommandTopic);
    
    Serial.println();
    Serial.println("Connecting to the network...");
    Serial.println();

    device.setUniqueId(mac, sizeof(mac));
    device.setName(deviceName);
    device.setSoftwareVersion(Version);

    if (whoAmI() == ROLE::PARENT) {
        // Assigning callbacks
        hvac.onTargetTemperatureCommand(onGoalTemperatureCommand);
        hvac.onModeCommand(onModeCommand);
        // hvac.setObjectId("TM2");
    
        hvac.setRetain(false);
        hvac.setName(deviceName);
        hvac.setMinTemp(MIN_GOAL_TEMP);
        hvac.setMaxTemp(MAX_GOAL_TEMP);
        hvac.setTempStep(1);
        hvac.setTargetTemperature(61);
        hvac.setModes(HAHVAC::OffMode | HAHVAC::AutoMode | HAHVAC::HeatMode | HAHVAC::CoolMode | HAHVAC::FanOnlyMode | HAHVAC::DryMode);
        hvac.setMode(HAHVAC::OffMode);
        hvac.setAction(HAHVAC::IdleAction);
    }

    xTaskCreatePinnedToCore(
        wifiMqttTask,       // Task function
        "WiFiMQTTTask",     // Name of the task
        8192,               // Stack size (in bytes)
        NULL,               // Task input parameter
        1,                  // Priority
        &wifiMqttTaskHandle,// Task handle
        0                   // Core 0
    );
}


bool printedWifiConnection = false;
void loopMQTT() {
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    if(!wifiConnected) {
        return;
    }

    if(!printedWifiConnection) {
        Serial.println("Wifi connected");
        printedWifiConnection = true;
    }
}






void sendAutoButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentModeTopic, String((int)MODE::Auto).c_str());
    xSemaphoreGive(mqttMutex);
}
void sendManualButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentModeTopic, String((int)MODE::Manual).c_str());
    xSemaphoreGive(mqttMutex);
}
void sendOffButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentModeTopic, String((int)MODE::Off).c_str());
    xSemaphoreGive(mqttMutex);
}
void sendFanButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentStateTopic, String((int)STATE::Fan).c_str());
    xSemaphoreGive(mqttMutex);
}
void sendCoolButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentStateTopic, String((int)STATE::Cool).c_str());
    xSemaphoreGive(mqttMutex);
}
void sendHeatButtonClick() {
    xSemaphoreTake(mqttMutex, portMAX_DELAY);
    mqtt.publish(toParentStateTopic, String((int)STATE::Heat).c_str());
    xSemaphoreGive(mqttMutex);
}