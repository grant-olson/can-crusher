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
  uint device_id;  // Slave, but done with slave terminology
} motor_t;

motor_t motor_init(motor_t *motor, uint enable, uint step, uint dir,
		   uint stall, uint device_id) {
  motor->enable_pin = enable;
  motor->step_pin = step;
  motor->dir_pin = dir;
  motor->stall_pin = stall;
  motor->device_id = device_id;
  
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

void motor_control_init() {
  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(4, GPIO_FUNC_UART);
  gpio_set_function(5, GPIO_FUNC_UART);
}

void motor_control_crc(char* buffer, int size) {
  uint8_t crc = 0;

  // Last byte holds the crc, ignore from calc
  for (int i=0;i<size-1;i++) {
    uint8_t byte = buffer[i];
    // process byte bit-by-bit
    for (int j=0;j<8;j++) {
      if ( (crc >> 7) ^ (byte & 0x1) ) {
	crc = (crc << 1) ^ 0x07;
      } else {
	crc = crc << 1;
      }
      
      byte = byte >> 1;
    }
  }

  buffer[size-1] = crc;
}

// Has BLOCKING calls. Do not use when we're doing timing
// Critical motor movements.
bool motor_control_register_read(motor_t *motor, uint8_t reg, uint32_t *result) {
  static char read_buffer[4] = {0x05, 0x00, 0x00, 0x00};
  static char result_buffer[8];

  read_buffer[1] = motor->device_id;
  read_buffer[2] = reg << 1;

  motor_control_crc(read_buffer, 4);

  while(uart_is_readable(UART_ID)){ uart_getc(UART_ID); }

  uart_write_blocking(UART_ID, read_buffer, 4);
  uart_tx_wait_blocking(UART_ID);
  
  // read back identical data, since we're sharing a uart line.
  char dummy_char;
  for (int i=0;i<4;i++) {
    if (uart_is_readable(UART_ID)) {
      dummy_char = uart_getc(UART_ID);
      if (dummy_char != read_buffer[i]) {
	printf("CHAR %d DIDN'T MATCH. Expected 0x%x, Got 0x%x\n", read_buffer[i], dummy_char);
	return -1;
      }
    } else {
      puts("ERROR READING UART, NO DATA\n");
      return -1;
    }
  }

  if(!uart_is_readable_within_us(UART_ID, 1000)) {
    puts("NO RESPONSE!\n");
    return -1;
  }
  
  uart_read_blocking(UART_ID, result_buffer, 8);
 
  printf("RESULT: %x %x %x %x %x %x %x %x\n", result_buffer[0], result_buffer[1],
	 result_buffer[2], result_buffer[3], result_buffer[4],
	 result_buffer[5], result_buffer[6], result_buffer[7]);
  
  char result_crc = result_buffer[7];
  motor_control_crc(result_buffer, 8);

  if (result_buffer[7] != result_crc) {
    puts("BAD RECALCULATED CRC\n");
    return -1;
  }
  
  if ((result_buffer[0] & 0x0F) != 5) {
    puts("BAD SYNC FIELD\n");
    return -1;
  }

  if (result_buffer[1] != 0xFF) {
    puts("BAD ID FIELD\n");
    return -1;
  }

  *result = (result_buffer[3] << 24) | (result_buffer[4] << 16) |
    (result_buffer[5] << 8) | result_buffer[6];

  return 0;
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
	     LEFT_DIR_PIN, LEFT_STALL_PIN, 0);
  
  motor_control_init();
  
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);


  sleep_ms(500);
  puts("=============================================================\n");
  puts("Can crusher initializing.\n");

  // Blink 10 times over 5 seconds to provide visual indication
  // that our code is actually running.
  for(int i=0;i<10;i++) {
    gpio_put(LED_PIN, 0);
    sleep_ms(250);
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
  }

  puts("\x1b[2JInitialization complete.\n");

  motor_enable(&left_motor);

  puts("Sending test packet to UART\n");

  uint32_t test_result;
  motor_control_register_read(&left_motor, 0x0, &test_result);

  printf("Test UART result data %x\n", test_result);

  
  motor_control_register_read(&left_motor, 0x1, &test_result);
  printf("Test UART result data %x\n", test_result);
  
  
  puts("Testing left forward...\n");


  for(int i=0;i<16000;i++) {
    motor_step(&left_motor);
    sleep_us(500);

    if(motor_is_stalled(&left_motor)) {
      puts("STALL DETECTED. ABORT.");
      break;
    }
  }

  //motor_control_register_read(&left_motor, 0x6, &test_result);

  printf("Test UART result data %x\n", test_result);

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

  motor_control_register_read(&left_motor, 0x0, &test_result);

  printf("Test UART result data %x\n", test_result);

  while(1) {};  
}
