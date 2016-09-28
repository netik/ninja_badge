#define LOW 0
#define HIGH 1

#define PIN_INPUT 0
#define PIN_OUTPUT 1

#define PIN_PULLUP 1
#define PIN_PULLDOWN 0


void pinPullupEnable(int gpio, int val);
void pinPullupSelect(int gpio, int val);
void pinFunc(int gpio, int val);
void pinDirection(int gpio, int dir);
void setPin(int gpio, int val);
int getPin(int gpio);
void shiftMsb(int gpio, int clockreg, uint8_t c);
