#ifndef THERMOSTAT_MODEL_H
#define THERMOSTAT_MODEL_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "core-structs.h"

// For HVAC relays
#define GPIO_RELAY1  40
#define GPIO_RELAY2  2
#define GPIO_RELAY3  1

using ThermostatObserver = std::function<void(const ThermostatState&)>;
using CommandSender = std::function<void(const Command&)>;
using StatePublisher = std::function<void(const ThermostatState&)>;

class ThermostatModel {
    public:
        ThermostatModel(ROLE role);
        void update();
        long int getRemainingDelay();
        long int getRemainingInteractionTime();
        void newInteraction() { ts_.lastInteractionTime = millis(); }
        void clearInteractionTimer() { ts_.lastInteractionTime = 0; }
        void setCommandSender(CommandSender sender);
        void setStatePublisher(StatePublisher publisher);

        bool isUnlocked() const { return ts_.unlocked; }
        void lock();
        bool unlockTest(int val1, int val2, int val3, int val4);

        void setMode(MODE newMode);
        MODE getMode() const { return ts_.mode; }
        void setGoalTemp(float newTemp);
        float getGoalTemp() const { return ts_.goalTemp; }
        void setGoalState(GOAL_STATE newGoal);
        GOAL_STATE getGoalState() const { return ts_.goalState; }
        void setTemp(float newTemp);
        float getTemp() const { return ts_.temp; }
        void requestManualState(STATE newState);
        STATE getCurrentState() const { return ts_.state; }
        ThermostatState getState() const { return ts_; }
        
        // Getting updates from it
        void subscribe(ThermostatObserver observer);

        // Remote thermostat updates
        void applyRemoteState(const ThermostatState& remote);

        // Storage initialization
        void initializeFromStorage(MODE lastMode,
                                   float lastTemp,
                                   STATE lastHeavyState);
        void restoreLastMode();
    private:
        STATE _computeAutoStateChange();
        STATE _computeManualStateChange(STATE requestedState);
        void _setRelaysFromState(STATE newState);
        long int _getCalculatedDelay(STATE fromState, STATE toState);
        ThermostatState ts_;
        ThermostatState oldTs_;
        STATE _lastHeavyState;
        long int _lastHeavyTime;
        long int _lastInteractionTime;
        CommandSender commandSender_;
        StatePublisher statePublisher_;

        // Observers
        std::vector<ThermostatObserver> observers_;
        void _notify();
};

#endif // THERMOSTAT_MODEL_H