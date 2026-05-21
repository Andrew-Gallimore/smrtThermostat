#include "stateMachine.h"
#include <Arduino.h>
#include "remoteThermostat.h"
#include "storage.h"
#include "locking.h"

float temp            = 70; // overwritten by storage
float tempGoal        = 70; // overwritten by storage
float lastTempGoal    = 70;
bool  isFirstTempGoal = true;
float tempMargin      = 1.0;

float getTemp() {
  return temp;
}
float getTempGoal() {
  return tempGoal;
}
float getTempMargin() {
  return tempMargin;
}

void setTemp(float newTemp) {
  // No matter who we are, we set it. This should be trusted
  temp = newTemp;
  TempNeedsUpdate = true;

  updateSharedTemp(newTemp);
  storeTemp(newTemp);
}
void setTempGoal(float newTempGoal) {
  if(whoAmI() == PEERTYPE::PARENT) {
    // We are parent, so we set it no matter what. Local first after all...
    if(lastTempGoal != newTempGoal) {
      // updateStorageTempGoal(newTempGoal);
    }
    lastTempGoal = tempGoal;
    tempGoal = newTempGoal;
    GoalNeedsUpdate = true;
  }

  updateSharedTempGoal(newTempGoal);
  storeTempGoal(newTempGoal);
}

void onHATempGoal(float newTempGoal) {
  if(isFirstTempGoal) {
    isFirstTempGoal = false;
    return;
  }
  lastTempGoal = tempGoal;
  tempGoal = newTempGoal;
  HAGoalNeedsUpdate = true;
}

void onRemoteTempGoal(float newTempGoal) {
  if(isFirstTempGoal) {
    isFirstTempGoal = false;
    return;
  }
  lastTempGoal = tempGoal;
  tempGoal = newTempGoal;
  GoalNeedsUpdate = true;
  storeTempGoal(newTempGoal);
}

void onRemoteTemp(float newTemp) {
  if(whoAmI() == PEERTYPE::PARENT) {
    updateSharedTemp(newTemp);
  }
  temp = newTemp;
  TempNeedsUpdate = true;
}

// NOTE: This function is for the thermometers/sensors
void onNewTempReading(float newTemp) {
  if(whoAmI() == PEERTYPE::PARENT) {
    updateSharedTemp(newTemp);
  }
  temp = newTemp;
  TempNeedsUpdate = true;
  storeTemp(newTemp);
}

MODE currentMode      = MODE::Off;
MODE lastMode         = MODE::Auto; // overwritten by storage
bool isFirstMode      = true;
STATE currentState    = STATE::Idle;
STATE lastState       = STATE::Idle;
STATE lastHeavyState  = STATE::Idle; // overwritten by storage

MODE getCurrentMode() {
  return currentMode;
}
MODE getLastMode() {
  return lastMode;
}
STATE getCurrentState() {
  return currentState;
}
STATE getLastHeavyState() {
  return lastHeavyState;
}

void setCurrentMode(MODE newMode) {
  Serial.print("Setting current mode to: ");
  Serial.println(newMode);

  if(whoAmI() == PEERTYPE::PARENT) {
    currentMode = newMode;
  }

  updateSharedMode(newMode);
  storeMode(newMode);
}

void setLastMode(MODE newMode) {
  lastMode = newMode;
  storeLastMode(newMode);
}

void onRemoteMode(MODE newMode) {
  //NOTE: This is a weird hack because whenever we startup it imediately sends a command controled from home assistant, regardless of previous state
  if(isFirstMode) {
    isFirstMode = false;
    return;
  }

  currentMode = newMode;
  ModeNeedsUpdate = true;
}
void onHARemoteMode(MODE newMode) {
  //NOTE: This is a weird hack because whenever we startup it imediately sends a command controled from home assistant, regardless of previous state
  if(isFirstMode) {
    isFirstMode = false;
    return;
  }

  // We are parent, so we update the mode
  currentMode = newMode;
  HAModeNeedsUpdate = true;
}

void onRemoteState(STATE newState) {
  possibleState = newState;
  StateNeedsUpdate = true;
}
void onHARemoteState(STATE newState) {
  // We are parent
  possibleState = newState;
  HAStateNeedsUpdate = true;
}

void setLastHeavyState(STATE newLastHeavyState) {
  // updateStorageHeavyState(newLastHeavyState);
  lastHeavyState = newLastHeavyState;
  storeLastHeavyState(newLastHeavyState);
}
void setCurrentState(STATE newState) {
  if(currentState != newState) {
    // updateStorageState(newState);
  }
  lastState = currentState;
  currentState = newState;
  updateSharedState(newState);
  storeLastState(lastState);
  storeState(newState);

  if(!isUnlocked()) {
    if(newState != STATE::Idle) {
      UIshowTimer();
    }else {
      UIhideTimer();
    }
  }
}

void setCurrentStateSilently(STATE newState) {
  // This is used to update the state without notifying network
  if(currentState != newState) {
    // updateStorageState(newState);
  }
  lastState = currentState;
  currentState = newState;
  storeLastState(lastState);
  storeState(newState);
}




long int lastHeavyTime = 0;
void resetHeavyEndedTimer() {
  lastHeavyTime = millis();
}

long int timeSinceLastHeavyState() {
  return millis() - lastHeavyTime;
}



// NOTE: 3600000ms = 1hr
long int RESET_LIMIT_MS = 70000; // 51 Seconds
// long int RESET_LIMIT_MS = 3 * 3600000; // 3 hours
long int lastInteractionTime = 0;

void newInteraction() {
  lastInteractionTime = millis();
}
long int getResetTimeLimit() {
  return RESET_LIMIT_MS;
}
long int timeSinceLastInteraction() {
  return millis() - lastInteractionTime;
}



const int LONG_STATE_DELAY = 480000;    // 8 minutes in ms
const int REG_STATE_DELAY = 300000;     // 5 minutes in ms
// const int LONG_STATE_DELAY = 15000;    // 15s in ms
// const int REG_STATE_DELAY = 10000;     // 10s in ms

/**
 * Gets the delay required between two given states.
 * @param fromState The STATE we are coming from.
 * @param toState The new STATE we are going to.
 * @return The delay in milliseconds required between the two states.
 */
long int getDelay(STATE fromState, STATE toState) {
  if(fromState == STATE::AwaitingCool) fromState = STATE::Cool;
  if(fromState == STATE::AwaitingHeat) fromState = STATE::Heat;
  if(toState == STATE::AwaitingCool) toState = STATE::Cool;
  if(toState == STATE::AwaitingHeat) toState = STATE::Heat;

  if(fromState == STATE::Heat && toState == STATE::Cool) {
    return LONG_STATE_DELAY;
  }else if(fromState == STATE::Cool && toState == STATE::Heat) {
    return LONG_STATE_DELAY;
  }else if(fromState == STATE::Heat && toState == STATE::Heat) {
    return REG_STATE_DELAY;
  }else if(fromState == STATE::Cool && toState == STATE::Cool) {
    return REG_STATE_DELAY;
  }

  return 0;
}

/**
 * Updates the state based on the current mode and selected state.
 * Ensures that state transitions respect the required delays.
 * @param selectedState The selected STATE by the user.
 */
void computeManualState(STATE selectedState) {
  STATE currentState = getCurrentState();

  // If the resetTimer is up and the device isn't unlocked
  if(!isUnlocked() && timeSinceLastInteraction() > RESET_LIMIT_MS) {
    Serial.println(">>>> Turning off due to timer...");

    if(currentState == STATE::Heat || currentState == STATE::Cool) {
      setLastHeavyState(currentState);
      setCurrentState(STATE::Idle);
      resetHeavyEndedTimer();
    }else {
      setCurrentState(STATE::Idle);
    }

    flag_offButton = true;
    return;
  }

  // ==== Switching between states ====
  switch (currentState) {

    // Current state
    case STATE::Idle:
      switch (selectedState) {

        // Selected state
        case STATE::Cool:
          setCurrentState(STATE::AwaitingCool);
          break;
        // Selected state
        case STATE::Heat:
          setCurrentState(STATE::AwaitingHeat);
          break;
        // Selected state
        case STATE::Fan:
          setCurrentState(STATE::Fan);
          break;
        
        default:
          break;
      }
      break;

    // Current state
    case STATE::AwaitingHeat:
      switch (selectedState) {

        // Selected state
        case STATE::Idle:
          setCurrentState(STATE::Idle);
          break;
        // Selected state
        case STATE::Cool:
          setCurrentState(STATE::AwaitingCool);
          break;
        // Selected state
        case STATE::Fan:
          setCurrentState(STATE::Fan);
          break;
        
        default:
          break;
      }
      break;

    // Current state
    case STATE::Heat:
      switch (selectedState) {

        // Selected state
        case STATE::Idle:
          setLastHeavyState(currentState);
          setCurrentState(STATE::Idle);
          resetHeavyEndedTimer();
          break;
        // Selected state
        case STATE::Cool:
          setLastHeavyState(currentState);
          setCurrentState(STATE::AwaitingCool);
          resetHeavyEndedTimer();
          break;
        // Selected state
        case STATE::Fan:
          setLastHeavyState(currentState);
          setCurrentState(STATE::Fan);
          resetHeavyEndedTimer();
          break;
        
        default:
          break;
      }
      break;

    // Current state
    case STATE::AwaitingCool:
      switch (selectedState) {

        // Selected state
        case STATE::Idle:
          setCurrentState(STATE::Idle);
          break;
        // Selected state
        case STATE::Heat:
          setCurrentState(STATE::AwaitingHeat);
          break;
        // Selected state
        case STATE::Fan:
          setCurrentState(STATE::Fan);
          break;
        
        default:
          break;
      }
      break;

    // Current state
    case STATE::Cool:
      switch (selectedState) {

        // Selected state
        case STATE::Idle:
          setLastHeavyState(currentState);
          setCurrentState(STATE::Idle);
          resetHeavyEndedTimer();
          break;
        // Selected state
        case STATE::Heat:
          setLastHeavyState(currentState);
          setCurrentState(STATE::AwaitingHeat);
          resetHeavyEndedTimer();
          break;
        // Selected state
        case STATE::Fan:
          setLastHeavyState(currentState);
          setCurrentState(STATE::Fan);
          resetHeavyEndedTimer();
          break;
        
        default:
          break;
      }
      break;
      
    // Current state
    case STATE::Fan:
      switch (selectedState) {

        // Selected state
        case STATE::Idle:
          setCurrentState(STATE::Idle);
          break;
        // Selected state
        case STATE::Heat:
          setCurrentState(STATE::AwaitingHeat);
          break;
        // Selected state
        case STATE::Cool:
          setCurrentState(STATE::AwaitingCool);
          break;
        
        default:
          break;
      }
      break;

    default:
      break;
  }


  // ==== Limiting active state switches by delay ====
  // Past this if statement, we can trust that we are allowed to go to heat/cool state
  if(timeSinceLastHeavyState() < getDelay(getLastHeavyState(), selectedState)) {
    Serial.println("Awaiting delay");
    return;
  }

  currentState = getCurrentState();

  // ==== awaiting --> active ====
  switch (currentState) {
    // Current state
    case STATE::AwaitingHeat:
      setCurrentState(STATE::Heat);
      break;
      
      // Current state
    case STATE::AwaitingCool:
      setCurrentState(STATE::Cool);
      break;
  }
}

void computeAutoState() {
  STATE currentState = getCurrentState();
  float temp = getTemp();
  float goalTemp = getTempGoal();
  float margin = getTempMargin();
  
  STATE goalState = STATE::Idle;

  switch(currentState) {
    case STATE::Idle:
    case STATE::AwaitingCool:
    case STATE::AwaitingHeat:
    case STATE::Fan:
      // If the resetTimer is up and the device isn't unlocked
      if(!isUnlocked() && timeSinceLastInteraction() > RESET_LIMIT_MS) {
        Serial.println(">>>> Turning off due to timer...");
        setCurrentState(STATE::Idle);
        flag_offButton = true;
        break;
      }

      // Otherwise, just do the regular automatic changes
      if(temp + margin < goalTemp) {
        setCurrentState(STATE::AwaitingHeat);
      }else if(temp - margin > goalTemp) {
        setCurrentState(STATE::AwaitingCool);
      }else {
        setCurrentState(STATE::Idle);
      }
      break;

    case STATE::Cool:
      // If the resetTimer is up and the device isn't unlocked
      if(!isUnlocked() && timeSinceLastInteraction() > RESET_LIMIT_MS) {
        Serial.println(">>>> Turning off due to timer...");
        setLastHeavyState(currentState);
        setCurrentState(STATE::Idle);
        resetHeavyEndedTimer();
        flag_offButton = true;
        break;
      }

      if(temp + margin < goalTemp) {
        setLastHeavyState(currentState);
        setCurrentState(STATE::AwaitingHeat);
        resetHeavyEndedTimer();
      }else if(abs(temp - goalTemp) < margin) {
        setLastHeavyState(currentState);
        setCurrentState(STATE::Idle);
        resetHeavyEndedTimer();
      }
      break;
    case STATE::Heat:
      // If the resetTimer is up and the device isn't unlocked
      if(!isUnlocked() && timeSinceLastInteraction() > RESET_LIMIT_MS) {
        Serial.println(">>>> Turning off due to timer...");
        setLastHeavyState(currentState);
        setCurrentState(STATE::Idle);
        resetHeavyEndedTimer();
        flag_offButton = true;
        break;
      }
      
      if(temp - margin > goalTemp) {
        setLastHeavyState(currentState);
        setCurrentState(STATE::AwaitingCool);
        resetHeavyEndedTimer();
      }else if(abs(temp - goalTemp) < margin) {
        setLastHeavyState(currentState);
        setCurrentState(STATE::Idle);
        resetHeavyEndedTimer();
      }
      break;
  }

  Serial.println(timeSinceLastHeavyState());
  Serial.println(getDelay(getLastHeavyState(), getCurrentState()));

  if(timeSinceLastHeavyState() < getDelay(getLastHeavyState(), getCurrentState())) {
    Serial.println("Awaiting delay (auto)");
    return;
  }
  
  if(temp < 0) {
    Serial.println("Temperature not set");
    return;
  }
  // Past these last two if statement, we can trust that we are allowed to go to heat/cool state

  currentState = getCurrentState();

  // ==== awaiting --> active ====
  switch (currentState) {
    // Current state
    case STATE::AwaitingHeat:
      setCurrentState(STATE::Heat);
      break;
      
      // Current state
    case STATE::AwaitingCool:
      setCurrentState(STATE::Cool);
      break;
  }
}


void initializeStateMachine() {
  // Get stored variables
  ulong  storedTime     = getStoredTimestamp();
  // Checking our time is less than stored time (meaning we restarted since last storage)
  if(storedTime > 0 && millis() < storedTime) {
    Serial.println("Restoring stored variables...");

    setTempGoal(getStoredTempGoal());
    setTemp(getStoredTemp());
    lastMode       = getStoredMode(); // We start in off mode, when it turns of it restores last mode
    lastHeavyState = getStoredLastHeavyState();

    Serial.println("Stored variables restored.");
  }
}