#ifndef CORE_STRUCTS_H
#define CORE_STRUCTS_H

enum ROLE {
  PARENT,
  CHILD
};
enum MODE {
  Off,
  Auto,
  Manual
};
enum STATE {
  Idle,
  Heat,
  Cool,
  Fan,
};

enum GOAL_STATE {
  None,
  AwaitingHeat,
  AwaitingCool,
};


struct ThermostatState {
  ROLE role = ROLE::PARENT;
  MODE mode = MODE::Off;
  MODE lastMode = MODE::Off;
  STATE state = STATE::Idle; // active physical state
  GOAL_STATE goalState = None; // desired next transition
  float temp = 70;
  float goalTemp = 70;
  float onMargin = 1.0f;
  float offMargin = 1.0f;
  bool unlocked = false;
  bool delayActive = false;
  long int lastHeavyTime = 0;
  STATE lastHeavyState = STATE::Idle;
  long int autoDelayStartTime = 0;
  long int lastInteractionTime = 0;
};


enum COMMAND_TYPE {
  SetMode,
  SetTempGoal,
  SetTemp,
  SetState
};

struct Command {
  COMMAND_TYPE type;
  MODE mode;
  float tempGoal;
  float temp;
  STATE state;
};

#endif // CORE_STRUCTS_H