#include "remoteThermostat.h"
#include "stateMachine.h"

TaskHandle_t wifiMqttTaskHandle = NULL;

// Making wifi using classes
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

bool wifiCredentialsChanged = false;

char deviceName[18] = "Gaming_Thermostat";
char HAaddr[12] = "10.1.10.132";
// char HAaddr[12] = "10.0.0.27";

// Intializing HVAC object 
HAHVAC hvac(
  deviceName,
  HAHVAC::TargetTemperatureFeature | HAHVAC::PowerFeature | HAHVAC::ModesFeature | HAHVAC::ActionFeature
);



void updateSharedTemp(float temp) {
    // Sending to homeassistant or remote thermostat
    if(whoAmI() == PEERTYPE::PARENT) {
        hvac.setCurrentTemperature(temp);
    } else {
        mqtt.publish("home/teen_room/current_temp", String(temp).c_str());
    }

    Serial.print("Updating shared temperature to: ");
    Serial.println(temp);
}

void updateSharedTempGoal(float goalTemp) {
    hvac.setTargetTemperature(goalTemp);
    // Sending to homeassistant or remote thermostat
    if(whoAmI() == PEERTYPE::PARENT) {
    } else {
        mqtt.publish("home/teen_room/goal_temp", String(goalTemp).c_str());
    }

    Serial.print("Updating shared goal temperature to: ");
    Serial.println(goalTemp);
}

void updateSharedMode(MODE mode) {
    // Sending to homeassistant or remote thermostat
    if(whoAmI() == PEERTYPE::PARENT) {
        if(mode == MODE::Off) {
            hvac.setMode(HAHVAC::OffMode);
        } else if (mode == MODE::Auto) {
            hvac.setMode(HAHVAC::AutoMode);
        } else if (mode == MODE::Manual) {
            STATE currentState = getCurrentState();
            if(currentState == STATE::Cool || currentState == STATE::AwaitingCool) {
                hvac.setMode(HAHVAC::CoolMode);
            } else if (currentState == STATE::Heat || currentState == STATE::AwaitingHeat) {
                hvac.setMode(HAHVAC::HeatMode);
            } else if (currentState == STATE::Fan) {
                hvac.setMode(HAHVAC::FanOnlyMode);
            } else if (currentState == STATE::Idle) {
                hvac.setMode(HAHVAC::DryMode); // Assuming Manual is equivalent to DryMode mode
            }
        }
    }else {
        mqtt.publish("home/teen_room/mode", String((int)mode).c_str());
    }

    Serial.print("Updating shared mode to: ");
    Serial.println(mode);
}

void updateSharedState(STATE state) {
    MODE mode = getCurrentMode();
    // Sending to homeassistant or remote thermostat
    if(whoAmI() == PEERTYPE::PARENT) {
        if(state == STATE::Idle) {
            if(mode == MODE::Manual) {
                hvac.setMode(HAHVAC::DryMode); // Assuming Manual Idle is equivalent to DryMode mode
            }
            hvac.setAction(HAHVAC::IdleAction);
        } else if (state == STATE::Heat) {
            if(mode == MODE::Manual) {
                hvac.setMode(HAHVAC::HeatMode);
            }
            hvac.setAction(HAHVAC::HeatingAction);
        } else if (state == STATE::AwaitingHeat) {
            if(mode == MODE::Manual) {
                hvac.setMode(HAHVAC::HeatMode);
            }
            hvac.setAction(HAHVAC::IdleAction);
        } else if (state == STATE::Cool) {
            if(mode == MODE::Manual) {
                hvac.setMode(HAHVAC::CoolMode);
            }
            hvac.setAction(HAHVAC::CoolingAction);
        } else if (state == STATE::AwaitingCool) {
            if(mode == MODE::Manual) {
                hvac.setMode(HAHVAC::CoolMode);
            }
            hvac.setAction(HAHVAC::IdleAction);
        } else if (state == STATE::Fan) {
            hvac.setAction(HAHVAC::FanAction);
            hvac.setMode(HAHVAC::FanOnlyMode);
        }
    } else {
        mqtt.publish("home/teen_room/state", String((int)state).c_str());
    }

    Serial.print("Updating shared state to: ");
    Serial.println(state);
}







void onGoalTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
    // This is from Home Assistant, so we are parent
    float temperatureFloat = temperature.toFloat();

    Serial.print("Target (goal) temperature: ");
    Serial.println(temperatureFloat);

    sender->setTargetTemperature(temperature); // report target temperature back to the HA panel
    onRemoteTempGoal(temperatureFloat); // Calling callback function
}

void onModeCommand(HAHVAC::Mode mode, HAHVAC* sender) {
    Serial.print("Mode: ");
    if (mode == HAHVAC::OffMode) {
        Serial.println("MODE: off");
        onRemoteMode(MODE::Off);
        sender->setCurrentAction(HAHVAC::OffAction);
    }else if (mode == HAHVAC::AutoMode) {
        Serial.println("MODE: auto");
        onRemoteMode(MODE::Auto);
    } else if (mode == HAHVAC::CoolMode) {
        Serial.println("MODE: cool");
        onRemoteMode(MODE::Manual);
        onRemoteState(STATE::Cool);
    } else if (mode == HAHVAC::HeatMode) {
        Serial.println("MODE: heat");
        onRemoteMode(MODE::Manual);
        onRemoteState(STATE::Heat);
    }else if (mode == HAHVAC::FanOnlyMode) {
        Serial.println("MODE: fan");
        onRemoteMode(MODE::Manual);
        onRemoteState(STATE::Fan);
    } else if (mode == HAHVAC::DryMode) {
        Serial.println("MODE: (dry) manual");
        onRemoteMode(MODE::Manual);
        sender->setCurrentAction(HAHVAC::IdleAction);
    } else {
        Serial.print("Mode was recived that wasn't planned for... ");
        Serial.println(mode);
    } 

    sender->setMode(mode); // report mode back to the HA panel
}





void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
    // This callback is called when message from MQTT broker is received.
    // Please note that you should always verify if the message's topic is the one you expect.
    // For example: if (memcmp(topic, "myCustomTopic") == 0) { ... }

    if(whoAmI() == PEERTYPE::CHILD) {
        if (strcmp(topic, "aha/8cbfea0ed0d4/Teen_Room/temp_stat_t") == 0) {
            Serial.println("Received temperature update from Home Assistant");
            float newGoalTemp = atof((const char*)payload);
    
            if(newGoalTemp < MAX_GOAL_TEMP && newGoalTemp > MIN_GOAL_TEMP) {
                onRemoteTempGoal(newGoalTemp);
            };
        }else if (strcmp(topic, "aha/8cbfea0ed0d4/Teen_Room/mode_cmd_t") == 0) {
            Serial.println("Received mode update from Home Assistant");
            if (strstr((const char*)payload, "auto") != nullptr) {
                onRemoteMode(MODE::Auto);
            }else if (strstr((const char*)payload, "off") != nullptr) {
                onRemoteMode(MODE::Off);
            }else if (strstr((const char*)payload, "dry") != nullptr) {
                onRemoteMode(MODE::Manual); // NOTE: DRY means MANUAL
            }else if (strstr((const char*)payload, "heat") != nullptr) {
                onRemoteMode(MODE::Manual);
                onRemoteState(STATE::Heat);
            }else if (strstr((const char*)payload, "cool") != nullptr) {
                onRemoteMode(MODE::Manual);
                onRemoteState(STATE::Cool);
            }else if (strstr((const char*)payload, "fan_only") != nullptr) {
                onRemoteMode(MODE::Manual);
                onRemoteState(STATE::Fan);
            };
        }else if (strcmp(topic, "aha/8cbfea0ed0d4/Teen_Room/act_t") == 0) {
            Serial.println("Received action update from Home Assistant");
            if(strstr((const char*)payload, "idle") != nullptr) {
                onRemoteState(STATE::Idle);
            }else if(strstr((const char*)payload, "heating") != nullptr) {
                onRemoteState(STATE::Heat);
            }else if(strstr((const char*)payload, "cooling") != nullptr) {
                onRemoteState(STATE::Cool);
            }else if(strstr((const char*)payload, "fan") != nullptr) {
                onRemoteState(STATE::Fan);
            }
        }
    }else {
        if(strcmp(topic, "home/teen_room/goal_temp") == 0) {
            Serial.println("Received goal temperature update from remote thermostat");
            // Passing on command to home assistant
            float newGoalTemp = atof((const char*)payload);
    
            if(newGoalTemp < MAX_GOAL_TEMP && newGoalTemp > MIN_GOAL_TEMP) {
                onHATempGoal(newGoalTemp);
            };
        }else if(strcmp(topic, "home/teen_room/current_temp") == 0) {
            Serial.println("Received current temperature update from remote thermostat");
            float newTemp = atof((const char*)payload);
            onRemoteTemp(newTemp);
        }else if(strcmp(topic, "home/teen_room/mode") == 0) {
            Serial.println("Received mode update from remote thermostat");
            // Passing on command to home assistant
            int newMode = (int)atoi((const char*)payload);
            Serial.println("New mode: ");
            Serial.println((int)newMode);
            onHARemoteMode(static_cast<MODE>(newMode));
        }else if(strcmp(topic, "home/teen_room/state") == 0) {
            Serial.println("Received state update from remote thermostat");
            // Passing on command to home assistant
            int newState = (int)atoi((const char*)payload);
            onHARemoteState(static_cast<STATE>(newState));
        }

    }

    if (strcmp(topic, "aha/dcb4d9049024/Teen_Room/temp_cmd_t") == 0) {
        Serial.println("Received temperature update from Home Assistant");
        float newGoalTemp = atof((const char*)payload);

        if(newGoalTemp < MAX_GOAL_TEMP && newGoalTemp > MIN_GOAL_TEMP) {
            onHATempGoal(newGoalTemp);
        };
    }

    Serial.print("New message on topic: ");
    Serial.println(topic);
    Serial.print("Data: ");
    Serial.println((const char*)payload);
}

void onMqttConnected() {
    Serial.println("Connected to the broker!");

    if(whoAmI() == PEERTYPE::PARENT) {
        // To recive messages from child devices
        mqtt.subscribe("home/teen_room/goal_temp");
        mqtt.subscribe("home/teen_room/current_temp");
        mqtt.subscribe("home/teen_room/mode");
        mqtt.subscribe("home/teen_room/state");
    }else {
        // To recive messages from home assistant, we are a child, and we don't get subscribed automatically/properly otherwise
        mqtt.subscribe("aha/8cbfea0ed0d4/Teen_Room/temp_stat_t");
        mqtt.subscribe("aha/8cbfea0ed0d4/Teen_Room/mode_cmd_t");
        mqtt.subscribe("aha/8cbfea0ed0d4/Teen_Room/act_t");
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



// void getAvailableNetworks(WiFiNetwork* networks, int& networkCount) {
//     networks = networkList;
//     networkCount = networkListCount;
// }
// void scanAvailableNetworks() {
//     networkListCount = 0;

//     int scanResult = WiFi.scanNetworks();
    
//     if (scanResult == WIFI_SCAN_FAILED || scanResult == 0) {
//         networkListCount = 0;
//     }

//     networkListCount = (scanResult < 16) ? scanResult : 16;

//     for (int i = 0; i < networkListCount; i++) {
//         strncpy(networkList[i].ssid, WiFi.SSID(i).c_str(), 32);
//         networkList[i].ssid[32] = '\0';
//         networkList[i].rssi = WiFi.RSSI(i);
//     }
    
//     WiFi.scanDelete();
// }


void setWiFiCredentials(char* ssid, char* password) {
    storeNetworkSSID(ssid);
    storeNetworkPWD(password);
    wifiCredentialsChanged = true;  // Signal the task to reconnect
    Serial.printf("WiFi credentials updated for SSID: %s\n", ssid);
}


void wifiMqttTask(void* parameter) {
    // Wait for ESP-NOW setup to complete
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay to reduce task load

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

    // Print out the MAC address of the device
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    WiFi.macAddress(mac);
    
    Serial.println();
    Serial.println("Connecting to the network...");
    Serial.println();

    device.setUniqueId(mac, sizeof(mac));
    device.setName(deviceName);
    device.setSoftwareVersion("1.2.5");

    if (whoAmI() == PEERTYPE::PARENT) {
        // Assigning callbacks
        hvac.onTargetTemperatureCommand(onGoalTemperatureCommand);
        hvac.onModeCommand(onModeCommand);
        // hvac.setObjectId("TM2");
    
        hvac.setRetain(true);
        hvac.setName(deviceName);
        hvac.setMinTemp(MIN_GOAL_TEMP);
        hvac.setMaxTemp(MAX_GOAL_TEMP);
        hvac.setTempStep(1);
        hvac.setRetain(true);
        // hvac.setTargetTemperature(22);
        hvac.setModes(HAHVAC::OffMode | HAHVAC::AutoMode | HAHVAC::HeatMode | HAHVAC::CoolMode | HAHVAC::FanOnlyMode | HAHVAC::DryMode);
        hvac.setMode(HAHVAC::OffMode);
        hvac.setAction(HAHVAC::IdleAction);
    }

    xTaskCreatePinnedToCore(
        wifiMqttTask,       // Task function
        "WiFiMQTTTask",     // Name of the task
        4096,               // Stack size (in bytes)
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