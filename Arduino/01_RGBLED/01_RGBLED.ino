/**********************************************************************
  Filename    : WS2812_Blink_Native
  Description : Control a WS2812 LED on GPIO 38 using ESP32 native function.
                (Fixes RMT driver conflicts with Adafruit library)
**********************************************************************/

// No external libraries needed!
// neopixelWrite is built into the ESP32 Arduino Core (versions 2.0.14+ and 3.x)

#define LED_PIN 38

// Brightness level (0-255). 
// 25 is roughly 10% brightness, which is plenty for direct viewing.
#define BRIGHTNESS 25 

// Helper function to write color with brightness scaling
void ledWrite(uint8_t r, uint8_t g, uint8_t b) {
  // Scale the values based on the BRIGHTNESS constant
  // We use map() or simple math to scale 0-255 input to 0-BRIGHTNESS
  uint8_t r_scaled = (r * BRIGHTNESS) / 255;
  uint8_t g_scaled = (g * BRIGHTNESS) / 255;
  uint8_t b_scaled = (b * BRIGHTNESS) / 255;
  
  neopixelWrite(LED_PIN, r_scaled, g_scaled, b_scaled);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("WS2812 Demo using ESP32 Native 'neopixelWrite'");
  Serial.println("Brightness set to level: " + String(BRIGHTNESS));
}

void loop() {
  // Red
  Serial.println("Color: Red");
  ledWrite(255, 0, 0);
  delay(1000);

  // Green
  Serial.println("Color: Green");
  ledWrite(0, 255, 0);
  delay(1000);

  // Blue
  Serial.println("Color: Blue");
  ledWrite(0, 0, 255);
  delay(1000);

  // Yellow (Red + Green)
  Serial.println("Color: Yellow");
  ledWrite(255, 255, 0);
  delay(1000);

  // White (Red + Green + Blue)
  Serial.println("Color: White");
  ledWrite(255, 255, 255); 
  delay(1000);
  
  // Off
  Serial.println("Color: Off");
  neopixelWrite(LED_PIN, 0, 0, 0); // Directly turn off
  delay(1000);
}
