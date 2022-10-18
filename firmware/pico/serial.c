#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "string.h"
#include "serial.h"

int serial_init() {
  uart_init(SERIAL_UART_ID, SERIAL_BAUD_RATE);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  
  char intro_string[] = "CC SERIAL CONSOLE\r\n";
  uart_write_blocking(SERIAL_UART_ID,  intro_string, strlen(intro_string));
  
  return 0;
}

void serial_process() {
  while(uart_is_readable(SERIAL_UART_ID)) {
    char x = uart_getc(SERIAL_UART_ID);
    uart_putc(SERIAL_UART_ID, x);
    if(x == '\r') {uart_putc(SERIAL_UART_ID, '\n');}
  }

}
