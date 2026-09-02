#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  EBIKE_PROTOCOL_NONE = 0,
  EBIKE_PROTOCOL_TSDZ2,
  EBIKE_PROTOCOL_BAFANG_UART,
  EBIKE_PROTOCOL_SIMULATED,
} ebike_protocol_kind_t;

typedef enum {
  EBIKE_LINK_UNKNOWN = 0,
  EBIKE_LINK_OK,
  EBIKE_LINK_STALE,
  EBIKE_LINK_LOST,
} ebike_link_status_t;

enum {
  EBIKE_TELEMETRY_SPEED             = (1UL << 0),
  EBIKE_TELEMETRY_BATTERY_VOLTAGE   = (1UL << 1),
  EBIKE_TELEMETRY_BATTERY_CURRENT   = (1UL << 2),
  EBIKE_TELEMETRY_BATTERY_POWER     = (1UL << 3),
  EBIKE_TELEMETRY_MOTOR_TEMPERATURE = (1UL << 4),
  EBIKE_TELEMETRY_ASSIST_LEVEL      = (1UL << 5),
  EBIKE_TELEMETRY_THROTTLE          = (1UL << 6),
  EBIKE_TELEMETRY_BRAKE             = (1UL << 7),
  EBIKE_TELEMETRY_ERROR_CODE        = (1UL << 8),
  EBIKE_TELEMETRY_BATTERY_SOC       = (1UL << 9),
  EBIKE_TELEMETRY_CONTROLLER_STATUS = (1UL << 10),
  EBIKE_TELEMETRY_NOMINAL_BATTERY_FIELD = (1UL << 11),
  EBIKE_TELEMETRY_SPEED_RAW         = (1UL << 12),
  EBIKE_TELEMETRY_MOVING           = (1UL << 13),
};

/*
 * Protocol-neutral application telemetry. Values use fixed-point integer
 * units so the display path does not require floating point or allocation.
 */
typedef struct {
  uint32_t valid_fields;
  uint16_t speed_x10_kph;
  uint16_t battery_voltage_x10;
  uint16_t battery_current_x10;
  uint16_t battery_power_w;
  int16_t motor_temperature_c;
  uint8_t assist_level;
  uint8_t throttle_percent;
  uint8_t brake_active;
  uint8_t error_code;
  uint8_t battery_soc_percent;
  uint8_t controller_status;
  uint8_t nominal_battery_field;
  uint16_t speed_raw;
  uint8_t moving;
  uint8_t age_100ms;
  ebike_protocol_kind_t protocol;
  ebike_link_status_t link_status;
} ebike_telemetry_t;

void protocol_telemetry_reset(ebike_protocol_kind_t protocol);
void protocol_telemetry_tick_100ms(void);
void protocol_telemetry_note_valid_rx(void);
void protocol_telemetry_publish(const ebike_telemetry_t *telemetry);
const ebike_telemetry_t *protocol_telemetry_get(void);
bool protocol_telemetry_is_fresh(void);
