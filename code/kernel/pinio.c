#include <mc1322x.h>
#include <board.h>
#include "pinio.h"

void pinFunc(int gpio, int val) {
    volatile uint32_t *reg;
    uint32_t mask, myval;
    gpio &= 0x3f;
    if(gpio > 47) {
        reg = GPIO_FUNC_SEL3;
        gpio -= 48;
    } else if(gpio > 31) {
        reg = GPIO_FUNC_SEL2;
        gpio -= 32;
    } else if(gpio > 15) {
        reg = GPIO_FUNC_SEL1;
        gpio -= 16;
    } else {
        reg = GPIO_FUNC_SEL0;
    }
    mask = ~(3 << (2*gpio));
    myval = ((val & 3) << (2*gpio));
    *reg = ((*reg & mask) | myval);
}
void pinPullupSelect(int gpio, int val) {
    volatile uint32_t *reg;
    if(gpio > 31) {
        reg = GPIO_PAD_PU_SEL1;
        gpio -= 32;
    } else {
        reg = GPIO_PAD_PU_SEL0;
    }
    if(val == PIN_PULLUP) {
        *reg |= (1 << gpio);
    } else if(val == PIN_PULLDOWN) {
        *reg = (*reg & ~(1 << gpio));
    }
}
void pinPullupEnable(int gpio, int val) {
    volatile uint32_t *reg;
    if(gpio > 31) {
        reg = GPIO_PAD_PU_EN1;
        gpio -= 32;
    } else {
        reg = GPIO_PAD_PU_EN0;
    }
    if(val) {
        *reg |= (1 << gpio);
    } else {
        *reg = (*reg & ~(1 << gpio));
    }
}


void pinDirection(int gpio, int dir) {
    volatile uint32_t *reg;
    if(gpio > 31) {
        reg = GPIO_PAD_DIR1;
        gpio -= 32;
    } else {
        reg = GPIO_PAD_DIR0;
    }
    if(dir) {
        *reg |= (1 << gpio);
    } else {
        *reg = (*reg & ~(1 << gpio));
    }
}

void shiftMsb(int gpio, int clockreg, uint8_t c) {
    int i;
    setPin(gpio, 0);
    for(i = 0; i < 8; i++) {
        setPin(gpio, ((c & 0x80) ? 1 : 0));
        c <<= 1;
        setPin(clockreg, 1);
        setPin(clockreg, 0);
    }
}

void setPin(int gpio, int val) {
    volatile uint32_t *reg;
    if(val) {
        if(gpio > 31) {
            reg = GPIO_DATA_SET1;
            gpio -= 32;
        } else {
            reg = GPIO_DATA_SET0;
        }
    } else {
        if(gpio > 31) {
            reg = GPIO_DATA_RESET1;
            gpio -= 32;
        } else {
            reg = GPIO_DATA_RESET0;
        }
    }
    *reg |= (1 << gpio);
}
int getPin(int gpio) {
    volatile uint32_t *reg, val;
    if(gpio > 31) {
        reg = GPIO_DATA1;
        gpio -= 32;
    } else {
        reg = GPIO_DATA0;
    }
    val = *reg;
    return ((val >> gpio) & 1);
}


