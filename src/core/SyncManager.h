#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "core-structs.h"

class SyncManager {
public:
    void publishThermState(const ThermostatState& s);
    void publishCommand(Command c);
    void publishMode(MODE m);
    void onRemoteState(std::function<void(const ThermostatState&)> cb);
    void onRemoteCommand(std::function<void(Command)> cb);
};

#endif // SYNC_MANAGER_H