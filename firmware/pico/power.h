#ifndef __POWER_H__
#define __POWER_H__

#define ENABLE_12V_PIN 22

void power_init();
void power_enable();
void power_disable();
bool power_is_enabled();

#endif
