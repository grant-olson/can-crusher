#ifndef __POWER_MANAGEMENT_H__

#define AUTO_SLEEP_TIME (60 * 1000000)
#define AUTO_POWER_DOWN_TIME (10 * 60 * 1000000)

int power_management_init();
void power_management_tick();
void power_management_kick();



#endif
