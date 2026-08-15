#include <lvgl.h>
#include "ui.h"

lv_obj_t* delayIcon = nullptr;
lv_obj_t* delayMsg = nullptr;
lv_anim_t delayAnim;
lv_anim_t delayMsgAnim;
bool delayVisible = false;

lv_obj_t* unlockIcon = nullptr;

lv_obj_t* timerIcon = nullptr;
lv_obj_t* timerMsg = nullptr;
lv_anim_t timerAnim;
lv_anim_t timerMsgAnim;
bool timerVisible = false;

lv_obj_t* heatZone = nullptr;
bool heatZoneVisible = false;

lv_obj_t* coolZone = nullptr;
bool coolZoneVisible = false;

lv_obj_t* autoBTN1 = nullptr;
lv_obj_t* autoBTN2 = nullptr;

lv_obj_t* manualBTN1 = nullptr;
lv_obj_t* manualBTN2 = nullptr;
lv_obj_t* manualBTN3 = nullptr;

lv_obj_t* tempText = nullptr;
lv_obj_t* tempErrorText = nullptr;
lv_obj_t* tempSpinner = nullptr;

lv_obj_t* goalText = nullptr;
lv_obj_t* goalErrorText = nullptr;
lv_obj_t* goalSpinner = nullptr;

lv_obj_t* onButton = nullptr;

bool tempInitialized = false;
bool tempInErrorState = false;
float lastGoodTemp = 0;
long int lastGoodTempTime = 0;
int TEMP_ERROR_TIME = 60000;

void UIinitializeDelay() {}
void UIshowDelay() {}
void UIhideDelay() {}

void UIinitializeTimer() {}
void UIshowTimer() {}
void UIhideTimer() {}

void UIinitializeLock() {}
void UIshowUnlock() {}
void UIhideUnlock() {}

void UIinitializeHeatZone() {}
void UIshowHeat() {}
void UIhideHeat() {}

void UIinitializeCoolZone() {}
void UIshowCool() {}
void UIhideCool() {}

void UItempInitialize() {}
void UItempSet(float value) {}
void UItempErrorCheck() {}

void UIgoalInitialize() {}
void UIgoalSet(float value) {}
void UIgoalSet(String msg) {}

void UIinitializeAutoBTNs() {}
void UIshowAutoBTNs() {}
void UIhideAutoBTNs() {}

void UIinitializeManualBTNs() {}
void UIshowManualBTNs() {}
void UIhideManualBTNs() {}
void UIsetManualBTNState(STATE state) {}

void UIinitializeOnButton() {}
void UIshowOnButton() {}
void UIhideOnButton() {}
