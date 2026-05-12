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
  Serial.println("Here5");
  Serial.print("Current mode: ");
  Serial.println((int)getCurrentMode());
  MODE currentMode = getCurrentMode();
  if(currentMode == MODE::Off) {
    setCurrentState(STATE::Idle);
    return;
  }

  Serial.println("Here6");

  if(whoAmI() == PEERTYPE::PARENT) {
    STATE preState = getCurrentState();

    if(currentMode == MODE::Auto) {
      computeAutoState();
    }else if(currentMode == MODE::Manual) {
      computeManualState(selectedState);
    }

    STATE newState = getCurrentState();

    if(currentMode == MODE::Manual) {
      UIsetManualBTNState(newState);
    }

    Serial.print(getLastHeavyState());
    Serial.print(" ");
    Serial.print(newState);
    Serial.print(" -- ");


    Serial.print(getTemp());
    Serial.print(" ");
    Serial.print(getTempGoal());
    Serial.println("");

    if(preState != newState) {
      updateUIfromStates(newState);
      setRelaysFromState(newState);
    }
  }else {
    Serial.println("Directly setting state from remote thermostat");
    setCurrentState(selectedState);
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
        }else {
            setTempGoal(MAX_GOAL_TEMP);
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
        }else {
            setTempGoal(MIN_GOAL_TEMP);
        }
        checkState();
    }
}

void onManualHeatClick() {
  if(getCurrentState() == STATE::Heat || getCurrentState() == STATE::AwaitingHeat) {
    updateState(STATE::Idle);
  }else {
    updateState(STATE::Heat);
  }
}

void onManualCoolClick() {
  if(getCurrentState() == STATE::Cool || getCurrentState() == STATE::AwaitingCool) {
    updateState(STATE::Idle);
  }else {
    updateState(STATE::Cool);
  }
}

void onManualFanClick() {
  if(getCurrentState() == STATE::Fan) {
    updateState(STATE::Idle);
  }else {
    updateState(STATE::Fan);
  }
}

void onOFFButtonClick() {
  printf("Off button selected\n");
  UIhideDelay();
  UIhideUnlock();
  UIhideCool();
  UIhideHeat();
  UIhideAutoBTNs();
  UIhideManualBTNs();
  UIhideMenuButton();

  UIgoalSet("Off");
  UIshowOnButton();

  updateState(STATE::Idle);
  setRelaysFromState(STATE::Idle);
  setLastMode(getCurrentMode());
  setCurrentMode(MODE::Off);
  setRelaysFromState(STATE::Idle);
}

void onONButtonClick() {
  printf("On button selected\n");
  UItempErrorCheck();
  UIshowMenuButton();

  // Restoring delay message
  STATE state = getCurrentState();
  if(state == STATE::AwaitingCool || state == STATE::AwaitingHeat) {
    UIshowDelay();
  }

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
}

void onManualButtonClick() {
  printf("Manual mode selected\n");
  setCurrentMode(MODE::Manual);
  checkState();
  UIgoalSet("");
  UIshowMenuButton();
  UIhideAutoBTNs();
  UIhideOnButton();
  UIshowManualBTNs();
  UIsetManualBTNState(getCurrentState());
}

void onAutoButtonClick() {
  printf("Auto mode selected\n");
  setCurrentMode(MODE::Auto);

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

void onLockButtonClick() {
  printf("Lock button selected\n");

  bool passed = lockTest();

  if(passed) {
    UIshowUnlock();
  }else {
    UIhideUnlock();
  }
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
volatile bool GoalNeedsUpdate = false;
volatile bool HAGoalNeedsUpdate = false;
volatile bool TempNeedsUpdate = false;
volatile bool ModeNeedsUpdate = false;
volatile bool HAModeNeedsUpdate = false;
volatile STATE possibleState = STATE::Idle;
volatile bool StateNeedsUpdate = false;
volatile bool HAStateNeedsUpdate = false;

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
  startBLESensorScan();

  setupDisplay();

  
  initializeStorage();
  
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

  UIinitializeDelay();

  UIinitializeLock();

  UItempInitialize();
  UIgoalInitialize();

  UIinitializeAutoBTNs();
  UIinitializeManualBTNs();

  UIinitializeHeatZone();
  UIinitializeCoolZone();
  
  UIinitializeMenu();
  UIinitializeOnButton();

  UIinitializeSettings();
  UIinitializeNetwork();



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

  if(GoalNeedsUpdate) {
    float newTempGoal = getTempGoal();
    UIgoalSet(newTempGoal);
    checkState();
    GoalNeedsUpdate = false;
  }
  if(HAGoalNeedsUpdate) {
    // Passing on to home assistant
    float newTempGoal = getTempGoal();
    setTempGoal(newTempGoal);
    checkState();
    HAGoalNeedsUpdate = false;
  }


  if(TempNeedsUpdate) {
    float newTemp = getTemp();
    UItempSet(newTemp);
    checkState();
    TempNeedsUpdate = false;
  }


  if(ModeNeedsUpdate) {
    MODE newMode = getCurrentMode();
    if(newMode == MODE::Manual) {
      onManualButtonClick();
    } else if(newMode == MODE::Auto) {
      onAutoButtonClick();
    } else if(newMode == MODE::Off) {
      if(getCurrentMode() == MODE::Off) {
        onOFFButtonClick();
        Serial.println("Turning on thermostat");
      }else {
        onONButtonClick();
        Serial.println("Turning off thermostat");
      }
    }

    ModeNeedsUpdate = false;
  }
  if(HAModeNeedsUpdate) {
    // Passing on to home assistant
    MODE newMode = getCurrentMode();
    setCurrentMode(newMode);
    HAModeNeedsUpdate = false;
  }

  if(StateNeedsUpdate) {
    if(whoAmI() == PEERTYPE::PARENT) {
      // We are parent, so we update the state
      updateState(possibleState);
    }else {
      setCurrentStateSilently(possibleState);
      UIsetManualBTNState(possibleState);
      updateUIfromStates(possibleState);
    }
    StateNeedsUpdate = false;
  }
  if(HAStateNeedsUpdate) {
    // Passing on to home assistant
    updateState(possibleState);
    HAStateNeedsUpdate = false;
  }

  // float temp123 = getBTHomeTemperature();
  // Serial.print("BTHome temperature: ");
  // Serial.println(temp123);
} 