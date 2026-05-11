#ifndef COLOR_HELPER_H
#define COLOR_HELPER_H

#include <lvgl.h>

inline lv_color_t fix_color(int8_t r, int8_t g, int8_t b) {
    // Pass the reversed values to lv_color_make
    return lv_color_make(b, g, r);
}

#define C_Background        fix_color(7  ,10 ,5  )
#define C_Background_Panel  lv_color_hex(0x232323)
#define C_BTN_BG            lv_color_hex(0x232323)
#define C_BTN_Highlight     lv_color_hex(0x434343)
#define C_Orange            fix_color(255,165,0  )
#define C_GoalTemp          fix_color(72 ,72 ,72 )
#define C_Red               fix_color(230,20 ,50 )
#define C_Blue              fix_color(50 ,20 ,210)
#define C_Teal              fix_color(20 ,210,230)

#endif // COLOR_HELPER_H