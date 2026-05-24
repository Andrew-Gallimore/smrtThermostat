#include "./locking.h"

int CODE_VAL1 = 4; // 0-9
int CODE_VAL2 = 1; // 0-9
int CODE_VAL3 = 6; // 0-9
int CODE_VAL4 = 0; // 0-9
bool unlocked = false;

bool isUnlocked() {
    return unlocked;
}

void lockSystem() {
    Serial.println("System locked");
    unlocked = false;
}

// IF the test passed, it returns TRUE
//    and has the side effect of
//    switching to unlocked
//    ELSE returns FALSE
bool unlockTest(int val1, int val2, int val3, int val4) {
    // locked = !locked;

    if(val1 == CODE_VAL1 && val2 == CODE_VAL2 && val3 == CODE_VAL3 && val4 == CODE_VAL4) {
        unlocked = true;
        Serial.println("System unlocked!!");
    }else {
        unlocked = false;
    }

    // if(locked) {
    //     UIhideTimer();
    // }else {
    //     if(getCurrentState() != STATE::Idle) {
    //         UIshowTimer();
    //     }
    // }

    return unlocked;
}