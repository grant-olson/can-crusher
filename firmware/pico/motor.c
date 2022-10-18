#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "power.h"
#include "motor.h"

static motor_t left_motor;
static motor_t right_motor;

static bool motor_stallguard_enabled = true;

motor_t motor_init(motor_t *motor, 
                   uint enable, uint step, uint dir,
                   uint stall, uint device_id,
                   uint ms1_ad0, uint ms2_ad1) {
  motor->enable_pin = enable;
  motor->step_pin = step;
  motor->dir_pin = dir;
  motor->stall_pin = stall;
  motor->device_id = device_id;
  motor->ms1_ad0_pin = ms1_ad0;
  motor->ms2_ad1_pin = ms2_ad1;
  
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
  
  gpio_init(motor->ms1_ad0_pin);
  gpio_set_dir(motor->ms1_ad0_pin, GPIO_OUT);
  gpio_put(motor->ms1_ad0_pin, 0);

  gpio_init(motor->ms2_ad1_pin);
  gpio_set_dir(motor->ms2_ad1_pin, GPIO_OUT);
  gpio_put(motor->ms2_ad1_pin, 0);

}

void motor_enable(motor_t *motor) {
  gpio_put(motor->enable_pin, 0);
}

void motor_disable(motor_t *motor) {
  gpio_put(motor->enable_pin, 1);
}

void motor_step(motor_t *motor, int step_duration_us) {
  gpio_put(motor->step_pin, 1);
  sleep_us(step_duration_us/2);
  gpio_put(motor->step_pin, 0);
  sleep_us(step_duration_us/2);
}

void motor_set_dir(motor_t *motor, uint dir) {
  gpio_put(motor->dir_pin, dir);
}

int motor_is_stalled(motor_t *motor) {
  return gpio_get(motor->stall_pin) != 0;
}

void motor_control_init() {
  uart_init(MC_UART_ID, MC_BAUD_RATE);
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

// Sending and receiving data has BLOCKING calls. All configuration
// Should be done before we start moving motors.
int motor_control_tx_data(motor_t *motor, char* buffer, int size) {
  sleep_ms(100); //Seems to get mad if we talk to two different chips to quickly, wait a bit.
  
  motor_control_crc(buffer, size);

  while(uart_is_readable(MC_UART_ID)){ uart_getc(MC_UART_ID); }

  uart_write_blocking(MC_UART_ID, buffer, size);
  uart_tx_wait_blocking(MC_UART_ID);
  
  // read back identical data, since we're sharing a uart line.
  char dummy_char;
  for (int i=0;i<size;i++) {
    if (uart_is_readable(MC_UART_ID)) {
      dummy_char = uart_getc(MC_UART_ID);
      if (dummy_char != buffer[i]) {
        printf("CHAR %d DIDN'T MATCH. Expected 0x%x, Got 0x%x\n", buffer[i], dummy_char);
        return -1;
      }
    } else {
      puts("ERROR READING UART, NO DATA\n");
      return -1;
    }
  }
  return 0;
}

int motor_control_register_read(motor_t *motor, uint8_t reg, uint32_t *result) {
  static char read_buffer[4] = {0x05, 0x00, 0x00, 0x00};
  static char result_buffer[8];

  read_buffer[1] = motor->device_id;
  read_buffer[2] = reg;

  if(motor_control_tx_data(motor, read_buffer, 4)) {
    puts("ERROR SENDING!");
    return -1;
  }

  if(!uart_is_readable_within_us(MC_UART_ID, 10000)) {
    puts("NO RESPONSE!");
    return -1;
  }
  
  uart_read_blocking(MC_UART_ID, result_buffer, 8);
 
  char result_crc = result_buffer[7];
  motor_control_crc(result_buffer, 8);

  if (result_buffer[7] != result_crc) {
    puts("BAD RECALCULATED CRC");
    return -1;
  }
  
  if ((result_buffer[0] & 0x0F) != 5) {
    puts("BAD SYNC FIELD");
    return -1;
  }

  if (result_buffer[1] != 0xFF) {
    puts("BAD ID FIELD");
    return -1;
  }

  *result = (result_buffer[3] << 24) | (result_buffer[4] << 16) |
    (result_buffer[5] << 8) | result_buffer[6];

  return 0;
}

bool motor_control_register_write(motor_t *motor, uint8_t reg, uint32_t value) {
  static char write_buffer[8] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  write_buffer[1] = motor->device_id;
  write_buffer[2] = reg + 128;
  write_buffer[3] = (value >> 24) & 0xFF;
  write_buffer[4] = (value >> 16) & 0xFF;
  write_buffer[5] = (value >> 8) & 0xFF;
  write_buffer[6] = value & 0xFF;

  
  return motor_control_tx_data(motor, write_buffer, 8);
}


void motor_control_enable() {
  // Can't step while we change the device IDs.
  motor_disable(&left_motor);
  motor_disable(&right_motor);

  printf("LEFT: ID %d BIT1 %d BIT2 %d\n", left_motor.device_id, left_motor.device_id & 1,
         left_motor.device_id & 2);
  gpio_put(left_motor.ms1_ad0_pin, left_motor.device_id & 0x1);
  gpio_put(left_motor.ms2_ad1_pin, left_motor.device_id & 0x2);

  printf("RIGHT ID %d BIT1 %d BIT2 %d\n", right_motor.device_id, right_motor.device_id & 1,
         right_motor.device_id & 2);
  gpio_put(right_motor.ms1_ad0_pin, right_motor.device_id & 0x1);
  gpio_put(right_motor.ms2_ad1_pin, (right_motor.device_id & 0x2) == 2);

}

void motor_control_disable() {
  // set back to stepping mode
  gpio_put(left_motor.ms1_ad0_pin, 0);
  gpio_put(left_motor.ms2_ad1_pin, 0);

  gpio_put(right_motor.ms1_ad0_pin, 0);
  gpio_put(right_motor.ms2_ad1_pin, 0);

  // Still don't explicitly enable.
}  

bool motor_control_stallguard(motor_t *motor) {
  uint32_t motor_write_counter_start;
  uint32_t motor_write_counter_finish;

  if (motor_control_register_read(motor, 0x2, &motor_write_counter_start)) {
    puts("Couldn't read counter START");
    return false;
  }

  motor_control_register_write(motor, 0x14, 0xFFFFF);
  motor_control_register_write(motor, 0x40, 0x30);

  if (motor_control_register_read(motor, 0x2, &motor_write_counter_finish)) {
    puts("Couldn't read counter FINISH");
    return false;
  }

  // puts("GOT FINISH");
  if (motor_write_counter_finish - motor_write_counter_start != 2) {
    puts("Counted WRITES doesn't add up");
    return false;
  } else {
    return true;
  }
}

void query_register(motor_t *left_motor, uint8_t address) {
  uint32_t test_result;
  if (!motor_control_register_read(left_motor, address, &test_result)) {
  
    printf("Register %x  data %08x\n", address, test_result);
  } else {
    printf("Register %x FAIL\n", address, test_result);
  }
}

void query_all_registers(motor_t *motor) {
  query_register(motor, 0x0);
  query_register(motor, 0x1);
  query_register(motor, 0x2);
  query_register(motor, 0x3);
  query_register(motor, 0x5);
  query_register(motor, 0x6);
  query_register(motor, 0x7);
  
  query_register(motor, 0x10);
  query_register(motor, 0x11);
  query_register(motor, 0x12);
  query_register(motor, 0x13);
  query_register(motor, 0x22);

  query_register(motor, 0x14);
  query_register(motor, 0x40);
  query_register(motor, 0x41);
  query_register(motor, 0x42);

  query_register(motor, 0x6A);
  query_register(motor, 0x6B);
  
  query_register(motor, 0x6C);
  query_register(motor, 0x6F);
  query_register(motor, 0x70);
  query_register(motor, 0x71);
  query_register(motor, 0x72);
}

int motors_init() {
  int res = 0;
  motor_control_init();

  motor_init(&left_motor, LEFT_ENABLE_PIN, LEFT_STEP_PIN, 
             LEFT_DIR_PIN, LEFT_STALL_PIN, LEFT_DEVICE_ID,
             LEFT_MS1_AD0_PIN, LEFT_MS2_AD1_PIN);
  motor_init(&right_motor, RIGHT_ENABLE_PIN, RIGHT_STEP_PIN, 
             RIGHT_DIR_PIN, RIGHT_STALL_PIN, RIGHT_DEVICE_ID,
             RIGHT_MS1_AD0_PIN, RIGHT_MS2_AD1_PIN);

  if (!power_is_enabled()) {
    puts("Can't init motors without power enabled!");
    res = -1;
  }
  
  motor_control_enable();

  if (!motor_control_stallguard(&left_motor)) { puts("LEFT FAILED!");res = -1;}

  if (!motor_control_stallguard(&right_motor)) { puts("RIGHT FAILED!"); res = -1;}
  
  motor_control_disable();

  return res;
}

void motors_enable() {
  motor_enable(&left_motor);
  motor_enable(&right_motor);
}

void motors_disable() {
  motor_disable(&left_motor);
  motor_disable(&right_motor);
}

void motors_set_dir(int dir) {
  motor_set_dir(&left_motor, dir);
  motor_set_dir(&right_motor, dir);
}

void motors_step(int step_duration_us) {
  motor_step(&left_motor, step_duration_us);
  motor_step(&right_motor, step_duration_us);
}

int motors_are_stalled() {
  int result = 0;
  if (motor_is_stalled(&left_motor)) {
    puts("LEFT STALL");
    result = result + left_motor.device_id;
  }

  if (motor_is_stalled(&right_motor)) {
    puts("RIGHT STALL");
    result = result + right_motor.device_id;
  }

  return result;
}

int motors_move_mm(bool left, bool right, int mm, int mm_per_second) {
  if (mm < 0) {
    mm = 0 - mm;
    motors_set_dir(1);
  } else {
    motors_set_dir(0);
  }
  
  int total_steps = SUBSTEPS_PER_MM * mm;
  int step_duration_us = (1000000 / mm_per_second ) / (SUBSTEPS_PER_MM);
  int stall_result = 0;

  if (left && right) {step_duration_us = step_duration_us / 2;};
  
  for(int i=0;i<total_steps;i++) {
    if (left) {motor_step(&left_motor, step_duration_us);}
    if (right) {motor_step(&right_motor, step_duration_us);}

    if (motor_stallguard_enabled) {
      if (left) {
        if (motor_is_stalled(&left_motor)) {
          stall_result += left_motor.device_id;
        }
      }
    
      if (right) {
        if (motor_is_stalled(&right_motor)) {
          stall_result += right_motor.device_id;
        }
      }
    }
    
    if(stall_result) {
      printf("STALL DETECTED. ABORT. %d\n", stall_result);
      return stall_result;
    }
  }

  return 0;
}

void motors_home() {
  
  uint stall_status = motors_move_mm(true, true, -400, 10);

  while (stall_status != 3) {
    if (stall_status == 1) {
      motors_move_mm(false, true, -5, 10);
    }
    

    if (stall_status == 2) {
      motors_move_mm(true, false, -5, 10);
    }

    // clear old status
    motors_disable();
    sleep_ms(5000);
    motors_enable();
    sleep_ms(5000);

    puts("Backing up...");
    motors_move_mm(true, true, 30, 20);
    puts("Re-homing");
    stall_status = motors_move_mm(true, true, -25, 10);
  }
}

void motors_stallguard_disable() {
  motor_stallguard_enabled = false;
}

void motors_stallguard_enable() {
  motor_stallguard_enabled = true;
}
