#include "ThermostatModel.h"

// Initial variables
// const int LONG_STATE_DELAY = 480000;    // 8 minutes in ms
// const int REG_STATE_DELAY = 300000;     // 5 minutes in ms
const int LONG_STATE_DELAY = 20000;    // 15s in ms
const int REG_STATE_DELAY = 10000;     // 10s in ms

long int RESET_LIMIT_MS = 2 * 3600000; // 2 hours



ThermostatModel::ThermostatModel(ROLE role, SyncManager& sync) : sync_(sync) {
    ts_.role = role;
    ts_.mode = MODE::Off;
    ts_.lastMode = MODE::Manual;
    ts_.state = STATE::Idle;
    ts_.goalState = None;
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

bool ThermostatModel::_isHeavyState(STATE state) const {
    return state == STATE::Heat || state == STATE::Cool;
}

void ThermostatModel::_recordHeavyExit() {
    if (!_isHeavyState(ts_.state)) {
        return;
    }
    ts_.lastHeavyState = ts_.state;
    _lastHeavyTime = millis();
    ts_.lastHeavyTime = _lastHeavyTime;
}

STATE ThermostatModel::_resolvePendingState(STATE targetState) {
    long int remaining = getRemainingDelay();
    if (remaining > 0) {
        if (ts_.state != targetState && ts_.state != STATE::Idle) {
            if (_isHeavyState(ts_.state)) {
                _recordHeavyExit();
            }
            return STATE::Idle;
        }
        return ts_.state;
    }

    ts_.goalState = None;
    return targetState;
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
    
    if (computedNewState != ts_.state && _isHeavyState(ts_.state) && !_isHeavyState(computedNewState)) {
        _recordHeavyExit();
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
    }

    sync_.publishThermState(ts_);
    _notify(); // Notify observers of the change
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

    STATE previousState = ts_.state;
    GOAL_STATE previousGoal = ts_.goalState;

    STATE computedNewState = _computeManualStateChange(newState);
    if (computedNewState != ts_.state) {
        if (_isHeavyState(ts_.state) && !_isHeavyState(computedNewState)) {
            _recordHeavyExit();
        }
        ts_.state = computedNewState;
    }

    if (ts_.state != previousState || ts_.goalState != previousGoal) {
        sync_.publishThermState(ts_);
        _notify();
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
        } else if(ts_.mode == MODE::Manual) {
            if (ts_.goalState == GOAL_STATE::AwaitingHeat) {
                newState = _computeManualStateChange(STATE::Heat);
            } else if (ts_.goalState == GOAL_STATE::AwaitingCool) {
                newState = _computeManualStateChange(STATE::Cool);
            } else {
                newState = ts_.state;
            }
        }

        if(newState != ts_.state) {
            if (_isHeavyState(ts_.state) && !_isHeavyState(newState)) {
                _recordHeavyExit();
            }
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
    STATE targetState = ts_.state;
    if (ts_.goalState == AwaitingHeat) {
        targetState = STATE::Heat;
    } else if (ts_.goalState == AwaitingCool) {
        targetState = STATE::Cool;
    }
    long int requiredDelay = getDelay(ts_.lastHeavyState, targetState);
    long int remainingDelay = requiredDelay - timeSinceLastHeavy;
    return (remainingDelay > 0) ? remainingDelay : 0;
}

long int ThermostatModel::getRemainingInteractionTime() {
    long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;
    long int remainingTime = RESET_LIMIT_MS - timeSinceLastInteraction;
    return (remainingTime > 0) ? remainingTime : 0;
}

STATE ThermostatModel::_computeManualStateChange(STATE requestedState) {
    if (requestedState == STATE::Idle) {
        if (_isHeavyState(ts_.state)) {
            _recordHeavyExit();
        }
        ts_.goalState = None;
        return STATE::Idle;
    }

    if (requestedState == STATE::Fan) {
        if (_isHeavyState(ts_.state)) {
            _recordHeavyExit();
        }
        ts_.goalState = None;
        return STATE::Fan;
    }

    if (requestedState == STATE::Heat) {
        ts_.goalState = AwaitingHeat;
    } else if (requestedState == STATE::Cool) {
        ts_.goalState = AwaitingCool;
    }

    return _resolvePendingState(requestedState);
}


STATE ThermostatModel::_computeAutoStateChange() {
    long int timeSinceLastInteraction = millis() - ts_.lastInteractionTime;
    if(!isUnlocked() && timeSinceLastInteraction > RESET_LIMIT_MS) {
        Serial.println(">>>> Turning off due to timer...");
        ts_.goalState = None;
        return STATE::Idle;
    }

    GOAL_STATE desiredGoalState = None;
    if(ts_.temp <= ts_.goalTemp - ts_.margin) {
        desiredGoalState = AwaitingHeat;
    } else if(ts_.temp >= ts_.goalTemp + ts_.margin) {
        desiredGoalState = AwaitingCool;
    }

    if (desiredGoalState == None) {
        ts_.goalState = None;
        return STATE::Idle;
    }

    ts_.goalState = desiredGoalState;
    STATE targetState = desiredGoalState == AwaitingHeat ? STATE::Heat : STATE::Cool;
    return _resolvePendingState(targetState);
}
