# xiaozhi-esp32 手表UI抽取模块

## 概述

本项目从 `xiaozhi-esp32` 小智AI语音助手项目中抽取了以下3个UI页面：
- **表盘页面 (WatchFace)** - 时间、日期、步数
- **菜单页面 (Menu)** - 2x2 图标网格导航
- **天气页面 (Weather)** - 温度、湿度、7天预报图表

同时包含页面切换管理和天气数据获取模块。

## 文件结构

```
extracted/
├── ui_common.h          # 公共定义：颜色、字体、页面索引、天气数据结构
├── ui_common.cpp        # 全局变量定义
├── watchface.h/.cpp     # 表盘页面
├── menu.h/.cpp          # 菜单页面
├── weather_page.h/.cpp  # 天气页面（UI部分）
├── page_manager.h/.cpp  # 页面管理器（水平滚动、页面切换）
├── weather_client.h/.cpp # 天气获取（心知天气/和风天气）
└── main.ino             # Arduino 示例主文件
```

## 依赖

### 必需
- **LVGL** v9.x（当前版本，后续需适配 v8.4）
- **Arduino ESP32 Core**（支持 ESP32-S3）
- **WiFi**（Arduino 内置）

### 天气功能需要
- **HTTPClient**（Arduino ESP32 内置）
- **ArduinoJson** v6.x（可通过 Arduino Library Manager 安装）

### 显示驱动（根据你的硬件选择）
- **TFT_eSPI**（推荐，支持多种 LCD 驱动）
- 或 **LovyanGFX**
- 或 **Arduino_GFX**

## 快速开始

### 1. 创建字体

你需要生成以下字体文件：

```
ui_font_text       - 正文字体（建议：Montserrat 16 + 中文字符子集）
ui_font_icon       - 图标字体（FontAwesome 16px Solid）
ui_font_large_icon - 大图标字体（FontAwesome 24px Solid）
```

使用 LVGL 字体转换工具生成：
- 工具地址：https://lvgl.io/tools/fontconverter
- 或离线工具：`lvgl/scripts/built_in_font` 中的 `lv_font_conv`

**FontAwesome 图标字体必须包含以下 Unicode 码点**：
- `\xef\x80\x84` - heart
- `\xef\x86\x85` - sun
- `\xef\x8d\x8e` - alarm-clock
- `\xef\x80\x93` - gear

### 2. 配置天气 API

编辑 `weather_client.h`：

```cpp
#define WEATHER_API_KEY  "your_api_key_here"   // 心知天气私钥
#define WEATHER_CITY     "guangzhou"           // 城市拼音
```

注册心知天气获取 API Key：https://www.seniverse.com/

### 3. 配置 WiFi

编辑 `main.ino`：

```cpp
const char* WIFI_SSID = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";
```

### 4. 修改显示驱动

根据你的 LCD 型号修改 `main.ino` 中的显示驱动代码：
- 修改 `TFT_eSPI` 初始化参数
- 修改引脚配置
- 修改触摸驱动（如有）

### 5. 编译上传

1. 将所有文件放入 Arduino 项目目录
2. 安装依赖库（ArduinoJson、TFT_eSPI 等）
3. 选择开发板：**ESP32S3 Dev Module**
4. 编译上传

## 适配 LVGL v8.4 指南

以下是需要修改的 LVGL v9 → v8 API：

### 1. lv_obj_set_style_pad_hor() / pad_ver()
```cpp
// v9
lv_obj_set_style_pad_hor(obj, pad, part);
lv_obj_set_style_pad_ver(obj, pad, part);

// v8
lv_obj_set_style_pad_left(obj, pad, part);
lv_obj_set_style_pad_right(obj, pad, part);
lv_obj_set_style_pad_top(obj, pad, part);
lv_obj_set_style_pad_bottom(obj, pad, part);
```

### 2. lv_chart_set_value_by_id()
```cpp
// v9
lv_chart_set_value_by_id(chart, series, id, value);

// v8
lv_chart_set_next_value(chart, series, value);
// 或重新设置所有点
```

### 3. lv_img_dsc_t / LV_IMAGE_HEADER_MAGIC
```cpp
// v9
lv_img_dsc_t dsc;
dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
dsc.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;

// v8
lv_img_dsc_t dsc;
dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
// v8 没有 magic 字段
```

### 4. lv_obj_set_style_bg_image_src()
```cpp
// v9
lv_obj_set_style_bg_image_src(obj, &img_dsc, part);

// v8
// 需要创建 lv_img 对象作为背景
lv_obj_t* bg = lv_img_create(obj);
lv_img_set_src(bg, &img_dsc);
lv_obj_move_background(bg);
```

### 5. 显示初始化
```cpp
// v9
lv_display_t* disp = lv_display_create(width, height);
lv_display_set_flush_cb(disp, flush_cb);
lv_display_set_buffers(disp, buf, nullptr, buf_size, mode);

// v8
lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = width;
disp_drv.ver_res = height;
disp_drv.flush_cb = flush_cb;
disp_drv.draw_buf = &draw_buf;
disp_drv.user_data = ...;
lv_disp_drv_register(&disp_drv);
```

### 6. 输入设备初始化
```cpp
// v9
lv_indev_t* indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, read_cb);

// v8
lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = read_cb;
indev_drv.user_data = ...;
lv_indev_drv_register(&indev_drv);
```

### 7. 图像缓存（可选）
```cpp
// v9
lv_image_cache_resize(2 * 1024 * 1024, true);

// v8
// v8 没有内置图像缓存 API，需手动管理
```

## 自定义扩展

### 添加新页面

1. 在 `ui_common.h` 的 `PageIndex` 枚举中添加新页面索引
2. 创建新页面的 `xxx.h/.cpp` 文件
3. 在 `page_manager.cpp` 的 `page_manager_init()` 中调用初始化函数
4. 在 `menu.cpp` 的 `menu_items[]` 数组中添加菜单项

### 修改主题颜色

编辑 `ui_common.h` 中的颜色定义：

```cpp
#define UI_BG_COLOR     lv_color_hex(0x000000)   // 改为白色背景
#define UI_TEXT_COLOR   lv_color_hex(0xFFFFFF)   // 改为黑色文字
```

### 使用自定义字体

在 `main.ino` 中声明字体：

```cpp
LV_FONT_DECLARE(my_custom_font);
const lv_font_t ui_font_text = my_custom_font;
```

## 性能优化建议

对于 ESP32-S3 N16R8：

1. **使用 PSRAM 分配绘制缓冲区**：
   ```cpp
   draw_buf = (lv_color_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
   ```

2. **启用 LVGL 图像缓存**（v9）：
   ```cpp
   lv_image_cache_resize(2 * 1024 * 1024, true);  // 使用 2MB PSRAM
   ```

3. **降低刷新率**（如果需要省电）：
   ```cpp
   // 在 loop() 中增加延迟
   delay(20);  // 约 50fps
   ```

4. **天气更新间隔**：默认10分钟，可根据需要调整

## 许可证

抽取的代码遵循原项目许可证。

## 问题反馈

如有问题，请检查：
1. 字体是否正确生成并包含所需字形
2. WiFi 是否连接成功
3. 天气 API Key 是否配置正确
4. LVGL 版本是否与代码匹配

---

*抽取日期: 2026-05-06*  
*基于 xiaozhi-esp32 v2.0.3*
