#ifndef __LED_H__
#define __LED_H__

#define LED_ONBOARD_PIN 25
#define LED_RED_PIN 7
#define LED_GREEN_PIN 8
#define LED_BLUE_PIN 6

#define LED_NONE_MASK 0
#define LED_ONBOARD_MASK 1
#define LED_RED_MASK 2
#define LED_GREEN_MASK 4
#define LED_BLUE_MASK 8

void led_init();
void led_display(uint mask);
void led_cycle();

#endif
