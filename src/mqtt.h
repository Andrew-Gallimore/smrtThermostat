// #ifndef HOME_ASSISTANT_H
// #define HOME_ASSISTANT_H

// #include <WiFi.h>
// #include <ArduinoHA.h>
// #include "credentials.h"

// unsigned long lastTempPublishAt = 0;
// float lastTemp = 0;

// TaskHandle_t wifiMqttTaskHandle = NULL;

// // Making wifi using classes
// WiFiClient client;
// HADevice device;
// HAMqtt mqtt(client, device);

// // Intializing HVAC object 
// HAHVAC hvac(
//   "Teen_Room",
//   HAHVAC::TargetTemperatureFeature | HAHVAC::PowerFeature | HAHVAC::ModesFeature | HAHVAC::ActionFeature
// );

// void onTargetTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
//     float temperatureFloat = temperature.toFloat();

//     Serial.print("Target temperature: ");
//     Serial.println(temperatureFloat);

//     sender->setTargetTemperature(temperature); // report target temperature back to the HA panel
//     onRemoteTempGoal(temperatureFloat); // Calling callback function
// }

// void onPowerCommand(bool state, HAHVAC* sender) {
//     if (state) {
//         Serial.println("Power on");
//     } else {
//         Serial.println("Power off");
//     }
// }

// void onModeCommand(HAHVAC::Mode mode, HAHVAC* sender) {
//     Serial.print("Mode: ");
//     if (mode == HAHVAC::OffMode) {
//         Serial.println("off");
//     } else if (mode == HAHVAC::AutoMode) {
//         Serial.println("auto");
//     } else if (mode == HAHVAC::CoolMode) {
//         Serial.println("cool");
//         sender->setCurrentAction(HAHVAC::CoolingAction);
//     } else if (mode == HAHVAC::HeatMode) {
//         Serial.println("heat");
//         sender->setCurrentAction(HAHVAC::HeatingAction);
//     } else if (mode == HAHVAC::FanOnlyMode) {
//         Serial.println("fan only");
//     } else {
//         Serial.print("Mode was recived that wasn't planned for... ");
//         Serial.println(mode);
//     }

//     sender->setMode(mode); // report mode back to the HA panel
// }

// void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
//     // This callback is called when message from MQTT broker is received.
//     // Please note that you should always verify if the message's topic is the one you expect.
//     // For example: if (memcmp(topic, "myCustomTopic") == 0) { ... }

//     Serial.print("New message on topic: ");
//     Serial.println(topic);
//     Serial.print("Data: ");
//     Serial.println((const char*)payload);
// }

// void onMqttConnected() {
//     Serial.println("Connected to the broker!");
// }

// void onMqttDisconnected() {
//     Serial.println("Disconnected from the broker!");
// }

// // WiFi and MQTT Task - Updated for AP+STA mode
// void wifiMqttTask(void* parameter) {
//     // Wait for ESP-NOW setup to complete
//     delay(2000);
    
//     Serial.println("Starting WiFi STA connection to router...");
    
//     // Connect to WiFi using STA interface (AP is already running for ESP-NOW)
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//     int attempts = 0;
//     while (WiFi.status() != WL_CONNECTED && attempts < 30) {
//         Serial.printf("WiFi connection attempt %d/30...\n", attempts + 1);
//         delay(1000);
//         attempts++;
//     }

//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("WiFi STA connected to router!");
//         Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        
//         // Start MQTT only after successful WiFi connection
//         mqtt.begin("10.0.0.132", 1883, "thermostat", "thermostat");
//         mqtt.onMessage(onMqttMessage);
//         mqtt.onConnected(onMqttConnected);
//         mqtt.onDisconnected(onMqttDisconnected);
//     } else {
//         Serial.println("Failed to connect WiFi STA to router");
//         // Don't start MQTT if WiFi failed
//     }

//     // Handle MQTT loop
//     while (true) {
//         if (WiFi.status() == WL_CONNECTED) {
//             mqtt.loop(); // Handle MQTT operations only when WiFi is connected
//         } else {
//             // Try to reconnect WiFi if disconnected
//             if (attempts < 30) {
//                 Serial.println("WiFi disconnected, attempting reconnection...");
//                 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//                 attempts++;
//             }
//         }
//         delay(100);   // Increased delay to reduce task load
//     }
// }

// void startMQTT() {
//     mqtt.begin("10.0.0.132", 1883, "thermostat", "thermostat");
// }

// void setupHomeAssistant() {
//     // Print out the MAC address of the device
//     uint8_t mac[6];
//     esp_read_mac(mac, ESP_MAC_WIFI_STA);
//     Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

//     WiFi.macAddress(mac);
//     device.setUniqueId(mac, sizeof(mac));
    
//     Serial.println();
//     Serial.println("Connecting to the network...");
//     Serial.println();

//     device.setName("Teen Room Thermostat");
//     device.setSoftwareVersion("1.0.0");

//     // Assigning callbacks
//     hvac.onTargetTemperatureCommand(onTargetTemperatureCommand);
//     hvac.onPowerCommand(onPowerCommand);
//     hvac.onModeCommand(onModeCommand);

//     hvac.setRetain(true);
//     hvac.setName("My HVAC");
//     hvac.setMinTemp(10);
//     hvac.setMaxTemp(30);
//     hvac.setTempStep(0.5);
//     hvac.setTargetTemperature(20);
//     hvac.setModes(HAHVAC::OffMode | HAHVAC::AutoMode | HAHVAC::HeatMode | HAHVAC::CoolMode | HAHVAC::FanOnlyMode);
//     hvac.setAction(HAHVAC::IdleAction);

//     xTaskCreatePinnedToCore(
//         wifiMqttTask,       // Task function
//         "WiFiMQTTTask",     // Name of the task
//         4096,               // Stack size (in bytes)
//         NULL,               // Task input parameter
//         1,                  // Priority
//         &wifiMqttTaskHandle,// Task handle
//         0                   // Core 0
//     );
// }

// bool printedWifiConnection = false;

// void loopHomeAssistant() {
//     bool wifiConnected = (WiFi.status() == WL_CONNECTED);
//     if(!wifiConnected) {
//         return;
//     }

//     if(!printedWifiConnection) {
//         Serial.println("Wifi connected");
//         printedWifiConnection = true;
//     }

//     if ((millis() - lastTempPublishAt) > 3000) {
//         hvac.setCurrentTemperature(lastTemp);
//         lastTempPublishAt = millis();
//         lastTemp += 0.5;
//     }
// }

// #endif //HOME_ASSISTANT_H