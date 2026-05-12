# SMRT Thermostat

> It is so smart, S-M-R-T... I mean S-M-A-R-T

An open source thermostat software built on top of the ESP32-4848S040 hardware. Working offline, it has built-in time protections for preventing short cycling. Natively, it supports bluetooth temerature sensors with fail-safe manual controls when temperature sensors aren't up to date. Additionally it optionally connects to Home Assistant for remote control.

## Hardware

This is built ontop of the ESP32-4848S040 device. As of the last two years, its common on sites like aliexpress. Here are some links to the board and related items I use:
- Aliexpress board is easily found via a [general search](https://www.aliexpress.us/w/wholesale-4848S040-480x480-devboard.html). Search terms: "ESP32-S3 Arduino LVGL WIFI&Bluetooth Dev Board, 480x480 Smart Display 4.0 inch LCD, TFT Module, Capacitive Touch"
- Here is the [specific listing I used](https://www.aliexpress.us/item/3256807420446535.html) for the board.
- I used this [wall box](https://www.aliexpress.us/item/3256801694520219.html). However, most boxes for "86 type" switches and sockets should fit this hardware.
- Wireless thermometers I used are [XIAOMI Mijia Bluetooth Thermometer 2](https://www.aliexpress.us/w/wholesale-New-XIAOMI-Mijia-Bluetooth-Thermometer-2-Wireless.html) (aka LYWSD03MMC). I flashed them with custom firmware as outlined in [pvvx's github repo](https://github.com/pvvx/ATC_MiThermometer) for this. Additionally, I used the python script provided by PiotrMachowski in [their github repo](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor) for extracting cloud tokens.

## Libraries and code

Of the standard libraries, I use PlatformIO via it's VSCode extension to program the esp32 board. I use lvgl@^9.2.2 for drawing graphics on the screen. All LVGL has been standardly programed in (not using a visual editor like [LVGL Pro](https://lvgl.io/pro)). Below LVGL I use Arduino_GFX for interfacing with the display.

The board definitions and core graphics/touch configuration and code for using it was taken from [sand1812's github repo](https://github.com/sand1812/ESP32-4848S040). Using their code was crucial in getting this project started, thank you sand1812! 

Notably, I joyously use the Home Assistant Integration by dawidchyrzynski, found in [his repo](https://github.com/dawidchyrzynski/arduino-home-assistant). Its the perfect balance between abstraction of home-assistant systems and an interface grounded in c++ classes and simply callable methods, special thanks to dawidchyrzynski's library.
