#include "ThermostatView.h"
#include <Arduino.h>
#include <cmath>
#include "../colorHelper.h"
#include "../fonts/chivo_mono_158.h"
#include "../fonts/chivo_mono_110.h"
#include "../fonts/chivo_mono_34.h"
#include "../icons/chevron_65.h"
#include "../icons/fan_65.h"
#include "../icons/flame_65.h"
#include "../icons/winter_65.h"
#include "../icons/unlock_color_30.h"
#include "../icons/exit_30.h"

extern void onTempUpButtonClick(lv_event_t* e);
extern void onTempDownButtonClick(lv_event_t* e);
extern void onManualHeatClick();
extern void onManualCoolClick();
extern void onManualFanClick();
extern void onONButtonClick();

static void ThermostatView_onButtonTimerCallback(lv_timer_t* t) {
    onONButtonClick();

    lv_obj_t* target = static_cast<lv_obj_t*>(lv_timer_get_user_data(t));
    if (target != nullptr) {
        lv_obj_add_flag(target, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_state(target, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(target, C_GoalTemp, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_INDICATOR);
    }
    lv_timer_del(t);
}

static void ThermostatView_onButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (target == nullptr) {
        return;
    }

    lv_obj_clear_flag(target, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(target, C_Orange, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_timer_t* timer = lv_timer_create_basic();
    lv_timer_set_period(timer, 850);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_set_cb(timer, ThermostatView_onButtonTimerCallback);
    lv_timer_set_user_data(timer, target);
}


static const long int RESET_LIMIT_MS = 2 * 3600000;
static const long int LONG_STATE_DELAY = 480000;
static const long int REG_STATE_DELAY = 300000;

ThermostatView::ThermostatView() {
    tempInitialized = false;
    tempInErrorState = false;
    lastGoodTemp = 0;
    lastGoodTempTime = 0;
}

ThermostatView::~ThermostatView() {
}

void ThermostatView::bind(ThermostatModel* model) {
    model_ = model;
    if (model_ == nullptr) {
        return;
    }
    model_->subscribe([this](const ThermostatState& ts) {
        ts_ = ts;
        render();
    });
}

void ThermostatView::initialize() {
    _initializeDelay();
    _initializeLock();
    _initializeTimer();
    _initializeHeatZone();
    _initializeCoolZone();
    _initializeTemperature();
    _initializeGoal();
    _initializeAutoButtons();
    _initializeManualButtons();
    _initializeOnButton();
}

void ThermostatView::render() {
    Serial1.println("Rendering ThermostatView...");
    _renderMode();
    _renderStateIndicators();
    _renderTemperature();
    _renderGoalTemperature();
    _renderLockIndicator();
    _renderDelayIndicator();
}

void ThermostatView::_initButtonStyle(lv_obj_t* btn, bool useLighterBg) {
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

void ThermostatView::_initializeDelay() {
    Serial.println("Initializing delay indicator");
    delayIcon = lv_obj_create(lv_scr_act());
    lv_obj_set_size(delayIcon, 32, 48);
    lv_obj_align(delayIcon, LV_ALIGN_TOP_LEFT, 30, 30);
    lv_obj_set_layout(delayIcon, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(delayIcon, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(delayIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(delayIcon, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(delayIcon, 0, LV_PART_MAIN);

    static lv_coord_t column_dsc[] = {12, 4, 12, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {12, 4, 12, 4, 12, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_style_grid_column_dsc_array(delayIcon, column_dsc, LV_PART_MAIN);
    lv_obj_set_style_grid_row_dsc_array(delayIcon, row_dsc, LV_PART_MAIN);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t* circle = lv_obj_create(delayIcon);
        lv_obj_set_size(circle, 12, 12);
        lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(circle, 0, LV_PART_MAIN);
        lv_obj_set_grid_cell(circle, LV_GRID_ALIGN_CENTER, i % 2, 1, LV_GRID_ALIGN_CENTER, i / 2, 1);
    }

    lv_obj_move_foreground(delayIcon);

    lv_anim_init(&delayAnim);
    lv_anim_set_var(&delayAnim, delayIcon);
    lv_anim_set_exec_cb(&delayAnim, [](void* obj, int32_t v) {
        lv_obj_t* parent = static_cast<lv_obj_t*>(obj);
        if (parent == nullptr) {
            return;
        }

        int step;
        if (v <= 1) {
            step = 0;
        } else if (v <= 3) {
            step = 1;
        } else if (v <= 5) {
            step = 2;
        } else if (v <= 8) {
            step = 3;
        } else {
            step = 4;
        }

        for (int i = 0; i < 6; ++i) {
            lv_obj_t* circle = lv_obj_get_child(parent, i);
            if (circle == nullptr) {
                continue;
            }

            bool hidden = false;
            if (step == 0) {
                hidden = (i == 1 || i == 4);
            } else if (step == 1) {
                hidden = (i == 0 || i == 1 || i == 3);
            } else if (step == 2) {
                hidden = (i == 0 || i == 1 || i == 2 || i == 3 || i == 5);
            } else if (step == 3) {
                hidden = (i == 0 || i == 2 || i == 3 || i == 4 || i == 5);
            } else if (step == 4) {
                hidden = (i == 2 || i == 4 || i == 5);
            }

            lv_obj_set_style_bg_opa(circle, hidden ? LV_OPA_TRANSP : LV_OPA_COVER, LV_PART_MAIN);
        }
    });
    lv_anim_set_values(&delayAnim, 0, 12);
    lv_anim_set_time(&delayAnim, 1000);
    lv_anim_set_path_cb(&delayAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&delayAnim, LV_ANIM_REPEAT_INFINITE);

    delayMsg = lv_label_create(lv_scr_act());
    lv_label_set_text(delayMsg, "5 min...");
    lv_obj_align_to(delayMsg, delayIcon, LV_ALIGN_OUT_RIGHT_MID, 10, -6);
    lv_obj_set_style_text_font(delayMsg, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(delayMsg, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_move_foreground(delayMsg);
    lv_obj_set_user_data(delayMsg, this);

    lv_anim_init(&delayMsgAnim);
    lv_anim_set_var(&delayMsgAnim, delayMsg);
    lv_anim_set_exec_cb(&delayMsgAnim, [](void* obj, int32_t v) {
        lv_obj_t* label = static_cast<lv_obj_t*>(obj);
        if (label == nullptr) {
            return;
        }

        ThermostatView* view = static_cast<ThermostatView*>(lv_obj_get_user_data(label));
        if (view == nullptr || view->model_ == nullptr) {
            return;
        }

        long int remainingS = view->model_->getRemainingDelay() / 1000;
        if (remainingS < 0) {
            remainingS = 0;
        }

        if (remainingS >= 60) {
            lv_label_set_text_fmt(label, "%ld min...", remainingS / 60);
        } else {
            lv_label_set_text_fmt(label, "%ld sec...", remainingS);
        }

        if (view->delayVisible && v == 12 && remainingS == 0) {
            view->model_->update();
        }
    });
    lv_anim_set_values(&delayMsgAnim, 0, 12);
    lv_anim_set_time(&delayMsgAnim, 1000);
    lv_anim_set_path_cb(&delayMsgAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&delayMsgAnim, LV_ANIM_REPEAT_INFINITE);

    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_showDelay() {
    if (delayIcon == nullptr || delayMsg == nullptr || delayVisible) {
        return;
    }
    Serial.println("Showing delay indicator");
    delayVisible = true;
    lv_anim_start(&delayAnim);
    lv_anim_start(&delayMsgAnim);
    lv_obj_remove_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideDelay() {
    if (delayIcon == nullptr || delayMsg == nullptr || !delayVisible) {
        return;
    }
    Serial.println("Hiding delay indicator");
    delayVisible = false;
    lv_anim_del(&delayAnim, nullptr);
    lv_anim_del(&delayMsgAnim, nullptr);
    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_initializeLock() {
    unlockIcon = lv_img_create(lv_scr_act());
    lv_obj_align(unlockIcon, LV_ALIGN_TOP_RIGHT, -100, 36);
    lv_img_set_src(unlockIcon, &unlock_color_30);
    lv_obj_set_size(unlockIcon, 30, 30);
    lv_obj_add_flag(unlockIcon, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_showUnlock() {
    if (unlockIcon == nullptr) {
        return;
    }
    lv_obj_remove_flag(unlockIcon, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideUnlock() {
    if (unlockIcon == nullptr) {
        return;
    }
    lv_obj_add_flag(unlockIcon, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_initializeTimer() {
    timerIcon = lv_obj_create(lv_scr_act());
    lv_obj_set_size(timerIcon, 32, 32);
    lv_obj_align(timerIcon, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    lv_obj_set_layout(timerIcon, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(timerIcon, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(timerIcon, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(timerIcon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(timerIcon, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(timerIcon, 0, LV_PART_MAIN);
    static lv_coord_t column_dsc[] = {12, 4, 12, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {12, 4, 12, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_style_grid_column_dsc_array(timerIcon, column_dsc, LV_PART_MAIN);
    lv_obj_set_style_grid_row_dsc_array(timerIcon, row_dsc, LV_PART_MAIN);
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* circle = lv_obj_create(timerIcon);
        lv_obj_set_size(circle, 12, 12);
        lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(circle, C_MinorMessage, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(circle, 0, LV_PART_MAIN);
        lv_obj_set_grid_cell(circle, LV_GRID_ALIGN_CENTER, i % 2, 1, LV_GRID_ALIGN_CENTER, i / 2, 1);
    }
    lv_obj_move_foreground(timerIcon);
    lv_anim_init(&timerAnim);
    lv_anim_set_var(&timerAnim, timerIcon);
    lv_anim_set_exec_cb(&timerAnim, [](void* obj, int32_t v) {
        lv_obj_t* parent = static_cast<lv_obj_t*>(obj);
        if (parent == nullptr) {
            return;
        }
        if (v == 0) {
            lv_obj_t* circle = lv_obj_get_child(parent, 3);
            if (circle) lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_t* circle2 = lv_obj_get_child(parent, 2);
            if (circle2) lv_obj_set_style_bg_opa(circle2, LV_OPA_COVER, LV_PART_MAIN);
        } else if (v == 2) {
            lv_obj_t* circle = lv_obj_get_child(parent, 0);
            if (circle) lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_t* circle2 = lv_obj_get_child(parent, 1);
            if (circle2) lv_obj_set_style_bg_opa(circle2, LV_OPA_COVER, LV_PART_MAIN);
        } else if (v == 6) {
            lv_obj_t* circle = lv_obj_get_child(parent, 2);
            if (circle) lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_t* circle2 = lv_obj_get_child(parent, 3);
            if (circle2) lv_obj_set_style_bg_opa(circle2, LV_OPA_COVER, LV_PART_MAIN);
        } else if (v == 8) {
            lv_obj_t* circle = lv_obj_get_child(parent, 1);
            if (circle) lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_t* circle2 = lv_obj_get_child(parent, 0);
            if (circle2) lv_obj_set_style_bg_opa(circle2, LV_OPA_COVER, LV_PART_MAIN);
        }
    });
    lv_anim_set_values(&timerAnim, 0, 12);
    lv_anim_set_time(&timerAnim, 6000);
    lv_anim_set_path_cb(&timerAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&timerAnim, LV_ANIM_REPEAT_INFINITE);
    timerMsg = lv_label_create(lv_scr_act());
    lv_label_set_text(timerMsg, "Resets in x hours...");
    lv_obj_align_to(timerMsg, timerIcon, LV_ALIGN_OUT_RIGHT_MID, 10, -6);
    lv_obj_set_style_text_font(timerMsg, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(timerMsg, C_MinorMessage, LV_PART_MAIN);
    lv_obj_move_foreground(timerMsg);
    lv_obj_add_flag(timerIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(timerMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_showTimer() {
    if (timerIcon == nullptr || timerMsg == nullptr || timerVisible) {
        return;
    }
    timerVisible = true;
    lv_anim_start(&timerAnim);
    lv_obj_remove_flag(timerIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(timerMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideTimer() {
    if (timerIcon == nullptr || timerMsg == nullptr || !timerVisible) {
        return;
    }
    timerVisible = false;
    lv_anim_del(&timerAnim, nullptr);
    lv_obj_add_flag(timerIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(timerMsg, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_initializeHeatZone() {
    heatZone = lv_obj_create(lv_scr_act());
    lv_obj_set_size(heatZone, lv_obj_get_width(lv_scr_act()), 70);
    lv_obj_align(heatZone, LV_ALIGN_TOP_MID, 0, -70);
    lv_obj_set_style_radius(heatZone, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(heatZone, 0, LV_PART_MAIN);
    static lv_style_t style;
    lv_style_init(&style);
    static lv_grad_dsc_t grad;
    static const lv_color_t grad_colors[] = {C_Red, C_Background};
    lv_gradient_init_stops(&grad, grad_colors, NULL, NULL, sizeof(grad_colors) / sizeof(lv_color_t));
    lv_grad_linear_init(&grad, 0, LV_GRAD_TOP, 0, LV_GRAD_BOTTOM, LV_GRAD_EXTEND_PAD);
    lv_style_set_bg_grad(&style, &grad);
    lv_obj_add_style(heatZone, &style, LV_PART_MAIN);
}

void ThermostatView::_showHeatZone() {
    if (heatZone == nullptr || heatZoneVisible) {
        return;
    }
    heatZoneVisible = true;
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, heatZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, -70, 0);
    lv_anim_set_time(&anim, 3000);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

void ThermostatView::_hideHeatZone() {
    if (heatZone == nullptr || !heatZoneVisible) {
        return;
    }
    heatZoneVisible = false;
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, heatZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 0, -70);
    lv_anim_set_time(&anim, 1500);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_start(&anim);
}

void ThermostatView::_initializeCoolZone() {
    coolZone = lv_obj_create(lv_scr_act());
    lv_obj_set_size(coolZone, lv_obj_get_width(lv_scr_act()), 70);
    lv_obj_align(coolZone, LV_ALIGN_BOTTOM_MID, 0, 70);
    lv_obj_set_style_radius(coolZone, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(coolZone, 0, LV_PART_MAIN);
    static lv_style_t style;
    lv_style_init(&style);
    static lv_grad_dsc_t grad;
    static const lv_color_t grad_colors[] = {C_Background, C_Blue};
    lv_gradient_init_stops(&grad, grad_colors, NULL, NULL, sizeof(grad_colors) / sizeof(lv_color_t));
    lv_grad_linear_init(&grad, 0, LV_GRAD_TOP, 0, LV_GRAD_BOTTOM, LV_GRAD_EXTEND_PAD);
    lv_style_set_bg_grad(&style, &grad);
    lv_obj_add_style(coolZone, &style, LV_PART_MAIN);
}

void ThermostatView::_showCoolZone() {
    if (coolZone == nullptr || coolZoneVisible) {
        return;
    }
    coolZoneVisible = true;
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, coolZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 70, 0);
    lv_anim_set_time(&anim, 3000);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

void ThermostatView::_hideCoolZone() {
    if (coolZone == nullptr || !coolZoneVisible) {
        return;
    }
    coolZoneVisible = false;
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, coolZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 0, 70);
    lv_anim_set_time(&anim, 1500);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_start(&anim);
}

void ThermostatView::_initializeTemperature() {
    tempText = lv_label_create(lv_scr_act());
    lv_label_set_text(tempText, "");
    lv_obj_align(tempText, LV_ALIGN_TOP_LEFT, 24, 115);
    lv_obj_set_style_text_color(tempText, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(tempText, &chivo_mono_158, LV_PART_MAIN);
    lv_obj_remove_flag(tempText, LV_OBJ_FLAG_HIDDEN);
    tempErrorText = lv_label_create(lv_scr_act());
    lv_label_set_text(tempErrorText, "Error 1233");
    lv_obj_align(tempErrorText, LV_ALIGN_TOP_LEFT, 170, 115);
    lv_obj_set_style_text_color(tempErrorText, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(tempErrorText, 12, LV_PART_MAIN);
    lv_obj_set_style_text_font(tempErrorText, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_add_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);
    tempSpinner = lv_spinner_create(lv_scr_act());
    lv_spinner_set_anim_params(tempSpinner, 2200, 200);
    lv_obj_set_size(tempSpinner, 112, 112);
    lv_obj_align(tempSpinner, LV_ALIGN_TOP_LEFT, 24, 115);
    lv_obj_set_style_arc_color(tempSpinner, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(tempSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(tempSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(tempSpinner, 15, LV_PART_INDICATOR);
    lv_obj_add_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);
    tempInitialized = true;
    tempInErrorState = false;
    lastGoodTemp = ts_.temp;
    lastGoodTempTime = millis();
}

void ThermostatView::_setTemperatureText(float temp) {
    if (tempText == nullptr) {
        return;
    }
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.0fF", round(temp));
    lv_label_set_text(tempText, buffer);
    lv_obj_remove_flag(tempText, LV_OBJ_FLAG_HIDDEN);
    if (tempErrorText != nullptr) {
        lv_obj_add_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);
    }
    if (tempSpinner != nullptr) {
        lv_obj_add_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void ThermostatView::_setGoalText() {
    if (goalText == nullptr) {
        return;
    }

    if (ts_.mode == MODE::Off) {
        lv_label_set_text(goalText, "Off");
    } else if (ts_.mode == MODE::Auto) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.0f", round(ts_.goalTemp));
        lv_label_set_text(goalText, buffer);
    } else {
        lv_label_set_text(goalText, "");
    }

    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
    if (goalErrorText != nullptr) {
        lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);
    }
    if (goalSpinner != nullptr) {
        lv_obj_add_flag(goalSpinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void ThermostatView::_initializeGoal() {
    goalText = lv_label_create(lv_scr_act());
    lv_label_set_text(goalText, "");
    lv_obj_align(goalText, LV_ALIGN_TOP_LEFT, 24, 270);
    lv_obj_set_style_text_color(goalText, C_GoalTemp, LV_PART_MAIN);
    lv_obj_set_style_text_font(goalText, &chivo_mono_110, LV_PART_MAIN);
    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
    goalErrorText = lv_label_create(lv_scr_act());
    lv_label_set_text(goalErrorText, "");
    lv_obj_align(goalErrorText, LV_ALIGN_TOP_LEFT, 170, 270);
    lv_obj_set_style_text_color(goalErrorText, C_GoalTemp, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(goalErrorText, 12, LV_PART_MAIN);
    lv_obj_set_style_text_font(goalErrorText, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);
    goalSpinner = lv_spinner_create(lv_scr_act());
    lv_spinner_set_anim_params(goalSpinner, 2200, 200);
    lv_obj_set_size(goalSpinner, 112, 112);
    lv_obj_align(goalSpinner, LV_ALIGN_TOP_LEFT, 24, 270);
    lv_obj_set_style_arc_color(goalSpinner, C_GoalTemp, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(goalSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(goalSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(goalSpinner, 15, LV_PART_INDICATOR);
    lv_obj_add_flag(goalSpinner, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_initializeAutoButtons() {
    autoBTN1 = lv_btn_create(lv_scr_act());
    lv_obj_align(autoBTN1, LV_ALIGN_TOP_RIGHT, -24, 115);
    lv_obj_set_size(autoBTN1, 106, 106);
    _initButtonStyle(autoBTN1);
    lv_obj_t* icon = lv_img_create(autoBTN1);
    lv_img_set_src(icon, &chevron_65);
    lv_obj_set_style_img_recolor(icon, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon);
    lv_obj_add_event_cb(autoBTN1, onTempUpButtonClick, LV_EVENT_ALL, nullptr);
    autoBTN2 = lv_btn_create(lv_scr_act());
    lv_obj_align(autoBTN2, LV_ALIGN_TOP_RIGHT, -24, 230);
    lv_obj_set_size(autoBTN2, 106, 106);
    _initButtonStyle(autoBTN2);
    lv_obj_t* icon2 = lv_img_create(autoBTN2);
    lv_img_set_src(icon2, &chevron_65);
    lv_img_set_angle(icon2, 1800);
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon2);
    lv_obj_add_event_cb(autoBTN2, onTempDownButtonClick, LV_EVENT_ALL, nullptr);
    lv_obj_add_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_showAutoButtons() {
    if (autoBTN1 == nullptr || autoBTN2 == nullptr) {
        return;
    }
    if (tempInErrorState) {
        return;
    }
    lv_obj_remove_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideAutoButtons() {
    if (autoBTN1 == nullptr || autoBTN2 == nullptr) {
        return;
    }
    lv_obj_add_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_initializeManualButtons() {
    manualBTN1 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN1, 106, 106);
    lv_obj_align(manualBTN1, LV_ALIGN_TOP_LEFT, 24, 270);
    _initButtonStyle(manualBTN1);
    lv_obj_t* icon1 = lv_img_create(manualBTN1);
    lv_img_set_src(icon1, &flame_65);
    lv_obj_set_style_img_recolor(icon1, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon1);
    lv_obj_add_event_cb(manualBTN1, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            onManualHeatClick();
        }
    }, LV_EVENT_ALL, nullptr);
    manualBTN2 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN2, 106, 106);
    lv_obj_align(manualBTN2, LV_ALIGN_TOP_LEFT, 146, 270);
    _initButtonStyle(manualBTN2);
    lv_obj_t* icon2 = lv_img_create(manualBTN2);
    lv_img_set_src(icon2, &winter_65);
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon2);
    lv_obj_add_event_cb(manualBTN2, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            onManualCoolClick();
        }
    }, LV_EVENT_ALL, nullptr);
    manualBTN3 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN3, 106, 106);
    lv_obj_align(manualBTN3, LV_ALIGN_TOP_LEFT, 268, 270);
    _initButtonStyle(manualBTN3);
    lv_obj_t* icon3 = lv_img_create(manualBTN3);
    lv_img_set_src(icon3, &fan_65);
    lv_obj_set_style_img_recolor(icon3, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon3, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon3);
    lv_obj_add_event_cb(manualBTN3, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            onManualFanClick();
        }
    }, LV_EVENT_ALL, nullptr);
    lv_obj_add_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_showManualButtons() {
    if (manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }
    lv_obj_remove_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideManualButtons() {
    if (manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }
    lv_obj_add_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_setManualButtonState(STATE state, GOAL_STATE goalState) {
    if (manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }
    bool heatActive = state == STATE::Heat || goalState == GOAL_STATE::AwaitingHeat;
    bool coolActive = state == STATE::Cool || goalState == GOAL_STATE::AwaitingCool;
    bool fanActive = state == STATE::Fan;

    if (heatActive) {
        lv_obj_set_style_bg_color(manualBTN1, C_Red, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(manualBTN1, C_BTN_BG, LV_PART_MAIN);
    }

    if (coolActive) {
        lv_obj_set_style_bg_color(manualBTN2, C_Blue, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(manualBTN2, C_BTN_BG, LV_PART_MAIN);
    }

    if (fanActive) {
        lv_obj_set_style_bg_color(manualBTN3, C_Teal, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(manualBTN3, C_BTN_BG, LV_PART_MAIN);
    }
}

void ThermostatView::_initializeOnButton() {
    onButton = lv_switch_create(lv_scr_act());
    lv_obj_set_size(onButton, 140, 70);
    lv_obj_align(onButton, LV_ALIGN_TOP_LEFT, 270, 280);
    lv_obj_add_flag(onButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(onButton, C_GoalTemp, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(onButton, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_add_event_cb(onButton, ThermostatView_onButtonEvent, LV_EVENT_ALL, nullptr);
}

void ThermostatView::_showOnButton() {
    if (onButton == nullptr) {
        return;
    }
    lv_obj_remove_flag(onButton, LV_OBJ_FLAG_HIDDEN);
}

void ThermostatView::_hideOnButton() {
    if (onButton == nullptr) {
        return;
    }
    lv_obj_add_flag(onButton, LV_OBJ_FLAG_HIDDEN);
}

long int ThermostatView::_getRemainingDelaySeconds() const {
    STATE fromState = ts_.lastHeavyState;
    STATE toState = ts_.state;
    long int delay = 0;
    if ((fromState == STATE::Heat && toState == STATE::Cool) ||
        (fromState == STATE::Cool && toState == STATE::Heat)) {
        delay = LONG_STATE_DELAY;
    } else if ((fromState == STATE::Heat && toState == STATE::Heat) ||
               (fromState == STATE::Cool && toState == STATE::Cool)) {
        delay = REG_STATE_DELAY;
    }
    if (ts_.lastHeavyTime == 0) {
        return 0;
    }
    long int elapsed = millis() - ts_.lastHeavyTime;
    long int remaining = delay - elapsed;
    if (remaining < 0) {
        remaining = 0;
    }
    return remaining / 1000;
}

long int ThermostatView::_getRemainingInteractionSeconds() const {
    if (ts_.lastInteractionTime == 0) {
        return 0;
    }
    long int elapsed = millis() - ts_.lastInteractionTime;
    long int remaining = RESET_LIMIT_MS - elapsed;
    if (remaining < 0) {
        remaining = 0;
    }
    return remaining / 1000;
}

void ThermostatView::_renderMode() {
    // Serial.println("Rendering mode: " + String(static_cast<int>(ts_.mode)));
    if (ts_.mode == MODE::Off) {
        _hideAutoButtons();
        _hideManualButtons();
        _hideTimer();
        _hideDelay();
        _hideUnlock();
        _showOnButton();
        if (goalText != nullptr) {
            lv_label_set_text(goalText, "Off");
            lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    _hideOnButton();
    if (ts_.mode == MODE::Auto) {
        _showAutoButtons();
        _hideManualButtons();
    } else {
        _showManualButtons();
        _hideAutoButtons();
    }
}

void ThermostatView::_renderStateIndicators() {
    bool showHeat = ts_.state == STATE::Heat;
    bool showCool = ts_.state == STATE::Cool;
    if (showHeat) {
        _showHeatZone();
    } else {
        _hideHeatZone();
    }
    if (showCool) {
        _showCoolZone();
    } else {
        _hideCoolZone();
    }
    _setManualButtonState(ts_.state, ts_.goalState);
}

void ThermostatView::_renderTemperature() {
    if (!tempInitialized) {
        return;
    }
    if (ts_.temp != lastGoodTemp) {
        lastGoodTemp = ts_.temp;
        lastGoodTempTime = millis();
        tempInErrorState = false;
        _setTemperatureText(ts_.temp);
        return;
    }
    long int elapsed = millis() - lastGoodTempTime;
    if (elapsed >= TEMP_ERROR_TIME) {
        tempInErrorState = true;
        if (tempText != nullptr) {
            lv_obj_add_flag(tempText, LV_OBJ_FLAG_HIDDEN);
        }
        if (tempErrorText != nullptr) {
            lv_obj_remove_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);
            if (((elapsed) / 2000) % 4 < 2) {
                lv_label_set_text(tempErrorText, "Waiting for\nthermometer\ndata");
            } else {
                char buffer[50];
                long minutesAgo = elapsed / 1000 / 60;
                snprintf(buffer, sizeof(buffer), "Was %.0fF\n%ld minute%s ago",
                         round(lastGoodTemp), minutesAgo, minutesAgo == 1 ? "" : "s");
                lv_label_set_text(tempErrorText, buffer);
            }
        }
        if (tempSpinner != nullptr) {
            lv_obj_remove_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);
        }
        _hideAutoButtons();
    } else {
        tempInErrorState = false;
        _setTemperatureText(lastGoodTemp);
    }
}

void ThermostatView::_renderGoalTemperature() {
    if (goalText == nullptr) {
        return;
    }
    lv_color_t goalColor = C_GoalTemp;
    if (ts_.goalState == GOAL_STATE::AwaitingHeat) {
        goalColor = C_Red;
    } else if (ts_.goalState == GOAL_STATE::AwaitingCool) {
        goalColor = C_Blue;
    }
    if (ts_.mode == MODE::Off) {
        lv_label_set_text(goalText, "Off");
    } else if (ts_.mode == MODE::Auto) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.0f", round(ts_.goalTemp));
        lv_label_set_text(goalText, buffer);
    } else {
        lv_label_set_text(goalText, "");
    }
    lv_obj_set_style_text_color(goalText, goalColor, LV_PART_MAIN);
    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
    if (goalErrorText != nullptr) {
        lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);
    }
    if (goalSpinner != nullptr) {
        lv_obj_add_flag(goalSpinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void ThermostatView::_renderLockIndicator() {
    if (ts_.unlocked) {
        _showUnlock();
        _hideTimer();
        return;
    }
    _hideUnlock();
    if (ts_.state != STATE::Idle) {
        _showTimer();
        if (timerMsg != nullptr) {
            long int remaining = _getRemainingInteractionSeconds();
            if (remaining / 3600 >= 1) {
                long int hours = (remaining + 1800) / 3600;
                lv_label_set_text_fmt(timerMsg, "Hold ends in %ldhr%s", hours, hours == 1 ? "" : "s");
            } else if (remaining / 60 >= 5) {
                long int minutes = ((remaining + 150) / 300) * 5;
                if (minutes == 0) {
                    minutes = 5;
                }
                lv_label_set_text_fmt(timerMsg, "Hold ends in %ldmin", minutes);
            } else {
                long int minutes = remaining / 60 + 1;
                if (minutes < 1) {
                    minutes = 1;
                }
                lv_label_set_text_fmt(timerMsg, "Hold ends in %ldmin", minutes);
            }
        }
    } else {
        _hideTimer();
    }
}

void ThermostatView::_renderDelayIndicator() {
    Serial.println("Called _renderDelayIndicator with state: " + String(static_cast<int>(ts_.state)));
    bool showDelay = ts_.goalState != None;
    if (showDelay) {
        Serial.println("Rendering delay indicator: " + String(static_cast<int>(ts_.state)));
        _showDelay();
        if (delayMsg != nullptr && model_ != nullptr) {
            long int remaining = model_->getRemainingDelay() / 1000;
            if (remaining >= 60) {
                lv_label_set_text_fmt(delayMsg, "%ld min...", remaining / 60);
            } else {
                lv_label_set_text_fmt(delayMsg, "%ld sec...", remaining);
            }
        }
    } else {
        Serial.println("Hiding delay indicator: " + String(static_cast<int>(ts_.state)));
        _hideDelay();
    }
}
