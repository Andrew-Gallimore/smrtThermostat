#include "ThermostatModel.h"

ThermostatModel::ThermostatModel(ROLE role, SyncManager& sync) : sync_(sync) {
    ts_.role = role;
    ts_.mode = MODE::Off;
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

void ThermostatModel::setMode(MODE newMode) {
    if(ts_.role == ROLE::PARENT) {
        // Update local state and sync
        ts_.mode = newMode;
        sync_.publishMode(newMode);
    } else {
        // Send command to parent to change mode
        Command cmd;
        cmd.type = COMMAND_TYPE::SetMode;
        cmd.mode = newMode;
        sync_.publishCommand(cmd);
    }
}

void ThermostatModel::setTargetTemp(float newTemp) {
    if(ts_.role == ROLE::PARENT) {
        ts_.goalTemp = newTemp;
        sync_.publishThermState(ts_);
    }else {
        // Send command to parent to change target temperature
        Command cmd;
        cmd.type = COMMAND_TYPE::SetTempGoal;
        cmd.tempGoal = newTemp;
        sync_.publishCommand(cmd);
    }
}

void ThermostatModel::setCurrentTemp(float newTemp) {
    if(ts_.role == ROLE::CHILD) {
        Serial.println("WARN: Child cannot set current temperature directly.");
        return;
    }

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
    STATE newState = _computeManualStateChange(newState); // This function should check if the state change is valid
    if(newState != ts_.state) {
        ts_.state = newState;
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
    // For parent, we might want to check if the state needs to
    //      be updated based in auto mode
    if(ts_.role == ROLE::PARENT && ts_.mode == MODE::Auto) {
        STATE newState = _computeAutoStateChange();
        if(newState != ts_.state) {
            ts_.state = newState;
            sync_.publishThermState(ts_);
            _notify(); // Notify observers of the state change
        }
    }
}

void ThermostatModel::_notify() {
    // Check what has changed and notify observers accordingly
    if(memcmp(&ts_, &oldTs_, sizeof(ThermostatState)) != 0) {
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
        return;
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
        return;
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
        return;
    }
    
    if(ts_.temp < 0) {
        Serial.println("Temperature not set");
        return;
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