#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bafang_protocol.h"

static void accept_for_pending(bafang_protocol_t *protocol) {
  switch (protocol->pending_command) {
    case BAFANG_READ_SPEED: {
      const uint8_t response[] = { 0x00u, 0x14u, 0x34u };
      assert(bafang_protocol_accept_response(protocol, response,
          sizeof(response)));
      break;
    }
    case BAFANG_READ_CURRENT: {
      const uint8_t response[] = { 0x02u, 0x02u };
      assert(bafang_protocol_accept_response(protocol, response,
          sizeof(response)));
      break;
    }
    case BAFANG_READ_STATUS: {
      const uint8_t response[] = { 0x01u };
      assert(bafang_protocol_accept_response(protocol, response,
          sizeof(response)));
      break;
    }
    case BAFANG_READ_BATTERY_PERCENT: {
      const uint8_t response[] = { 0x64u, 0x64u };
      assert(bafang_protocol_accept_response(protocol, response,
          sizeof(response)));
      break;
    }
    case BAFANG_READ_MOVING: {
      const uint8_t response[] = { 0x01u, 0x01u };
      assert(bafang_protocol_accept_response(protocol, response,
          sizeof(response)));
      break;
    }
    case BAFANG_READ_COUNT:
      assert(0);
  }
}

static void next_ready(bafang_protocol_t *protocol,
    uint8_t request[BAFANG_MAX_REQUEST_SIZE], size_t *length) {
  bafang_poll_result_t result;
  do {
    result = bafang_protocol_next_read_only_poll(protocol, request, length);
  } while (result == BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL);
  assert(result == BAFANG_POLL_FRAME_READY);
}

static void test_captured_read_only_schedule(void) {
  static const uint8_t core[][2] = {
    { 0x11u, 0x20u }, { 0x11u, 0x0au }, { 0x11u, 0x08u }
  };
  bafang_protocol_t protocol;
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t length;
  unsigned block;
  unsigned phase;
  unsigned suppressed = 0;

  bafang_protocol_init(&protocol, 2070u);
  for (block = 0; block < 5u; block++) {
    for (phase = 0; phase < 3u; phase++) {
      assert(bafang_protocol_next_read_only_poll(&protocol, request,
          &length) == BAFANG_POLL_FRAME_READY);
      assert(length == 2u);
      assert(memcmp(request, core[phase], 2u) == 0);
      assert(request[0] != 0x16u);
      accept_for_pending(&protocol);
    }

    if (block < 3u || block == 4u) {
      assert(bafang_protocol_next_read_only_poll(&protocol, request,
          &length) == BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL);
      assert(length == 0u);
      suppressed++;
    } else {
      assert(bafang_protocol_next_read_only_poll(&protocol, request,
          &length) == BAFANG_POLL_FRAME_READY);
      assert(request[0] == 0x11u);
      if (block == 3u) {
        const uint8_t expected[] = { 0x11u, 0x11u };
        assert(length == sizeof(expected));
        assert(memcmp(request, expected, sizeof(expected)) == 0);
      }
      accept_for_pending(&protocol);
    }
  }
  assert(suppressed == 4u);
}

static void test_capture_derived_responses(void) {
  static const uint8_t speed_frames[][3] = {
    { 0x00u, 0x14u, 0x34u },
    { 0x00u, 0xa0u, 0xc0u },
    { 0x00u, 0xbeu, 0xdeu },
    { 0x00u, 0x00u, 0x20u },
  };
  bafang_protocol_t protocol;
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t length;
  size_t i;

  for (i = 0; i < sizeof(speed_frames) / sizeof(speed_frames[0]); i++) {
    bafang_protocol_init(&protocol, 2070u);
    assert(bafang_protocol_next_read_only_poll(&protocol, request,
        &length) == BAFANG_POLL_FRAME_READY);
    assert(bafang_protocol_accept_response(&protocol, speed_frames[i], 3u));
    assert(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_SPEED_RAW);
  }
  assert(protocol.telemetry.speed_raw == 0u);
  assert(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_SPEED);
  assert(protocol.telemetry.speed_x10_kph == 0u);

  /* Capture peak: raw wheel RPM 240 at about 31.1 km/h, 28-inch/2160 mm. */
  bafang_protocol_init(&protocol, 2160u);
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  assert(bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0x00u, 0xf0u, 0x10u }, 3u));
  assert(protocol.telemetry.speed_raw == 240u);
  assert(protocol.telemetry.speed_x10_kph == 311u);

  bafang_protocol_init(&protocol, 2070u);
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  accept_for_pending(&protocol); /* speed */
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  {
    const uint8_t current[] = { 0x28u, 0x28u };
    assert(bafang_protocol_accept_response(&protocol, current,
        sizeof(current)));
    assert(protocol.telemetry.battery_current_x10 == 200u);
  }
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  {
    const uint8_t status[] = { 0x01u };
    assert(bafang_protocol_accept_response(&protocol, status,
        sizeof(status)));
    assert(protocol.telemetry.controller_status == 0x01u);
    assert(!(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_BRAKE));
    assert(!(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_ASSIST_LEVEL));
  }

  /* Advance across the three suppressed control slots to captured 11 11. */
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL);
  for (i = 0; i < 2u; i++) {
    unsigned core;
    for (core = 0; core < 3u; core++) {
      assert(bafang_protocol_next_read_only_poll(&protocol, request,
          &length) == BAFANG_POLL_FRAME_READY);
      accept_for_pending(&protocol);
    }
    assert(bafang_protocol_next_read_only_poll(&protocol, request,
        &length) == BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL);
  }
  for (i = 0; i < 3u; i++) {
    assert(bafang_protocol_next_read_only_poll(&protocol, request,
        &length) == BAFANG_POLL_FRAME_READY);
    accept_for_pending(&protocol);
  }
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  assert(request[0] == 0x11u && request[1] == 0x11u);
  {
    const uint8_t nominal[] = { 0x64u, 0x64u };
    assert(bafang_protocol_accept_response(&protocol, nominal,
        sizeof(nominal)));
    assert(protocol.telemetry.nominal_battery_field == 100u);
    assert(protocol.telemetry.battery_soc_percent == 100u);
    assert(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_BATTERY_SOC);
  }
}

static void test_malformed_timeout_and_recovery(void) {
  bafang_protocol_t protocol;
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t length;
  unsigned timeout;

  bafang_protocol_init(&protocol, 2070u);
  assert(!bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0u }, 1u)); /* unsolicited/unknown */

  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  assert(!bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0x00u, 0x14u }, 2u)); /* truncated */
  assert(!bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0x00u, 0x14u, 0x35u }, 3u)); /* checksum */
  bafang_protocol_tick(&protocol, BAFANG_RESPONSE_TIMEOUT_MS);
  assert(protocol.state == BAFANG_LINK_TIMEOUT);
  assert(!(protocol.telemetry.valid_fields &
      (EBIKE_TELEMETRY_SPEED_RAW | EBIKE_TELEMETRY_SPEED)));

  /* Valid response recovers and resets consecutive timeout count. */
  assert(bafang_protocol_next_read_only_poll(&protocol, request,
      &length) == BAFANG_POLL_FRAME_READY);
  assert(!bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0x02u, 0x03u }, 2u));
  assert(bafang_protocol_accept_response(&protocol,
      (const uint8_t[]) { 0x02u, 0x02u }, 2u));
  assert(protocol.state == BAFANG_LINK_ACTIVE);
  assert(protocol.consecutive_timeouts == 0u);

  for (timeout = 0; timeout < BAFANG_LOST_AFTER_TIMEOUTS; timeout++) {
    next_ready(&protocol, request, &length);
    bafang_protocol_tick(&protocol, BAFANG_RESPONSE_TIMEOUT_MS);
  }
  assert(protocol.state == BAFANG_LINK_LOST);
  assert(protocol.telemetry.link_status == EBIKE_LINK_LOST);
  assert(!(protocol.telemetry.valid_fields &
      (EBIKE_TELEMETRY_SPEED_RAW | EBIKE_TELEMETRY_BATTERY_CURRENT |
       EBIKE_TELEMETRY_CONTROLLER_STATUS)));
}

static void test_captured_control_vectors_are_not_generated(void) {
  static const uint8_t captured_only[][5] = {
    { 0x16u, 0x1au, 0xf1u, 0x00u, 0x00u },
    { 0x16u, 0x0bu, 0x0bu, 0x2cu, 0x00u },
    { 0x16u, 0x1fu, 0x02u, 0xe0u, 0x17u },
  };
  static const uint8_t lengths[] = { 3u, 4u, 5u };
  bafang_protocol_t protocol;
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t length;
  unsigned i;
  unsigned emitted = 0;

  /* Preserve the exact write vectors as disabled capture-derived fixtures. */
  for (i = 0; i < 3u; i++) {
    assert(lengths[i] >= 3u && lengths[i] <= 5u);
    assert(captured_only[i][0] == 0x16u);
  }

  bafang_protocol_init(&protocol, 2070u);
  for (i = 0; i < 40u; i++) {
    bafang_poll_result_t result = bafang_protocol_next_read_only_poll(
        &protocol, request, &length);
    if (result == BAFANG_POLL_FRAME_READY) {
      assert(length >= 2u && length <= BAFANG_MAX_REQUEST_SIZE);
      assert(request[0] == 0x11u);
      assert(request[1] != 0x22u && request[1] != 0x24u);
      assert(request[1] == 0x08u || request[1] == 0x0au ||
          request[1] == 0x11u || request[1] == 0x20u ||
          request[1] == 0x31u);
      emitted++;
      accept_for_pending(&protocol);
    } else {
      assert(result == BAFANG_POLL_SUPPRESSED_FACTORY_CONTROL);
      assert(length == 0u);
    }
  }
  assert(emitted > 0u);
}

static void test_per_field_staleness(void) {
  bafang_protocol_t protocol;
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t length;

  bafang_protocol_init(&protocol, 2070u);
  assert(bafang_protocol_next_read_only_poll(&protocol, request, &length) ==
      BAFANG_POLL_FRAME_READY);
  accept_for_pending(&protocol);
  assert(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_SPEED_RAW);
  bafang_protocol_tick(&protocol, BAFANG_FIELD_STALE_MS - 1u);
  assert(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_SPEED_RAW);
  bafang_protocol_tick(&protocol, 1u);
  assert(!(protocol.telemetry.valid_fields & EBIKE_TELEMETRY_SPEED_RAW));
}

int main(void) {
  test_captured_read_only_schedule();
  test_capture_derived_responses();
  test_malformed_timeout_and_recovery();
  test_captured_control_vectors_are_not_generated();
  test_per_field_staleness();
  return 0;
}
