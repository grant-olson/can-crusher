#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "led.h"
#include "button.h"
#include "power.h"
#include "motor.h"

int main() {
  bi_decl(bi_program_description("Firmware for can crusher."));
  
  bi_decl(bi_1pin_with_name(LED_ONBOARD_PIN, "LED - Onboard"));
  bi_decl(bi_1pin_with_name(LED_RED_PIN, "LED - RGB Red"));
  bi_decl(bi_1pin_with_name(LED_GREEN_PIN, "LED - RGB Green"));
  bi_decl(bi_1pin_with_name(LED_BLUE_PIN, "LED - RGB Blue"));

  bi_decl(bi_1pin_with_name(USER_BUTTON_PIN, "User Interaction Button"));
  
  bi_decl(bi_1pin_with_name(LEFT_ENABLE_PIN, "Left Stepper Enable - active low."));
  bi_decl(bi_1pin_with_name(LEFT_STEP_PIN, "Left Stepper Step"));
  bi_decl(bi_1pin_with_name(LEFT_DIR_PIN, "Left Direction Step"));
  bi_decl(bi_1pin_with_name(LEFT_STALL_PIN, "Left Stall Step"));

  stdio_init_all();

  puts("STARTING INITIALIZATION");
  
  // Run first to immediately DISABLE the 12 volt power supply.
  // Don't leave it to chance.
  if(power_init()) {
    puts("power_init() failed! ABORTING!");
    return -1;
  }

  sleep_ms(500);
  
  if(led_init()) {
    puts("led_init() failed! ABORTING!");
    return -1;
  }
  
  if(button_init()) {
    puts("button_init() failed! ABORTING!");
    return -1;
  }
  
  puts("Enable 12V...");
  power_enable();

  if(motors_init()) {
    puts("motors_init() failed! ABORTING!");
    return -1;
  }
  
  // Boot sequence
  puts("Initialization complete.\n");
  led_display(LED_GREEN_MASK);
  motors_enable();

  motors_move_mm(true, true, -30, 20);
  motors_move_mm(true, true, 30, 20);

  // motors_home();
  
  motors_disable();
  power_disable();

  while(1) {
    if (button_status()) {
      puts("BUTTON PRESSED");
      led_display(LED_RED_MASK);
      sleep_ms(250);
      led_display(LED_GREEN_MASK);

      power_enable();
      motors_enable();
      
      motors_move_mm(true, true, -30, 20);
      motors_move_mm(true, true, 30, 20);

      motors_disable();
      power_disable();

      led_display(LED_NONE_MASK);

    }
    
    sleep_ms(250);
  };

  return 0;
}
