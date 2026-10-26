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
lv_obj_t* menu_settings_icon = nullptr;
lv_obj_t* btn5_icon = nullptr;
bool menuVisible = false;

lv_obj_t* settingsPage = nullptr;
lv_obj_t* settingsContent = nullptr;

lv_obj_t* networkPage = nullptr;
lv_obj_t* networkContent = nullptr;
lv_obj_t* currentNetworkLabel = nullptr;
lv_obj_t* networkSSIDInput = nullptr;
lv_obj_t* networkPWDInput = nullptr;
lv_obj_t* networkConnectBtn = nullptr;
lv_obj_t* networkKeyboard = nullptr;
lv_timer_t* networkStatusTimer = nullptr;

lv_obj_t* thermometersPage = nullptr;
lv_obj_t* thermometersContent = nullptr;
lv_obj_t* thermometersLabel = nullptr;
lv_obj_t* thermometersList = nullptr;
lv_obj_t* thermometersAddBtn = nullptr;

lv_obj_t* thermometersAddInput = nullptr;


void UIapplyButtonStyle(lv_obj_t* btn, bool useLighterBg = false) {
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


struct TextPopupContext {
    lv_obj_t* popup;
    lv_obj_t* textarea;
    std::function<void(const char*)> callback;
};

static TextPopupContext* g_popupContext = nullptr;
static void textPopupSubmitCallback(lv_event_t* e) {
    const char* text = lv_textarea_get_text(g_popupContext->textarea);
    if (!text) {
        Serial.println("ERROR: text is null");
        return;
    }
    
    Serial.printf("Text from textarea: %s\n", text);
    
    // Copy the text to a static buffer
    static char textBuffer[256] = {0};
    memset(textBuffer, 0, sizeof(textBuffer));
    strncpy(textBuffer, text, sizeof(textBuffer) - 1);
    
    auto savedCallback = g_popupContext->callback;
    lv_obj_del(g_popupContext->popup);
    delete g_popupContext;
    g_popupContext = nullptr;

    // Calling the callback
    savedCallback(textBuffer);
}

void createTextPopup(const char* label, const char* placeholder, std::function<void(const char*)> onSubmit) {
    lv_obj_t* popupBox = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popupBox, 480, 480);
    lv_obj_align(popupBox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(popupBox, C_Background, LV_PART_MAIN);
    lv_obj_set_style_border_width(popupBox, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(popupBox, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(popupBox, LV_SCROLLBAR_MODE_OFF);

    // Create close button (top right of panel)
    lv_obj_t* closeButton = lv_btn_create(popupBox);
    lv_obj_align(closeButton, LV_ALIGN_TOP_RIGHT, -24, 24);
    lv_obj_set_size(closeButton, 60, 60);
    UIapplyButtonStyle(closeButton, true);

    lv_obj_t* closeIcon = lv_img_create(closeButton);
    lv_img_set_src(closeIcon, &exit_30);
    lv_obj_set_size(closeIcon, 30, 30);
    lv_obj_center(closeIcon);

    lv_obj_add_event_cb(closeButton, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            lv_obj_del(g_popupContext->popup);
            delete g_popupContext;
            g_popupContext = nullptr;
        }
    }, LV_EVENT_ALL, nullptr);



    lv_obj_t* textLabel = lv_label_create(popupBox);
    lv_label_set_text(textLabel, label);
    lv_obj_set_style_text_color(textLabel, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(textLabel, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_align(textLabel, LV_ALIGN_TOP_LEFT, 40, 100);

    lv_obj_t* textArea = lv_textarea_create(popupBox);
    lv_obj_set_size(textArea, 400, 56);
    lv_obj_align(textArea, LV_ALIGN_TOP_LEFT, 40, 140);
    lv_textarea_set_placeholder_text(textArea, placeholder);
    lv_textarea_set_one_line(textArea, true);
    lv_obj_set_style_text_font(textArea, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(textArea, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(textArea, C_BTN_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(textArea, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(textArea, 12, LV_PART_MAIN);



    lv_obj_t* keyboard = lv_keyboard_create(popupBox);
    lv_keyboard_set_textarea(keyboard, textArea);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    g_popupContext = new TextPopupContext{popupBox, textArea, onSubmit};
    lv_obj_add_event_cb(keyboard, textPopupSubmitCallback, LV_EVENT_READY, nullptr);
}

struct CodePopupContext {
    lv_obj_t* popup;
    lv_obj_t* textarea;
    std::function<void(int, int, int, int)> callback;
};

static CodePopupContext* c_popupContext = nullptr;
static void codePopupSubmitCallback(lv_event_t* e) {
    Serial.println("Code popup submit callback triggered");
    const char* text = lv_textarea_get_text(c_popupContext->textarea);
    if (!text) {
        Serial.println("ERROR: text is null");
        return;
    }
    
    Serial.printf("Text from textarea: %s\n", text);
    
    // Copy the text to a static buffer
    static char textBuffer[256] = {0};
    memset(textBuffer, 0, sizeof(textBuffer));
    strncpy(textBuffer, text, sizeof(textBuffer) - 1);

    // Turning text into 4 integers
    int val1 = 0, val2 = 0, val3 = 0, val4 = 0;
    sscanf(textBuffer, "%1d%1d%1d%1d", &val1, &val2, &val3, &val4);
    Serial.printf("Parsed values: %d %d %d %d\n", val1, val2, val3, val4);
    
    auto savedCallback = c_popupContext->callback;
    lv_obj_del(c_popupContext->popup);
    delete c_popupContext;
    c_popupContext = nullptr;

    // Calling the callback
    savedCallback(val1, val2, val3, val4);
}

void createCodePopup(std::function<void(int, int, int, int)> onSubmit) {
    // Similar to createTextPopup but with 4 text areas for code input
    // and a submit button that calls onSubmit with the 4 code values

    lv_obj_t* popupBox = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popupBox, 480, 480);
    lv_obj_align(popupBox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(popupBox, C_Background, LV_PART_MAIN);
    lv_obj_set_style_border_width(popupBox, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(popupBox, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(popupBox, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* titleLabel = lv_label_create(popupBox);
    lv_label_set_text(titleLabel, "Unlock");
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 27);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(titleLabel, &chivo_mono_34, LV_PART_MAIN);

    // Create close button (top right of panel)
    lv_obj_t* closeButton = lv_btn_create(popupBox);
    lv_obj_align(closeButton, LV_ALIGN_TOP_RIGHT, -24, 24);
    lv_obj_set_size(closeButton, 60, 60);
    UIapplyButtonStyle(closeButton, true);

    lv_obj_t* closeIcon = lv_img_create(closeButton);
    lv_img_set_src(closeIcon, &exit_30);
    lv_obj_set_size(closeIcon, 30, 30);
    lv_obj_center(closeIcon);

    lv_obj_add_event_cb(closeButton, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
            lv_obj_del(c_popupContext->popup);
            delete c_popupContext;
            c_popupContext = nullptr;
        }
    }, LV_EVENT_ALL, nullptr);



    lv_obj_t* textArea = lv_textarea_create(popupBox);
    lv_obj_set_size(textArea, 400, 56);
    lv_obj_align(textArea, LV_ALIGN_TOP_LEFT, 40, 140);
    lv_textarea_set_one_line(textArea, true);
    lv_textarea_set_max_length(textArea, 4);
    lv_obj_set_style_text_font(textArea, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(textArea, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(textArea, C_BTN_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(textArea, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(textArea, 12, LV_PART_MAIN);


    static const char * kb_map[] = {
        "1", "2", "3", "0", "\n",
        "4", "5", "6", LV_SYMBOL_BACKSPACE, "\n",
        "7", "8", "9", LV_SYMBOL_OK, NULL
    };

    /* Set relative button widths (all buttons same width) */
    static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {
        LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG,           /* Row 1: 1 2 3 4 */
        LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG,           /* Row 2: 5 6 7 8 */
        LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG, LV_BUTTONMATRIX_CTRL_CLICK_TRIG            /* Row 3: 9 0 [x] [✓] */
    };

    lv_obj_t* keyboard = lv_keyboard_create(popupBox);
    lv_keyboard_set_textarea(keyboard, textArea);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_obj_set_style_bg_color(keyboard, C_Background, LV_PART_MAIN);
    lv_obj_set_style_bg_color(keyboard, C_BTN_BG, LV_PART_ITEMS);
    // lv_obj_set_style_text_font(keyboard, &chivo_mono_34, LV_PART_MAIN);
    
    chivo_mono_34_fb = chivo_mono_34;   // copy
    chivo_mono_34_fb.fallback = LV_FONT_DEFAULT;
    lv_obj_set_style_text_font(
        keyboard,
        &chivo_mono_34_fb,
        LV_PART_ITEMS
    );
    lv_obj_set_style_text_color(keyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    c_popupContext = new CodePopupContext{popupBox, textArea, onSubmit};
    lv_obj_add_event_cb(keyboard, codePopupSubmitCallback, LV_EVENT_READY, nullptr);
}



void UIinitializeMenu() {
    menuShowButton = lv_btn_create(lv_scr_act());
    lv_obj_align(menuShowButton, LV_ALIGN_TOP_RIGHT, -24, 24);
    lv_obj_set_size(menuShowButton, 60, 60);
    lv_obj_set_style_bg_color(menuShowButton, C_Background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menuShowButton, 3, LV_PART_MAIN); // 0.01*255 ≈ 3
    lv_obj_set_style_border_width(menuShowButton, 0, LV_PART_MAIN);
    lv_obj_add_flag(menuShowButton, LV_OBJ_FLAG_HIDDEN);
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
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
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
    menu_settings_icon = lv_img_create(menuButton3);
    lv_img_set_src(menu_settings_icon, &settings_36);
    lv_obj_set_size(menu_settings_icon, 36, 36);
    lv_obj_center(menu_settings_icon);



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
            if(model->isUnlocked()) {
                UIshowSettings();
                UIhideMenu();
            }            
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

            if(model->isUnlocked()) {
                // Locking the system
                model->lock();

                // Showing the locking change
                lv_img_set_src(btn5_icon, &lock_36);
                // Setting settings icon to be half transparent if locked
                lv_obj_set_style_opa(menu_settings_icon, LV_OPA_20, LV_PART_MAIN);
                
                // Restoring timer if we are in a correct state
                // STATE state = model->getCurrentState();
                // if(state != STATE::Idle) {
                //     UIshowTimer();
                // }
                return;
            }

            // Else, show a popup to enter the 4 digit code
            createCodePopup([](int val1, int val2, int val3, int val4) {
                bool passed = model->unlockTest(val1, val2, val3, val4);

                if(passed) {
                    // UIshowUnlock();
                    lv_img_set_src(btn5_icon, &unlock_36);
                    // Setting settings icon to be full opacity if unlocked
                    lv_obj_set_style_opa(menu_settings_icon, LV_OPA_100, LV_PART_MAIN);

                    // Hiding timer since we are unlocked
                    // UIhideTimer();
                }
            });
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

    if(model->isUnlocked()) {
        lv_img_set_src(btn5_icon, &unlock_36);
        // Setting settings icon to be full opacity if unlocked
        lv_obj_set_style_opa(menu_settings_icon, LV_OPA_100, LV_PART_MAIN);
    } else {
        lv_img_set_src(btn5_icon, &lock_36);
        // Setting settings icon to be half transparent if locked
        lv_obj_set_style_opa(menu_settings_icon, LV_OPA_20, LV_PART_MAIN);
    }

    MODE currentMode = model->getMode();
    if(currentMode == MODE::Auto) {
        lv_obj_set_style_bg_color(menuButton2, C_BTN_Highlight, LV_PART_MAIN);
        lv_obj_set_style_bg_color(menuButton4, C_BTN_BG, LV_PART_MAIN);
    } else if(currentMode == MODE::Manual) {
        lv_obj_set_style_bg_color(menuButton2, C_BTN_BG, LV_PART_MAIN);
        lv_obj_set_style_bg_color(menuButton4, C_BTN_Highlight, LV_PART_MAIN);
    }
    
    lv_obj_remove_flag(menuOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(menuOverlay);
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
    UIapplyButtonStyle(pageCloseButton, true);

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
                UIhideThermometers();
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
            UIshowThermometers();
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
    lv_obj_set_size(restartButton, 100, 55);
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



static NetworkStatus networkStatus = NetworkStatus::NotSet;
static char networkStatusSSID[33] = "";

void UIsetNetworkStatus(NetworkStatus status, const char* ssid) {
    networkStatus = status;
    strncpy(networkStatusSSID, ssid ? ssid : "", sizeof(networkStatusSSID) - 1);
    networkStatusSSID[sizeof(networkStatusSSID) - 1] = '\0';

    if (currentNetworkLabel == nullptr) {
        return;
    }

    char text[64];
    lv_color_t color;
    switch (status) {
        case NetworkStatus::Connected:
            snprintf(text, sizeof(text), "Connected: %s", networkStatusSSID);
            color = lv_color_hex(0x4CD964);
            break;
        case NetworkStatus::Connecting:
            snprintf(text, sizeof(text), "Connecting: %s", networkStatusSSID);
            color = C_Orange;
            break;
        default:
            snprintf(text, sizeof(text), "Not set");
            color = lv_color_hex(0xA1A1A1);
            break;
    }
    lv_label_set_text(currentNetworkLabel, text);
    lv_obj_set_style_text_color(currentNetworkLabel, color, LV_PART_MAIN);
}

// Temporary: polls WiFi directly until the status is piped in from the core model
static void networkStatusPoll() {
    char ssid[33];
    getStoredNetworkSSID(ssid, sizeof(ssid));

    NetworkStatus status;
    if (ssid[0] == '\0') {
        status = NetworkStatus::NotSet;
    } else if (WiFi.status() == WL_CONNECTED) {
        status = NetworkStatus::Connected;
    } else {
        status = NetworkStatus::Connecting;
    }
    UIsetNetworkStatus(status, ssid);
}

static void networkHideKeyboard() {
    if (networkKeyboard == nullptr) {
        return;
    }
    lv_keyboard_set_textarea(networkKeyboard, nullptr);
    lv_obj_add_flag(networkKeyboard, LV_OBJ_FLAG_HIDDEN);
}

static void networkConnect() {
    const char* ssid = lv_textarea_get_text(networkSSIDInput);
    const char* pwd = lv_textarea_get_text(networkPWDInput);
    if (ssid == nullptr || ssid[0] == '\0') {
        return;
    }

    char ssidBuf[33];
    char pwdBuf[65];
    strncpy(ssidBuf, ssid, sizeof(ssidBuf) - 1);
    ssidBuf[sizeof(ssidBuf) - 1] = '\0';
    strncpy(pwdBuf, pwd ? pwd : "", sizeof(pwdBuf) - 1);
    pwdBuf[sizeof(pwdBuf) - 1] = '\0';

    setWiFiCredentials(ssidBuf, pwdBuf);

    lv_textarea_set_text(networkPWDInput, "");
    networkHideKeyboard();
    UIsetNetworkStatus(NetworkStatus::Connecting, ssidBuf);
}

static lv_obj_t* networkCreateInput(const char* placeholder, int y, uint32_t maxLen, bool isPassword) {
    lv_obj_t* input = lv_textarea_create(networkContent);
    lv_obj_set_size(input, 432, 56);
    lv_obj_align(input, LV_ALIGN_TOP_MID, 0, y);
    lv_textarea_set_one_line(input, true);
    lv_textarea_set_max_length(input, maxLen);
    lv_textarea_set_password_mode(input, isPassword);
    lv_textarea_set_placeholder_text(input, placeholder);
    lv_obj_set_style_text_font(input, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_set_style_text_color(input, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(input, C_BTN_BG_Lighter, LV_PART_MAIN);
    lv_obj_set_style_border_width(input, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(input, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(input, C_BTN_Highlight_Lighter, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(input, C_Orange, LV_PART_CURSOR);
    lv_obj_set_style_border_color(input, C_Orange, LV_PART_CURSOR | LV_STATE_FOCUSED);

    lv_obj_add_event_cb(input, [](lv_event_t* e) {
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_keyboard_set_textarea(networkKeyboard, target);
        lv_obj_remove_flag(networkKeyboard, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_CLICKED, nullptr);

    return input;
}

void UIinitializeNetwork() {
    networkPage = lv_obj_create(lv_scr_act());
    networkContent = lv_obj_create(networkPage);
    settingsPageCreator(networkPage, "Network", networkContent);

    // Tapping empty space dismisses the keyboard
    lv_obj_add_event_cb(networkPage, [](lv_event_t* e) {
        networkHideKeyboard();
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(networkContent, [](lv_event_t* e) {
        networkHideKeyboard();
    }, LV_EVENT_CLICKED, nullptr);

    currentNetworkLabel = lv_label_create(networkContent);
    lv_label_set_text(currentNetworkLabel, "Not set");
    lv_obj_align(currentNetworkLabel, LV_ALIGN_TOP_LEFT, 12, 4);
    lv_obj_set_width(currentNetworkLabel, 432);
    lv_label_set_long_mode(currentNetworkLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(currentNetworkLabel, &chivo_mono_34, LV_PART_MAIN);

    networkSSIDInput = networkCreateInput("SSID", 54, 32, false);
    networkPWDInput  = networkCreateInput("Password", 120, 64, true);

    networkConnectBtn = lv_btn_create(networkContent);
    lv_obj_align(networkConnectBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -24);
    UIapplyButtonStyle(networkConnectBtn, true);
    lv_obj_t* connectLabel = lv_label_create(networkConnectBtn);
    lv_label_set_text(connectLabel, "Connect");
    lv_obj_set_style_text_font(connectLabel, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_add_event_cb(networkConnectBtn, [](lv_event_t* e) {
        networkConnect();
    }, LV_EVENT_CLICKED, nullptr);

    // Parented to the top layer (not the page) so it can span the full 480px screen without being clipped or scrolling the page
    networkKeyboard = lv_keyboard_create(lv_layer_top());
    lv_obj_set_size(networkKeyboard, 480, 182);
    lv_obj_align(networkKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(networkKeyboard, C_Background, LV_PART_MAIN);
    lv_obj_set_style_bg_color(networkKeyboard, C_BTN_BG, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(networkKeyboard, C_BTN_Highlight, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(networkKeyboard, C_BTN_BG, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(networkKeyboard, C_BTN_Highlight, LV_PART_ITEMS | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(networkKeyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_color_filter_opa(networkKeyboard, LV_OPA_TRANSP, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(networkKeyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_text_color(networkKeyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(networkKeyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_font(networkKeyboard, LV_FONT_DEFAULT, LV_PART_ITEMS);
    lv_obj_add_flag(networkKeyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(networkKeyboard, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_READY) {
            // Enter on SSID moves on to password, enter on password connects
            if (lv_keyboard_get_textarea(networkKeyboard) == networkSSIDInput) {
                lv_keyboard_set_textarea(networkKeyboard, networkPWDInput);
            } else {
                networkConnect();
            }
        } else if (code == LV_EVENT_CANCEL) {
            networkHideKeyboard();
        }
    }, LV_EVENT_ALL, nullptr);
}

void UIshowNetwork() {
    if (networkPage == nullptr) {
        return;
    }
    
    lv_obj_remove_flag(networkPage, LV_OBJ_FLAG_HIDDEN);

    // SSID is prefilled, password is never shown
    char storedSSID[33];
    getStoredNetworkSSID(storedSSID, sizeof(storedSSID));
    lv_textarea_set_text(networkSSIDInput, storedSSID);
    lv_textarea_set_text(networkPWDInput, "");

    networkStatusPoll();
    if (networkStatusTimer == nullptr) {
        networkStatusTimer = lv_timer_create([](lv_timer_t*) { networkStatusPoll(); }, 1000, nullptr);
    }
}

void UIhideNetwork() {
    if (networkPage == nullptr) {
        return;
    }
    
    lv_obj_add_flag(networkPage, LV_OBJ_FLAG_HIDDEN);

    networkHideKeyboard();
    lv_textarea_set_text(networkPWDInput, "");

    if (networkStatusTimer != nullptr) {
        lv_timer_delete(networkStatusTimer);
        networkStatusTimer = nullptr;
    }
}



const int thermBtnHeight    = 50;
const int thermBtnTopMargin = 12;   // top margin before first button
const int thermBtnSpacing   = 0;

void UIpopulateThermometers() {
    if (thermometersList) {
        lv_obj_clean(thermometersList);
    }

    std::vector<SENSOR> sensors = getSensorList();

    for(int i = 0; i < sensors.size(); i++) {
        lv_obj_t* listItem = lv_obj_create(thermometersList);

        // Creating list item
        lv_obj_set_size(listItem, 456, thermBtnHeight);
        lv_obj_align(listItem, LV_ALIGN_TOP_LEFT, 0, thermBtnTopMargin + ((thermBtnSpacing + thermBtnHeight) * i));
        lv_obj_set_style_bg_opa(listItem, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(listItem, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(listItem, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(listItem, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(listItem, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(listItem, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t* label = lv_label_create(listItem);
        lv_label_set_text(label, sensors[i].name);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_set_size(label, 360, 24);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(label, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &chivo_mono_34, LV_PART_MAIN);

        lv_obj_t* btn = lv_btn_create(listItem);
        lv_obj_set_size(btn, thermBtnHeight - 2, thermBtnHeight - 2);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -12, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        
        lv_obj_t* icon = lv_img_create(btn);
        lv_img_set_src(icon, &trash_30);
        lv_obj_set_size(icon, 30, 30);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

        // Copying into static memory
        char* nameCopy = strdup(sensors[i].name);
        if (nameCopy == nullptr) {
            nameCopy = strdup("");
        }

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

            char* name = static_cast<char*>(lv_event_get_user_data(e));
            lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));

            removeSensor(name);
            lv_obj_clean(thermometersList);
            UIpopulateThermometers();
            free(name);
        }, LV_EVENT_CLICKED, nameCopy);
    }
}

void UIinitializeThermometers() {
    thermometersPage = lv_obj_create(lv_scr_act());
    thermometersContent = lv_obj_create(thermometersPage);
    settingsPageCreator(thermometersPage, "Thermometers", thermometersContent);

    // Make the parent container non-scrollable
    lv_obj_set_scrollbar_mode(thermometersContent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(thermometersContent, LV_DIR_NONE);

    thermometersLabel = lv_label_create(thermometersContent);
    lv_label_set_text(thermometersLabel, "Name:");
    lv_obj_align(thermometersLabel, LV_ALIGN_TOP_LEFT, 30, 0);
    lv_obj_set_size(thermometersLabel, 360, 24);
    lv_obj_set_style_text_color(thermometersLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(thermometersLabel, &chivo_mono_34, LV_PART_MAIN);

    // List of current thermometers (which is scrollable)
    thermometersList = lv_obj_create(thermometersContent);
    lv_obj_set_size(thermometersList, 456, 300);  // Reduced height to leave room for button
    lv_obj_align(thermometersList, LV_ALIGN_TOP_LEFT, 0, thermBtnTopMargin + ((thermBtnSpacing + thermBtnHeight) * 1));
    lv_obj_set_style_bg_color(thermometersList, C_Background_Panel, LV_PART_MAIN);
    lv_obj_set_style_border_width(thermometersList, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(thermometersList, 0, LV_PART_MAIN);
    
    // Enable scrolling ONLY on this list
    lv_obj_set_scrollbar_mode(thermometersList, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(thermometersList, LV_DIR_VER);
    lv_obj_set_flex_flow(thermometersList, LV_FLEX_FLOW_COLUMN);  // Stack items vertically

    UIpopulateThermometers();

    // Add thermometer button
    thermometersAddBtn = lv_btn_create(thermometersContent);
    lv_obj_set_size(thermometersAddBtn, 280, 55);
    lv_obj_align(thermometersAddBtn, LV_ALIGN_BOTTOM_RIGHT, -24, -24);
    UIapplyButtonStyle(thermometersAddBtn, true);

    lv_obj_t* addMessage = lv_label_create(thermometersAddBtn);
    lv_label_set_text(addMessage, "Add therm.");
    lv_obj_set_style_text_color(addMessage, lv_color_hex(0xD7D7D7), LV_PART_MAIN);
    lv_obj_set_style_text_font(addMessage, &chivo_mono_34, LV_PART_MAIN);
    lv_obj_center(addMessage);

    lv_obj_add_event_cb(thermometersAddBtn, [](lv_event_t* e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

        createTextPopup("Thermometer name", "Enter name...", [](const char* name) {
            if (strlen(name) > 0) {
                addSensor(name);
                UIpopulateThermometers();
            }
        });
    }, LV_EVENT_CLICKED, nullptr);
}


void UIshowThermometers() {
    if (thermometersPage == nullptr) {
        return;
    }
    
    lv_obj_remove_flag(thermometersPage, LV_OBJ_FLAG_HIDDEN);
}

void UIhideThermometers() {
    if (thermometersPage == nullptr) {
        return;
    }
    
    lv_obj_add_flag(thermometersPage, LV_OBJ_FLAG_HIDDEN);
}