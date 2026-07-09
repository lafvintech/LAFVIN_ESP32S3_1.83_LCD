/**
 * @file weather_client.h
 * @brief Weather fetch helpers for the watch demo.
 *
 * The project now uses OpenWeatherMap as the primary provider and keeps
 * Seniverse (Xinzhi) as a fallback provider.
 */

#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include "ui_common.h"

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// Primary provider: OpenWeatherMap
#ifndef WEATHER_API_KEY
  #define WEATHER_API_KEY "Your Openweather api key"
#endif
#ifndef WEATHER_API_HOST
  #define WEATHER_API_HOST "https://api.openweathermap.org"
#endif
#ifndef WEATHER_CITY
  #define WEATHER_CITY "London"
#endif
#ifndef WEATHER_CITY_DISPLAY
  #define WEATHER_CITY_DISPLAY "London"
#endif
#ifndef WEATHER_LANGUAGE
  #define WEATHER_LANGUAGE "en"
#endif
#ifndef WEATHER_UNITS
  #define WEATHER_UNITS "metric"
#endif
#ifndef WEATHER_TIMEOUT_MS
  #define WEATHER_TIMEOUT_MS 10000
#endif

// Backup provider: Seniverse / Xinzhi
#ifndef XINZHI_API_KEY
  #define XINZHI_API_KEY "your_xinzhi_api_key_here"
#endif
#ifndef XINZHI_API_HOST
  #define XINZHI_API_HOST "https://api.seniverse.com"
#endif
#ifndef XINZHI_LANGUAGE
  #define XINZHI_LANGUAGE "en"
#endif
#ifndef XINZHI_UNIT
  #define XINZHI_UNIT "c"
#endif

/**
 * @brief Fetch current weather.
 *
 * The function tries OpenWeatherMap first. If that fails and a Seniverse key is
 * configured, it falls back to Seniverse.
 */
bool weather_client_fetch(WeatherData *data);

/**
 * @brief Fetch current weather from OpenWeatherMap.
 */
bool weather_client_fetch_openweathermap(WeatherData *data);

/**
 * @brief Fetch current weather from Seniverse (backup provider).
 */
bool weather_client_fetch_xinzhi(WeatherData *data);

/**
 * @brief Fill WeatherData with local demo values when network fetch is not used.
 */
void weather_client_generate_demo(WeatherData *data);

#ifdef __cplusplus
}
#endif

#endif  // WEATHER_CLIENT_H
