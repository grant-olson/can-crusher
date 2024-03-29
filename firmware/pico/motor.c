#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "power.h"
#include "property.h"
#include "step.pio.h"
#include "motor.h"

static motor_t left_motor;
static motor_t right_motor;

static bool motor_awake = false;

static double motor_position = MOTOR_NOT_HOMED;

static PIO pio = pio0;
static uint offset_both, offset_one;

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
  gpio_set_pulls(motor->stall_pin, false, true);
  
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

  uint32_t tcoolthrs = property_get_prop(PROP_TCOOL_THRESHOLD);
  uint32_t sgthrs = property_get_prop(PROP_STALLGUARD_THRESHOLD);

  motor_control_register_write(motor, 0x14, tcoolthrs);
  motor_control_register_write(motor, 0x40, sgthrs);

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

  if (STEP_STRATEGY_PIO) {
    step_gpio_init(pio, LEFT_STEP_PIN, RIGHT_STEP_PIN, LEFT_STALL_PIN, RIGHT_STALL_PIN);
    
    offset_both = pio_add_program(pio, &step_both_program);
    step_both_program_init(pio, STEP_PIO_SM_BOTH, offset_both, LEFT_STEP_PIN, RIGHT_STEP_PIN, LEFT_STALL_PIN);
    pio_sm_set_enabled(pio, STEP_PIO_SM_BOTH, true);

    offset_one = pio_add_program(pio, &step_one_program);
    step_one_program_init(pio, STEP_PIO_SM_ONE, offset_one, LEFT_STEP_PIN, LEFT_STALL_PIN);
    pio_sm_set_enabled(pio, STEP_PIO_SM_ONE, true);

  }

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

int motors_wake() {
  int res = 0;
  
  motors_disable();
  
 if (!power_is_enabled()) {
    puts("Can't init motors without power enabled!");
    return -1;
  }
  
  motor_control_enable();

  if (!motor_control_stallguard(&left_motor)) { puts("LEFT FAILED!");res = -1;}

  if (!motor_control_stallguard(&right_motor)) { puts("RIGHT FAILED!"); res = -1;}
  
  motor_control_disable();

  motors_enable();

  motor_awake = true;
  return res;
}

int motors_sleep() {
  motors_disable();

  motor_awake = false;
  return 0;
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

// PIO based driver to move motors
int motors_move_mm_pio(bool left, bool right, int mm, int mm_per_second) {
  int res = 0;
  bool is_dir_down = (mm < 0);
  
  if (is_dir_down) {
    mm = 0 - mm;
    motors_set_dir(1);
  } else {
    motors_set_dir(0);
  }

  int substeps_per_mm = property_get_prop(PROP_STEPS_PER_MM) *
    property_get_prop(PROP_SUBSTEPS_PER_STEP);
  int total_steps =  substeps_per_mm * mm;

  int steps_per_second = substeps_per_mm * mm_per_second;

  int sm;
  
  if (left && right) {
    step_x_times(pio, STEP_PIO_SM_BOTH, total_steps, steps_per_second);
    sm = STEP_PIO_SM_BOTH;
  } else {
    sm = STEP_PIO_SM_ONE;

    pio_sm_set_enabled(pio, STEP_PIO_SM_ONE, false);
    if (left) {
      step_one_set_pins(pio, sm, offset_one, LEFT_STEP_PIN, LEFT_DIR_PIN);
    } else {
      step_one_set_pins(pio, sm, offset_one, RIGHT_STEP_PIN, RIGHT_DIR_PIN);
    }
    pio_sm_set_enabled(pio, STEP_PIO_SM_ONE, true);

    step_x_times(pio, STEP_PIO_SM_ONE, total_steps, steps_per_second);
  }

  // For now, just block and wait until we're done,
  // even though we can do work in the background now.
  int32_t remaining_ticks = pio_sm_get_blocking(pio, sm);
  
  if(remaining_ticks >= 0) { // We aborted
    if (left && motor_is_stalled(&left_motor)) {
      res += left_motor.device_id;
    }
    
    if (right && motor_is_stalled(&right_motor)) {
      res += right_motor.device_id;
    }
  }

  // Assume a wiggle on left or right doesn't affect position.
  if(left && right && motor_position > MOTOR_NOT_HOMED) {
    // remaining_ticks + 1 because pio pre-decrements results.
    remaining_ticks += 1;
    
    double mm_moved = (double)(total_steps - remaining_ticks) / (double)substeps_per_mm;

    if (is_dir_down) {
      motor_position -= mm_moved;
    } else {
      motor_position += mm_moved;
    }
  }
  
  return res;
}

// Basic all C implementation. Timing isn't exact
int motors_move_mm_naive(bool left, bool right, int mm, int mm_per_second) {
  int res = 0;
  bool is_dir_down = (mm < 0);
  
  if (is_dir_down) {
    mm = 0 - mm;
    motors_set_dir(1);
  } else {
    motors_set_dir(0);
  }

  int substeps_per_mm = property_get_prop(PROP_STEPS_PER_MM) *
    property_get_prop(PROP_SUBSTEPS_PER_STEP);
  int total_steps =  substeps_per_mm * mm;
  int step_duration_us = (1000000 / mm_per_second ) / substeps_per_mm;
  
  int stall_result = 0;
  bool stallguard_enabled = property_get_prop(PROP_STALLGUARD_ENABLED);
  
  if (left && right) {step_duration_us = step_duration_us / 2;};

  int step; // Capture index for later use
  for(step=0;step<total_steps;step++) {
    if (left) {motor_step(&left_motor, step_duration_us);}
    if (right) {motor_step(&right_motor, step_duration_us);}

    if (stallguard_enabled) {
      if (left && motor_is_stalled(&left_motor)) {
        stall_result += left_motor.device_id;
      }
    
      if (right && motor_is_stalled(&right_motor)) {
        stall_result += right_motor.device_id;
      }
    }
    
    if(stall_result) {
      printf("STALL DETECTED. ABORT. %d\n", stall_result);
      res = stall_result;
      break;
    }
  }

  // In case we stalled, only modify position by
  // Actual movement.
  //
  // But if we haven't homed, keep motor position at
  // MOTOR_NOT_HOMED so we know we're not homed.
  //
  // Also assume a left-only or right-only movement
  // is to level the platform so it shouldn't affect
  // real position
  if(left && right && motor_position > MOTOR_NOT_HOMED) {
    double mm_moved = (double)step / (double)substeps_per_mm;

    if (is_dir_down) {
      motor_position -= mm_moved;
    } else {
      motor_position += mm_moved;
    }
  }
  
  return stall_result;
}


int motors_move_mm(bool left, bool right, int mm, int mm_per_second) {
  if (STEP_STRATEGY_PIO) {
    return motors_move_mm_pio(left, right, mm, mm_per_second);
  } else {
    return motors_move_mm_naive(left, right, mm, mm_per_second);
  }
}

int motors_home() {
  int home_speed = property_get_prop(PROP_HOME_SPEED);
  
  uint stall_status = motors_move_mm(true, true, -400, home_speed);

  puts("Starting Home");
  
  for (int i=0;i<4;i++) {
    printf("Iteration %d", i);
    motors_sleep(); motors_wake(); // clear stallguard bit

    puts("Backing up...");
    motors_move_mm(true, true, 25, home_speed);
    motors_sleep(); motors_wake(); // clear stallguard bit
    
    puts("Re-homing");
    stall_status = motors_move_mm(true, true, -30, home_speed);
  }

  puts("HOMED");
  motors_zero_position();

  int retract_mm = property_get_prop(PROP_HOME_RETRACT_MM);

  motors_sleep(); motors_wake();

  return motors_move_mm(true, true, retract_mm, home_speed);

}

bool motors_is_awake() {
  return motor_awake;
}

double motors_get_position() {
  return motor_position;
}

void motors_zero_position() {
  motor_position = 0;
}

void motors_invalidate_position() {
  motor_position = MOTOR_NOT_HOMED;
}
