#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

enum MODE {
  Off,
  Auto,
  Manual
};

enum STATE {
  Idle,
  AwaitingHeat,
  Heat,
  AwaitingCool,
  Cool,
  Fan,
};

static const char* STRING_FROM_STATE[6] = {
  "Idle",
  "AwaitingHeat",
  "Heat",
  "AwaitingCool",
  "Cool",
  "Fan"
};


// Forward declarations
void onOFFButtonClick();
extern volatile bool flag_offButton;
extern volatile bool GoalNeedsUpdate;
extern volatile bool HAGoalNeedsUpdate;
extern volatile bool TempNeedsUpdate;
extern volatile bool ModeNeedsUpdate;
extern volatile bool HAModeNeedsUpdate;
extern volatile STATE possibleState;
extern volatile bool StateNeedsUpdate;
extern volatile bool HAStateNeedsUpdate;





// Function declarations
void initializeStateMachine();

float getTemp();
float getTempGoal();
float getTempMargin();
void setTemp(float newTemp);
void setTempGoal(float newTempGoal);

void onNewTempReading(float temp);

void onRemoteTempGoal(float newGoalTemp);
void onHATempGoal(float newTempGoal);

MODE getLastMode();
MODE getCurrentMode();
STATE getCurrentState();
STATE getLastHeavyState();

void setCurrentMode(MODE newMode);
void setLastMode(MODE newMode);
void setLastHeavyState(STATE newLastHeavyState);
void setCurrentState(STATE newState);
void setCurrentStateSilently(STATE newState);

void onRemoteMode(MODE newMode);
void onHARemoteMode(MODE newMode);

void resetHeavyEndedTimer();
long int timeSinceLastHeavyState();
long int getDelay(STATE fromState, STATE toState);

void newInteraction();
long int getResetTimeLimit();
long int timeSinceLastInteraction();

void computeManualState(STATE selectedState);
void computeAutoState();

#endif // STATE_MACHINE_H
