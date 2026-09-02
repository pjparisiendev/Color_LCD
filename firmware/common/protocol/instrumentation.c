#include "instrumentation.h"

#include <string.h>

#define MIN_AVERAGE_DISTANCE_X1000_KM 500u

instrumentation_t g_instrumentation;
static uint16_t energy_remainder;
static uint32_t previous_trip_distance_x1000_km;

void instrumentation_reset(void) {
  memset(&g_instrumentation, 0, sizeof(g_instrumentation));
  energy_remainder = 0;
  previous_trip_distance_x1000_km = 0;
}

void instrumentation_update_100ms(uint16_t power_w,
                                  uint16_t current_x10_a,
                                  uint16_t speed_x10_kph,
                                  uint8_t temperature_c,
                                  bool temperature_valid,
                                  bool telemetry_fresh,
                                  uint32_t trip_distance_x1000_km,
                                  uint32_t remaining_energy_x10_wh) {
  if (trip_distance_x1000_km < previous_trip_distance_x1000_km)
    instrumentation_reset();
  previous_trip_distance_x1000_km = trip_distance_x1000_km;

  if (telemetry_fresh) {
    /* Called at 10 Hz: 1 Wh x10 = 3600 watt-ticks. */
    uint32_t accumulated = (uint32_t)energy_remainder + power_w;
    g_instrumentation.trip_energy_x10_wh += accumulated / 3600u;
    energy_remainder = (uint16_t)(accumulated % 3600u);

    if (power_w > g_instrumentation.peak_power_w)
      g_instrumentation.peak_power_w = power_w;
    if (current_x10_a > g_instrumentation.peak_current_x10_a)
      g_instrumentation.peak_current_x10_a = current_x10_a;
    if (speed_x10_kph > g_instrumentation.peak_speed_x10_kph)
      g_instrumentation.peak_speed_x10_kph = speed_x10_kph;
    if (temperature_valid && temperature_c > g_instrumentation.peak_temperature_c)
      g_instrumentation.peak_temperature_c = temperature_c;
  }
  g_instrumentation.temperature_valid = temperature_valid;

  bool average_valid = trip_distance_x1000_km >= MIN_AVERAGE_DISTANCE_X1000_KM;
  uint32_t average = average_valid
      ? (g_instrumentation.trip_energy_x10_wh * 1000u) / trip_distance_x1000_km
      : 0;
  if (average > UINT16_MAX) average = UINT16_MAX;

  range_estimate_t range = range_estimate_calculate(
      remaining_energy_x10_wh, (uint16_t)average, average_valid,
      power_w, speed_x10_kph, telemetry_fresh);
  g_instrumentation.average_efficiency_x10_wh_km = (uint16_t)average;
  g_instrumentation.instant_efficiency_x10_wh_km = range.instant_efficiency_x10_wh_km;
  g_instrumentation.average_range_x10_km = range.average_range_x10_km;
  g_instrumentation.instant_range_x10_km = range.instant_range_x10_km;
  g_instrumentation.average_valid = range.average_valid;
  g_instrumentation.instant_valid = range.instant_valid;
}
