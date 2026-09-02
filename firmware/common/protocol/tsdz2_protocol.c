#include "tsdz2_protocol.h"

#include "protocol.h"

void tsdz2_protocol_publish_telemetry(const tsdz2_telemetry_input_t *input) {
  ebike_telemetry_t telemetry = {
    .valid_fields = EBIKE_TELEMETRY_SPEED |
                    EBIKE_TELEMETRY_BATTERY_VOLTAGE |
                    EBIKE_TELEMETRY_BATTERY_CURRENT |
                    EBIKE_TELEMETRY_BATTERY_POWER |
                    EBIKE_TELEMETRY_ASSIST_LEVEL |
                    EBIKE_TELEMETRY_BRAKE |
                    EBIKE_TELEMETRY_ERROR_CODE,
    .speed_x10_kph = input->speed_x10_kph,
    .battery_voltage_x10 = input->battery_voltage_x10,
    .battery_current_x10 = input->battery_current_x10,
    .battery_power_w = input->battery_power_w,
    .motor_temperature_c = input->motor_temperature_c,
    .assist_level = input->assist_level,
    .throttle_percent = input->throttle_percent,
    .brake_active = input->brake_active,
    .error_code = input->error_code,
    .protocol = input->simulated ? EBIKE_PROTOCOL_SIMULATED : EBIKE_PROTOCOL_TSDZ2,
  };

  if (input->temperature_valid)
    telemetry.valid_fields |= EBIKE_TELEMETRY_MOTOR_TEMPERATURE;
  if (input->throttle_valid)
    telemetry.valid_fields |= EBIKE_TELEMETRY_THROTTLE;

  protocol_telemetry_publish(&telemetry);
}

