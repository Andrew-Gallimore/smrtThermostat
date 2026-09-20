#ifndef THERMOSTAT_VIEW_H
#define THERMOSTAT_VIEW_H

#include <lvgl.h>
#include "core/core-structs.h"
#include "core/ThermostatModel.h"
#include <Arduino.h>

class ThermostatView {
public:
    ThermostatView();
    ~ThermostatView();

    void bind(ThermostatModel* model);
    void initialize();
    void render();

private:
    ThermostatState ts_;
    ThermostatModel* model_ = nullptr;

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
    lv_anim_t heatZoneAnim;
    lv_timer_t* heatZoneFallbackTimer = nullptr;
    int32_t heatZoneFallbackTargetY = -70;

    lv_obj_t* coolZone = nullptr;
    bool coolZoneVisible = false;
    lv_anim_t coolZoneAnim;
    lv_timer_t* coolZoneFallbackTimer = nullptr;
    int32_t coolZoneFallbackTargetY = 70;

    lv_obj_t* autoBTN1 = nullptr;
    lv_obj_t* autoBTN2 = nullptr;

    lv_obj_t* manualBTN1 = nullptr;
    lv_obj_t* manualBTN2 = nullptr;
    lv_obj_t* manualBTN3 = nullptr;

    lv_obj_t* tempText = nullptr;
    lv_obj_t* tempErrorText = nullptr;
    lv_obj_t* tempSpinner = nullptr;
    lv_anim_t tempErrorMsgAnim;

    lv_obj_t* goalText = nullptr;
    lv_obj_t* goalErrorText = nullptr;
    lv_obj_t* goalSpinner = nullptr;

    lv_obj_t* onButton = nullptr;
    lv_timer_t* temperatureTimer = nullptr;

    bool tempInitialized = false;
    bool tempInErrorState = false;
    float lastGoodTemp = 0;
    long int lastGoodTempTime = 0;
    int TEMP_ERROR_TIME = 60000;

    void _initButtonStyle(lv_obj_t* btn, bool useLighterBg = false);
    void _initializeDelay();
    void _initializeLock();
    void _initializeTimer();
    void _initializeHeatZone();
    void _initializeCoolZone();
    void _initializeTemperature();
    void _initializeTemperatureRefreshTimer();
    void _initializeGoal();
    void _initializeAutoButtons();
    void _initializeManualButtons();
    void _initializeOnButton();

    void _renderMode();
    void _renderStateIndicators();
    void _renderTemperature();
    void _renderGoalTemperature();
    void _renderLockIndicator();
    void _renderDelayIndicator();

    static void _temperatureTimerCallback(lv_timer_t* timer);

    void _showDelay();
    void _hideDelay();
    void _showUnlock();
    void _hideUnlock();
    void _showTimer();
    void _hideTimer();
    void _showHeatZone();
    void _hideHeatZone();
    void _showCoolZone();
    void _hideCoolZone();
    void _cancelStateZoneAnimations();
    static void _heatZoneAnimationReadyCb(lv_anim_t* anim);
    static void _coolZoneAnimationReadyCb(lv_anim_t* anim);
    void _showAutoButtons();
    void _hideAutoButtons();
    void _showManualButtons();
    void _hideManualButtons();
    void _setManualButtonState(STATE state, GOAL_STATE goalState);
    void _showOnButton();
    void _hideOnButton();
    void _setTemperatureText(float temp);
    void _setGoalText();
    long int _getRemainingInteractionSeconds() const;
};

#endif // THERMOSTAT_VIEW_H