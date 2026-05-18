#ifndef LOCKING_H
#define LOCKING_H

bool isUnlocked();
bool lockTest();

long getLockingResetLimit();
long getLockingOffLimit();

#endif // LOCKING_H