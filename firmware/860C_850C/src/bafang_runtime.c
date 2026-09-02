#include "bafang_runtime.h"

#ifdef TARGET_APT_850C_GD32F303RET6

#include <stddef.h>

#include "bafang_protocol.h"
#include "state.h"
#include "usart1.h"

static bafang_protocol_t protocol;
static uint8_t response[3];
static uint8_t response_length;

static uint8_t expected_response_length(void)
{
  switch (protocol.pending_command) {
    case BAFANG_READ_SPEED: return 3U;
    case BAFANG_READ_CURRENT: return 2U;
    case BAFANG_READ_STATUS: return 1U;
    case BAFANG_READ_BATTERY_PERCENT: return 2U;
    case BAFANG_READ_MOVING: return 2U;
    case BAFANG_READ_COUNT: default: return 0U;
  }
}

void bafang_runtime_init(void)
{
  bafang_protocol_init(&protocol, 2160U);
  response_length = 0U;
}

void bafang_runtime_tick(uint16_t elapsed_ms)
{
  uint8_t request[BAFANG_MAX_REQUEST_SIZE];
  size_t request_length = 0U;
  bafang_poll_result_t result;

  protocol.wheel_perimeter_mm = ui_vars.ui16_wheel_perimeter;
  bafang_protocol_tick(&protocol, elapsed_ms);
  result = bafang_protocol_next_read_only_poll(&protocol, request,
      &request_length);
  if (result == BAFANG_POLL_FRAME_READY)
    (void)usart1_send_bafang_read(request, (uint8_t)request_length);
  bafang_protocol_publish(&protocol);
}

void bafang_runtime_receive_byte(uint8_t byte)
{
  uint8_t expected;

  if (!protocol.waiting)
    return;
  expected = expected_response_length();
  if (expected == 0U)
    return;
  if (response_length < sizeof(response))
    response[response_length++] = byte;
  if (response_length == expected) {
    (void)bafang_protocol_accept_response(&protocol, response, response_length);
    response_length = 0U;
    bafang_protocol_publish(&protocol);
  }
}

#else

void bafang_runtime_init(void) {}
void bafang_runtime_tick(uint16_t elapsed_ms) { (void)elapsed_ms; }
void bafang_runtime_receive_byte(uint8_t byte) { (void)byte; }

#endif
