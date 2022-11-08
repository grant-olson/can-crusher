#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "string.h"
#include "motor.h"
#include "power.h"
#include "property.h"
#include "serial.h"

const int max_buffer_size = 1024;
static char input_buffer[1024];
static char output_buffer[1024];
static int input_buffer_pos = 0;
static char word_buffer[128];

#define serial_putc(x) uart_putc(SERIAL_UART_ID, x);
#define serial_puts(x) uart_puts(SERIAL_UART_ID, x);
#define serial_printf(...) {sprintf(output_buffer, __VA_ARGS__ );uart_puts(SERIAL_UART_ID, output_buffer);}

int serial_init() {
  uart_init(SERIAL_UART_ID, SERIAL_BAUD_RATE);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  
  serial_printf("CC SERIAL_CONSOLE [%s]\r\n", BUILD_ID);
  
  // Flush garbage
  while(uart_is_readable(SERIAL_UART_ID)) {
    uart_getc(SERIAL_UART_ID);
  }

  
  return 0;
}

int serial_extract_next_word(int index) {
  int word_index = 0;
  
  char c = input_buffer[index];
  while (c != '\n' && c != ' ' && c != 0x0) {
    word_buffer[word_index] = c;
    index++; word_index++;
    if (word_index >= 127) {
      puts("BAD WORD. ABORTING.");
      return -1;
    }
    c = input_buffer[index];
  }
    
  word_buffer[word_index] = 0x0;

  return index;
}

bool serial_is_eol(int index) {
  char delimiter = input_buffer[index];

  return (delimiter == '\n') || (delimiter == 0x0) || (delimiter == '\r');
}

bool serial_is_valid_int(int number) {
  if (number != 0) return true;

  if (strlen(word_buffer) == 1 && word_buffer[0] == '0') return true;

  return false;
}



int serial_cmd_version(int index) {
  if (serial_is_eol(index)) {
    serial_printf("VERSION: %s\r\n", SERIAL_VERSION);
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_echo(int index) {
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  } else {
    index++;
    uart_write_blocking(SERIAL_UART_ID, &input_buffer[index], input_buffer_pos - index);
    uart_putc_raw(SERIAL_UART_ID, '\r');
    return ERR_OK;
  }
}

int serial_extract_move_params(int index, int* mm_to_move, int* mm_per_sec) {
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }

  index++;
  index = serial_extract_next_word(index);
  *mm_to_move = atoi(word_buffer);

  if (!serial_is_valid_int(*mm_to_move)) return ERR_BAD_INT;
  
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }
  
  index++;
  index = serial_extract_next_word(index);
  *mm_per_sec = atoi(word_buffer);

  if (!serial_is_valid_int(*mm_per_sec)) return ERR_BAD_INT;
  
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  return ERR_OK;
}

int serial_cmd_move_handle_stall(int move_result) {
  int res = ERR_OK;
  switch (move_result) {
  case 0:
    res = ERR_OK;
    break;
  case 1:
    res = ERR_STALL_LEFT;
    break;
  case 2:
    res = ERR_STALL_RIGHT;
    break;
  case 3:
    res = ERR_STALL_BOTH;
    break;
  default:
    res = ERR_STALL_UNKNOWN;
  }

  return res;
}

int serial_cmd_move(int index) {
  int mm_to_move, mm_per_sec;
  int result = serial_extract_move_params(index, &mm_to_move, &mm_per_sec);

  if (result != ERR_OK) {return result;}

  if (!motors_is_awake()) {return ERR_MOTORS_SLEEPING;}
  if (!power_is_enabled()) {return ERR_NO_12V_POWER;}
  
  printf("MOVE %d %d\n", mm_to_move, mm_per_sec);
  int move_result = motors_move_mm(true, true, mm_to_move, mm_per_sec);
  return serial_cmd_move_handle_stall(move_result);
}

int serial_cmd_move_left(int index) {
  int mm_to_move, mm_per_sec;
  int result = serial_extract_move_params(index, &mm_to_move, &mm_per_sec);

  if (result != ERR_OK) {return result;}

  // need to be gentle when syncing
  if (mm_to_move > 5) { return ERR_OUT_OF_BOUNDS; } 
  
  if (!motors_is_awake()) {return ERR_MOTORS_SLEEPING;}
  if (!power_is_enabled()) {return ERR_NO_12V_POWER;}

  printf("MOVE_LEFT %d %d\n", mm_to_move, mm_per_sec);
  int move_result = motors_move_mm(true, false, mm_to_move, mm_per_sec);
  return serial_cmd_move_handle_stall(move_result);
}

int serial_cmd_move_right(int index) {
  int mm_to_move, mm_per_sec;
  int result = serial_extract_move_params(index, &mm_to_move, &mm_per_sec);

  if (result != ERR_OK) {return result;}
  
  // need to be gentle when syncing
  if (mm_to_move > 5) { return ERR_OUT_OF_BOUNDS; } 
  
  if (!motors_is_awake()) {return ERR_MOTORS_SLEEPING;}
  if (!power_is_enabled()) {return ERR_NO_12V_POWER;}

  printf("MOVE_RIGHT %d %d\n", mm_to_move, mm_per_sec);
  int move_result = motors_move_mm(false, true, mm_to_move, mm_per_sec);
  return serial_cmd_move_handle_stall(move_result);
}

int serial_cmd_wake(int index) {
  if (serial_is_eol(index)) {
    if(motors_wake()) return ERR_EXECUTION_FAILED;
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_sleep(int index) {
  if (serial_is_eol(index)) {
    if (motors_sleep()) return ERR_EXECUTION_FAILED;
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_power_enable(int index) {
  if (serial_is_eol(index)) {
    power_enable();
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_power_disable(int index) {
  if (serial_is_eol(index)) {
    power_disable();
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_home(int index) {
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  if (!motors_is_awake()) {return ERR_MOTORS_SLEEPING;}
  if (!power_is_enabled()) {return ERR_NO_12V_POWER;}

  int home_result = motors_home();
  return serial_cmd_move_handle_stall(home_result);
}

int serial_cmd_position(int index) {
  if (serial_is_eol(index)) {
    serial_printf("POSITION: %0.2f\r\n", motors_get_position());
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_zero(int index) {
  if (serial_is_eol(index)) {
    motors_zero_position();
  } else {
    return ERR_TOO_MANY_ARGS;
  }
  return ERR_OK;
}

int serial_cmd_get_prop(int index) {
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }

  index++;
  index = serial_extract_next_word(index);

  can_prop prop_name = property_get_prop_id(word_buffer);

  if (prop_name == PROP_UNKNOWN) { return ERR_BAD_PROP_NAME;}

  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  uint32_t value = property_get_prop(prop_name);

  if (prop_name == PROP_MOTOR_NOT_HOMED) {
    // Hack for signed int. Do better if we get more.
    serial_printf("PROP: %d\r\n", (int32_t)value);
  } else {
    serial_printf("PROP: %d\r\n", value);
  }
  
  return ERR_OK;
}

int serial_cmd_set_prop(int index) {
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }

  index++;
  index = serial_extract_next_word(index);

  can_prop prop_name = property_get_prop_id(word_buffer);

  if (prop_name == PROP_UNKNOWN) { return ERR_BAD_PROP_NAME;}

  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }

  index++;
  index = serial_extract_next_word(index);
  
  uint32_t prop_val = atoi(word_buffer);

  if (!serial_is_valid_int(prop_val)) return ERR_BAD_INT;
  
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  property_set_prop(prop_name, prop_val);
  
  return ERR_OK;
}

int serial_cmd_reset_props(int index) {
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  property_set_defaults();

  return ERR_OK;
}

int serial_cmd_save_props(int index) {
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  property_save();

  return ERR_OK;
}

int serial_cmd_load_props(int index) {
  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  property_load();

  return ERR_OK;
}

int serial_dispatch_cmd() {
  int index = 0;

  index = serial_extract_next_word(index);
  
  if (!strcmp(word_buffer,"VERSION?")) {
    return serial_cmd_version(index);
  } else if (!strcmp(word_buffer, "ECHO")) {
    return serial_cmd_echo(index);
  } else if (!strcmp(word_buffer, "MOVE")) {
    return serial_cmd_move(index);
  } else if (!strcmp(word_buffer, "MOVE_LEFT")) {
    return serial_cmd_move_left(index);
  } else if (!strcmp(word_buffer, "MOVE_RIGHT")) {
    return serial_cmd_move_right(index);
  } else if (!strcmp(word_buffer, "WAKE")) {
    return serial_cmd_wake(index);
  } else if (!strcmp(word_buffer, "SLEEP")) {
    return serial_cmd_sleep(index);
  } else if (!strcmp(word_buffer, "POWER_ON")) {
    return serial_cmd_power_enable(index);
  } else if (!strcmp(word_buffer, "POWER_OFF")) {
    return serial_cmd_power_disable(index);
  } else if (!strcmp(word_buffer, "HOME")) {
    return serial_cmd_home(index);
  } else if (!strcmp(word_buffer, "POSITION?")) {
    return serial_cmd_position(index);
  } else if (!strcmp(word_buffer, "ZERO")) {
    return serial_cmd_zero(index);
  } else if (!strcmp(word_buffer, "PROP?")) {
    return serial_cmd_get_prop(index);
  } else if (!strcmp(word_buffer, "PROP=")) {
    return serial_cmd_set_prop(index);
  } else if (!strcmp(word_buffer, "RESET_PROPS")) {
    return serial_cmd_reset_props(index);
  } else if (!strcmp(word_buffer, "LOAD_PROPS")) {
    return serial_cmd_load_props(index);
  } else if (!strcmp(word_buffer, "SAVE_PROPS")) {
    return serial_cmd_save_props(index);
  }

  return ERR_UNKNOWN_CMD;
  
}

void serial_flush() {
  while(uart_is_readable(SERIAL_UART_ID)) {
    char x = uart_getc(SERIAL_UART_ID);
  }
}

void serial_process() {
  int result;
  
  while(uart_is_readable(SERIAL_UART_ID)) {
    char x = uart_getc(SERIAL_UART_ID);

    // Skip most non-printable characters.
    if (x >= 0x0 && x <= 0x1F &&
	x != '\r' && x != '\n' && x != 0x8)
      {continue;}

    // Make everything CAPS to simplify parsing
    if (x >= 'a' && x <= 'z') {x -= 0x20;}
    
    serial_putc(x);

    if(x == 0x8) {
      if (input_buffer_pos > 0) {input_buffer_pos--;}
      continue;
    }

    if(x == '\r') {
      x = '\n'; // So we throw \n in our buffer
      serial_putc(x);
    }

    input_buffer[input_buffer_pos] = x;
    input_buffer_pos++;

    if (x == '\n') {
      input_buffer[input_buffer_pos] = 0x0;
      input_buffer_pos++;

      result = serial_dispatch_cmd();
      if(!result) {
	serial_puts("OK\r\n");
      } else {
	serial_printf("ERR %d\r\n", result);
      }
      
      input_buffer_pos = 0;
    } else if(input_buffer_pos > (max_buffer_size - 1)) {
      // Need to leave last slot to null terminate.
      // If we get this far something is totally wrong so just
      // wrap and don't try to return error here. Eventually
      // we'll get an invalid command.
      puts("BUFFER OVERFLOW. RESETTING");
      input_buffer_pos = 0;
    }
  }

}
