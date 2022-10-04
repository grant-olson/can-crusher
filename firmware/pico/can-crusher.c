#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

const uint LED_PIN = 25;

const uint LEFT_STEP_PIN = 15;
const uint LEFT_ENABLE_PIN = 14;
const uint LEFT_DIR_PIN = 13;

int main() {
  bi_decl(bi_program_description("Firmware for can crusher."));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

  stdio_init_all();

  gpio_init(LEFT_ENABLE_PIN);
  gpio_set_dir(LEFT_ENABLE_PIN, GPIO_OUT);
  gpio_put(LEFT_ENABLE_PIN, 1); // High, disable until explictly enabled.

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  gpio_init(LEFT_STEP_PIN);
  gpio_set_dir(LEFT_STEP_PIN, GPIO_OUT);
  gpio_put(LEFT_STEP_PIN, 0);

  gpio_init(LEFT_DIR_PIN);
  gpio_set_dir(LEFT_DIR_PIN, GPIO_OUT);
  gpio_put(LEFT_DIR_PIN, 0);
  
  puts("Can crusher initializing.\n");

  // Blink 10 times over 5 seconds to provide visual indication
  // that our code is actually running.
  for(int i=0;i<10;i++) {
    gpio_put(LED_PIN, 0);
    sleep_ms(250);
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
  }

  puts("Testing left forward...\n");
  
  gpio_put(LEFT_ENABLE_PIN,0); // ENABLE
  for(int i=0;i<16000;i++) {
    gpio_put(LEFT_STEP_PIN, 1);
    gpio_put(LED_PIN, 0);
    sleep_us(250);
    gpio_put(LEFT_STEP_PIN, 0);
    gpio_put(LED_PIN, 1);
    sleep_us(250);
  }

  sleep_ms(500);
  gpio_put(LEFT_DIR_PIN, 1);

  puts("Testing left backward\n");
  
  for(int i=0;i<16000;i++) {
    gpio_put(LEFT_STEP_PIN, 1);
    gpio_put(LED_PIN, 0);
    sleep_us(250);
    gpio_put(LEFT_STEP_PIN, 0);
    gpio_put(LED_PIN, 1);
    sleep_us(250);
  }

  gpio_put(LEFT_ENABLE_PIN, 1); // shut er down.

}
