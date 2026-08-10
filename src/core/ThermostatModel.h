#ifndef THERMOSTAT_MODEL_H
#define THERMOSTAT_MODEL_H

#include <Arduino.h>
#include <vector>
#include "core-structs.h"
#include "../locking.h"
#include "SyncManager.h"

const int LONG_STATE_DELAY = 480000;    // 8 minutes in ms
const int REG_STATE_DELAY = 300000;     // 5 minutes in ms
// const int LONG_STATE_DELAY = 15000;    // 15s in ms
// const int REG_STATE_DELAY = 10000;     // 10s in ms

long int RESET_LIMIT_MS = 2 * 3600000; // 2 hours

using ThermostatObserver = std::function<void(const ThermostatState&)>;

class ThermostatModel {
    public:
        ThermostatModel(ROLE role, SyncManager& sync);
        void update();

        void setMode(MODE newMode);
        void setTargetTemp(float newTemp);
        void setCurrentTemp(float newTemp);
        void requestManualState(STATE newState);
        
        // Getting updates from it
        void subscribe(ThermostatObserver observer);

        // Remote thermostat updates
        void applyRemoteState(const ThermostatState& remote);

        // Storage initialization
        void initializeFromStorage(MODE lastMode,
                                   float lastTempGoal,
                                   float lastTemp,
                                   STATE lastHeavyState);
    private:
        STATE _computeAutoStateChange();
        STATE _computeManualStateChange(STATE requestedState);
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