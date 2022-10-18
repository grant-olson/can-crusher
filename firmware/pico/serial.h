#ifndef __SERIAL_H__
#define __SERIAL_H__

#define SERIAL_UART_ID uart0
#define SERIAL_BAUD_RATE 115200

int serial_init();
void serial_process();

#endif
