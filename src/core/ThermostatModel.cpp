#include "ThermostatModel.h"

// Initial variables
const int LONG_STATE_DELAY = 480000;    // 8 minutes in ms
const int REG_STATE_DELAY = 300000;     // 5 minutes in ms
// const int LONG_STATE_DELAY = 15000;    // 15s in ms
// const int REG_STATE_DELAY = 10000;     // 10s in ms

long int RESET_LIMIT_MS = 2 * 3600000; // 2 hours



ThermostatModel::ThermostatModel(ROLE role, SyncManager& sync) : sync_(sync) {
    ts_.role = role;
    ts_.mode = MODE::Off;
    ts_.lastMode = MODE::Off;
    ts_.state = STATE::Idle;
    ts_.temp = 70;
    ts_.goalTemp = 70;
    ts_.margin = 1.0f;
    ts_.unlocked = false;
    ts_.delayActive = false;
    ts_.lastHeavyState = STATE::Idle;
    ts_.lastHeavyTime = 0;
    ts_.lastInteractionTime = 0;
}

void ThermostatModel::initializeFromStorage(MODE lastMode,
                                             float lastTempGoal,
                                             float lastTemp,
                                             STATE lastHeavyState) {
    ts_.mode = lastMode;
    ts_.goalTemp = lastTempGoal;
    ts_.temp = lastTemp;
    ts_.lastHeavyState = lastHeavyState;
}

void ThermostatModel::restoreLastMode() {
    if(ts_.role == ROLE::CHILD) {
        Serial.println("WARN: Child cannot restore last mode.");
        return;
    }

    ts_.mode = ts_.lastMode;
    sync_.publishThermState(ts_);
    _notify(); // Notify observers of the change
}

void ThermostatModel::_setRelaysFromState(STATE newState) {
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


void ThermostatModel::setMode(MODE newMode) {
    if(ts_.role == ROLE::CHILD) {
        // Send command to parent to change mode
        Command cmd;
        cmd.type = COMMAND_TYPE::SetMode;
        cmd.mode = newMode;
        sync_.publishCommand(cmd);
        return;
    }

    // Parent logic
    STATE computedNewState = ts_.state;

    // Update local state and sync
    if(ts_.mode == newMode) return;
    
    if(newMode == MODE::Off) {
        computedNewState = STATE::Idle;
        ts_.lastMode = ts_.mode;
    }else if(newMode == MODE::Auto) {
        computedNewState = _computeAutoStateChange();
    }else if(newMode == MODE::Manual) {
        // Switching to manual mode, so we keep the current state as is
        computedNewState = _computeManualStateChange(ts_.state);
    }
    
    // Actually setting the new mode
    ts_.mode = newMode;

    // Let child thermostats know about the mode/state change
    if(computedNewState != ts_.state) {
        ts_.state = computedNewState;
        sync_.publishThermState(ts_);
    }

    // Notify observers of the change
    _notify();
}

void ThermostatModel::setGoalTemp(float newTemp) {
    if(ts_.role == ROLE::CHILD) {
        // Send command to parent to change target temperature
        Command cmd;
        cmd.type = COMMAND_TYPE::SetTempGoal;
        cmd.tempGoal = newTemp;
        sync_.publishCommand(cmd);
        return;
    }

    // Parent logic
    ts_.goalTemp = newTemp;
    sync_.publishThermState(ts_);
    _notify(); // Notify observers of the change
}

void ThermostatModel::setTemp(float newTemp) {
    if(ts_.role == ROLE::CHILD) {
        Serial.println("WARN: Child cannot set current temperature directly.");
        return;
    }

    // Parent logic
    ts_.temp = newTemp;
    sync_.publishThermState(ts_);
}

/**
 * Requests a manual state change.
 * Side Effects: If a child, sends a command to the parent. But
 *      if it's a partent, updates the local state and syncs it.
 * Note: Change can be denied if the thermostat is locked or if
 *      the state transition is invalid.
 * @param newState The desired STATE to transition to.
 */
void ThermostatModel::requestManualState(STATE newState) {
    if(ts_.mode != MODE::Manual) {
        Serial.println("WARN: Cannot request manual state change when not in Manual mode.");
        return;
    }
    
    if(ts_.role == ROLE::CHILD) {
        // Send command to parent to change state
        Command cmd;
        cmd.type = COMMAND_TYPE::SetState;
        cmd.state = newState;
        sync_.publishCommand(cmd);
        return;
    }

    // TODO: Implement logic to properly handle manual state change
    // Update local state and sync
    STATE computedNewState = _computeManualStateChange(newState); // This function should check if the state change is valid
    if(computedNewState != ts_.state) {
        ts_.state = computedNewState;
        sync_.publishThermState(ts_);
    }
}


/**
 * For child to apply remote thermState updates received from parent.
 * Side Effects: It updates the local thermState (ts_) with
 *      the received remote state.
 * @param remoteThermState The thermState received from the parent.
 */
void ThermostatModel::applyRemoteState(const ThermostatState& remoteThermState) {
    if(ts_.role == ROLE::PARENT) {
        Serial.println("WARN: Parent received remote state update, ignoring.");
        return;
    }

    // TODO: Have more complex logic here?
    ts_ = remoteThermState;

    _notify(); // Notify observers of the state change
}

void ThermostatModel::subscribe(ThermostatObserver observer) {
    observers_.push_back(observer);
}

void ThermostatModel::update() {
    STATE newState = ts_.state;
    // For parent, we might want to check if the state needs to
    //      be updated
    if(ts_.role == ROLE::PARENT) {
        if(ts_.mode == MODE::Auto) {
            newState = _computeAutoStateChange();
        }else if(ts_.mode == MODE::Manual) {
            newState = _computeManualStateChange(ts_.state);
        }

        if(newState != ts_.state) {
            ts_.state = newState;
            sync_.publishThermState(ts_);
            _notify(); // Notify observers of the state change
        }
    }
}

void ThermostatModel::_notify() {
    // Check that something has changed
    if(memcmp(&ts_, &oldTs_, sizeof(ThermostatState)) != 0) {
        // Update relays based on the new state
        _setRelaysFromState(ts_.state);

        // Notify observers of the change(s)
        for(auto& observer : observers_) {
            observer(ts_);
        }
    }
}

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

long int ThermostatModel::getRemainingDelay() {
    long int timeSinceLastHeavy = millis() - _lastHeavyTime;
    long int requiredDelay = getDelay(ts_.lastHeavyState, ts_.state);
    long int remainingDelay = requiredDelay - timeSinceLastHeavy;
    return (remainingDelay > 0) ? remainingDelay : 0;
}

long int ThermostatModel::getRemainingInteractionTime() {
    long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;
    long int remainingTime = RESET_LIMIT_MS - timeSinceLastInteraction;
    return (remainingTime > 0) ? remainingTime : 0;
}

STATE ThermostatModel::_computeManualStateChange(STATE requestedState) {
    STATE computedNewState = ts_.state;
    

    // If the resetTimer is up and the device isn't unlocked
    long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;
    if(!isUnlocked() && timeSinceLastInteraction > RESET_LIMIT_MS) {
        Serial.println(">>>> Turning off due to timer...");

        if(ts_.state == STATE::Heat || ts_.state == STATE::Cool) {
            ts_.lastHeavyState = ts_.state;
            computedNewState =  STATE::Idle;
            _lastHeavyTime = millis();
        }else {
            computedNewState = STATE::Idle;
        }

        Serial.println("TODO: Flag off button here");
        // flag_offButton = true;
        return computedNewState;
    }


    // Min and Max safty starting the thermostat
    if(1 == 0) {
        // TODO: Implement min/max temperature checks
    }else {
        // We are within the good temps, so we can do standard temp changes
    
        // ==== Switching between states ====
        switch (ts_.state) {
    
        // Current state
        case STATE::Idle:
            switch (requestedState) {
    
            // Selected state
            case STATE::Cool:
                computedNewState = STATE::AwaitingCool;
                break;
            // Selected state
            case STATE::Heat:
                computedNewState = STATE::AwaitingHeat;
                break;
            // Selected state
            case STATE::Fan:
                computedNewState = STATE::Fan;
                break;
            
            default:
                break;
            }
            break;
    
        // Current state
        case STATE::AwaitingHeat:
            switch (requestedState) {
    
            // Selected state
            case STATE::Idle:
                computedNewState = STATE::Idle;
                break;
            // Selected state:
            case STATE::Heat:
                computedNewState = STATE::AwaitingHeat;
                break;
            // Selected state
            case STATE::Cool:
                computedNewState = STATE::AwaitingCool;
                break;
            // Selected state
            case STATE::Fan:
                computedNewState = STATE::Fan;
                break;
            
            default:
                break;
            }
            break;
    
        // Current state
        case STATE::Heat:
            switch (requestedState) {
    
            // Selected state
            case STATE::Idle:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
                break;
            // Selected state
            case STATE::Cool:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::AwaitingCool;
                _lastHeavyTime = millis();
                break;
            // Selected state
            case STATE::Fan:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Fan;
                _lastHeavyTime = millis();
                break;
            
            default:
                break;
            }
            break;
    
        // Current state
        case STATE::AwaitingCool:
            switch (requestedState) {
    
            // Selected state
            case STATE::Idle:
                computedNewState = STATE::Idle;
                break;
            // Selected state
            case STATE::Heat:
                computedNewState = STATE::AwaitingHeat;
                break;
            // Selected state
            case STATE::Cool:
                computedNewState = STATE::AwaitingCool;
                break;
            // Selected state
            case STATE::Fan:
                computedNewState = STATE::Fan;
                break;
            
            default:
                break;
            }
            break;
    
        // Current state
        case STATE::Cool:
            switch (requestedState) {
    
            // Selected state
            case STATE::Idle:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
                break;
            // Selected state
            case STATE::Heat:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::AwaitingHeat;
                _lastHeavyTime = millis();
                break;
            // Selected state
            case STATE::Fan:
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Fan;
                _lastHeavyTime = millis();
                break;
            
            default:
                break;
            }
            break;
            
        // Current state
        case STATE::Fan:
            switch (requestedState) {
    
            // Selected state
            case STATE::Idle:
                computedNewState = STATE::Idle;
                break;
            // Selected state
            case STATE::Heat:
                computedNewState = STATE::AwaitingHeat;
                break;
            // Selected state
            case STATE::Cool:
                computedNewState = STATE::AwaitingCool;
                break;
            
            default:
                break;
            }
            break;
    
        default:
            break;
        }
    
    }


    // ==== Limiting active state switches by delay ====
    // Past this if statement, we can trust that we are allowed to go to heat/cool state
    long int timeSinceLastHeavy = millis() - _lastHeavyTime;
    if(timeSinceLastHeavy < getDelay(ts_.lastHeavyState, computedNewState)) {
        Serial.println("Awaiting delay");
        return computedNewState;
    }

    // ==== awaiting --> active ====
    switch (computedNewState) {
        // Current state
        case STATE::AwaitingHeat:
            computedNewState = STATE::Heat;
            break;
        
        // Current state
        case STATE::AwaitingCool:
            computedNewState = STATE::Cool;
            break;
    }

    return computedNewState;
}


STATE ThermostatModel::_computeAutoStateChange() {
    STATE computedNewState = ts_.state;    

    // Min and Max safty starting the thermostat
    if(1 == 0) {
        // TODO: Implement min/max temperature checks
    }else {
        // We are within the good temps, so we can do standard temp changes
        long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;

        switch(ts_.state) {
        case STATE::Idle:
        case STATE::AwaitingCool:
        case STATE::AwaitingHeat:
        case STATE::Fan:
            // If the resetTimer is up and the device isn't unlocked
            if(!isUnlocked() && timeSinceLastInteraction > RESET_LIMIT_MS) {
                Serial.println(">>>> Turning off due to timer...");
                computedNewState = STATE::Idle;
                Serial.println("TODO: Flag off button here");
                // flag_offButton = true;
                break;
            }

            // Otherwise, just do the regular automatic changes
            if(ts_.temp + ts_.margin < ts_.goalTemp) {
                computedNewState = STATE::AwaitingHeat;
            }else if(ts_.temp - ts_.margin > ts_.goalTemp) {
                computedNewState = STATE::AwaitingCool;
            }else {
                computedNewState = STATE::Idle;
            }
            break;

        case STATE::Cool:
            // If the resetTimer is up and the device isn't unlocked
            if(!isUnlocked() && timeSinceLastInteraction > RESET_LIMIT_MS) {
                Serial.println(">>>> Turning off due to timer...");
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
                Serial.println("TODO: Flag off button here");
                // flag_offButton = true;
                break;
            }

            if(ts_.temp + ts_.margin < ts_.goalTemp) {
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::AwaitingHeat;
                _lastHeavyTime = millis();
            }else if(abs(ts_.temp - ts_.goalTemp) < ts_.margin) {
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
            }
            break;
        case STATE::Heat:
            // If the resetTimer is up and the device isn't unlocked
            if(!isUnlocked() && timeSinceLastInteraction > RESET_LIMIT_MS) {
                Serial.println(">>>> Turning off due to timer...");
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
                Serial.println("TODO: Flag off button here");
                // flag_offButton = true;
                break;
            }
            
            if(ts_.temp - ts_.margin > ts_.goalTemp) {
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::AwaitingCool;
                _lastHeavyTime = millis();
            }else if(abs(ts_.temp - ts_.goalTemp) < ts_.margin) {
                ts_.lastHeavyState = ts_.state;
                computedNewState = STATE::Idle;
                _lastHeavyTime = millis();
            }
            break;
        }

    }

    long int timeSinceLastHeavy = millis() - _lastHeavyTime;
    if(timeSinceLastHeavy < getDelay(ts_.lastHeavyState, computedNewState)) {
        Serial.println("Awaiting delay (auto)");
        return computedNewState;
    }
    
    if(ts_.temp < 0) {
        Serial.println("Temperature not set");
        return computedNewState;
    }
    // Past these last two if statement, we can trust that we are allowed to go to heat/cool state


    // ==== awaiting --> active ====
    switch (computedNewState) {
        // Current state
        case STATE::AwaitingHeat:
        computedNewState = STATE::Heat;
        break;
        
        // Current state
        case STATE::AwaitingCool:
        computedNewState = STATE::Cool;
        break;
    }

    return computedNewState;
}