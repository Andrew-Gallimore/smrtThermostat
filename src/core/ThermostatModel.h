#ifndef THERMOSTAT_MODEL_H
#define THERMOSTAT_MODEL_H

#include <Arduino.h>
#include <vector>
#include "core-structs.h"
#include "SyncManager.h"

// For HVAC relays
#define GPIO_RELAY1  40
#define GPIO_RELAY2  2
#define GPIO_RELAY3  1


using ThermostatObserver = std::function<void(const ThermostatState&)>;

class ThermostatModel {
    public:
        ThermostatModel(ROLE role, SyncManager& sync);
        void update();
        long int getRemainingDelay();
        long int getRemainingInteractionTime();
        void newInteraction() { ts_.lastInteractionTime = millis(); }

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

        // Observers
        std::vector<ThermostatObserver> observers_;
        void _notify();

        // Sync manager for network communication
        SyncManager& sync_;
};

#endif // THERMOSTAT_MODEL_H