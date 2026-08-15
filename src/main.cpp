#include <Arduino.h>
#include <cmath>
#include <nvs_flash.h>
// #include "./homeAssistant.h"
#include "./displaySetup.h"
#include "./menu.h"
// #include "./stateMachine.h"
#include "./remoteThermostat.h"
#include "./storage.h"
#include "./thermometers.h"
#include "./locking.h"

#include "./core/ThermostatModel.h"
#include "./view/ThermostatView.h"

ThermostatModel* model;
SyncManager* syncManager;
ThermostatView view;






// Callback functions for UI buttons
void onTempUpButtonClick(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
      return;
  }
  printf("autoBTN1 clicked\n");
  const float currentGoal = model->getGoalTemp();
  printf("Current temperature goal: %.1f\n", currentGoal);

  if (currentGoal < MAX_GOAL_TEMP) {
      model->setGoalTemp(currentGoal + 1.0);
  } else {
      model->setGoalTemp(MAX_GOAL_TEMP);
  }
}

void onTempDownButtonClick(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
      return;
  }
  printf("autoBTN2 clicked\n");
  const float currentGoal = model->getGoalTemp();
  printf("Current temperature goal: %.1f\n", currentGoal);

  if (currentGoal > MIN_GOAL_TEMP) {
      model->setGoalTemp(currentGoal - 1.0);
  } else {
      model->setGoalTemp(MIN_GOAL_TEMP);
  }
}

void onManualHeatClick() {  
  if(whoAmI() == ROLE::PARENT) {
    Serial.println("Manual heat button clicked");
    Serial.println("Current state: " + String(model->getCurrentState()));
    Serial.println("Current goal state: " + String(model->getGoalState()));
    if(model->getCurrentState() == STATE::Heat || model->getGoalState() == GOAL_STATE::AwaitingHeat) {
      model->requestManualState(STATE::Idle);
    } else {
      model->requestManualState(STATE::Heat);
    }
  } else {
    // sendHeatButtonClick();
  }
}

void onManualCoolClick() {
  if(whoAmI() == ROLE::PARENT) {
    if(model->getCurrentState() == STATE::Cool || model->getGoalState() == GOAL_STATE::AwaitingCool) {
      model->requestManualState(STATE::Idle);
    } else {
      model->requestManualState(STATE::Cool);
    }
  } else {
    // sendCoolButtonClick();
  }
}

void onManualFanClick() {
  if(whoAmI() == ROLE::PARENT) {
    if(model->getCurrentState() == STATE::Fan) {
      model->requestManualState(STATE::Idle);
    }else {
      model->requestManualState(STATE::Fan);
    }
  }else {
    // sendFanButtonClick();
  }
    
}

void onOFFButtonClick() {
  if (whoAmI() == ROLE::CHILD) {
    // sendOffButtonClick();
    return;
  }

  printf("Off button selected\n");
  model->setMode(MODE::Off);
}

void onONButtonClick() {
  printf("On button selected\n");
  model->restoreLastMode();
}

void onManualButtonClick() {
  printf("Manual mode selected\n");
  model->setMode(MODE::Manual);
}

void onAutoButtonClick() {
  printf("Auto mode selected\n");
  model->setMode(MODE::Auto);
}

void onLockButtonClick() {
  printf("Lock button selected\n");
}




// ===== UI UPDATING CALLBACKS =====
// Safe for cross-task use
volatile bool flag_offButton = false;
volatile bool flag_manualButton = false;
volatile bool flag_autoButton = false;
volatile bool flag_manualHeatButton = false;
volatile bool flag_manualCoolButton = false;
volatile bool flag_manualFanButton = false;

volatile bool parentGoalNeedsUpdate = false;
volatile bool childGoalNeedsUpdate = false;
volatile bool TempNeedsUpdate = false;
volatile bool parentModeNeedsUpdate = false;
volatile bool childModeNeedsUpdate = false;
volatile STATE possibleState = STATE::Idle;
volatile bool parentStateNeedsUpdate = false;
volatile bool childStateNeedsUpdate = false;





void setup()
{
  Serial.begin(115200);
  Serial.println("Setup...");

  pinMode(GPIO_RELAY1, OUTPUT);
  pinMode(GPIO_RELAY2, OUTPUT);
  pinMode(GPIO_RELAY3, OUTPUT);

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      nvs_flash_erase();
      err = nvs_flash_init();
  }
  if (err != ESP_OK) {
      Serial.printf("NVS init failed: %d\n", err);
  }


  // Set up ESP-NOW first (this will set the WiFi channel)
  // makeThermostatConnection();

  // initializeStorage();
  
  // delay(4000);
  
  // NOTE: Needs to happen before initializing storage
  //    because I think it messes with SPI stuff
  setupDisplay();

  delay(500);
  
  initializeStorage();
  
  delay(500);
  
  // This starts the BLE scan task for thermometers
  startBLESensorScan();
  
  setupMQTT();

  // Create the thermostat model and managers
  syncManager = new SyncManager();
  model = new ThermostatModel(ROLE::PARENT, *syncManager);

  // NOTE: Should come after storage initialization
  MODE lastMode = getStoredLastMode();
  float lastTempGoal = getStoredTempGoal();
  float lastTemp = getStoredTemp();
  STATE lastHeavyState = getStoredLastHeavyState();
  model->initializeFromStorage(lastMode, lastTempGoal, lastTemp, lastHeavyState);

  // Initialize and bind the view to the model
  view.initialize();
  view.bind(model);
  view.render();

  // Disable scrolling on the main screen
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);



  // // Create a button
  // lv_obj_t *restartButton = lv_btn_create(lv_scr_act());
  // lv_obj_set_size(restartButton, 100, 50);
  // lv_obj_align(restartButton, LV_ALIGN_BOTTOM_MID, 0, 0);

  // // Add a label to the button
  // lv_obj_t *label = lv_label_create(restartButton);
  // lv_label_set_text(label, "Restart");
  // lv_obj_set_style_bg_color(restartButton, fix_color(255,52,52), LV_PART_MAIN);
  // lv_obj_center(label);

  // // Add an event callback to the button
  // lv_obj_add_event_cb(restartButton, [](lv_event_t *e) {
  //   lv_event_code_t code = lv_event_get_code(e);
  //   if (code == LV_EVENT_CLICKED) {
  //     ESP.restart(); // Restart the ESP32
  //   }
  // }, LV_EVENT_ALL, NULL);
  
  // Thermostat view setup is handled by ThermostatView
  UIinitializeMenu();
  UIinitializeSettings();
  UIinitializeNetwork();
  UIinitializeThermometers();



  // float currentGoal = getTempGoal();
  // UIgoalSet(currentGoal);

  // MODE currentMode = getCurrentMode();
  // Serial.print("Current mode: ");
  // Serial.println((int)currentMode);

  // if(currentMode == MODE::Manual) {
  //   onManualButtonClick();
  // } else if(currentMode == MODE::Auto) {
  //   onAutoButtonClick();
  // } else if(currentMode == MODE::Off) {
  //   onOFFButtonClick();
  // }

  // STATE currentState = getCurrentState();
  // updateUIfromStates(currentState);
  // UIsetManualBTNState(currentState);

  // onOFFButtonClick();

  // Starting the system in a sort of 'off' mode
  // The view is already bound to the model and will render mode = Off.

  printf("Setup done\n");

  // Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
}

void loop() {  
  lv_timer_handler(); /* let the GUI do its work */
  delay(10);

  loopMQTT();

  // view.render();

  if(flag_offButton) {
    flag_offButton = false;
    onOFFButtonClick();
  }
  if(flag_manualButton) {
    flag_manualButton = false;
    onManualButtonClick();
  }
  if(flag_autoButton) {
    flag_autoButton = false;
    onAutoButtonClick();
  }
  if(flag_manualHeatButton) {
    flag_manualHeatButton = false;
    onManualHeatClick();
  }
  if(flag_manualCoolButton) {
    flag_manualCoolButton = false;
    onManualCoolClick();
  }
  if(flag_manualFanButton) {
    flag_manualFanButton = false;
    onManualFanClick();
  }

  if(parentGoalNeedsUpdate) {
    float newTempGoal = model->getGoalTemp();
    Serial.print("[PARENT] New temp goal: ");
    Serial.println(newTempGoal);
    model->setGoalTemp(newTempGoal);
    parentGoalNeedsUpdate = false;
  }
  if(childGoalNeedsUpdate) {
    float newTempGoal = model->getGoalTemp();
    Serial.print("[CHILD] New temp goal: ");
    Serial.println(newTempGoal);
    model->setGoalTemp(newTempGoal);
    childGoalNeedsUpdate = false;
  }

  if(TempNeedsUpdate) {
    float newTemp = model->getTemp();
    model->setTemp(newTemp);
    TempNeedsUpdate = false;
  }

  if(parentModeNeedsUpdate) {
    MODE newMode = model->getMode();
    if(newMode == MODE::Manual) {
      onManualButtonClick();
    } else if(newMode == MODE::Auto) {
      onAutoButtonClick();
    } else if(newMode == MODE::Off) {
      onOFFButtonClick();
      Serial.println("Turning off thermostat");
    }
    parentModeNeedsUpdate = false;
  }
  if(childModeNeedsUpdate) {
    MODE newMode = model->getMode();
    if(newMode == MODE::Manual) {
      onManualButtonClick();
    } else if(newMode == MODE::Auto) {
      onAutoButtonClick();
    } else if(newMode == MODE::Off) {
      onOFFButtonClick();
      Serial.println("Turning off thermostat");
    }

    childModeNeedsUpdate = false;
  }

  if(parentStateNeedsUpdate) {
    parentStateNeedsUpdate = false;
  }
  if(childStateNeedsUpdate) {
    childStateNeedsUpdate = false;
  }

  // float temp123 = getBTHomeTemperature();
  // Serial.print("BTHome temperature: ");
  // Serial.println(temp123);
} 