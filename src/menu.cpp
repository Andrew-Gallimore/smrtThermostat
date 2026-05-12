#include "menu.h"

lv_obj_t* menuShowButton = nullptr;

lv_obj_t* menuOverlay = nullptr;
lv_obj_t* menuPanel = nullptr;
lv_obj_t* menuCloseButton = nullptr;
lv_obj_t* menuButton1 = nullptr;
lv_obj_t* menuButton2 = nullptr;
lv_obj_t* menuButton3 = nullptr;
lv_obj_t* menuButton4 = nullptr;
lv_obj_t* menuButton5 = nullptr;
lv_obj_t* btn5_icon = nullptr;
bool menuVisible = false;

lv_obj_t* settingsPage = nullptr;
lv_obj_t* settingsContent = nullptr;
lv_obj_t* networkPage = nullptr;
lv_obj_t* networkContent = nullptr;
lv_obj_t* networkAvailableList = nullptr;
lv_anim_t networkAnim;
lv_obj_t* currentNetworkLabel = nullptr;
lv_obj_t* currentNetworkNameLabel = nullptr;
lv_obj_t* currentNetworkSignalIcon = nullptr;


void UIinitializeMenu() {
    menuShowButton = lv_btn_create(lv_scr_act());
    lv_obj_align(menuShowButton, LV_ALIGN_TOP_RIGHT, -24, 24);
    lv_obj_set_size(menuShowButton, 60, 60);
    lv_obj_set_style_bg_color(menuShowButton, C_Background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menuShowButton, 3, LV_PART_MAIN); // 0.01*255 ≈ 3
    lv_obj_set_style_border_width(menuShowButton, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(menuShowButton, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(menuShowButton, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(menuShowButton, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(menuShowButton, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(menuShowButton, LV_OPA_TRANSP, LV_PART_MAIN);
    // UIapplyButtonStyle(menuShowButton);

    // Create a container for the 2x2 dot grid icon
    lv_obj_t* menuDotGrid = lv_obj_create(menuShowButton);
    lv_obj_set_size(menuDotGrid, 32, 32);
    lv_obj_center(menuDotGrid);

    lv_obj_set_layout(menuDotGrid, LV_LAYOUT_GRID);
    lv_obj_set_style_bg_opa(menuDotGrid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(menuDotGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(menuDotGrid, LV_OBJ_FLAG_CLICKABLE); // Not interactable
    lv_obj_set_style_border_width(menuDotGrid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuDotGrid, 0, LV_PART_MAIN);

    // Define grid columns and rows for 2x2
    static lv_coord_t menu_col_dsc[] = {12, 7, 12, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t menu_row_dsc[] = {12, 7, 12, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_style_grid_column_dsc_array(menuDotGrid, menu_col_dsc, LV_PART_MAIN);
    lv_obj_set_style_grid_row_dsc_array(menuDotGrid, menu_row_dsc, LV_PART_MAIN);

    // Create 4 dots in a 2x2 grid
    for (int i = 0; i < 4; ++i) {
        if(i == 2) continue; // Making the upper corner shape

        lv_obj_t* dot = lv_obj_create(menuDotGrid);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot, C_Orange, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
        lv_obj_set_grid_cell(dot, LV_GRID_ALIGN_CENTER, i % 2, 1, LV_GRID_ALIGN_CENTER, i / 2, 1);
    }
    
    // Add callback to show menu
    lv_obj_add_event_cb(menuShowButton, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            UIshowMenu();
        }
    }, LV_EVENT_ALL, nullptr);





    // Create overlay (initially hidden)
    menuOverlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(menuOverlay, lv_obj_get_width(lv_scr_act()), lv_obj_get_height(lv_scr_act()));
    lv_obj_align(menuOverlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(menuOverlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menuOverlay, LV_OPA_80, LV_PART_MAIN); // Semi-transparent overlay
    lv_obj_set_style_margin_all(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuOverlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(menuOverlay);




    // Create menu panel
    menuPanel = lv_obj_create(menuOverlay);
    lv_obj_set_size(menuPanel, lv_obj_get_width(lv_scr_act()) - 48, lv_obj_get_height(lv_scr_act()) - 48);
    lv_obj_align(menuPanel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(menuPanel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuPanel, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(menuPanel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(menuPanel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(menuPanel, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(menuPanel, false, LV_PART_MAIN); // Make overflow visible

    
    // Create title
    lv_obj_t* titleLabel = lv_label_create(menuPanel);
    lv_label_set_text(titleLabel, "Menu");
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(titleLabel, &chivo_mono_34, LV_PART_MAIN);

    // Create close button (top right of panel)
    menuCloseButton = lv_btn_create(menuPanel);
    lv_obj_align(menuCloseButton, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(menuCloseButton, 60, 60);
    UIapplyButtonStyle(menuCloseButton);

    lv_obj_t* closeIcon = lv_img_create(menuCloseButton);
    lv_img_set_src(closeIcon, &exit_30);
    lv_obj_set_size(closeIcon, 30, 30);
    lv_obj_center(closeIcon);
    
    lv_obj_add_event_cb(menuCloseButton, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);

    // Calculate button size for quarter screen layout
    int panelWidth       = lv_obj_get_width(menuPanel);
    int panelHeight      = lv_obj_get_height(menuPanel);
    int buttonWidth      = 210;
    int buttonHeight     = 170;
    int buttonHALFHeight = 79;
    
    /*
        Layout:

        xxxxxx xxxxxx            -       -
        xOFFxx xAUTOx           btn1    btn2
        xxxxxx xxxxxx            -       -

        xSETxx xxxxxx           btn3     -
               xMANUx                   btn4
        xLOCKx xxxxxx           btn5     -
    */

    // ==== Creating 6 menu buttons ====
    menuButton1 = lv_btn_create(menuPanel);
    lv_obj_set_size(menuButton1, buttonWidth, buttonHeight);
    lv_obj_align(menuButton1, LV_ALIGN_TOP_LEFT, 0, 80);
    UIapplyButtonStyle(menuButton1);

    // Create English label for menuButton1
    lv_obj_t* label1_en = lv_label_create(menuButton1);
    lv_label_set_text(label1_en, "Off");
    lv_obj_set_style_text_color(label1_en, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(label1_en, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(label1_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label1_en, LV_ALIGN_CENTER, 0, -22);

    // Create Spanish label for menuButton1
    lv_obj_t* label1_es = lv_label_create(menuButton1);
    lv_label_set_text(label1_es, "Apagar");
    lv_obj_set_style_text_color(label1_es, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_set_style_text_font(label1_es, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(label1_es, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label1_es, LV_ALIGN_CENTER, 0, 22);



    menuButton2 = lv_btn_create(menuPanel);
    lv_obj_set_size(menuButton2, buttonWidth, buttonHeight);
    lv_obj_align(menuButton2, LV_ALIGN_TOP_RIGHT, 0, 80);
    UIapplyButtonStyle(menuButton2);

    // Create English label for menuButton2
    lv_obj_t* label2_en = lv_label_create(menuButton2);
    lv_label_set_text(label2_en, "Auto");
    lv_obj_set_style_text_color(label2_en, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(label2_en, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(label2_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(label2_en); // Center since it's only one line



    menuButton3 = lv_btn_create(menuPanel);
    lv_obj_set_size(menuButton3, buttonWidth, buttonHALFHeight);
    lv_obj_align(menuButton3, LV_ALIGN_BOTTOM_LEFT, 0, -92);
    UIapplyButtonStyle(menuButton3);
    
    // Create icon for menuButton5
    lv_obj_t* btn3_icon = lv_img_create(menuButton3);
    lv_img_set_src(btn3_icon, &settings_36);
    lv_obj_set_size(btn3_icon, 36, 36);
    lv_obj_center(btn3_icon);



    menuButton4 = lv_btn_create(menuPanel);
    lv_obj_set_size(menuButton4, buttonWidth, buttonHeight);
    lv_obj_align(menuButton4, LV_ALIGN_BOTTOM_RIGHT, 0, -1);
    UIapplyButtonStyle(menuButton4);
    
    // Create English label for menuButton4
    lv_obj_t* label4_en = lv_label_create(menuButton4);
    lv_label_set_text(label4_en, "Manual");
    lv_obj_set_style_text_color(label4_en, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(label4_en, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(label4_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(label4_en); // Center since it's only one word



    menuButton5 = lv_btn_create(menuPanel);
    lv_obj_set_size(menuButton5, buttonWidth, buttonHALFHeight);
    lv_obj_align(menuButton5, LV_ALIGN_BOTTOM_LEFT, 0, -1);
    UIapplyButtonStyle(menuButton5);
    
    // Create icon for menuButton5
    btn5_icon = lv_img_create(menuButton5);
    lv_img_set_src(btn5_icon, &unlock_36);
    lv_obj_set_size(btn5_icon, 36, 36);
    lv_obj_center(btn5_icon);


    // Add callbacks for menu buttons (you can customize these)
    lv_obj_add_event_cb(menuButton1, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            onOFFButtonClick();
            UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);

    lv_obj_add_event_cb(menuButton2, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            onAutoButtonClick();
            UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);

    lv_obj_add_event_cb(menuButton3, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            UIshowSettings();
            UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);

    lv_obj_add_event_cb(menuButton4, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            onManualButtonClick();
            UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);

    lv_obj_add_event_cb(menuButton5, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            // onLockButtonClick();

            bool passed = lockTest();

            if(passed) {
                UIshowUnlock();
                lv_img_set_src(btn5_icon, &unlock_36);
            }else {
                UIhideUnlock();
                lv_img_set_src(btn5_icon, &lock_36);
            }

            // if(isUnlocked()) {

            // }
            // UIhideMenu();
        }
    }, LV_EVENT_ALL, nullptr);
}

void UIhideMenuButton() {
    if (menuShowButton == nullptr) {
        return;
    }
    
    lv_obj_add_flag(menuShowButton, LV_OBJ_FLAG_HIDDEN);
}

void UIshowMenuButton() {
    if (menuShowButton == nullptr) {
        return;
    }
    
    lv_obj_remove_flag(menuShowButton, LV_OBJ_FLAG_HIDDEN);
}

void UIFadeInMenuButtons() {
    lv_obj_t* buttons[] = {menuButton1, menuButton2, menuButton3, menuButton4, menuButton5};
    int numButtons = 5;
    int delayStep = 80; // ms between each button

    for (int i = 0; i < numButtons; ++i) {
        lv_obj_set_style_opa(buttons[i], LV_OPA_TRANSP, LV_PART_MAIN); // Start transparent

        // Use a simple timer to pop in each button after a delay
        lv_timer_t* timer = lv_timer_create_basic();
        lv_timer_set_period(timer, (i * delayStep) + delayStep);
        lv_timer_set_repeat_count(timer, 1);
        lv_timer_set_cb(timer, [](lv_timer_t* t) {
            lv_obj_t* btn = static_cast<lv_obj_t*>(lv_timer_get_user_data(t));
            lv_obj_set_style_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
            lv_timer_del(t);
        });
        lv_timer_set_user_data(timer, buttons[i]);
    }
}

void UIshowMenu() {
    if (menuOverlay == nullptr || btn5_icon == nullptr || menuVisible) {
        return;
    }

    menuVisible = true;

    if(isUnlocked()) {
        lv_img_set_src(btn5_icon, &unlock_36);
    } else {
        lv_img_set_src(btn5_icon, &lock_36);
    }
    
    lv_obj_remove_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
    UIFadeInMenuButtons();
}

void UIhideMenu() {
    if (menuOverlay == nullptr || !menuVisible) {
        return;
    }

    menuVisible = false;
    
    lv_obj_add_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
}



void settingsPageCreator(lv_obj_t* page, const char* title, lv_obj_t* content) {
    lv_obj_set_size(page, 456, 456);
    lv_obj_align(page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(page, C_Background_Panel, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* titleLabel = lv_label_create(page);
    lv_label_set_text(titleLabel, title);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 27);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(titleLabel, &chivo_mono_34, LV_PART_MAIN);

    // Create close button (top right of panel)
    lv_obj_t* pageCloseButton = lv_btn_create(page);
    lv_obj_align(pageCloseButton, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_set_size(pageCloseButton, 60, 60);
    UIapplyButtonStyle(pageCloseButton);

    lv_obj_t* closeIcon = lv_img_create(pageCloseButton);
    lv_img_set_src(closeIcon, &exit_30);
    lv_obj_set_size(closeIcon, 30, 30);
    lv_obj_center(closeIcon);
    
    if(title == "Settings") {
        lv_obj_add_event_cb(pageCloseButton, [](lv_event_t* e) {
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_CLICKED) {
                UIhideSettings();
                UIshowMenu();
            }
        }, LV_EVENT_ALL, nullptr);
    }else {
        lv_obj_add_event_cb(pageCloseButton, [](lv_event_t* e) {
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_CLICKED) {
                UIhideNetwork();
                UIshowSettings();
            }
        }, LV_EVENT_ALL, nullptr);
    }

    // Using the provided content object
    if (content) {
        // lv_obj_set_parent(content, page);
        lv_obj_set_size(content, 456, 356);
        lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 100);
        lv_obj_set_style_bg_color(content, C_Background_Panel, LV_PART_MAIN);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    }
}




void UIinitializeSettings() {
    settingsPage = lv_obj_create(lv_scr_act());
    settingsContent = lv_obj_create(settingsPage);
    settingsPageCreator(settingsPage, "Settings", settingsContent);

    const int btnHeight    = 60;
    const int btnTopMargin = 12;   // top margin before first button
    const int btnSpacing   = 6;

    
    // Row 0: Thermometers
    lv_obj_t* thermometersBtn = lv_btn_create(settingsContent);
    lv_obj_set_size(thermometersBtn, 456, btnHeight);
    lv_obj_align(thermometersBtn, LV_ALIGN_TOP_LEFT, 0, btnTopMargin + ((btnSpacing + btnHeight) * 0));
    lv_obj_set_style_bg_opa(thermometersBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(thermometersBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(thermometersBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(thermometersBtn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t* thermometersLabel = lv_label_create(thermometersBtn);
    lv_label_set_text(thermometersLabel, "Thermometers");
    lv_obj_align(thermometersLabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_size(thermometersLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(thermometersLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(thermometersLabel, &chivo_mono_34, LV_PART_MAIN);

    lv_obj_t* thermometersIcon = lv_img_create(thermometersBtn);
    lv_img_set_src(thermometersIcon, &right_30);
    lv_obj_set_size(thermometersIcon, 30, 30);
    lv_obj_align(thermometersIcon, LV_ALIGN_RIGHT_MID, -12, 0);

    lv_obj_add_event_cb(thermometersBtn, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            UIhideSettings();
            UIshowNetwork();
        }
    }, LV_EVENT_ALL, NULL);



    // Row 1: NetworkBtn
    lv_obj_t* networkBtn = lv_btn_create(settingsContent);
    lv_obj_set_size(networkBtn, 456, btnHeight);
    lv_obj_align(networkBtn, LV_ALIGN_TOP_LEFT, 0, btnTopMargin + ((btnSpacing + btnHeight) * 1));
    lv_obj_set_style_bg_opa(networkBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(networkBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(networkBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(networkBtn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t* networkLabel = lv_label_create(networkBtn);
    lv_label_set_text(networkLabel, "Network");
    lv_obj_align(networkLabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_size(networkLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(networkLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(networkLabel, &chivo_mono_34, LV_PART_MAIN);

    lv_obj_t* networkIcon = lv_img_create(networkBtn);
    lv_img_set_src(networkIcon, &right_30);
    lv_obj_set_size(networkIcon, 30, 30);
    lv_obj_align(networkIcon, LV_ALIGN_RIGHT_MID, -12, 0);

    lv_obj_add_event_cb(networkBtn, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            UIhideSettings();
            UIshowNetwork();
        }
    }, LV_EVENT_ALL, NULL);



    // Row 3: Home Assistant
    lv_obj_t* HABtn = lv_btn_create(settingsContent);
    lv_obj_set_size(HABtn, 456, btnHeight);
    lv_obj_align(HABtn, LV_ALIGN_TOP_LEFT, 0, btnTopMargin + ((btnSpacing + btnHeight) * 2));
    lv_obj_set_style_bg_opa(HABtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(HABtn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(HABtn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(HABtn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t* HALabel = lv_label_create(HABtn);
    lv_label_set_text(HALabel, "Home Assistant");
    lv_obj_align(HALabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_size(HALabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(HALabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(HALabel, &chivo_mono_34, LV_PART_MAIN);

    lv_obj_t* HAIcon = lv_img_create(HABtn);
    lv_img_set_src(HAIcon, &right_30);
    lv_obj_set_size(HAIcon, 30, 30);
    lv_obj_align(HAIcon, LV_ALIGN_RIGHT_MID, -12, 0);


    // Row 4: Peer thermostats
    lv_obj_t* ThermostatsBtn = lv_btn_create(settingsContent);
    lv_obj_set_size(ThermostatsBtn, 456, btnHeight);
    lv_obj_align(ThermostatsBtn, LV_ALIGN_TOP_LEFT, 0, btnTopMargin + ((btnSpacing + btnHeight) * 3));
    lv_obj_set_style_bg_opa(ThermostatsBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ThermostatsBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(ThermostatsBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(ThermostatsBtn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t* ThermostatsLabel = lv_label_create(ThermostatsBtn);
    lv_label_set_text(ThermostatsLabel, "Peer Thermostats");
    lv_obj_align(ThermostatsLabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_size(ThermostatsLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(ThermostatsLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(ThermostatsLabel, &chivo_mono_34, LV_PART_MAIN);

    lv_obj_t* ThermostatsIcon = lv_img_create(ThermostatsBtn);
    lv_img_set_src(ThermostatsIcon, &right_30);
    lv_obj_set_size(ThermostatsIcon, 30, 30);
    lv_obj_align(ThermostatsIcon, LV_ALIGN_RIGHT_MID, -12, 0);


    // Reset button
    lv_obj_t *restartButton = lv_btn_create(settingsContent);
    lv_obj_set_size(restartButton, 100, 50);
    lv_obj_align(restartButton, LV_ALIGN_TOP_LEFT, 28, 6 + btnTopMargin + ((btnSpacing + btnHeight) * 4));
    UIapplyButtonStyle(restartButton);
    lv_obj_set_style_bg_color(restartButton, C_Red, LV_PART_MAIN);
    lv_obj_set_style_bg_color(restartButton, C_Red, LV_STATE_PRESSED | LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(restartButton);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    lv_obj_add_event_cb(restartButton, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            ESP.restart(); // Restart the ESP32
        }
    }, LV_EVENT_ALL, NULL);
}

void UIshowSettings() {
    if (settingsPage == nullptr) {
        return;
    }
    
    lv_obj_remove_flag(settingsPage, LV_OBJ_FLAG_HIDDEN);
}

void UIhideSettings() {
    if (settingsPage == nullptr) {
        return;
    }
    
    lv_obj_add_flag(settingsPage, LV_OBJ_FLAG_HIDDEN);
}



const int networkBtnHeight    = 50;
const int networkBtnTopMargin = 12;   // top margin before first button
const int networkBtnSpacing   = 0;

void UIinitializeNetwork() {
    networkPage = lv_obj_create(lv_scr_act());
    networkContent = lv_obj_create(networkPage);
    settingsPageCreator(networkPage, "Network", networkContent);

    currentNetworkLabel = lv_label_create(networkContent);
    lv_label_set_text(currentNetworkLabel, "Connected");
    lv_obj_align(currentNetworkLabel, LV_ALIGN_TOP_LEFT, 12, networkBtnTopMargin);
    lv_obj_set_size(currentNetworkLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(currentNetworkLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(currentNetworkLabel, &chivo_mono_34, LV_PART_MAIN);

    lv_obj_t* availableNetworkLabel = lv_label_create(networkContent);
    lv_label_set_text(availableNetworkLabel, "Available");
    lv_obj_align(availableNetworkLabel, LV_ALIGN_TOP_LEFT, 12, networkBtnTopMargin + ((networkBtnSpacing + networkBtnHeight) * 3));
    lv_obj_set_size(availableNetworkLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(availableNetworkLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(availableNetworkLabel, &chivo_mono_34, LV_PART_MAIN);


    lv_obj_t* currentNetworkBtn = lv_btn_create(networkContent);
    lv_obj_set_size(currentNetworkBtn, 456, networkBtnHeight);
    lv_obj_align(currentNetworkBtn, LV_ALIGN_TOP_LEFT, 0, networkBtnTopMargin + ((networkBtnSpacing + networkBtnHeight) * 1));
    lv_obj_set_style_bg_opa(currentNetworkBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(currentNetworkBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(currentNetworkBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(currentNetworkBtn, LV_OPA_TRANSP, LV_PART_MAIN);

    currentNetworkNameLabel = lv_label_create(currentNetworkBtn);
    lv_label_set_text(currentNetworkNameLabel, "UI Error");
    lv_obj_align(currentNetworkNameLabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_size(currentNetworkNameLabel, LV_SIZE_CONTENT, 32);
    lv_obj_set_style_text_color(currentNetworkNameLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(currentNetworkNameLabel, &chivo_mono_34, LV_PART_MAIN);

    currentNetworkSignalIcon = lv_img_create(currentNetworkBtn);
    lv_img_set_src(currentNetworkSignalIcon, &wifi_30_1);
    lv_obj_set_size(currentNetworkSignalIcon, 30, 30);
    lv_obj_align(currentNetworkSignalIcon, LV_ALIGN_RIGHT_MID, -12, 0);


    // List of available networks
    networkAvailableList = lv_obj_create(networkContent);
    lv_obj_set_size(networkAvailableList, 456, 356);
    lv_obj_align(networkAvailableList, LV_ALIGN_TOP_LEFT, 0, networkBtnTopMargin + ((networkBtnSpacing + networkBtnHeight) * 4));
    lv_obj_set_style_bg_color(networkAvailableList, C_Background_Panel, LV_PART_MAIN);
    lv_obj_set_style_border_width(networkAvailableList, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(networkAvailableList, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(networkAvailableList, LV_SCROLLBAR_MODE_OFF);


    // LVGL animation for refresing list of available networks
    lv_anim_init(&networkAnim);
    lv_anim_set_var(&networkAnim, networkAvailableList);
    lv_anim_set_exec_cb(&networkAnim, [](void* obj, int32_t v) {
        lv_obj_t* parent = static_cast<lv_obj_t*>(obj);

        WiFiNetwork networks[16];
        int numNetworks = 0;
        getAvailableNetworks(networks, numNetworks);

        // Remove all contents of parent first
        while(lv_obj_get_child_cnt(parent) > 0) {
            lv_obj_t* lastChild = lv_obj_get_child(parent, lv_obj_get_child_cnt(parent) - 1);
            lv_obj_del(lastChild);
        }

        // Adding loading message if no networks found
        if(numNetworks == 0) {
            lv_obj_t* loadingLabel = lv_label_create(parent);
            char label[14] = "Scanning...";

            lv_label_set_text(loadingLabel, label);
            lv_obj_align(loadingLabel, LV_ALIGN_TOP_MID, 0, 30);
            lv_obj_set_style_text_color(loadingLabel, lv_color_hex(0xA1A1A1), LV_PART_MAIN);
            lv_obj_set_style_text_font(loadingLabel, &chivo_mono_34, LV_PART_MAIN);
            return;
        }

        // For number of networks, display that list
        for(int i = 0; i < numNetworks; i++) {
            lv_obj_t* btn = lv_obj_get_child(parent, i);
            if(btn == nullptr) {
                // Create button
                btn = lv_btn_create(parent);
                lv_obj_set_size(btn, 456, networkBtnHeight);
                lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, networkBtnTopMargin + ((networkBtnSpacing + networkBtnHeight) * i));
                lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
                lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
                lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
                lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);

                lv_obj_t* label = lv_label_create(btn);
                lv_label_set_text(label, networks[i].ssid);
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
                lv_obj_set_size(label, 360, 24);
                lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
                lv_obj_set_style_text_color(label, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
                lv_obj_set_style_text_font(label, &chivo_mono_34, LV_PART_MAIN);

                lv_obj_t* icon = lv_img_create(btn);
                if(networks[i].rssi > -65) {
                    // Best
                    lv_img_set_src(icon, &wifi_30_3);
                }else if(networks[i].rssi > -75) {
                    // Medium
                    lv_img_set_src(icon, &wifi_30_2);
                }else {
                    // Low
                    lv_img_set_src(icon, &wifi_30_1);
                }
                lv_obj_set_size(icon, 30, 30);
                lv_obj_align(icon, LV_ALIGN_RIGHT_MID, -12, 0);


                // copy SSID into heap memory so it survives after this scope
                char* ssid_cpy = strdup(networks[i].ssid);
                if (ssid_cpy == nullptr) ssid_cpy = strdup("");

                // Add event to connect to network on click
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    lv_event_code_t code = lv_event_get_code(e);
                    char* ssid = static_cast<char*>(lv_event_get_user_data(e));
                    if (code == LV_EVENT_CLICKED) {
                        Serial.printf("New SSID: %s\n", ssid);
                        // char pwd[17] = "\0"; // For testing purposes only
                        // setWiFiCredentials(ssid, pwd);
                    } else if (code == LV_EVENT_DELETE) {
                        free(ssid);
                    }
                }, LV_EVENT_ALL, ssid_cpy);
            }
        }

    });
    lv_anim_set_values(&networkAnim, 0, 4);
    lv_anim_set_time(&networkAnim, 8000); // 1 second
    lv_anim_set_path_cb(&networkAnim, lv_anim_path_linear);
    lv_anim_set_repeat_count(&networkAnim, 1); // Repeat 8 times
}

void UIshowNetwork() {
    if (networkPage == nullptr) {
        return;
    }
    
    lv_obj_remove_flag(networkPage, LV_OBJ_FLAG_HIDDEN);

    // Get current network info and update UI
    scanAvailableNetworks();

    bool isConnected = WiFi.isConnected();
    char currentNetworkBuffer[33];
    getStoredNetworkSSID(currentNetworkBuffer, sizeof(currentNetworkBuffer));

    lv_label_set_text(currentNetworkNameLabel, currentNetworkBuffer);

    if(isConnected) {
        lv_label_set_text(currentNetworkLabel, "Connected");
    }else {
        // Current connected/connecting network
        lv_label_set_text(currentNetworkLabel, "Connecting to...");
    }


    // Start animation to refresh available networks
    lv_anim_start(&networkAnim);


    // Available networks
    // const int numThermostats = 5;
    // for (int i = 0; i < numThermostats; ++i) {
    //     lv_obj_t* ThermostatsBtn = lv_btn_create(networkAvailableList);
    //     lv_obj_set_size(ThermostatsBtn, 456, networkBtnHeight);
    //     lv_obj_align(ThermostatsBtn, LV_ALIGN_TOP_LEFT, 0, networkBtnTopMargin + ((networkBtnSpacing + networkBtnHeight) * i));
    //     lv_obj_set_style_bg_opa(ThermostatsBtn, LV_OPA_TRANSP, LV_PART_MAIN);
    //     lv_obj_set_style_border_width(ThermostatsBtn, 0, LV_PART_MAIN);
    //     lv_obj_set_style_outline_width(ThermostatsBtn, 0, LV_PART_MAIN);
    //     lv_obj_set_style_shadow_opa(ThermostatsBtn, LV_OPA_TRANSP, LV_PART_MAIN);

    //     lv_obj_t* ThermostatsLabel = lv_label_create(ThermostatsBtn);
    //     char labelText[32];
    //     snprintf(labelText, sizeof(labelText), "Peer Thermostat %d", i + 1);
    //     lv_label_set_text(ThermostatsLabel, labelText);
    //     lv_obj_align(ThermostatsLabel, LV_ALIGN_LEFT_MID, 12, 0);
    //     lv_obj_set_size(ThermostatsLabel, LV_SIZE_CONTENT, 32);
    //     lv_obj_set_style_text_color(ThermostatsLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    //     lv_obj_set_style_text_font(ThermostatsLabel, &chivo_mono_34, LV_PART_MAIN);

    //     lv_obj_t* ThermostatsIcon = lv_img_create(ThermostatsBtn);
    //     lv_img_set_src(ThermostatsIcon, &right_30);
    //     lv_obj_set_size(ThermostatsIcon, 30, 30);
    //     lv_obj_align(ThermostatsIcon, LV_ALIGN_RIGHT_MID, -12, 0);
    // }
}

void UIhideNetwork() {
    if (networkPage == nullptr) {
        return;
    }
    
    lv_obj_add_flag(networkPage, LV_OBJ_FLAG_HIDDEN);

    lv_anim_del(&networkAnim, nullptr);
}