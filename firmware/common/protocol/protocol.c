#include "protocol.h"

#include <string.h>

#define LINK_STALE_AFTER_100MS 5
#define LINK_LOST_AFTER_100MS  20

static ebike_telemetry_t g_telemetry;

void protocol_telemetry_reset(ebike_protocol_kind_t protocol) {
  memset(&g_telemetry, 0, sizeof(g_telemetry));
  g_telemetry.protocol = protocol;
  g_telemetry.link_status = EBIKE_LINK_UNKNOWN;
}

void protocol_telemetry_tick_100ms(void) {
  if (g_telemetry.age_100ms < UINT8_MAX)
    g_telemetry.age_100ms++;

  if (g_telemetry.age_100ms >= LINK_LOST_AFTER_100MS)
    g_telemetry.link_status = EBIKE_LINK_LOST;
  else if (g_telemetry.age_100ms >= LINK_STALE_AFTER_100MS)
    g_telemetry.link_status = EBIKE_LINK_STALE;
}

void protocol_telemetry_note_valid_rx(void) {
  g_telemetry.age_100ms = 0;
  g_telemetry.link_status = EBIKE_LINK_OK;
}

void protocol_telemetry_publish(const ebike_telemetry_t *telemetry) {
  uint8_t age = g_telemetry.age_100ms;
  ebike_link_status_t link_status = g_telemetry.link_status;

  g_telemetry = *telemetry;
  g_telemetry.age_100ms = age;
  g_telemetry.link_status = link_status;
}

const ebike_telemetry_t *protocol_telemetry_get(void) {
  return &g_telemetry;
}

bool protocol_telemetry_is_fresh(void) {
  return g_telemetry.link_status == EBIKE_LINK_OK;
}
