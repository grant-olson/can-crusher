#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "led.h"

void led_init() {
  gpio_init(LED_ONBOARD_PIN);
  gpio_set_dir(LED_ONBOARD_PIN, GPIO_OUT);

  gpio_init(LED_RED_PIN);
  gpio_set_dir(LED_RED_PIN, GPIO_OUT);
  gpio_put(LED_RED_PIN, 0);
  
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_put(LED_GREEN_PIN, 0);
  
  gpio_init(LED_BLUE_PIN);
  gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
  gpio_put(LED_BLUE_PIN, 0);
}

void led_display(uint mask) {
  gpio_put(LED_ONBOARD_PIN, (mask & LED_ONBOARD_MASK));
  gpio_put(LED_RED_PIN, (mask & LED_RED_MASK));
  gpio_put(LED_GREEN_PIN, (mask & LED_GREEN_MASK));
  gpio_put(LED_BLUE_PIN, (mask & LED_BLUE_MASK));
}

void led_cycle() {
  led_display(LED_ONBOARD_MASK);
  sleep_ms(250);
  led_display(LED_RED_MASK);
  sleep_ms(250);
  led_display(LED_GREEN_MASK);
  sleep_ms(250);
  led_display(LED_BLUE_MASK);
  sleep_ms(250);
  led_display(LED_NONE_MASK);
}

