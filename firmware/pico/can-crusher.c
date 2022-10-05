#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"

const uint LED_PIN = 25;

#define UART_ID uart1 // 0 is logging
#define BAUD_RATE 115200

#define UART_TX_PIN 6
#define UART_RX_PIN 7


const uint LEFT_ENABLE_PIN = 10;
const uint LEFT_STEP_PIN = 11;
const uint LEFT_DIR_PIN = 12;
const uint LEFT_STALL_PIN = 13;

typedef struct {
  uint enable_pin;
  uint step_pin;
  uint dir_pin;
  uint stall_pin;
} motor_t;

motor_t motor_init(motor_t *motor, uint enable, uint step, uint dir, uint stall) {
  motor->enable_pin = enable;
  motor->step_pin = step;
  motor->dir_pin = dir;
  motor->stall_pin = stall;
  
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

  gpio_init(motor->stall_pin);
  gpio_set_dir(motor->stall_pin, GPIO_IN);
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

bool motor_is_stalled(motor_t *motor) {
  return gpio_get(motor->stall_pin) != 0;
}

int main() {
  bi_decl(bi_program_description("Firmware for can crusher."));
  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
  bi_decl(bi_1pin_with_name(LEFT_ENABLE_PIN, "Left Stepper Enable - active low."));
  bi_decl(bi_1pin_with_name(LEFT_STEP_PIN, "Left Stepper Step"));
  bi_decl(bi_1pin_with_name(LEFT_DIR_PIN, "Left Direction Step"));
  bi_decl(bi_1pin_with_name(LEFT_STALL_PIN, "Left Stall Step"));
  
  stdio_init_all();

  motor_t left_motor;
  motor_init(&left_motor, LEFT_ENABLE_PIN, LEFT_STEP_PIN, 
	     LEFT_DIR_PIN, LEFT_STALL_PIN);
  
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

  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(4, GPIO_FUNC_UART);
  gpio_set_function(5, GPIO_FUNC_UART);

  puts("Sending test packet to UART\n");
  
  uart_putc_raw(UART_ID, 0x5);
  uart_putc_raw(UART_ID, 0x0);
  uart_putc_raw(UART_ID, 0x0);
  uart_putc_raw(UART_ID, 0x48);

  motor_enable(&left_motor);
  
  puts("Done sending, listening for reply.\n");
  for(int i=0;i<12;i++) {
    printf("0x%x\n", uart_getc(UART_ID));
  }
  
  puts("Testing left forward...\n");


  for(int i=0;i<16000;i++) {
    motor_step(&left_motor);
    sleep_us(500);

    if(motor_is_stalled(&left_motor)) {
      puts("STALL DETECTED. ABORT.");
      break;
    }
  }

  sleep_ms(500);
  motor_set_dir(&left_motor, 1);

  puts("Testing left backward\n");
  
  for(int i=0;i<16000;i++) {
    motor_step(&left_motor);
    sleep_us(500);

    if(motor_is_stalled(&left_motor)) {
      puts("STALL DETECTED. ABORT.");
      break;
    }
  }

  motor_disable(&left_motor);

}
