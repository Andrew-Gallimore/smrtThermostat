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

void UIapplyButtonStyle(lv_obj_t* btn, bool useLighterBg) {
    if (btn == nullptr) {
        return;
    }
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 3, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 5, LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(btn, 1, LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_50, LV_PART_MAIN);
    if (useLighterBg) {
        lv_obj_set_style_bg_color(btn, C_BTN_BG_Lighter, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, C_BTN_Highlight_Lighter, LV_STATE_PRESSED | LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(btn, C_BTN_BG, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, C_BTN_Highlight, LV_STATE_PRESSED | LV_PART_MAIN);
    }
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
}

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
void UIsetManualBTNState(STATE state) {
    if(manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }
    switch(state) {
        case STATE::Heat:
            lv_obj_set_style_bg_color(manualBTN1, C_Red, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN2, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN3, C_BTN_BG, LV_PART_MAIN);
            break;
        case STATE::Cool:
            lv_obj_set_style_bg_color(manualBTN1, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN2, C_Blue, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN3, C_BTN_BG, LV_PART_MAIN);
            break;
        case STATE::Fan:
            lv_obj_set_style_bg_color(manualBTN1, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN2, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN3, C_Teal, LV_PART_MAIN);
            break;
        default:
            lv_obj_set_style_bg_color(manualBTN1, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN2, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN3, C_BTN_BG, LV_PART_MAIN);
            break;
    }
}

void UIinitializeOnButton() {}
void UIshowOnButton() {}
void UIhideOnButton() {}
