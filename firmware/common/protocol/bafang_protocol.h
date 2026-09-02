#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

#define BAFANG_MAX_REQUEST_SIZE 3u
#define BAFANG_RESPONSE_TIMEOUT_MS 250u
#define BAFANG_LOST_AFTER_TIMEOUTS 3u
#define BAFANG_FIELD_STALE_MS 1000u

typedef enum {
  BAFANG_LINK_NOT_INITIALIZED = 0,
  BAFANG_LINK_WAITING_FOR_RESPONSE,
  BAFANG_LINK_ACTIVE,
  BAFANG_LINK_TIMEOUT,
  BAFANG_LINK_LOST,
} bafang_link_state_t;

typedef enum {
  BAFANG_READ_SPEED = 0,
  BAFANG_READ_CURRENT,
  BAFANG_READ_STATUS,
  BAFANG_READ_BATTERY_PERCENT,
  BAFANG_READ_MOVING,
  BAFANG_READ_COUNT,
} bafang_read_command_t;

typedef enum {
  BAFANG_POLL_FRAME_READY = 0,
  BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL,
  BAFANG_POLL_WAITING,
} bafang_poll_result_t;

typedef struct {
  ebike_telemetry_t telemetry;
  bafang_link_state_t state;
  bafang_read_command_t pending_command;
  uint16_t wheel_perimeter_mm;
  uint16_t response_age_ms;
  uint8_t poll_phase;
  uint8_t slow_slot;
  uint8_t consecutive_timeouts;
  uint8_t waiting;
  uint16_t field_age_ms[BAFANG_READ_COUNT];
} bafang_protocol_t;

/*
 * Captured Green Pedel E2.3 read-only scheduler. Every block contains speed,
 * current and status reads. Its fourth slot follows the factory five-slot
 * rotation. The three 0x16 controller frames and experimental 0x11/0x22 read
 * are suppressed. This API can therefore never produce an unverified request.
 */
void bafang_protocol_init(bafang_protocol_t *protocol,
    uint16_t wheel_perimeter_mm);
bafang_poll_result_t bafang_protocol_next_read_only_poll(
    bafang_protocol_t *protocol, uint8_t request[BAFANG_MAX_REQUEST_SIZE],
    size_t *request_length);
bool bafang_protocol_accept_response(bafang_protocol_t *protocol,
    const uint8_t *response, size_t length);
void bafang_protocol_tick(bafang_protocol_t *protocol, uint16_t elapsed_ms);
void bafang_protocol_publish(const bafang_protocol_t *protocol);
