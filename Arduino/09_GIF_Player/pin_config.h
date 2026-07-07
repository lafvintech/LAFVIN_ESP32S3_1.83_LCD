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

// ST7789 memory offset. Adjust these if the image is shifted on a different
// panel batch.
#define LCD_OFFSET_X 0
#define LCD_OFFSET_Y 0

// RGB565 colors.
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define ORANGE  0xFD20

#endif
