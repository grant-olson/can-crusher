#ifndef __MOTOR_H__
#define __MOTOR_H__


// Specifically, the UART that we use to set STALLGUARD
#define UART_ID uart1 
#define BAUD_RATE 115200
#define UART_TX_PIN 6
#define UART_RX_PIN 7

#define SUBSTEPS_PER_STEP 8
#define STEPS_PER_MM 25
#define SUBSTEPS_PER_MM (SUBSTEPS_PER_STEP * STEPS_PER_MM)

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


typedef struct {
  uint enable_pin;
  uint step_pin;
  uint dir_pin;
  uint stall_pin;
  uint device_id;  // Slave, but done with slave terminology
  uint ms1_ad0_pin;
  uint ms2_ad1_pin;
} motor_t;

void motors_enable();
void motors_disable();
int motors_move_mm(bool left, bool right, int mm, int mm_per_second);
int motors_init();
void motors_home();

void motors_stallguard_enable();
void motors_stallguard_disable();

#endif
