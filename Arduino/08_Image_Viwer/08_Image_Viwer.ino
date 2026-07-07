/**
 * 08_WiFi_ImageViewer - WiFi image upload display for 1.83 inch ST7789.
 *
 * Hardware: ESP32-S3 N16R8 + ST7789 TFT (240x284)
 *
 * Usage:
 *   1. Upload this sketch.
 *   2. Connect to WiFi AP: Image_Viewer, password: 12345678.
 *   3. Open browser: 192.168.4.1.
 *   4. Select an image. The browser resizes it and uploads a JPEG.
 *
 * Dependencies: Arduino_GFX_Library, JPEGDEC
 * Partition: app3M_fat9M_16MB, or another partition table with FFat.
 */

#include <Arduino.h>
#include <FFat.h>
#include <JPEGDEC.h>
#include <WebServer.h>
#include <WiFi.h>

#include "Arduino_GFX_Library.h"
#include "pin_config.h"

#define AP_SSID     "Image_Viewer"
#define AP_PASSWORD "12345678"
#define IMG_FILENAME "/image.jpg"
#define CLEAN_NEAR_BLACK 1
#define BLACK_CLEAN_THRESHOLD 18

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT,
    LCD_OFFSET_X, LCD_OFFSET_Y, 0, 0);

JPEGDEC jpeg;
File jpegFile;
WebServer server(80);

bool hasImage = false;
bool isUploading = false;
bool needRefresh = false;
File uploadFile;

static uint16_t drawLineBuffer[LCD_WIDTH];

void *jpegOpen(const char *filename, int32_t *size) {
  jpegFile = FFat.open(filename, "r");
  if (!jpegFile) {
    return nullptr;
  }

  *size = jpegFile.size();
  return &jpegFile;
}

void jpegClose(void *handle) {
  (void)handle;
  if (jpegFile) {
    jpegFile.close();
  }
}

int32_t jpegRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  (void)handle;
  if (!jpegFile) {
    return 0;
  }
  return jpegFile.read(buffer, length);
}

int32_t jpegSeek(JPEGFILE *handle, int32_t position) {
  (void)handle;
  if (!jpegFile) {
    return 0;
  }
  return jpegFile.seek(position) ? position : 0;
}

static bool isNearBlack565BE(uint8_t hi, uint8_t lo) {
  uint8_t r = hi & 0xF8;
  uint8_t g = static_cast<uint8_t>(((hi & 0x07) << 5) | ((lo & 0xE0) >> 3));
  uint8_t b = static_cast<uint8_t>((lo & 0x1F) << 3);
  return r <= BLACK_CLEAN_THRESHOLD &&
         g <= BLACK_CLEAN_THRESHOLD &&
         b <= BLACK_CLEAN_THRESHOLD;
}

int jpegDraw(JPEGDRAW *pDraw) {
  int16_t x = pDraw->x;
  int16_t y = pDraw->y;
  int16_t w = pDraw->iWidth;
  int16_t h = pDraw->iHeight;

  if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
    return 1;
  }
  if (x + w > LCD_WIDTH) {
    w = LCD_WIDTH - x;
  }
  if (y + h > LCD_HEIGHT) {
    h = LCD_HEIGHT - y;
  }
  if (w <= 0 || h <= 0) {
    return 1;
  }

  uint8_t *src = reinterpret_cast<uint8_t *>(pDraw->pPixels);
  for (int16_t row = 0; row < h; row++) {
    uint8_t *srcLine = src + (row * pDraw->iWidth * 2);
    uint8_t *dstLine = reinterpret_cast<uint8_t *>(drawLineBuffer);

    for (int16_t col = 0; col < w; col++) {
      uint8_t hi = srcLine[col * 2];
      uint8_t lo = srcLine[col * 2 + 1];

#if CLEAN_NEAR_BLACK
      if (isNearBlack565BE(hi, lo)) {
        hi = 0;
        lo = 0;
      }
#endif

      dstLine[col * 2] = hi;
      dstLine[col * 2 + 1] = lo;
    }

    gfx->draw16bitBeRGBBitmap(x, y + row, drawLineBuffer, w, 1);
  }
  return 1;
}

void handleRoot() {
  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Image Upload</title><style>");
  html += F("body{font-family:Arial;text-align:center;padding:20px;background:#0d1117;color:#fff}");
  html += F("h1{color:#58a6ff}.box{background:#161b22;padding:30px;border-radius:10px;margin:20px auto;max-width:400px;border:1px solid #30363d}");
  html += F("button{background:#238636;color:#fff;padding:15px 30px;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin:10px}");
  html += F("button:hover{background:#2ea043}button:disabled{background:#555;cursor:not-allowed}");
  html += F("#preview{max-width:240px;margin:10px auto;border:2px solid #30363d;border-radius:4px;display:none}");
  html += F("#status{color:#f0883e;margin:10px 0}.info{color:#8b949e;font-size:14px;margin-top:20px}");
  html += F("</style></head><body><h1>Image Viewer</h1><div class='box'>");
  html += F("<h3>Upload Image</h3><input type='file' id='f' accept='image/*'><br>");
  html += F("<img id='preview'><canvas id='c' style='display:none'></canvas>");
  html += F("<div id='status'></div><button id='btn' disabled>Upload</button></div>");
  html += F("<div class='info'>Auto resize to 240x284<br>Supports JPG, PNG</div>");
  html += F("<script>");
  html += F("var b=null,W=240,H=284;");
  html += F("document.getElementById('f').onchange=function(e){var f=e.target.files[0];if(!f)return;");
  html += F("document.getElementById('status').textContent='Processing...';var i=new Image();i.onload=function(){");
  html += F("var w=i.width,h=i.height,s=Math.min(W/w,H/h,1);w=Math.round(w*s);h=Math.round(h*s);");
  html += F("var c=document.getElementById('c');c.width=w;c.height=h;c.getContext('2d').drawImage(i,0,0,w,h);");
  html += F("c.toBlob(function(x){b=x;document.getElementById('preview').src=URL.createObjectURL(x);");
  html += F("document.getElementById('preview').style.display='block';document.getElementById('btn').disabled=false;");
  html += F("document.getElementById('status').textContent='Ready: '+w+'x'+h+' ('+(x.size/1024|0)+'KB)';},'image/jpeg',1.0);");
  html += F("};i.src=URL.createObjectURL(f);};");
  html += F("document.getElementById('btn').onclick=function(){if(!b)return;var s=document.getElementById('status'),t=this;");
  html += F("s.textContent='Uploading...';t.disabled=true;var d=new FormData();d.append('file',b,'image.jpg');");
  html += F("fetch('/upload',{method:'POST',body:d}).then(function(r){return r.text();}).then(function(h){document.body.innerHTML=h;})");
  html += F(".catch(function(e){s.textContent='Error: '+e;t.disabled=false;});};");
  html += F("</script></body></html>");
  server.send(200, "text/html", html);
}

void handleUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    isUploading = true;
    gfx->fillScreen(BLACK);
    gfx->setTextColor(CYAN);
    gfx->setTextSize(2);
    gfx->setCursor(42, 132);
    gfx->println("Uploading...");

    if (FFat.exists(IMG_FILENAME)) {
      FFat.remove(IMG_FILENAME);
    }
    uploadFile = FFat.open(IMG_FILENAME, "w");
    Serial.printf("Upload start: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload done: %u bytes\n", upload.totalSize);
      hasImage = true;
      needRefresh = true;
    }
    isUploading = false;
  }
}

void handleUploadComplete() {
  String html = F("<!DOCTYPE html><html><body style='background:#0d1117;color:#fff;text-align:center;padding:50px;font-family:Arial'>");
  html += F("<h1 style='color:#58a6ff'>Upload Success!</h1>");
  html += F("<p>Image will display now.</p>");
  html += F("<a href='/' style='color:#58a6ff'>Upload Another</a></body></html>");
  server.send(200, "text/html", html);
}

void showWaitingScreen() {
  gfx->fillScreen(BLACK);
  gfx->setTextColor(CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(30, 40);
  gfx->println("Image Viewer");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(20, 90);
  gfx->println("1. Connect WiFi:");

  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor(20, 110);
  gfx->println(AP_SSID);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(20, 142);
  gfx->print("Password: ");
  gfx->println(AP_PASSWORD);

  gfx->setCursor(20, 172);
  gfx->println("2. Open browser:");

  gfx->setTextColor(GREEN);
  gfx->setTextSize(2);
  gfx->setCursor(20, 192);
  gfx->println("192.168.4.1");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(20, 238);
  gfx->println("3. Upload image");
}

void showImage() {
  if (!FFat.exists(IMG_FILENAME)) {
    Serial.println("Image file not found");
    showWaitingScreen();
    return;
  }

  gfx->fillScreen(BLACK);

  if (jpeg.open(IMG_FILENAME, jpegOpen, jpegClose, jpegRead, jpegSeek, jpegDraw)) {
    int w = jpeg.getWidth();
    int h = jpeg.getHeight();
    int x = (LCD_WIDTH - w) / 2;
    int y = (LCD_HEIGHT - h) / 2;

    if (x < 0) {
      x = 0;
    }
    if (y < 0) {
      y = 0;
    }

    Serial.printf("Image: %dx%d\n", w, h);
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    jpeg.decode(x, y, 0);
    jpeg.close();
    Serial.println("Image displayed");
  } else {
    Serial.println("Failed to decode JPEG");
    gfx->setTextColor(RED);
    gfx->setTextSize(2);
    gfx->setCursor(30, 132);
    gfx->println("Decode Error!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("08 WiFi Image Viewer started");

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);

  if (!FFat.begin(true)) {
    Serial.println("FFat mount failed");
    gfx->setTextColor(RED);
    gfx->setTextSize(2);
    gfx->setCursor(20, 110);
    gfx->println("FFat Error!");
    while (true) {
      delay(1000);
    }
  }

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(20, 110);
  gfx->println("Starting AP...");

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, handleUploadComplete, handleUpload);
  server.begin();

  hasImage = FFat.exists(IMG_FILENAME);
  if (hasImage) {
    needRefresh = true;
  } else {
    showWaitingScreen();
  }
}

void loop() {
  server.handleClient();

  if (needRefresh && !isUploading) {
    needRefresh = false;
    showImage();
  }

  delay(10);
}
