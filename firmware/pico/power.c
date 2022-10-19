#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "motor.h"
#include "power.h"

static bool is_enabled = false;

void power_enable() {
  gpio_put(ENABLE_12V_PIN, 1);
  is_enabled = true;
}

void power_disable() {
  motors_sleep(); // Just to force state update
  gpio_put(ENABLE_12V_PIN, 0);
  is_enabled = false;
}

bool power_is_enabled() {
  return is_enabled;
}

int power_init() {
  gpio_init(ENABLE_12V_PIN);
  gpio_set_dir(ENABLE_12V_PIN, GPIO_OUT);
  power_disable();

  return 0;
}
