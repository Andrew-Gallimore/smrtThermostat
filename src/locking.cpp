#include "./locking.h"

bool locked = false;

bool isUnlocked() {
    return locked;
}

// Returns true if test passed
// Has side effect of unlocking
bool lockTest() {
    locked = !locked;

    // if(locked) {
    //     UIhideTimer();
    // }else {
    //     if(getCurrentState() != STATE::Idle) {
    //         UIshowTimer();
    //     }
    // }

    return locked;
}