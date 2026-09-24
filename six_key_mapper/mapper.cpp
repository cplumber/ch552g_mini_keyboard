#include <Arduino.h>

#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "mapper.h"

// Defined by the shared HID transport.  A mapper label needs two USB reports
// (key down, then key up), so do not overwrite a report that the host has not
// yet consumed.
extern volatile __xdata uint8_t UpPoint1_Busy;

typedef struct
{
  uint8_t pin;
  char label;
  bool use_for_recovery;
  bool stable_active;
  bool sample_active;
  uint8_t sample_count;
  unsigned long last_emit_ms;
} mapper_input_t;

// Do not use P3.6/P3.7: they are USB D+/D-.  P3.4 is sampled so a clone that
// puts a switch there is discoverable, but it is excluded from the software
// recovery guard because it is also a likely LED-data line and may be low at
// power-up without a key being held.
static mapper_input_t inputs_s[] = {
    {10, 'a', true, false, false, 0, 0},  // P1.0
    {11, 'b', true, false, false, 0, 0},  // P1.1
    {12, 'c', true, false, false, 0, 0},  // P1.2
    {13, 'd', true, false, false, 0, 0},  // P1.3
    {14, 'e', true, false, false, 0, 0},  // P1.4
    {15, 'f', true, false, false, 0, 0},  // P1.5 / SW2 on the reference board
    {16, 'g', true, false, false, 0, 0},  // P1.6
    {17, 'h', true, false, false, 0, 0},  // P1.7
    {30, 'i', true, false, false, 0, 0},  // P3.0
    {31, 'j', true, false, false, 0, 0},  // P3.1
    {32, 'k', true, false, false, 0, 0},  // P3.2
    {33, 'l', true, false, false, 0, 0},  // P3.3
    {35, 'n', true, false, false, 0, 0},  // P3.5
};

static const uint8_t debounce_samples_s = 4;
static const unsigned long emit_interval_ms_s = 20UL;
#define MAPPER_EVENT_QUEUE_SIZE 16
#define MAPPER_BOOT_MARKER 0xD3
static char event_queue_s[MAPPER_EVENT_QUEUE_SIZE];
static uint8_t event_queue_read_s = 0;
static uint8_t event_queue_write_s = 0;
static bool output_key_down_s = false;
static char output_key_s = 0;

bool mapper_should_enter_bootloader(void)
{
  if (RESET_KEEP == MAPPER_BOOT_MARKER)
  {
    return false;
  }

  // RESET_KEEP is retained through the CH552 ROM-loader return/reset path.
  // This makes the automatic request one-shot and avoids an endless loop when
  // the loader times out without receiving a download.
  RESET_KEEP = MAPPER_BOOT_MARKER;
  return true;
}

void mapper_clear_bootloader_marker(void)
{
  RESET_KEEP = 0;
}

void mapper_setup_inputs(void)
{
  for (uint8_t i = 0; i < sizeof(inputs_s) / sizeof(inputs_s[0]); ++i)
  {
    pinMode(inputs_s[i].pin, INPUT_PULLUP);
    const bool active = !digitalRead(inputs_s[i].pin);
    inputs_s[i].stable_active = active;
    inputs_s[i].sample_active = active;
  }
}

bool mapper_recovery_requested(void)
{
  for (uint8_t i = 0; i < sizeof(inputs_s) / sizeof(inputs_s[0]); ++i)
  {
    if (inputs_s[i].use_for_recovery && !digitalRead(inputs_s[i].pin))
    {
      return true;
    }
  }
  return false;
}

static void mapper_emit(char label)
{
  const uint8_t next = (event_queue_write_s + 1) % MAPPER_EVENT_QUEUE_SIZE;
  if (next == event_queue_read_s)
  {
    return; // A very fast/noisy input cannot corrupt a queued diagnostic event.
  }

  event_queue_s[event_queue_write_s] = label;
  event_queue_write_s = next;
}

static void mapper_flush_output(void)
{
  if (UpPoint1_Busy)
  {
    return;
  }

  if (output_key_down_s)
  {
    Keyboard_release((uint8_t)output_key_s);
    output_key_down_s = false;
    return;
  }

  if (event_queue_read_s == event_queue_write_s)
  {
    return;
  }

  output_key_s = event_queue_s[event_queue_read_s];
  event_queue_read_s = (event_queue_read_s + 1) % MAPPER_EVENT_QUEUE_SIZE;
  if (Keyboard_press((uint8_t)output_key_s))
  {
    output_key_down_s = true;
  }
}

void mapper_update(void)
{
  const unsigned long now = millis();

  mapper_flush_output();

  for (uint8_t i = 0; i < sizeof(inputs_s) / sizeof(inputs_s[0]); ++i)
  {
    mapper_input_t *input = &inputs_s[i];
    const bool active = !digitalRead(input->pin);

    if (active != input->sample_active)
    {
      input->sample_active = active;
      input->sample_count = 0;
      continue;
    }

    if (input->sample_count < debounce_samples_s)
    {
      ++input->sample_count;
      continue;
    }

    if (input->stable_active == input->sample_active)
    {
      continue;
    }

    input->stable_active = input->sample_active;
    if ((unsigned long)(now - input->last_emit_ms) >= emit_interval_ms_s)
    {
      input->last_emit_ms = now;
      mapper_emit(input->label);
    }
  }
}
