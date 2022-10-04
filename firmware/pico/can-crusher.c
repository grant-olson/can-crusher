#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

const uint LED_PIN = 25;

const uint LEFT_STEP_PIN = 15;
const uint LEFT_ENABLE_PIN = 14;
const uint LEFT_DIR_PIN = 13;

typedef struct {
  uint enable_pin;
  uint step_pin;
  uint dir_pin;
} motor_t;

motor_t motor_init(motor_t *motor, uint enable, uint step, uint dir) {
  motor->enable_pin = enable;
  motor->step_pin = step;
  motor->dir_pin = dir;

  // Immediately DISABLE the motor->
  gpio_init(motor->enable_pin);
  gpio_set_dir(motor->enable_pin, GPIO_OUT);
  gpio_put(motor->enable_pin, 1);

  gpio_init(motor->step_pin);
  gpio_set_dir(motor->step_pin, GPIO_OUT);
  gpio_put(motor->step_pin, 0);

  gpio_init(motor->dir_pin);
  gpio_set_dir(motor->dir_pin, GPIO_OUT);
  gpio_put(motor->dir_pin, 0);
}

void motor_enable(motor_t *motor) {
  gpio_put(motor->enable_pin, 0);
}

void motor_disable(motor_t *motor) {
  gpio_put(motor->enable_pin, 1);
}

void motor_step(motor_t *motor) {
  gpio_put(motor->step_pin, 1);
  sleep_us(250);
  gpio_put(motor->step_pin, 0);
}

void motor_set_dir(motor_t *motor, uint dir) {
  gpio_put(motor->dir_pin, dir);
}

int main() {
  bi_decl(bi_program_description("Firmware for can crusher."));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

  stdio_init_all();

  motor_t left_motor;
  motor_init(&left_motor, LEFT_ENABLE_PIN, LEFT_STEP_PIN, LEFT_DIR_PIN);
  
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

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

  motor_enable(&left_motor);

  for(int i=0;i<16000;i++) {
    motor_step(&left_motor);
    sleep_us(500);
  }

  sleep_ms(500);
  motor_set_dir(&left_motor, 1);

  puts("Testing left backward\n");
  
  for(int i=0;i<16000;i++) {
    motor_step(&left_motor);
    sleep_us(500);
  }

  motor_disable(&left_motor);

}
