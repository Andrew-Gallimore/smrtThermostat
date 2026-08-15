#include "ThermostatModel.h"

// Initial variables
// const int LONG_STATE_DELAY = 480000;    // 8 minutes in ms
// const int REG_STATE_DELAY = 300000;     // 5 minutes in ms
const int LONG_STATE_DELAY = 20000;    // 15s in ms
const int REG_STATE_DELAY = 10000;     // 10s in ms

long int RESET_LIMIT_MS = 2 * 3600000; // 2 hours

int CODE_VAL1 = 4; // 0-9
int CODE_VAL2 = 1; // 0-9
int CODE_VAL3 = 6; // 0-9
int CODE_VAL4 = 0; // 0-9



ThermostatModel::ThermostatModel(ROLE role, SyncManager& sync) : sync_(sync) {
    ts_.role = role;
    ts_.mode = MODE::Off;
    ts_.lastMode = MODE::Manual;
    ts_.state = STATE::Idle;
    ts_.goalState = None;
    ts_.temp = 70;
    ts_.goalTemp = 70;
    ts_.onMargin = 1.0f;
    ts_.offMargin = 1.0f;
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
    ts_.goalState = None;
}

void ThermostatModel::restoreLastMode() {
    if(ts_.role == ROLE::CHILD) {
        Serial.println("WARN: Child cannot restore last mode.");
        return;
    }

    setMode(ts_.lastMode);
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
      // Set relays for idle
      digitalWrite(GPIO_RELAY1, LOW);  // Ensure heating relay is off
      digitalWrite(GPIO_RELAY2, LOW);  // Ensure cooling relay is off
      digitalWrite(GPIO_RELAY3, LOW);  // Ensure fan relay is off
      break;
  }
}




bool ThermostatModel::unlockTest(int val1, int val2, int val3, int val4) {
    // locked = !locked;

    if(val1 == CODE_VAL1 && val2 == CODE_VAL2 && val3 == CODE_VAL3 && val4 == CODE_VAL4) {
        ts_.unlocked = true;
        Serial.println("System unlocked!!");
    }else {
        ts_.unlocked = false;
    }

    sync_.publishThermState(ts_);
    _notify();
    return ts_.unlocked;
}

void ThermostatModel::lock() {
    ts_.unlocked = false;
    sync_.publishThermState(ts_);
    _notify();
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
        ts_.goalState = None;
    } else if(newMode == MODE::Auto) {
        // Clear any stale manual pending goal when switching into Auto mode.
        ts_.goalState = None;
        computedNewState = _computeAutoStateChange();
    } else if(newMode == MODE::Manual) {
        // Switching to manual mode preserves the current state and any pending manual goal.
        computedNewState = ts_.state;
    }

    // Actually setting the new mode
    ts_.mode = newMode;

    // Let child thermostats know about the mode/state change
    if(computedNewState != ts_.state) {
        // If leaving heavy state, record that
        if(ts_.state == STATE::Heat || ts_.state == STATE::Cool) {
            _lastHeavyTime = millis();
            ts_.lastHeavyTime = _lastHeavyTime;
            ts_.lastHeavyState = ts_.state;
        }

        // Resetting goalState if we are doing the change
        if((ts_.state == STATE::Idle || ts_.state == STATE::Fan)
          && (computedNewState == STATE::Heat || computedNewState == STATE::Cool)) {
            ts_.goalState = None;
        }

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

    // Parent logic: change the goal temperature and let update() handle auto recomputation.
    ts_.goalTemp = newTemp;

    if (ts_.mode == MODE::Auto) {
        update();
    }

    sync_.publishThermState(ts_);
    _notify(); // Notify observers of the change
}

void ThermostatModel::setTemp(float newTemp) {
    if(ts_.role == ROLE::CHILD) {
        Serial.println("WARN: Child cannot set current temperature directly.");
        return;
    }

    // Parent logic: change the current temperature and let update() handle auto recomputation.
    ts_.temp = newTemp;

    if (ts_.mode == MODE::Auto) {
        update();
    }else {
        // The reason this is in the else is because update automatically
        //      handles sync and notifying
        sync_.publishThermState(ts_);
        _notify(); // Notify observers of the change
    }

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

    STATE computedNewState = _computeManualStateChange(newState);

    Serial.print("Computed new state: ");
    Serial.println(computedNewState);
    if (computedNewState != ts_.state) {
        // If leaving heavy state, record that
        if(ts_.state == STATE::Heat || ts_.state == STATE::Cool) {
            _lastHeavyTime = millis();
            ts_.lastHeavyTime = _lastHeavyTime;
            ts_.lastHeavyState = ts_.state;
        }

        Serial.print("Changing state from ");
        Serial.print(ts_.state);
        Serial.print(" to ");
        Serial.println(computedNewState);
        // Resetting goalState if we are doing the change
        if((ts_.state == STATE::Idle || ts_.state == STATE::Fan)
          && (computedNewState == STATE::Heat || computedNewState == STATE::Cool)) {
            ts_.goalState = None;
        }

        ts_.state = computedNewState;
        sync_.publishThermState(ts_);
    }

    _notify();
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
    STATE computedNewState = ts_.state;
    // For parent, we might want to check if the state needs to
    //      be updated
    if(ts_.role == ROLE::PARENT) {
        if(ts_.mode == MODE::Auto) {
            computedNewState = _computeAutoStateChange();
        }else if(ts_.mode == MODE::Manual) {
            if(ts_.goalState == AwaitingHeat) {
                computedNewState = _computeManualStateChange(STATE::Heat);
            } else if(ts_.goalState == AwaitingCool) {
                computedNewState = _computeManualStateChange(STATE::Cool);
            }
        }

        if(computedNewState != ts_.state) {
            // If leaving heavy state, record that
            if(ts_.state == STATE::Heat || ts_.state == STATE::Cool) {
                _lastHeavyTime = millis();
                ts_.lastHeavyTime = _lastHeavyTime;
                ts_.lastHeavyState = ts_.state;
            }

            // Resetting goalState if we are doing the change
            if((ts_.state == STATE::Idle || ts_.state == STATE::Fan)
              && (computedNewState == STATE::Heat || computedNewState == STATE::Cool)) {
                ts_.goalState = None;
            }

            ts_.state = computedNewState;
            sync_.publishThermState(ts_);
        }

        _notify(); // Notify observers of the state change
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
long int ThermostatModel::_getCalculatedDelay(STATE fromState, STATE toState) {
    long int requiredDelay = 0;
    if(fromState == STATE::Heat && toState == STATE::Cool) {
        requiredDelay = LONG_STATE_DELAY;
    }else if(fromState == STATE::Cool && toState == STATE::Heat) {
        requiredDelay = LONG_STATE_DELAY;
    }else if(fromState == STATE::Heat && toState == STATE::Heat) {
        requiredDelay = REG_STATE_DELAY;
    }else if(fromState == STATE::Cool && toState == STATE::Cool) {
        requiredDelay = REG_STATE_DELAY;
    }
    
    long int timeSinceLastHeavy = millis() - _lastHeavyTime;
    long int remainingDelay = requiredDelay - timeSinceLastHeavy;
    return (remainingDelay > 0) ? remainingDelay : 0;
}

long int ThermostatModel::getRemainingDelay() {
    STATE targetState = ts_.state;
    if (ts_.goalState == AwaitingHeat) {
        targetState = STATE::Heat;
    } else if (ts_.goalState == AwaitingCool) {
        targetState = STATE::Cool;
    }

    return _getCalculatedDelay(ts_.lastHeavyState, targetState);
}

long int ThermostatModel::getRemainingInteractionTime() {
    long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;
    long int remainingTime = RESET_LIMIT_MS - timeSinceLastInteraction;
    return (remainingTime > 0) ? remainingTime : 0;
}

STATE ThermostatModel::_computeManualStateChange(STATE requestedState) {
    if(!isUnlocked() && getRemainingInteractionTime() == 0) {
        Serial.println(">>>> Turning off due to timer...");
        ts_.goalState = None;
        return STATE::Idle;
    }

    STATE computedNewState = ts_.state;

    // Switching goals
    switch (ts_.state) {
        case STATE::Idle:
            if(requestedState == STATE::Heat) {
                ts_.goalState = AwaitingHeat;
            }else if(requestedState == STATE::Cool) {
                ts_.goalState = AwaitingCool;
            }else if(requestedState == STATE::Fan) {
                ts_.goalState = None;
                return STATE::Fan;
            }else {
                ts_.goalState = None;
            }
            break;
        case STATE::Heat:
            if(requestedState == STATE::Cool) {
                ts_.goalState = AwaitingCool;
            }else if(requestedState == STATE::Fan) {
                ts_.goalState = None;
                return STATE::Fan;
            }else if(requestedState == STATE::Idle) {
                ts_.goalState = None;
                return STATE::Idle;
            }
            break;
        case STATE::Cool:
            if(requestedState == STATE::Heat) {
                ts_.goalState = AwaitingHeat;
            }else if(requestedState == STATE::Fan) {
                ts_.goalState = None;
                return STATE::Fan;
            }else if(requestedState == STATE::Idle) {
                ts_.goalState = None;
                return STATE::Idle;
            }
            break;
        case STATE::Fan:
            if(requestedState == STATE::Idle) {
                ts_.goalState = None;
            }else if(requestedState == STATE::Heat) {
                ts_.goalState = AwaitingHeat;
                computedNewState = STATE::Idle;
                // TODO: When switching from manual fan to awaiting heat/cool, it should go to
                //       idle state but it should have the ablility to go directly to heat 
                //       within this method call...
            }else if(requestedState == STATE::Cool) {
                ts_.goalState = AwaitingCool;
                computedNewState = STATE::Idle;
            }
            break;
        default:
            // Should never get here...
            Serial.println("WARN: Unhandled manual goalState change case! AHHHHHHHHH");
            ts_.goalState = None;
            break;
    }


    // Switching states now...
    
    // If we are in a heavy state, we need to wait for the delay in Idle
    if(computedNewState == STATE::Heat) {
        if(ts_.goalState == GOAL_STATE::AwaitingCool) {
            return STATE::Idle;
        }
    }else if(computedNewState == STATE::Cool) {
        if(ts_.goalState == GOAL_STATE::AwaitingHeat) {
            return STATE::Idle;
        }
    }else if(computedNewState == STATE::Idle || computedNewState == STATE::Fan) {
        // Checking if we can transition to the requested
        //      heat/cool state now because of the delay timer
        if(getRemainingDelay() > 0) {
            // Stay in the current state until the delay is over
            return computedNewState;
        }


        if(ts_.goalState == AwaitingHeat) {
            return STATE::Heat;
        }else if(ts_.goalState == AwaitingCool) {
            return STATE::Cool;
        }
    }

    // If nothing else, we stay in the current state
    return ts_.state;
}


STATE ThermostatModel::_computeAutoStateChange() {
    if(!isUnlocked() && getRemainingInteractionTime() == 0) {
        Serial.println(">>>> Turning off due to timer...");
        ts_.goalState = None;
        return STATE::Idle;
    }

    // Setting goalState 
    if(ts_.temp <= ts_.goalTemp - ts_.onMargin) {
        if(ts_.state != STATE::Heat) {
            ts_.goalState = AwaitingHeat;
        }
    } else if(ts_.temp >= ts_.goalTemp + ts_.onMargin) {
        if(ts_.state != STATE::Cool) {
            ts_.goalState = AwaitingCool;
        }
    }else {
        ts_.goalState = None;
        return STATE::Idle;
    }

    // Changing state
    switch(ts_.state) {
        case STATE::Heat:
            if(ts_.goalState == AwaitingCool) {
                return STATE::Idle;
            }
            break;
        case STATE::Cool:
            if(ts_.goalState == AwaitingHeat) {
                return STATE::Idle;
            }
            break;
        case STATE::Idle:
        case STATE::Fan:
            if(getRemainingDelay() > 0) {
                return ts_.state;
            }

            if(ts_.goalState == AwaitingHeat) {
                return STATE::Heat;
            } else if(ts_.goalState == AwaitingCool) {
                return STATE::Cool;
            }
            break;
        default:
            // Should never get here...
            Serial.println("WARN: Unhandled auto state change case! AHHHHHHHHH");
            ts_.goalState = None;
            break;
    }

    // If nothing else, we stay in the current state
    return ts_.state;
}
