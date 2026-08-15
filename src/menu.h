#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "./fonts/chivo_mono_34.h"

#include "./icons/exit_30.h"
#include "./icons/right_30.h"
#include "./icons/wifi_30.h"
#include "./icons/trash_30.h"
#include "./icons/lock_36.h"
#include "./icons/unlock_36.h"
#include "./icons/settings_36.h"

// #include "./stateMachine.h"
#include "./core/core-structs.h"
// #include "./ui.h"
#include "./view/ThermostatView.h"
#include "./remoteThermostat.h"
#include "./colorHelper.h"
#include "./thermometers.h"

// Forward delcarations for view
extern ThermostatView view;

// Forward declarations for callback functions defined in main.cpp
void onOFFButtonClick();
void onManualButtonClick();
void onAutoButtonClick();
void onLockButtonClick();



// Regular declarations...
void UIapplyButtonStyle(lv_obj_t* btn, bool useLighterBg);
void UIinitializeMenu();
void UIhideMenuButton();
void UIshowMenuButton();
void UIshowMenu();
void UIhideMenu();

void UIinitializeSettings();
void UIshowSettings();
void UIhideSettings();

void UIinitializeNetwork();
void UIshowNetwork();
void UIhideNetwork();

void UIinitializeThermometers();
void UIshowThermometers();
void UIhideThermometers();



#endif //MENU_H