#include "range_estimator.h"
#include <limits.h>
#define MIN_INSTANT_SPEED_X10_KPH 50
#define MIN_EFFICIENCY_X10_WH_KM 10
static uint16_t range_x10(uint32_t wh_x10, uint16_t wh_km_x10) { uint32_t value = (wh_x10 * 100u) / wh_km_x10; return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value; }
range_estimate_t range_estimate_calculate(uint32_t remaining_wh_x10, uint16_t average_efficiency_x10_wh_km, bool average_efficiency_valid, uint16_t power_w, uint16_t speed_x10_kph, bool telemetry_fresh) {
  range_estimate_t result = {0};
  if (average_efficiency_valid && average_efficiency_x10_wh_km >= MIN_EFFICIENCY_X10_WH_KM) { result.average_range_x10_km = range_x10(remaining_wh_x10, average_efficiency_x10_wh_km); result.average_valid = true; }
  if (telemetry_fresh && speed_x10_kph >= MIN_INSTANT_SPEED_X10_KPH && power_w > 0) { uint32_t efficiency = ((uint32_t)power_w * 100u) / speed_x10_kph; if (efficiency >= MIN_EFFICIENCY_X10_WH_KM && efficiency <= UINT16_MAX) { result.instant_efficiency_x10_wh_km = (uint16_t)efficiency; result.instant_range_x10_km = range_x10(remaining_wh_x10, result.instant_efficiency_x10_wh_km); result.instant_valid = true; } }
  return result;
}
