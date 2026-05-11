#ifndef UI_H
#define UI_H

#include <cstdio>
#include <Arduino.h>
#include <chrono>
#include <string>

#include "./fonts/chivo_mono_158.h"
#include "./fonts/chivo_mono_110.h"
#include "./fonts/chivo_mono_34.h"

#include "./icons/chevron_65.h"
#include "./icons/fan_65.h"
#include "./icons/flame_65.h"
#include "./icons/winter_65.h"

#include "./colorHelper.h"
#include "stateMachine.h"

// Forward declarations for callback functions defined in main.cpp
void onTempUpButtonClick(lv_event_t* e);
void onTempDownButtonClick(lv_event_t* e);
void onManualHeatClick();
void onManualCoolClick();
void onManualFanClick();
void onSwitchOnClick();
void checkState();


// Regular declarations...
void UIinitializeDelay();
void UIshowDelay();
void UIhideDelay();

void UIinitializeHeatZone();
void UIshowHeat();
void UIhideHeat();

void UIinitializeCoolZone();
void UIshowCool();
void UIhideCool();

void UItempInitialize();
void UItempSet(float value);
void UItempErrorCheck();

void UIgoalInitialize();
void UIgoalSet(float value);
void UIgoalSet(String msg);

void UIinitializeAutoBTNs();
void UIshowAutoBTNs();
void UIhideAutoBTNs();

void UIinitializeManualBTNs();
void UIshowManualBTNs();
void UIhideManualBTNs();
void UIsetManualBTNState(STATE state);

void UIinitializeOnButton();
void UIshowOnButton();
void UIhideOnButton();

// Helper function for button styling
void UIapplyButtonStyle(lv_obj_t* btn);

#endif //UI_H
