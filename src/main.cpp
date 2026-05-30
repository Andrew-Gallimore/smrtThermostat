#include <Arduino.h>
#include <cmath>
#include <nvs_flash.h>
// #include "./homeAssistant.h"
#include "./displaySetup.h"
#include "./ui.h"
#include "./menu.h"
#include "./stateMachine.h"
#include "./remoteThermostat.h"
#include "./storage.h"
#include "./thermometers.h"
#include "./locking.h"


void setRelaysFromState(STATE newState) {
  // Set relays based on the new state
  switch (newState) {
    case STATE::Heat:
      // Set relays for heating
      digitalWrite(GPIO_RELAY1, HIGH); // Turn on heating relay (pin 40)
      digitalWrite(GPIO_RELAY2, LOW);  // Ensure cooling relay is off
      digitalWrite(GPIO_RELAY3, HIGH);  // Ensure fan relay is off
      break;
    case STATE::Cool:
      // Set relays for cooling
      digitalWrite(GPIO_RELAY1, LOW);  // Ensure heating relay is off
      digitalWrite(GPIO_RELAY2, HIGH); // Turn on cooling relay (pin 2)
      digitalWrite(GPIO_RELAY3, HIGH);  // Ensure fan relay is off
      break;
    case STATE::Fan:
      // Set relays for fan
      digitalWrite(GPIO_RELAY1, LOW);  // Ensure heating relay is off
      digitalWrite(GPIO_RELAY2, LOW);  // Ensure cooling relay is off
      digitalWrite(GPIO_RELAY3, HIGH); // Turn on fan relay (pin 1)
      break;
    case STATE::Idle:
    case STATE::AwaitingCool:
    case STATE::AwaitingHeat:
      // Set relays for idle
      digitalWrite(GPIO_RELAY1, LOW);  // Ensure heating relay is off
      digitalWrite(GPIO_RELAY2, LOW);  // Ensure cooling relay is off
      digitalWrite(GPIO_RELAY3, LOW);  // Ensure fan relay is off
      break;
  }
}


void updateUIfromStates(STATE state) {
  // Update the UI based on the state change
  if(state == STATE::Cool) {
    UIhideDelay();
    UIhideHeat();
    UIshowCool();
  }else if(state == STATE::Heat) {
    UIhideDelay();
    UIhideCool();
    UIshowHeat();
  }else if(state == STATE::AwaitingCool) {
    UIshowDelay();
    UIhideCool();
    UIhideHeat();
  }else if(state == STATE::AwaitingHeat) {
    UIshowDelay();
    UIhideCool();
    UIhideHeat();
  }else {
    UIhideDelay();
    UIhideCool();
    UIhideHeat();
  }
}

void updateState(STATE selectedState) {
  Serial.println("# Updating State...");
  MODE currentMode = getCurrentMode();
  if(currentMode == MODE::Off) {
    Serial.println("# Setting state to Idle for OFF mode");
    if(getCurrentState() == STATE::Heat || getCurrentState() == STATE::Cool) {
      setLastHeavyState(getCurrentState());
      setCurrentState(STATE::Idle);
      resetHeavyEndedTimer();
    }else {
      setCurrentState(STATE::Idle);
    }
    return;
  }
  
  if(whoAmI() == PEERTYPE::PARENT) {
    STATE preState = getCurrentState();
    Serial.print("# Was state (");
    Serial.print(STRING_FROM_STATE[preState]);
    Serial.println(").");

    if(currentMode == MODE::Auto) {
      computeAutoState();
    }else if(currentMode == MODE::Manual) {
      computeManualState(selectedState);
    }

    STATE newState = getCurrentState();
    Serial.print("# Now is (");
    Serial.print(STRING_FROM_STATE[newState]);
    Serial.println(").");

    if(currentMode == MODE::Manual) {
      UIsetManualBTNState(newState);
    }

    Serial.print("# Last Heavy is (");
    Serial.print(STRING_FROM_STATE[getLastHeavyState()]);
    Serial.println(").");
  
    if(preState != newState) {
      updateUIfromStates(newState);
      setRelaysFromState(newState);
    }
  }
}

void checkState() {
  if(whoAmI() == PEERTYPE::PARENT) {
    updateState(getCurrentState());
  }
}




// Callback functions for UI buttons
void onTempUpButtonClick(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    // Handle button click event
    printf("autoBTN1 clicked\n");
    const float currentTemp = getTempGoal();
    printf("Current temperature: %.1f\n", currentTemp);

    if(currentTemp < MAX_GOAL_TEMP) {
        setTempGoal(currentTemp + 1.0);
        UIgoalSet(currentTemp + 1.0);
    }else {
        setTempGoal(MAX_GOAL_TEMP);
        UIgoalSet(MAX_GOAL_TEMP);
    }
    checkState();

  }
}

void onTempDownButtonClick(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    // Handle button click event
    printf("autoBTN2 clicked\n");
    const float currentTemp = getTempGoal();
    printf("Current temperature: %.1f\n", currentTemp);

    if(currentTemp > MIN_GOAL_TEMP) {
      setTempGoal(currentTemp - 1.0);
      UIgoalSet(currentTemp - 1.0);
    }else {
      setTempGoal(MIN_GOAL_TEMP);
      UIgoalSet(MIN_GOAL_TEMP);
    }
    checkState();
  }
}

void onManualHeatClick() {  
  if(whoAmI() == PEERTYPE::PARENT) {
      if(getCurrentState() == STATE::Heat || getCurrentState() == STATE::AwaitingHeat) {
        updateState(STATE::Idle);
      }else {
        updateState(STATE::Heat);
      }
  }else {
    sendHeatButtonClick();
  }
}

void onManualCoolClick() {
  if(whoAmI() == PEERTYPE::PARENT) {
    if(getCurrentState() == STATE::Cool || getCurrentState() == STATE::AwaitingCool) {
      updateState(STATE::Idle);
    }else {
      updateState(STATE::Cool);
    }
  }else {
    sendCoolButtonClick();
  }

}

void onManualFanClick() {
  if(whoAmI() == PEERTYPE::PARENT) {
    if(getCurrentState() == STATE::Fan) {
      updateState(STATE::Idle);
    }else {
      updateState(STATE::Fan);
    }
  }else {
    sendFanButtonClick();
  }
    
}

void onOFFButtonClick() {
  if(whoAmI() == PEERTYPE::CHILD) {
    sendOffButtonClick();
  }

  printf("Off button selected\n");
  UIhideDelay();
  UIhideUnlock();
  UIhideCool();
  UIhideHeat();
  UIhideAutoBTNs();
  UIhideManualBTNs();
  UIhideMenuButton();
  UIhideMenu();

  UIgoalSet("Off");
  UIshowOnButton();

  setRelaysFromState(STATE::Idle);
  setLastMode(getCurrentMode());
  setCurrentMode(MODE::Off);

  updateState(STATE::Idle);
  setRelaysFromState(STATE::Idle);

  // Has to happen after state changes, otherwise the state machine might switch it back on immediately
  UIhideTimer();
}

void onONButtonClick() {
  printf("On button selected\n");
  UItempErrorCheck();
  UIshowMenuButton();

  // Restoring unlocked icon
  if(isUnlocked()) {
    UIshowUnlock();
  }

  // Restoring last mode
  MODE lastMode = getLastMode();
  if(lastMode == MODE::Manual) {
    onManualButtonClick();
  } else if(lastMode == MODE::Auto) {
    onAutoButtonClick();
  } else if(lastMode == MODE::Off) {
    // If last mode was Off, we set it to Auto
    onAutoButtonClick();
  }

  // Restoring delay message
  STATE state = getCurrentState();
  if(state == STATE::AwaitingCool || state == STATE::AwaitingHeat) {
    UIshowDelay();
  }
}

void manualButtonExecution() {
  checkState();
  UIgoalSet("");
  UIshowMenuButton();
  UIhideAutoBTNs();
  UIhideOnButton();
  UIshowManualBTNs();
  UIsetManualBTNState(getCurrentState());
}

void onManualButtonClick() {
  printf("Manual mode selected\n");
  if(whoAmI() == PEERTYPE::PARENT) {
    setCurrentMode(MODE::Manual);
  }else {
    sendManualButtonClick();
  }

  manualButtonExecution();
}

void autoButtonExecution() {
  const float currentGoal = getTempGoal();
  UIgoalSet(currentGoal);

  const STATE currentState = getCurrentState();
  updateUIfromStates(currentState);

  UIshowAutoBTNs();
  UIshowMenuButton();
  UIhideOnButton();
  UIhideManualBTNs();
  checkState();
}

void onAutoButtonClick() {
  printf("Auto mode selected\n");
  if(whoAmI() == PEERTYPE::PARENT) {
    setCurrentMode(MODE::Auto);
  }else {
    sendAutoButtonClick();
  }

  autoButtonExecution();
}

void onLockButtonClick() {
  printf("Lock button selected\n");
}

void onSwitchOnClick() {
  UIgoalSet("On");
  lv_timer_t* timer = lv_timer_create([](lv_timer_t* t) {
    onONButtonClick();
    UIhideOnButton();
    lv_timer_del(t);
  }, 1000, NULL);
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

void onNewUIState() {
  STATE newState = getCurrentState();
  updateUIfromStates(newState);
}




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

  // NOTE: Should come after storage initialization
  initializeStateMachine();


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
  
  UIinitializeHeatZone();
  UIinitializeCoolZone();

  UIinitializeDelay();
  UIinitializeTimer();

  UIinitializeLock();

  UItempInitialize();
  UIgoalInitialize();

  UIinitializeAutoBTNs();
  UIinitializeManualBTNs();

  
  UIinitializeMenu();
  UIinitializeOnButton();

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
  UIhideMenuButton();
  UIgoalSet("Off");
  UIshowOnButton();
  // updateState(STATE::Idle);
  setCurrentMode(MODE::Off);
  setRelaysFromState(STATE::Idle);
  

  printf("Setup done\n");

  // Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
}

void loop() {  
  lv_timer_handler(); /* let the GUI do its work */
  delay(10);

  loopMQTT();

  UItempErrorCheck();

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
    float newTempGoal = getTempGoal();
    Serial.print("[PARENT] New temp goal: ");
    Serial.println(newTempGoal);

    UIgoalSet(newTempGoal);
    checkState();
    parentGoalNeedsUpdate = false;
  }
  if(childGoalNeedsUpdate) {
    float newTempGoal = getTempGoal();
    Serial.print("[CHILD] New temp goal: ");
    Serial.println(newTempGoal);

    UIgoalSet(newTempGoal);
    childGoalNeedsUpdate = false;
  }


  if(TempNeedsUpdate) {
    float newTemp = getTemp();
    UItempSet(newTemp);
    checkState();
    TempNeedsUpdate = false;
  }


  if(parentModeNeedsUpdate) {
    MODE newMode = getCurrentMode();
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
    // Comes from peer, so needs passing on to home assistant
    MODE newMode = getCurrentMode();
    setCurrentModeSilently(newMode);
    if(newMode == MODE::Manual) {
      manualButtonExecution();
    } else if(newMode == MODE::Auto) {
      autoButtonExecution();
    } else if(newMode == MODE::Off) {
      onOFFButtonClick();
      Serial.println("Turning off thermostat");
    }

    childModeNeedsUpdate = false;
  }

  if(parentStateNeedsUpdate) {
    updateState(possibleState);
    parentStateNeedsUpdate = false;
  }
  if(childStateNeedsUpdate) {
    // Passing on to home assistant
    setCurrentStateSilently(possibleState);
    UIsetManualBTNState(possibleState);
    updateUIfromStates(possibleState);

    childStateNeedsUpdate = false;
  }

  // float temp123 = getBTHomeTemperature();
  // Serial.print("BTHome temperature: ");
  // Serial.println(temp123);
} 