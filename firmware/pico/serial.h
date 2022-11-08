#ifndef __SERIAL_H__
#define __SERIAL_H__

#define SERIAL_VERSION "1.0"
#define BUILD_ID __DATE__ " " __TIME__

#define SERIAL_UART_ID uart0
#define SERIAL_BAUD_RATE 115200

#define ERR_OK 0
#define ERR_TOO_MANY_ARGS 100
#define ERR_TOO_FEW_ARGS 101
#define ERR_UNKNOWN_CMD 102
#define ERR_EXECUTION_FAILED 103
#define ERR_OUT_OF_BOUNDS 104
#define ERR_MOTORS_SLEEPING 105
#define ERR_NO_12V_POWER 106
#define ERR_STALL_LEFT 107
#define ERR_STALL_RIGHT 108
#define ERR_STALL_BOTH 109
#define ERR_STALL_UNKNOWN 110
#define ERR_BAD_PROP_NAME 111
#define ERR_BAD_INT 112
#define ERR_BAD_VALUE 113

int serial_init();
void serial_process();
void serial_flush();

#endif
