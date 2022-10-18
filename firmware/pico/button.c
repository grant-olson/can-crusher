#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "button.h"


int button_init() {
  gpio_init(USER_BUTTON_PIN);
  gpio_set_dir(USER_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(USER_BUTTON_PIN);

  return 0;
}

bool button_status() {
  return !gpio_get(USER_BUTTON_PIN); // Pulled low when pressed
}
