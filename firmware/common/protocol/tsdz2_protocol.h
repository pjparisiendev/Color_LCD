#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t speed_x10_kph;
  uint16_t battery_voltage_x10;
  uint16_t battery_current_x10;
  uint16_t battery_power_w;
  int16_t motor_temperature_c;
  uint8_t assist_level;
  uint8_t throttle_percent;
  uint8_t brake_active;
  uint8_t error_code;
  bool temperature_valid;
  bool throttle_valid;
  bool simulated;
} tsdz2_telemetry_input_t;

void tsdz2_protocol_publish_telemetry(const tsdz2_telemetry_input_t *input);

