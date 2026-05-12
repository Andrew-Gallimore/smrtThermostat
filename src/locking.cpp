#include "./locking.h"

bool locked = false;

bool isUnlocked() {
    return locked;
}

// Returns true if test passed
// Has side effect of unlocking
bool lockTest() {
    locked = !locked;

    return locked;
}