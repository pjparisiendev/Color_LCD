#include "bafang_protocol.h"

#include <string.h>

#define BAFANG_READ_PREFIX 0x11u

typedef enum {
  SLOW_FACTORY_LIGHTS = 0,
  SLOW_FACTORY_PAS,
  SLOW_FACTORY_SPEED_LIMIT,
  SLOW_READ_BATTERY_PERCENT,
  SLOW_FACTORY_EXPERIMENTAL_22,
  SLOW_SLOT_COUNT,
} slow_slot_t;

static bool checksum_data(const uint8_t *response, size_t data_length) {
  uint8_t checksum = 0;
  size_t i;

  for (i = 0; i < data_length; i++)
    checksum = (uint8_t) (checksum + response[i]);
  return checksum == response[data_length];
}

static void update_power(ebike_telemetry_t *telemetry) {
  if ((telemetry->valid_fields & (EBIKE_TELEMETRY_BATTERY_VOLTAGE |
      EBIKE_TELEMETRY_BATTERY_CURRENT)) ==
      (EBIKE_TELEMETRY_BATTERY_VOLTAGE |
      EBIKE_TELEMETRY_BATTERY_CURRENT)) {
    uint32_t watts = ((uint32_t) telemetry->battery_voltage_x10 *
        telemetry->battery_current_x10) / 100u;
    telemetry->battery_power_w = watts > UINT16_MAX ? UINT16_MAX :
        (uint16_t) watts;
    telemetry->valid_fields |= EBIKE_TELEMETRY_BATTERY_POWER;
  }
}

static void begin_request(bafang_protocol_t *protocol,
    bafang_read_command_t command, uint8_t opcode,
    uint8_t request[BAFANG_MAX_REQUEST_SIZE], size_t length) {
  protocol->pending_command = command;
  protocol->response_age_ms = 0;
  protocol->waiting = 1;
  protocol->state = BAFANG_LINK_WAITING_FOR_RESPONSE;
  request[0] = BAFANG_READ_PREFIX;
  request[1] = opcode;
  if (length == 3u)
    request[2] = (uint8_t) (request[0] + request[1]);
}

static void invalidate_field(bafang_protocol_t *protocol,
    bafang_read_command_t command) {
  uint32_t fields = 0;

  switch (command) {
    case BAFANG_READ_SPEED:
      fields = EBIKE_TELEMETRY_SPEED_RAW | EBIKE_TELEMETRY_SPEED;
      break;
    case BAFANG_READ_CURRENT:
      fields = EBIKE_TELEMETRY_BATTERY_CURRENT |
          EBIKE_TELEMETRY_BATTERY_POWER;
      break;
    case BAFANG_READ_STATUS:
      fields = EBIKE_TELEMETRY_CONTROLLER_STATUS;
      break;
    case BAFANG_READ_BATTERY_PERCENT:
      fields = EBIKE_TELEMETRY_BATTERY_SOC;
      break;
    case BAFANG_READ_MOVING:
      fields = EBIKE_TELEMETRY_MOVING;
      break;
    case BAFANG_READ_COUNT:
      break;
  }
  protocol->telemetry.valid_fields &= ~fields;
}

void bafang_protocol_init(bafang_protocol_t *protocol,
    uint16_t wheel_perimeter_mm) {
  memset(protocol, 0, sizeof(*protocol));
  protocol->wheel_perimeter_mm = wheel_perimeter_mm;
  protocol->state = BAFANG_LINK_NOT_INITIALIZED;
  protocol->telemetry.protocol = EBIKE_PROTOCOL_BAFANG_UART;
  protocol->telemetry.link_status = EBIKE_LINK_UNKNOWN;
  {
    unsigned i;
    for (i = 0; i < BAFANG_READ_COUNT; i++)
      protocol->field_age_ms[i] = UINT16_MAX;
  }
}

bafang_poll_result_t bafang_protocol_next_read_only_poll(
    bafang_protocol_t *protocol, uint8_t request[BAFANG_MAX_REQUEST_SIZE],
    size_t *request_length) {
  uint8_t phase;

  if (request_length == NULL || request == NULL)
    return BAFANG_POLL_WAITING;
  *request_length = 0;
  if (protocol->waiting)
    return BAFANG_POLL_WAITING;

  phase = protocol->poll_phase;
  protocol->poll_phase = (uint8_t) ((protocol->poll_phase + 1u) % 4u);
  if (phase == 0u) {
    begin_request(protocol, BAFANG_READ_SPEED, 0x20u, request, 2u);
    *request_length = 2u;
  } else if (phase == 1u) {
    begin_request(protocol, BAFANG_READ_CURRENT, 0x0au, request, 2u);
    *request_length = 2u;
  } else if (phase == 2u) {
    begin_request(protocol, BAFANG_READ_STATUS, 0x08u, request, 2u);
    *request_length = 2u;
  } else {
    uint8_t slow = protocol->slow_slot;
    protocol->slow_slot = (uint8_t) ((protocol->slow_slot + 1u) %
        SLOW_SLOT_COUNT);
    if (slow == SLOW_READ_BATTERY_PERCENT) {
      begin_request(protocol, BAFANG_READ_BATTERY_PERCENT, 0x11u,
          request, 2u);
      *request_length = 2u;
    } else {
      return BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL;
    }
  }
  return BAFANG_POLL_FRAME_READY;
}

bool bafang_protocol_accept_response(bafang_protocol_t *protocol,
    const uint8_t *response, size_t length) {
  ebike_telemetry_t *telemetry = &protocol->telemetry;
  uint16_t raw;

  if (!protocol->waiting || response == NULL)
    return false;

  switch (protocol->pending_command) {
    case BAFANG_READ_SPEED:
      if (length != 3u ||
          (uint8_t) (response[0] + response[1] + 0x20u) != response[2])
        return false;
      raw = (uint16_t) (((uint16_t) response[0] << 8) | response[1]);
      telemetry->speed_raw = raw;
      /* Captured BBS02B value is wheel RPM. Convert using configured mm. */
      telemetry->speed_x10_kph = (uint16_t) ((((uint32_t) raw *
          protocol->wheel_perimeter_mm * 3u) + 2500u) / 5000u);
      telemetry->valid_fields |= EBIKE_TELEMETRY_SPEED_RAW |
          EBIKE_TELEMETRY_SPEED;
      break;

    case BAFANG_READ_CURRENT:
      if (length != 2u || !checksum_data(response, 1u) || response[0] > 200u)
        return false;
      /* Documentation-derived 0.5 A/count; loaded-bike validation pending. */
      telemetry->battery_current_x10 = (uint16_t) response[0] * 5u;
      telemetry->valid_fields |= EBIKE_TELEMETRY_BATTERY_CURRENT;
      update_power(telemetry);
      break;

    case BAFANG_READ_STATUS:
      if (length != 1u)
        return false;
      /* Captured 0x01 is a status value, not a proven pedaling indication. */
      telemetry->controller_status = response[0];
      telemetry->valid_fields |= EBIKE_TELEMETRY_CONTROLLER_STATUS;
      break;

    case BAFANG_READ_BATTERY_PERCENT:
      if (length != 2u || !checksum_data(response, 1u))
        return false;
      telemetry->nominal_battery_field = response[0];
      if (response[0] > 100u)
        return false;
      telemetry->battery_soc_percent = response[0];
      telemetry->valid_fields |= EBIKE_TELEMETRY_BATTERY_SOC;
      break;

    case BAFANG_READ_MOVING:
      if (length != 2u || !checksum_data(response, 1u))
        return false;
      telemetry->moving = response[0] != 0u;
      telemetry->valid_fields |= EBIKE_TELEMETRY_MOVING;
      break;

    case BAFANG_READ_COUNT:
    default:
      return false;
  }

  protocol->waiting = 0;
  protocol->response_age_ms = 0;
  protocol->consecutive_timeouts = 0;
  protocol->state = BAFANG_LINK_ACTIVE;
  protocol->field_age_ms[protocol->pending_command] = 0U;
  telemetry->age_100ms = 0;
  telemetry->link_status = EBIKE_LINK_OK;
  return true;
}

void bafang_protocol_tick(bafang_protocol_t *protocol, uint16_t elapsed_ms) {
  uint32_t age;
  unsigned i;

  for (i = 0; i < BAFANG_READ_COUNT; i++) {
    if (protocol->field_age_ms[i] != UINT16_MAX) {
      age = (uint32_t)protocol->field_age_ms[i] + elapsed_ms;
      protocol->field_age_ms[i] = age > UINT16_MAX ? UINT16_MAX : (uint16_t)age;
      if (protocol->field_age_ms[i] >= BAFANG_FIELD_STALE_MS)
        invalidate_field(protocol, (bafang_read_command_t)i);
    }
  }

  if (protocol->telemetry.age_100ms < UINT8_MAX) {
    uint16_t ticks = (uint16_t) ((elapsed_ms + 99u) / 100u);
    age = (uint32_t) protocol->telemetry.age_100ms + ticks;
    protocol->telemetry.age_100ms = age > UINT8_MAX ? UINT8_MAX :
        (uint8_t) age;
  }
  if (!protocol->waiting)
    return;

  age = (uint32_t) protocol->response_age_ms + elapsed_ms;
  protocol->response_age_ms = age > UINT16_MAX ? UINT16_MAX : (uint16_t) age;
  if (protocol->response_age_ms < BAFANG_RESPONSE_TIMEOUT_MS)
    return;

  invalidate_field(protocol, protocol->pending_command);
  protocol->waiting = 0;
  if (protocol->consecutive_timeouts < UINT8_MAX)
    protocol->consecutive_timeouts++;
  protocol->state = protocol->consecutive_timeouts >=
      BAFANG_LOST_AFTER_TIMEOUTS ? BAFANG_LINK_LOST : BAFANG_LINK_TIMEOUT;
  protocol->telemetry.link_status = protocol->state == BAFANG_LINK_LOST ?
      EBIKE_LINK_LOST : EBIKE_LINK_STALE;
  if (protocol->state == BAFANG_LINK_LOST)
    protocol->telemetry.valid_fields &= ~(
        EBIKE_TELEMETRY_SPEED_RAW | EBIKE_TELEMETRY_SPEED |
        EBIKE_TELEMETRY_BATTERY_CURRENT |
        EBIKE_TELEMETRY_BATTERY_POWER | EBIKE_TELEMETRY_CONTROLLER_STATUS |
        EBIKE_TELEMETRY_BATTERY_SOC | EBIKE_TELEMETRY_MOVING);
}

void bafang_protocol_publish(const bafang_protocol_t *protocol) {
  protocol_telemetry_publish(&protocol->telemetry);
  if (protocol->telemetry.link_status == EBIKE_LINK_OK)
    protocol_telemetry_note_valid_rx();
}
