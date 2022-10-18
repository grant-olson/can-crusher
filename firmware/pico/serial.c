#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "string.h"
#include "motor.h"
#include "serial.h"

const int max_buffer_size = 1024;
static char input_buffer[1024];
static char output_buffer[1024];
static int input_buffer_pos = 0;
static char word_buffer[128];

#define SERIAL_VERSION "1.0"

#define serial_putc(x) uart_putc(SERIAL_UART_ID, x);
#define serial_puts(x) uart_puts(SERIAL_UART_ID, x);
#define serial_printf(...) {sprintf(output_buffer, __VA_ARGS__ );uart_puts(SERIAL_UART_ID, output_buffer);}

int serial_init() {
  uart_init(SERIAL_UART_ID, SERIAL_BAUD_RATE);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  
  char intro_string[] = "CC SERIAL CONSOLE\r\n";
  uart_write_blocking(SERIAL_UART_ID,  intro_string, strlen(intro_string));

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


#define ERR_OK 0
#define ERR_TOO_MANY_ARGS 100
#define ERR_TOO_FEW_ARGS 101
#define ERR_UNKNOWN_CMD 102
#define ERR_EXECUTION_FAILED 103

bool serial_is_eol(int index) {
  char delimiter = input_buffer[index];

  return (delimiter == '\n') || (delimiter == 0x0) || (delimiter == '\r');
}


int serial_cmd_version(int index) {
  if (serial_is_eol(index)) {
    serial_printf("VERSION %s\r\n", SERIAL_VERSION);
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

int serial_cmd_move(int index) {
  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }

  index++;
  index = serial_extract_next_word(index);
  int mm_to_move = atoi(word_buffer);

  if (serial_is_eol(index)) {
    return ERR_TOO_FEW_ARGS;
  }
  
  index++;
  index = serial_extract_next_word(index);
  int mm_per_sec = atoi(word_buffer);

  if (!serial_is_eol(index)) {
    return ERR_TOO_MANY_ARGS;
  }

  printf("MOVE %d %d\n", mm_to_move, mm_per_sec);
  motors_move_mm(true, true, mm_to_move, mm_per_sec);
  
  return ERR_OK;
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

int serial_dispatch_cmd() {
  int index = 0;

  index = serial_extract_next_word(index);
  
  if (!strcmp(word_buffer,"VERSION")) {
    return serial_cmd_version(index);
    // return;
  } else if (!strcmp(word_buffer, "ECHO")) {
    return serial_cmd_echo(index);
  } else if (!strcmp(word_buffer, "MOVE")) {
    return serial_cmd_move(index);
  } else if (!strcmp(word_buffer, "WAKE")) {
    return serial_cmd_wake(index);
  } else if (!strcmp(word_buffer, "SLEEP")) {
    return serial_cmd_sleep(index);
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

    serial_putc(x);

    if(x == 0x8) {
      input_buffer_pos--;
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
