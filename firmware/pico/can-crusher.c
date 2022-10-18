#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "led.h"
#include "button.h"
#include "motor.h"

const uint ENABLE_12V_PIN = 22;


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

  // Immediately DISABLE the 12 volt power supply.
  gpio_init(ENABLE_12V_PIN);
  gpio_set_dir(ENABLE_12V_PIN, GPIO_OUT);
  gpio_put(ENABLE_12V_PIN, 0);

  motors_init();
  
  led_init();
  button_init();
  
  sleep_ms(500);
  puts("=============================================================\n");
  puts("Can crusher initializing.\n");

  // Boot sequence
  led_cycle();
  
  puts("\x1b[2JInitialization complete.\n");

  puts("Test 12V Enable...");
  gpio_put(ENABLE_12V_PIN, 1);
  
  
  motors_enable();

  motors_move_mm(true, true, -30, 20);
  motors_move_mm(true, true, 30, 20);

  // motors_home();
  
  motors_disable();
  gpio_put(ENABLE_12V_PIN, 0);

  while(1) {
    if (button_status()) {
      puts("BUTTON PRESSED");
      led_display(LED_RED_MASK);
      sleep_ms(250);
      led_display(LED_GREEN_MASK);

      gpio_put(ENABLE_12V_PIN, 1);
      motors_enable();
      
      motors_move_mm(true, true, -30, 20);
      motors_move_mm(true, true, 30, 20);

      motors_disable();
      gpio_put(ENABLE_12V_PIN, 0);

      led_display(LED_NONE_MASK);

    }
    
    sleep_ms(250);
  };  
}
