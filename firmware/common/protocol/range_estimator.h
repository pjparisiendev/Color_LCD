#ifndef RANGE_ESTIMATOR_H
#define RANGE_ESTIMATOR_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint16_t average_range_x10_km; uint16_t instant_range_x10_km; uint16_t instant_efficiency_x10_wh_km; bool average_valid; bool instant_valid; } range_estimate_t;
range_estimate_t range_estimate_calculate(uint32_t remaining_wh_x10, uint16_t average_efficiency_x10_wh_km, bool average_efficiency_valid, uint16_t power_w, uint16_t speed_x10_kph, bool telemetry_fresh);
#endif
