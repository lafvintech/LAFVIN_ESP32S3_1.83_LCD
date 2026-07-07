/**********************************************************************
  Filename    : 03_QMI8658A_Sensor_Demo
  Description : QMI8658A 6-axis IMU sensor + LVGL visualization
  Hardware    : ESP32-S3-N16R8 + 1.83inch LCD + QMI8658A
  Library     : TFT_eSPI + lvgl v8.x + Wire
**********************************************************************/

#include "display.h"
#include <Wire.h>
#include <SensorQMI8658.hpp>

static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 70;

#define PIN_IMU_SDA  48
#define PIN_IMU_SCL  47
#define CALIBRATION_SAMPLES 180


Display screen;
SensorQMI8658 imu;

struct SensorData {
  float accX = 0.0f;
  float accY = 0.0f;
  float accZ = 0.0f;
  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
  float temp = 0.0f;
  bool available = false;
};

SensorData sensor;

struct GyroBias {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

static GyroBias gyro_bias;

static void map_board_axes(float raw_x, float raw_y, float raw_z,
                           float &mapped_x, float &mapped_y, float &mapped_z) {
  mapped_x = raw_y;
  mapped_y = raw_x;
  mapped_z = -raw_z;
}

static bool calibrate_imu() {
  float bias_sum_x = 0.0f;
  float bias_sum_y = 0.0f;
  float bias_sum_z = 0.0f;
  int bias_count = 0;

  Serial.println("      Calibrating IMU bias...");
  for (int i = 0; i < CALIBRATION_SAMPLES; ++i) {
    uint32_t wait_start = millis();
    while (!imu.getDataReady()) {
      delay(1);
      if (millis() - wait_start > 100) {
        Serial.println("      Calibration timeout while waiting gyro data");
        return false;
      }
    }

    float gx = 0.0f, gy = 0.0f, gz = 0.0f;
    if (!imu.getGyroscope(gx, gy, gz)) {
      continue;
    }

    bias_sum_x += gx;
    bias_sum_y += gy;
    bias_sum_z += gz;
    bias_count++;
  }

  if (bias_count == 0) {
    Serial.println("      Calibration failed: no gyro samples");
    return false;
  }

  gyro_bias.x = bias_sum_x / bias_count;
  gyro_bias.y = bias_sum_y / bias_count;
  gyro_bias.z = bias_sum_z / bias_count;
  Serial.printf("      Gyro bias: %.3f, %.3f, %.3f\n", gyro_bias.x, gyro_bias.y, gyro_bias.z);
  return true;
}

static lv_obj_t *label_title;
static lv_obj_t *label_acc;
static lv_obj_t *label_gyro;
static lv_obj_t *label_temp;
static lv_obj_t *chart_acc;
static lv_obj_t *chart_gyro;
static lv_chart_series_t *ser_acc_x;
static lv_chart_series_t *ser_acc_y;
static lv_chart_series_t *ser_acc_z;
static lv_chart_series_t *ser_gyro_x;
static lv_chart_series_t *ser_gyro_y;
static lv_chart_series_t *ser_gyro_z;
static lv_obj_t *bar_acc_x;
static lv_obj_t *bar_acc_y;
static lv_obj_t *bar_acc_z;

void update_sensor_ui(void) {
  if (!sensor.available) {
    return;
  }

  char text_buf[96];

  snprintf(text_buf, sizeof(text_buf), "ACC(g)\nX:%6.2f\nY:%6.2f\nZ:%6.2f",
           sensor.accX, sensor.accY, sensor.accZ);
  lv_label_set_text(label_acc, text_buf);

  snprintf(text_buf, sizeof(text_buf), "GYRO(dps)\nX:%7.1f\nY:%7.1f\nZ:%7.1f",
           sensor.gyroX, sensor.gyroY, sensor.gyroZ);
  lv_label_set_text(label_gyro, text_buf);

  snprintf(text_buf, sizeof(text_buf), "Temp: %.1f C", sensor.temp);
  lv_label_set_text(label_temp, text_buf);

  int bar_val_x = constrain(static_cast<int>((sensor.accX + 2.0f) * 25.0f), 0, 100);
  int bar_val_y = constrain(static_cast<int>((sensor.accY + 2.0f) * 25.0f), 0, 100);
  int bar_val_z = constrain(static_cast<int>((sensor.accZ + 2.0f) * 25.0f), 0, 100);
  lv_bar_set_value(bar_acc_x, bar_val_x, LV_ANIM_OFF);
  lv_bar_set_value(bar_acc_y, bar_val_y, LV_ANIM_OFF);
  lv_bar_set_value(bar_acc_z, bar_val_z, LV_ANIM_OFF);

  lv_chart_set_next_value(chart_acc, ser_acc_x, static_cast<lv_coord_t>(sensor.accX * 20.0f + 50.0f));
  lv_chart_set_next_value(chart_acc, ser_acc_y, static_cast<lv_coord_t>(sensor.accY * 20.0f + 50.0f));
  lv_chart_set_next_value(chart_acc, ser_acc_z, static_cast<lv_coord_t>(sensor.accZ * 20.0f + 50.0f));

  lv_chart_set_next_value(chart_gyro, ser_gyro_x, static_cast<lv_coord_t>(sensor.gyroX + 50.0f));
  lv_chart_set_next_value(chart_gyro, ser_gyro_y, static_cast<lv_coord_t>(sensor.gyroY + 50.0f));
  lv_chart_set_next_value(chart_gyro, ser_gyro_z, static_cast<lv_coord_t>(sensor.gyroZ + 50.0f));

  lv_chart_refresh(chart_acc);
  lv_chart_refresh(chart_gyro);

}

void create_ui(void) {
  label_title = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(label_title, lv_color_hex(0x0F172A), 0);
  lv_label_set_text(label_title, "QMI8658A IMU Sensor");
  lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

  label_temp = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label_temp, lv_color_hex(0xB45309), 0);
  lv_label_set_text(label_temp, "Temp: --.- C");
  lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 10, 20);

  label_acc = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_acc, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(label_acc, lv_color_hex(0x1F2937), 0);
  lv_label_set_text(label_acc, "ACC(g)\nX: --.--\nY: --.--\nZ: --.--");
  lv_obj_align(label_acc, LV_ALIGN_TOP_LEFT, 10, 40);

  label_gyro = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_gyro, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(label_gyro, lv_color_hex(0x1F2937), 0);
  lv_label_set_text(label_gyro, "GYRO(dps)\nX: ---.-\nY: ---.-\nZ: ---.-");
  lv_obj_align(label_gyro, LV_ALIGN_TOP_RIGHT, -10, 40);

  bar_acc_x = lv_bar_create(lv_scr_act());
  lv_obj_set_size(bar_acc_x, 60, 8);
  lv_obj_align(bar_acc_x, LV_ALIGN_BOTTOM_MID, 0, -100);
  lv_bar_set_range(bar_acc_x, 0, 100);
  lv_obj_set_style_bg_color(bar_acc_x, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_acc_x, lv_color_hex(0xDC2626), LV_PART_INDICATOR);

  bar_acc_y = lv_bar_create(lv_scr_act());
  lv_obj_set_size(bar_acc_y, 60, 8);
  lv_obj_align(bar_acc_y, LV_ALIGN_BOTTOM_MID, 0, -87);
  lv_bar_set_range(bar_acc_y, 0, 100);
  lv_obj_set_style_bg_color(bar_acc_y, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_acc_y, lv_color_hex(0x16A34A), LV_PART_INDICATOR);

  bar_acc_z = lv_bar_create(lv_scr_act());
  lv_obj_set_size(bar_acc_z, 60, 8);
  lv_obj_align(bar_acc_z, LV_ALIGN_BOTTOM_MID, 0, -74);
  lv_bar_set_range(bar_acc_z, 0, 100);
  lv_obj_set_style_bg_color(bar_acc_z, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_acc_z, lv_color_hex(0x2563EB), LV_PART_INDICATOR);

  lv_obj_t *lbl_bar = lv_label_create(lv_scr_act());
  lv_label_set_text(lbl_bar, "Acc: X Y Z");
  lv_obj_set_style_text_font(lbl_bar, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lbl_bar, lv_color_hex(0x64748B), 0);
  lv_obj_align(lbl_bar, LV_ALIGN_BOTTOM_MID, 0, -120);

  chart_acc = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_acc, 110, 50);
  lv_obj_align(chart_acc, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
  lv_chart_set_type(chart_acc, LV_CHART_TYPE_LINE);
  lv_chart_set_range(chart_acc, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(chart_acc, 20);
  lv_obj_set_style_bg_color(chart_acc, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_color(chart_acc, lv_color_hex(0xCBD5E1), 0);
  lv_obj_set_style_border_width(chart_acc, 1, 0);

  ser_acc_x = lv_chart_add_series(chart_acc, lv_color_hex(0xDC2626), LV_CHART_AXIS_PRIMARY_Y);
  ser_acc_y = lv_chart_add_series(chart_acc, lv_color_hex(0x16A34A), LV_CHART_AXIS_PRIMARY_Y);
  ser_acc_z = lv_chart_add_series(chart_acc, lv_color_hex(0x2563EB), LV_CHART_AXIS_PRIMARY_Y);

  chart_gyro = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_gyro, 110, 50);
  lv_obj_align(chart_gyro, LV_ALIGN_BOTTOM_LEFT, 5, -5);
  lv_chart_set_type(chart_gyro, LV_CHART_TYPE_LINE);
  lv_chart_set_range(chart_gyro, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(chart_gyro, 20);
  lv_obj_set_style_bg_color(chart_gyro, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_color(chart_gyro, lv_color_hex(0xCBD5E1), 0);
  lv_obj_set_style_border_width(chart_gyro, 1, 0);

  ser_gyro_x = lv_chart_add_series(chart_gyro, lv_color_hex(0xDC2626), LV_CHART_AXIS_PRIMARY_Y);
  ser_gyro_y = lv_chart_add_series(chart_gyro, lv_color_hex(0x16A34A), LV_CHART_AXIS_PRIMARY_Y);
  ser_gyro_z = lv_chart_add_series(chart_gyro, lv_color_hex(0x2563EB), LV_CHART_AXIS_PRIMARY_Y);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  03 - QMI8658A IMU Sensor Demo");
  Serial.println("  ESP32-S3 / ST7789 / QMI8658A");
  Serial.println("========================================\n");

  Serial.println("[1/4] Initializing LCD...");
  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);
  Serial.println("      LCD OK!");

  Serial.println("[2/4] Initializing LVGL UI...");
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xF1F1F1), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
  Serial.println("      LVGL OK!");

  create_ui();

  Serial.println("[3/4] Initializing QMI8658A...");
  if (imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS, PIN_IMU_SDA, PIN_IMU_SCL)) {
    Serial.println("      QMI8658A OK!");
    Serial.println("      ID: 0x" + String(imu.getChipID(), HEX));
    imu.configAccelerometer(
      SensorQMI8658::ACC_RANGE_4G,
      SensorQMI8658::ACC_ODR_125Hz,
      SensorQMI8658::LPF_MODE_0);
    imu.configGyroscope(
      SensorQMI8658::GYR_RANGE_64DPS,
      SensorQMI8658::GYR_ODR_112_1Hz,
      SensorQMI8658::LPF_MODE_3);
    imu.enableAccelerometer();
    imu.enableGyroscope();
    imu.dumpCtrlRegister();
    sensor.available = calibrate_imu();
  } else {
    Serial.println("      ERROR: QMI8658A not found!");
    Serial.println("      Check I2C connection (SDA:48, SCL:47)");
    sensor.available = false;
    lv_label_set_text(label_title, "QMI8658A - ERROR!");
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xDC2626), 0);
  }

  Serial.println("[4/4] System ready!");
  Serial.println("\n========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================\n");
}

void loop() {
  if (sensor.available && imu.getDataReady()) {
    float raw_acc_x = 0.0f, raw_acc_y = 0.0f, raw_acc_z = 0.0f;
    float raw_gyro_x = 0.0f, raw_gyro_y = 0.0f, raw_gyro_z = 0.0f;

    bool accel_ok = imu.getAccelerometer(raw_acc_x, raw_acc_y, raw_acc_z);
    bool gyro_ok  = imu.getGyroscope(raw_gyro_x, raw_gyro_y, raw_gyro_z);
    float mapped_acc_x = 0.0f, mapped_acc_y = 0.0f, mapped_acc_z = 0.0f;
    float mapped_gyro_x = 0.0f, mapped_gyro_y = 0.0f, mapped_gyro_z = 0.0f;

    if (accel_ok) {
      map_board_axes(raw_acc_x, raw_acc_y, raw_acc_z, mapped_acc_x, mapped_acc_y, mapped_acc_z);
      sensor.accX = mapped_acc_x;
      sensor.accY = mapped_acc_y;
      sensor.accZ = mapped_acc_z;
    }
    if (gyro_ok) {
      map_board_axes(raw_gyro_x - gyro_bias.x,
                     raw_gyro_y - gyro_bias.y,
                     raw_gyro_z - gyro_bias.z,
                     mapped_gyro_x, mapped_gyro_y, mapped_gyro_z);
      sensor.gyroX = mapped_gyro_x;
      sensor.gyroY = mapped_gyro_y;
      sensor.gyroZ = mapped_gyro_z;
    }

    if (accel_ok || gyro_ok) {
      sensor.temp = imu.getTemperature_C();
    }

    static uint32_t lastPrint = 0;
    if ((accel_ok || gyro_ok) && millis() - lastPrint > 100) {
      Serial.printf("Acc: %.2f, %.2f, %.2f | Gyro: %.1f, %.1f, %.1f | T: %.1f C\n",
                    sensor.accX, sensor.accY, sensor.accZ,
                    sensor.gyroX, sensor.gyroY, sensor.gyroZ,
                    sensor.temp);
      lastPrint = millis();
    }
  }

  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate > 33) {
    update_sensor_ui();
    lastUpdate = millis();
  }

  screen.routine();
  delay(5);
}