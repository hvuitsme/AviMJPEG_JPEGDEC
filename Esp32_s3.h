#pragma once

// Display
#include <Arduino_GFX_Library.h>
#define GFX_BL 48
// Arduino_DataBus *bus = new Arduino_ESP32QSPI(
//     40 /* cs */, 41 /* dc */, 42 /* rst */, 36 /* sck */, 35 /* mosi */, GFX_NOT_DEFINED /* miso */);
Arduino_DataBus *bus = create_default_Arduino_DataBus();
Arduino_GFX *gfx = new Arduino_ST7789(bus, 42 /* RST */, 0 /* rotation */, true /* IPS */, 170, 320, 35, 0, 35, 0);

#define GFX_SPEED 80000000UL

// Button
// #define BTN_A_PIN 0
// #define BTN_B_PIN 21

// I2C
#define I2C_SDA 8
#define I2C_SCL 4
#define I2C_FREQ 800000UL

// SD card
#define SD_SCK  12
#define SD_MOSI 11 // CMD
#define SD_MISO 13 // D0
#define SD_D1   14
#define SD_D2   9
#define SD_CS   10 // D3

// I2S
#define I2S_DEFAULT_GAIN_LEVEL 0.5
#define I2S_OUTPUT_NUM I2S_NUM_0
#define I2S_MCLK -1
#define I2S_BCLK 17
#define I2S_LRCK 2
#define I2S_DOUT 16
#define I2S_DIN -1

// #define AUDIO_MUTE_PIN 48   // LOW for mute