#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

#include <stdbool.h>
#include <stdint.h>
#include "range_estimator.h"

typedef struct {
  uint32_t trip_energy_x10_wh;
  uint16_t average_efficiency_x10_wh_km;
  uint16_t instant_efficiency_x10_wh_km;
  uint16_t average_range_x10_km;
  uint16_t instant_range_x10_km;
  uint16_t peak_power_w;
  uint16_t peak_current_x10_a;
  uint16_t peak_speed_x10_kph;
  uint8_t peak_temperature_c;
  bool average_valid;
  bool instant_valid;
  bool temperature_valid;
} instrumentation_t;

extern instrumentation_t g_instrumentation;

void instrumentation_reset(void);
void instrumentation_update_100ms(uint16_t power_w,
                                  uint16_t current_x10_a,
                                  uint16_t speed_x10_kph,
                                  uint8_t temperature_c,
                                  bool temperature_valid,
                                  bool telemetry_fresh,
                                  uint32_t trip_distance_x1000_km,
                                  uint32_t remaining_energy_x10_wh);

#endif
