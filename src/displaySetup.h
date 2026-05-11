#ifndef DISPLAY_SETUP_H
#define DISPLAY_SETUP_H

#include <Arduino_GFX_Library.h>
#include <ESP32_4848S040.h>
#include <lvgl.h>
#include "touch.h"
#include "./colorHelper.h"

// Display variables
#define GFX_BL 38
Arduino_ESP32SPI* bus;
Arduino_RGB_Display* gfx;


/* LVGL will draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   480
#define BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 6 * (LV_COLOR_DEPTH / 8))

uint32_t* draw_buf  = (uint32_t*)heap_caps_malloc(BUF_SIZE / 4 * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
uint32_t* draw_buf2 = (uint32_t*)heap_caps_malloc(BUF_SIZE / 4 * sizeof(uint32_t), MALLOC_CAP_SPIRAM);


/* Display flushing callback */
void flushDisplay( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  // lv_draw_sw_rgb565_swap(px_map, TFT_HOR_RES * TFT_VER_RES / 4);

  #if (LV_COLOR_16_SWAP != 0)
    // gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  #else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
  #endif

  

  lv_disp_flush_ready(disp);
}

/* Reading touchpad callback */
void readTouchpad(lv_indev_t * indev, lv_indev_data_t * data ) {
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PRESSED;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      //Serial.print("Touched : "); Serial.print(touch_last_x); Serial.print(" x ");Serial.println(touch_last_y);
    }
    else if (touch_released()) {
      data->state = LV_INDEV_STATE_RELEASED;
    }
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static uint32_t getTick(void) {
  return millis();
}

void setupDisplay() {
    // Initialize the touch part of the display
    touch_init();

    // 9-bit mode SPI
    bus = new Arduino_ESP32SPI(
    GFX_NOT_DEFINED /* DC */, 39 /* CS */, 48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

    // Panal hardware specific config
    Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */,
    4 /* R0 */, 5 /* R1 */, 6 /* R2 */, 7 /* R3 */, 15 /* R4 */,
    8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 9 /* G4 */, 10 /* G5 */,
    11 /* B0 */, 12 /* B1 */, 13 /* B2 */, 14 /* B3 */, 0 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

    // Creading that pannel in gfx
    gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_4848s040_init_operations, sizeof(st7701_4848s040_init_operations));

    // Catching if it failed to start
    if (!gfx->begin()) {
    printf("GFX Failed to start\n");
    }

    // Turning on backlight, and filling screen with black
    #ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    #endif
    gfx->setCursor(100, 200);
    gfx->displayOn();
    gfx->fillScreen(BLACK);
    // gfx->setTextColor(BLUE);
    // gfx->setTextSize(6 /* x scale */, 6 /* y scale */, 2 /* pixel_margin */);
    // gfx->println("Prout !");

    /* Starting up LVGL */
    lv_init();

    // Set a tick source so that LVGL will know how much time elapsed.
    lv_tick_set_cb(getTick);
    lv_display_t * disp;

    // Create the display in LVGL
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, flushDisplay);
    lv_display_set_buffers(disp, draw_buf, draw_buf2, BUF_SIZE / 4 * sizeof(uint32_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Touch Driver in LVGL
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);   /*See below.*/
    lv_indev_set_read_cb(indev, readTouchpad);

    // Fill the background with background color initially
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_set_style_bg_color(screen, C_Background, 0);
    lv_scr_load(screen);
}

#endif // DISPLAY_SETUP_H