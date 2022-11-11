#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "motor.h"
#include "power_management.h"


static uint64_t last_kick;

int power_management_init() {
  power_management_kick();

  return 0;
}

void power_management_kick() {
  absolute_time_t now = get_absolute_time();
  last_kick = to_us_since_boot(now);
}

void power_management_tick() {
  absolute_time_t now = get_absolute_time();
  uint64_t now_in_us = to_us_since_boot(now);

  if(now_in_us - last_kick > AUTO_SLEEP_TIME) {
    if (motors_is_awake()) {
      printf("POWER_MANAGEMENT: Motors awake after %d seconds uS inactivity. Sleeping...", AUTO_SLEEP_TIME);
      motors_sleep();
    }
  }
  
  if(now_in_us - last_kick > AUTO_POWER_DOWN_TIME) {
    if(power_is_enabled()) {
      printf("POWER_MANAGEMENT: 12V Power supply enabled after %d seconds uS inactivity. Shutting Down...", AUTO_POWER_DOWN_TIME);
      power_disable();
      
    }
  }
}
