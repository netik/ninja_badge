#ifndef PINOUT_H
#define PINOUT_H

#define BUZZER_PIN 11

#define BUTTON_UP 22
#define BUTTON_DOWN 23
#define BUTTON_RIGHT 24
#define BUTTON_LEFT 25
#define BUTTON_A 27
#define BUTTON_B 26

#ifdef BROKEN_BOARD
#define SER1P 21        // XXX MODIFIED; normally 1
#define SHIFT_OUT 20    // XXX MODIFIED; normally 0
#define LED_PWM 9       // XXX differs
#else
#define SER1P 1        // XXX MODIFIED; normally 1
#define SHIFT_OUT 0    // XXX MODIFIED; normally 0
#define LED_PWM 9       // XXX differs
#endif /* BROKEN_BOARD */
#define SCK1P 3
#define RCK1P 2
#define LINK_PIN 10 


#endif /* PINOUT_H */
