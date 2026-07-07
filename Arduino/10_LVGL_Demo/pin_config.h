#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// 1.83 inch ESP32-S3 board with ST7789 TFT, 240x284.
#define LCD_WIDTH   240
#define LCD_HEIGHT  284

// ST7789 SPI pins.
#define LCD_RST   39
#define LCD_MOSI  45
#define LCD_SCK   40
#define LCD_CS    42
#define LCD_DC    41
#define LCD_BL    46

// ST7789 RAM window offset. Keep this aligned with the working 08/09 demos.
#define LCD_OFFSET_X 0
#define LCD_OFFSET_Y 0

#endif
