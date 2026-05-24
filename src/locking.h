#ifndef LOCKING_H
#define LOCKING_H

#include <Arduino.h>

bool isUnlocked();
void lockSystem();
bool unlockTest(int val1, int val2, int val3, int val4);

long getLockingResetLimit();
long getLockingOffLimit();

#endif // LOCKING_H