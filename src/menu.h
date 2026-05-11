#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "./fonts/chivo_mono_34.h"

#include "./icons/exit_30.h"
#include "./icons/right_30.h"
#include "./icons/wifi_30.h"

#include "./remoteThermostat.h"
#include "./colorHelper.h"

// Forward declarations for callback functions defined in main.cpp
void onOFFButtonClick();
void onSettingsButtonClick();
void onManualButtonClick();
void onAutoButtonClick();

// Forward delcaration for helper function in ui.cpp
void UIapplyButtonStyle(lv_obj_t* btn);


// Regular declarations...
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



#endif //MENU_H