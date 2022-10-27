#ifndef __MOTOR_H__
#define __MOTOR_H__


// Specifically, the UART that we use to set STALLGUARD
#define MC_UART_ID uart1 
#define MC_BAUD_RATE 115200

#define LEFT_DIR_PIN 16
#define LEFT_STEP_PIN 17
#define LEFT_MS2_AD1_PIN 18
#define LEFT_MS1_AD0_PIN 19
#define LEFT_ENABLE_PIN 20
#define LEFT_STALL_PIN 21

#define LEFT_DEVICE_ID 1

#define RIGHT_DIR_PIN 10
#define RIGHT_STEP_PIN 11
#define RIGHT_MS2_AD1_PIN 12
#define RIGHT_MS1_AD0_PIN 13
#define RIGHT_ENABLE_PIN 14
#define RIGHT_STALL_PIN 15

#define RIGHT_DEVICE_ID 2

#define STEP_PIO_STATE_MACHINE 0
#define STEP_STRATEGY_PIO true // False to use naive strategy which doesn't have precise timing

// Use an excessively large negative number here.
// We might have some drift on undetected stalls,
// but not 1000 times the height of the whole device
#define MOTOR_NOT_HOMED -400000

typedef struct {
  uint enable_pin;
  uint step_pin;
  uint dir_pin;
  uint stall_pin;
  uint device_id;  // Slave, but done with slave terminology
  uint ms1_ad0_pin;
  uint ms2_ad1_pin;
} motor_t;

int motors_move_mm(bool left, bool right, int mm, int mm_per_second);
int motors_init();
int motors_home();

int motors_wake();
int motors_sleep();

bool motors_is_awake();

double motors_get_position();

void motors_zero_position();
void motors_invalidate_position();

#endif
