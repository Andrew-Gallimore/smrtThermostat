#include "ui.h"

// Global variable definitions
lv_obj_t* delayIcon = nullptr;
lv_obj_t* delayMsg = nullptr;
lv_anim_t delayAnim;
lv_anim_t msgAnim;
bool delayVisible = false;

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
int TEMP_ERROR_TIME = 60000; // 1 minute

// Helper function for button styling
void UIapplyButtonStyle(lv_obj_t* btn) {
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 3, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 5, LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn, 0, LV_PART_MAIN); 
    lv_obj_set_style_shadow_spread(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(btn, 1, LV_STATE_PRESSED | LV_PART_MAIN); 
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_50, LV_PART_MAIN); 
    lv_obj_set_style_bg_color(btn, C_BTN_BG, LV_PART_MAIN); 
    lv_obj_set_style_bg_color(btn, C_BTN_Highlight, LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
}


void UIinitializeDelay() {
    // ==== Icon for delay ====
    delayIcon = lv_obj_create(lv_scr_act());
    lv_obj_set_size(delayIcon, 32, 48); // Corrected size to fit 12px circles and 4px spacing
    lv_obj_align(delayIcon, LV_ALIGN_TOP_LEFT, 30, 30); // Position the grid

    lv_obj_set_layout(delayIcon, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(delayIcon, LV_OPA_TRANSP, LV_PART_MAIN); // No background
    lv_obj_set_scrollbar_mode(delayIcon, LV_SCROLLBAR_MODE_OFF); // Not scrollable
    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_CLICKABLE); // Not interactable
    lv_obj_set_style_border_width(delayIcon, 0, LV_PART_MAIN); // No border
    lv_obj_set_style_pad_all(delayIcon, 0, LV_PART_MAIN);

    // Define static arrays for grid column and row descriptors
    static lv_coord_t column_dsc[] = {12, 4, 12, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {12, 4, 12, 4, 12, LV_GRID_TEMPLATE_LAST}; // Ensure proper spacing and alignment

    lv_obj_set_style_grid_column_dsc_array(delayIcon, column_dsc, LV_PART_MAIN);
    lv_obj_set_style_grid_row_dsc_array(delayIcon, row_dsc, LV_PART_MAIN);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t* circle = lv_obj_create(delayIcon);
        lv_obj_set_size(circle, 12, 12); // Circle size
        lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN); // Make it circular
        lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Set color
        lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN); // Fully opaque
        lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN); // No border
        lv_obj_set_style_pad_all(circle, 0, LV_PART_MAIN); // No padding or margins
        lv_obj_set_grid_cell(circle, LV_GRID_ALIGN_CENTER, i % 2, 1, LV_GRID_ALIGN_CENTER, i / 2, 1);
    }

    // Move delayIcon to foreground so it's above heat/cool zones
    lv_obj_move_foreground(delayIcon);

    // Animation for the delayIcon
    lv_anim_init(&delayAnim);
    lv_anim_set_var(&delayAnim, delayIcon);
    lv_anim_set_exec_cb(&delayAnim, [](void* obj, int32_t v) {
        lv_obj_t* parent = static_cast<lv_obj_t*>(obj);
        if(v == 0) {
            for(int i = 0; i < 6; i++) {
                lv_obj_t* circle = lv_obj_get_child(parent, i);
                if(i == 1 || i == 4) {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0x000000), LV_PART_MAIN);
                }else {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
            }
        }else if(v == 2) {
            for(int i = 0; i < 6; i++) {
                lv_obj_t* circle = lv_obj_get_child(parent, i);
                if(i == 0 || i == 1 || i == 3) {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0x000000), LV_PART_MAIN);
                }else {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
            }
        }else if(v == 4) {
            for(int i = 0; i < 6; i++) {
                lv_obj_t* circle = lv_obj_get_child(parent, i);
                if(i == 0 || i == 1 || i == 2 || i == 3 || i == 5) {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0x000000), LV_PART_MAIN);
                }else {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
            }
        }else if(v == 7) {
            for(int i = 0; i < 6; i++) {
                lv_obj_t* circle = lv_obj_get_child(parent, i);
                if(i == 0 || i == 2 || i == 3 || i == 4 || i == 5) {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0x000000), LV_PART_MAIN);
                }else {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
            }
        }else if(v == 10) {
            for(int i = 0; i < 6; i++) {
                lv_obj_t* circle = lv_obj_get_child(parent, i);
                if(i == 2 || i == 4 || i == 5) {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0x000000), LV_PART_MAIN);
                }else {
                    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                }
            }
        }
    });
    lv_anim_set_values(&delayAnim, 0, 12);
    lv_anim_set_time(&delayAnim, 1000); // 1 second
    lv_anim_set_path_cb(&delayAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&delayAnim, LV_ANIM_REPEAT_INFINITE);

    // ==== Message for delay ====
    delayMsg = lv_label_create(lv_scr_act());
    lv_label_set_text(delayMsg, "5 min...");
    lv_obj_align_to(delayMsg, delayIcon, LV_ALIGN_OUT_RIGHT_MID, 10, -6); // Align to the right of the icon
    lv_obj_set_style_text_font(delayMsg, &chivo_mono_34, LV_PART_MAIN); // Set font
    lv_obj_set_style_text_color(delayMsg, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Set text color to white

    // Move delayMsg to foreground as well
    lv_obj_move_foreground(delayMsg);

    // Animation to update the delay message every second
    lv_anim_init(&msgAnim);
    lv_anim_set_var(&msgAnim, delayMsg);
    lv_anim_set_exec_cb(&msgAnim, [](void* obj, int32_t v) {
        long int remaining = (getDelay(getLastHeavyState(), getCurrentState()) - timeSinceLastHeavyState()) / 1000;
        if(remaining < 0) remaining = 0;

        if(delayVisible && remaining == 0) {
            checkState();
        }
        
        if (remaining >= 60) {
            lv_label_set_text_fmt(static_cast<lv_obj_t*>(obj), "%ld min...", remaining / 60);
        } else {
            lv_label_set_text_fmt(static_cast<lv_obj_t*>(obj), "%ld sec...", remaining);
        }
    });
    lv_anim_set_values(&msgAnim, 0, 12);
    lv_anim_set_time(&msgAnim, 1000);
    lv_anim_set_path_cb(&msgAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&msgAnim, LV_ANIM_REPEAT_INFINITE);

    // ==== Hide both initially ====
    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}

void UIshowDelay() {
    if (delayIcon == nullptr || &delayAnim == nullptr || &msgAnim == nullptr) {
        return;
    }

    if(delayVisible) {
        return;
    }
    delayVisible = true;

    lv_anim_start(&delayAnim);
    lv_anim_start(&msgAnim);
    lv_obj_remove_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}

void UIhideDelay() {
    if(&delayAnim == nullptr || &msgAnim == nullptr || delayIcon == nullptr || delayMsg == nullptr) {
        return;
    }

    if(!delayVisible) {
        return;
    }
    delayVisible = false;

    lv_anim_del(&delayAnim, nullptr);
    lv_anim_del(&msgAnim, nullptr);
    lv_obj_add_flag(delayIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(delayMsg, LV_OBJ_FLAG_HIDDEN);
}



void UIinitializeHeatZone() {
    heatZone = lv_obj_create(lv_scr_act());
    lv_obj_set_size(heatZone, lv_obj_get_width(lv_scr_act()), 70); // Full width, height of 70
    lv_obj_align(heatZone, LV_ALIGN_TOP_MID, 0, -70); // Align to the top center

    // Set the style for the heatZone
    lv_obj_set_style_radius(heatZone, 0, LV_PART_MAIN); // No rounded corners
    lv_obj_set_style_border_opa(heatZone, 0, LV_PART_MAIN);

    static lv_style_t style;
    lv_style_init(&style);

    static lv_grad_dsc_t grad;
    static const lv_color_t grad_colors[] = {
        // lv_palette_main(LV_PALETTE_RED),    // Red
        // lv_palette_main(LV_PALETTE_ORANGE), // Orange
        // lv_palette_main(LV_PALETTE_YELLOW), // Yellow
        // fix_color(255,52,52),  // Green
        C_Red,  // Green
        C_Background,   // Blue
        // lv_palette_main(LV_PALETTE_INDIGO), // Indigo
        // lv_palette_main(LV_PALETTE_PURPLE)  // Purple
    };

    lv_gradient_init_stops(&grad, grad_colors, NULL, NULL, sizeof(grad_colors) / sizeof(lv_color_t));
    lv_grad_linear_init(&grad, 0, LV_GRAD_TOP, 0, LV_GRAD_BOTTOM, LV_GRAD_EXTEND_PAD);

    lv_style_set_bg_grad(&style, &grad);
    lv_obj_add_style(heatZone, &style, LV_PART_MAIN);
    // lv_obj_add_flag(heatZone, LV_OBJ_FLAG_HIDDEN); // Ensure the heatZone is visible
}

void UIshowHeat() {
    if(heatZone == nullptr) {
        return;
    }

    if(heatZoneVisible) {
        return;
    }
    heatZoneVisible = true;

    // Heat zone
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, heatZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, -70, 0); // Start off-screen and move to the target position
    lv_anim_set_time(&anim, 3000); // 3 seconds
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);

    // Goal text
    lv_obj_set_style_text_color(goalText, C_Red, LV_PART_MAIN);
}

void UIhideHeat() {
    if (heatZone == nullptr) {
        return;
    }

    if(!heatZoneVisible) {
        return;
    }
    heatZoneVisible = false;
    
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, heatZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 0, -70); // Start off-screen and move to the target position
    lv_anim_set_time(&anim, 1500); // 3 seconds
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_start(&anim);

    // Goal text
    lv_obj_set_style_text_color(goalText, C_GoalTemp, LV_PART_MAIN);
}




void UIinitializeCoolZone() {
    coolZone = lv_obj_create(lv_scr_act());
    lv_obj_set_size(coolZone, lv_obj_get_width(lv_scr_act()), 70); // Full width, height of 70
    lv_obj_align(coolZone, LV_ALIGN_BOTTOM_MID, 0, 70); // Align to the top center

    // Set the style for the coolZone
    lv_obj_set_style_radius(coolZone, 0, LV_PART_MAIN); // No rounded corners
    lv_obj_set_style_border_opa(coolZone, 0, LV_PART_MAIN);

    static lv_style_t style;
    lv_style_init(&style);

    static lv_grad_dsc_t grad;
    static const lv_color_t grad_colors[] = {
        // lv_palette_main(LV_PALETTE_RED),    // Red
        // lv_palette_main(LV_PALETTE_ORANGE), // Orange
        // lv_palette_main(LV_PALETTE_YELLOW), // Yellow
        // fix_color(255,52,52),  // Green
        C_Background,   // Blue
        C_Blue,  // Green
        // lv_palette_main(LV_PALETTE_INDIGO), // Indigo
        // lv_palette_main(LV_PALETTE_PURPLE)  // Purple
    };

    lv_gradient_init_stops(&grad, grad_colors, NULL, NULL, sizeof(grad_colors) / sizeof(lv_color_t));
    lv_grad_linear_init(&grad, 0, LV_GRAD_TOP, 0, LV_GRAD_BOTTOM, LV_GRAD_EXTEND_PAD);

    lv_style_set_bg_grad(&style, &grad);
    lv_obj_add_style(coolZone, &style, LV_PART_MAIN);
    // lv_obj_add_flag(coolZone, LV_OBJ_FLAG_HIDDEN); // Ensure the coolZone is visible
}

void UIshowCool() {
    if (coolZone == nullptr) {
        return;
    }

    if(coolZoneVisible) {
        return;
    }
    coolZoneVisible = true;

    // Heat zone
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, coolZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 70, 0); // Start off-screen and move to the target position
    lv_anim_set_time(&anim, 3000); // 3 seconds
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);

    // Goal text
    lv_obj_set_style_text_color(goalText, C_Blue, LV_PART_MAIN);
}

void UIhideCool() {
    if (coolZone == nullptr) {
        return;
    }

    if(!coolZoneVisible) {
        return;
    }
    coolZoneVisible = false;
    
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, coolZone);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
    });
    lv_anim_set_values(&anim, 0, 70); // Start off-screen and move to the target position
    lv_anim_set_time(&anim, 1500); // 3 seconds
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_start(&anim);

    // Goal text
    lv_obj_set_style_text_color(goalText, C_GoalTemp, LV_PART_MAIN);
}




void UItempInitialize() {
    // Initialize the temperature text
    tempText = lv_label_create(lv_scr_act());
    lv_label_set_text(tempText, "1234");
    lv_obj_align(tempText, LV_ALIGN_TOP_LEFT, 24, 115);
    lv_obj_set_style_text_color(tempText, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(tempText, &chivo_mono_158, LV_PART_MAIN);
    lv_obj_remove_flag(tempText, LV_OBJ_FLAG_HIDDEN);

    // Initialize the error text
    tempErrorText = lv_label_create(lv_scr_act());
    lv_label_set_text(tempErrorText, "Error 1233");
    lv_obj_align(tempErrorText, LV_ALIGN_TOP_LEFT, 170, 115); // Adjusted to leave space for the spinner
    lv_obj_set_style_text_color(tempErrorText, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(tempErrorText, 12, LV_PART_MAIN);
    lv_obj_set_style_text_font(tempErrorText, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_add_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);

    // Initialize the spinner
    tempSpinner = lv_spinner_create(lv_scr_act());
    lv_spinner_set_anim_params(tempSpinner, 2200, 200);
    lv_obj_set_size(tempSpinner, 112, 112);
    lv_obj_align(tempSpinner, LV_ALIGN_TOP_LEFT, 24, 115);
    lv_obj_set_style_arc_color(tempSpinner, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(tempSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(tempSpinner, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_width(tempSpinner, 15, LV_PART_INDICATOR);
    lv_obj_add_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);

    lastGoodTempTime = TEMP_ERROR_TIME * -1; // Forcing going into error as soon as it starts
    tempInitialized = true;
}

void UItempSet(float value) {
    if (!tempInitialized) {
        return;
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.0fF", round(value));
    lv_label_set_text(tempText, buffer);

    // Show the temperature text and hide the error text and spinner
    lv_obj_remove_flag(tempText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);

    if(getCurrentMode() == MODE::Auto) {
        // If in Auto mode, show the auto buttons
        UIshowAutoBTNs();
    }

    tempInErrorState = false;
    lastGoodTemp = value;
    lastGoodTempTime = millis();
}

void UItempErrorCheck() {
    if (!tempInitialized) {
        return;
    }

    if (millis() - lastGoodTempTime < TEMP_ERROR_TIME) {
        return;
    }

    tempInErrorState = true;

    // Show the error text and spinner, hide the temperature text
    lv_obj_add_flag(tempText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(tempErrorText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(tempSpinner, LV_OBJ_FLAG_HIDDEN);
    UIhideAutoBTNs();

    if (((millis() - lastGoodTempTime) / 2000) % 4 < 2) {
        lv_label_set_text(tempErrorText, "Waiting for\nthermometer\ndata");
    } else {
        char buffer[50];
        long minutesAgo = (millis() - lastGoodTempTime) / 1000 / 60;
        snprintf(buffer, sizeof(buffer), "Was %.0fF\n%ld minute%s ago", 
            round(lastGoodTemp), 
            minutesAgo, 
            minutesAgo == 1 ? "" : "s");
        lv_label_set_text(tempErrorText, buffer);
    }
}




void UIgoalInitialize() {
    // Initialize the goal temp
    goalText = lv_label_create(lv_scr_act());
    lv_label_set_text(goalText, "");
    lv_obj_align(goalText, LV_ALIGN_TOP_LEFT, 24, 270);
    lv_obj_set_style_text_color(goalText, C_GoalTemp, LV_PART_MAIN);
    lv_obj_set_style_text_font(goalText, &chivo_mono_110, LV_PART_MAIN);
    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);

    // Initialize the error text
    goalErrorText = lv_label_create(lv_scr_act());
    lv_label_set_text(goalErrorText, "");
    lv_obj_align(goalErrorText, LV_ALIGN_TOP_LEFT, 170, 270); // Adjusted to leave space for the spinner
    lv_obj_set_style_text_color(goalErrorText, C_GoalTemp, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(goalErrorText, 12, LV_PART_MAIN);
    lv_obj_set_style_text_font(goalErrorText, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);

    // Initialize the spinner
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

void UIgoalSet(float value) {
    if(goalText == nullptr) {
        return;
    }

    if(getCurrentMode() != MODE::Auto) {
        return; // Only set goal temp in Auto mode
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.0f", round(value));
    lv_label_set_text(goalText, buffer);

    // Show the goal temp text and hide the error text and spinner
    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(goalSpinner, LV_OBJ_FLAG_HIDDEN);

    // tempInErrorState = false;
    // lastGoodTemp = value;
    // lastGoodTempTime = millis();
}

void UIgoalSet(String msg) {
    if(goalText == nullptr) {
        return;
    }

    lv_label_set_text(goalText, msg.c_str());

    // Show the goal temp text and hide the error text and spinner
    lv_obj_remove_flag(goalText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(goalErrorText, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(goalSpinner, LV_OBJ_FLAG_HIDDEN);
}





void UIinitializeAutoBTNs() {
    autoBTN1 = lv_btn_create(lv_scr_act());
    lv_obj_align(autoBTN1, LV_ALIGN_TOP_RIGHT, -24, 115);
    lv_obj_set_size(autoBTN1, 106, 106);
    UIapplyButtonStyle(autoBTN1);
    
    // Add the chevron_65 icon to the button
    lv_obj_t* icon = lv_img_create(autoBTN1);
    lv_img_set_src(icon, &chevron_65);
    lv_obj_set_style_img_recolor(icon, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon);

    lv_obj_remove_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);

    // Set a callback for autoBTN1
    lv_obj_add_event_cb(autoBTN1, onTempUpButtonClick, LV_EVENT_ALL, nullptr);
    

    autoBTN2 = lv_btn_create(lv_scr_act());
    lv_obj_align(autoBTN2, LV_ALIGN_TOP_RIGHT, -24, 230);
    lv_obj_set_size(autoBTN2, 106, 106);
    UIapplyButtonStyle(autoBTN2);

    // Add the chevron_65 icon to the button
    lv_obj_t* icon2 = lv_img_create(autoBTN2);
    lv_img_set_src(icon2, &chevron_65);
    lv_img_set_angle(icon2, 1800); // Rotate the icon 180 degrees (angle in tenths of a degree)
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon2);

    lv_obj_remove_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);

    // Set a callback for autoBTN2
    lv_obj_add_event_cb(autoBTN2, onTempDownButtonClick, LV_EVENT_ALL, nullptr);
}

void UIshowAutoBTNs() {
    if(autoBTN1 == nullptr || autoBTN2 == nullptr) {
        return;
    }

    Serial.print("Showing auto buttons: ");
    Serial.println(tempInErrorState);
    if(tempInErrorState) {
        return;
    }
    
    lv_obj_remove_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);
}

void UIhideAutoBTNs() {
    if(autoBTN1 == nullptr || autoBTN2 == nullptr) {
        return;
    }
    
    lv_obj_add_flag(autoBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(autoBTN2, LV_OBJ_FLAG_HIDDEN);
}




void UIinitializeManualBTNs() {
    manualBTN1 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN1, 106, 106);
    lv_obj_align(manualBTN1, LV_ALIGN_TOP_LEFT, 24, 270);
    UIapplyButtonStyle(manualBTN1);
    lv_obj_t* icon1 = lv_img_create(manualBTN1);
    lv_img_set_src(icon1, &flame_65); // Replace with your icon variable
    lv_obj_set_style_img_recolor(icon1, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon1);

    lv_obj_add_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    lv_obj_add_event_cb(manualBTN1, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            // Handle manual button click
            Serial.println("Manual heat clicked");
            onManualHeatClick();
        }
    }, LV_EVENT_ALL, nullptr);

    manualBTN2 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN2, 106, 106);
    lv_obj_align(manualBTN2, LV_ALIGN_TOP_LEFT, 146, 270); // 24 + 106 + 16 spacing
    UIapplyButtonStyle(manualBTN2);
    lv_obj_t* icon2 = lv_img_create(manualBTN2);
    lv_img_set_src(icon2, &winter_65); // Replace with your icon variable
    lv_obj_set_style_img_recolor(icon2, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon2);

    lv_obj_add_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    lv_obj_add_event_cb(manualBTN2, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            // Handle manual button click
            Serial.println("Manual cool clicked");
            onManualCoolClick();
        }
    }, LV_EVENT_ALL, nullptr);

    manualBTN3 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(manualBTN3, 106, 106);
    lv_obj_align(manualBTN3, LV_ALIGN_TOP_LEFT, 268, 270); // 146 + 106 + 16 spacing
    UIapplyButtonStyle(manualBTN3);
    lv_obj_t* icon3 = lv_img_create(manualBTN3);
    lv_img_set_src(icon3, &fan_65); // Replace with your icon variable
    lv_obj_set_style_img_recolor(icon3, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon3, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(icon3);

    lv_obj_add_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    lv_obj_add_event_cb(manualBTN3, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            // Handle manual button click
            Serial.println("Manual fan clicked");
            onManualFanClick();
        }
    }, LV_EVENT_ALL, nullptr);
}

void UIshowManualBTNs() {
    if(manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }

    lv_obj_remove_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN);
}

void UIhideManualBTNs() {
    if(manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }

    lv_obj_add_flag(manualBTN1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(manualBTN3, LV_OBJ_FLAG_HIDDEN);
}

void UIsetManualBTNState(STATE state) {
    if(manualBTN1 == nullptr || manualBTN2 == nullptr || manualBTN3 == nullptr) {
        return;
    }

    switch(state) {
        case STATE::Heat:
        case STATE::AwaitingHeat:
            // Set manualBTN1 background to red, others to normal
            lv_obj_set_style_bg_color(manualBTN1, C_Red, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN2, C_BTN_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_color(manualBTN3, C_BTN_BG, LV_PART_MAIN);
            break;
        case STATE::Cool:
        case STATE::AwaitingCool:
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






void UIinitializeOnButton() {
    onButton = lv_switch_create(lv_scr_act());
    lv_obj_set_size(onButton, 140, 70);
    lv_obj_align(onButton, LV_ALIGN_TOP_LEFT, 270, 280);
    lv_obj_add_flag(onButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(onButton, C_GoalTemp, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(onButton, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_add_event_cb(onButton, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            onSwitchOnClick();

            lv_obj_set_style_bg_color(onButton, C_Orange, LV_PART_INDICATOR);
            lv_obj_set_style_bg_opa(onButton, LV_OPA_COVER, LV_PART_INDICATOR);

            lv_timer_t* timer = lv_timer_create_basic();
            lv_timer_set_period(timer, 1500);
            lv_timer_set_repeat_count(timer, 1);
            lv_timer_set_cb(timer, [](lv_timer_t* t) {
                if (onButton) {
                    lv_obj_clear_state(onButton, LV_STATE_CHECKED); // Switch OFF
                    lv_obj_set_style_bg_color(onButton, C_GoalTemp, LV_PART_INDICATOR);
                    lv_obj_set_style_bg_opa(onButton, LV_OPA_COVER, LV_PART_INDICATOR);
                }
                lv_timer_del(t);
            });
        }
    }, LV_EVENT_ALL, nullptr);
}

void UIshowOnButton() {
    if (onButton == nullptr) {
        return;
    }

    lv_obj_remove_flag(onButton, LV_OBJ_FLAG_HIDDEN);
}

void UIhideOnButton() {
    if (onButton == nullptr) {
        return;
    }

    lv_obj_add_flag(onButton, LV_OBJ_FLAG_HIDDEN);
}
