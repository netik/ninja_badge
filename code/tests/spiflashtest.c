#include <mc1322x.h>
#include <board.h>
#include <pinio.h>

void dbg_put_hex(uint8_t __attribute__((unused))val) {
}
void dbg_putstr(char __attribute__((unused))*str) {
}

void delaycs(uint32_t centis) {
    volatile uint32_t count = *CRM_RTC_COUNT;
    //count += (centis * 207);
    count += (centis * 40);
    while((*CRM_RTC_COUNT) < count) {
        continue;
    }
#if 0
    volatile int j = 100 * 830000;
    j /= centis;
    for(; j > 0; j--) {
        continue;
    }
#endif
}

#define SPI_SCK 7      // SPI serial clock
#define SPI_MOSI 6     // SPI master out, slave in
#define SPI_MISO 5     // SPI master in, slave out
#define FLASH_E 4      // flash enable
#define FLASH_HOLD 6   // flash hold mode (NOT USED; tied high)


void flash_init(void) {
    pinFunc(SPI_SCK, 3);
    pinFunc(SPI_MOSI, 3);
    pinFunc(SPI_MISO, 3);
    pinFunc(FLASH_E, 3);

    pinDirection(SPI_SCK, PIN_OUTPUT);
    pinDirection(SPI_MOSI, PIN_OUTPUT);
    pinDirection(SPI_MISO, PIN_INPUT);
    pinDirection(FLASH_E, PIN_OUTPUT);
}

uint8_t spi_op(uint8_t outb) {
    uint8_t inb = 0;
    uint8_t i;
    for(i = 0; i < 8; i++) {
        setPin(SPI_SCK, LOW);
        if(getPin(SPI_MISO) == HIGH) {
            inb |= (1 << (7-i));
        }
        setPin(SPI_MOSI, ((((outb >> (7-i)) & 0x1) == 1) ? HIGH : LOW));
        setPin(SPI_SCK, HIGH);
    }
    return inb;
}

void flash_readid(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    uint8_t b = spi_op(0x90);
    b = spi_op(0x0);
    b = spi_op(0x0);
    b = spi_op(0x1);

    b = spi_op(0x0);
    uint8_t c = spi_op(0x0);
    uint8_t d = spi_op(0x0);
    uint8_t e = spi_op(0x0);
    dbg_put_hex(b);
    dbg_putstr(" ");
    dbg_put_hex(c);
    dbg_putstr(" ");
    dbg_put_hex(d);
    dbg_putstr(" ");
    dbg_put_hex(e);
    dbg_putstr("\r\n");

    b = spi_op(0x0);
    c = spi_op(0x0);
    d = spi_op(0x0);
    e = spi_op(0x0);
    setPin(FLASH_E, HIGH);
    dbg_put_hex(b);
    dbg_putstr(" ");
    dbg_put_hex(c);
    dbg_putstr(" ");
    dbg_put_hex(d);
    dbg_putstr(" ");
    dbg_put_hex(e);
    dbg_putstr("\r\n");

    setPin(FLASH_E, HIGH);
}

uint8_t flash_rdsr(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    uint8_t b = spi_op(0x5);
    b = spi_op(0x0);

    setPin(FLASH_E, HIGH);
    return b;
}

void flash_wren(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x6);
    setPin(FLASH_E, HIGH);
}

void flash_wrsr(uint8_t sr) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x50);

    setPin(FLASH_E, HIGH);
    // XXX: supposed to be 10 us
    delaycs(1);

    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x1);
    spi_op(sr);
    setPin(FLASH_E, HIGH);
}

void flash_secterase(int addr) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x20);
    // casts not really necessary
    spi_op((uint8_t)((addr >> 16) & 0xff));
    spi_op((uint8_t)((addr >> 8) & 0xff));
    spi_op((uint8_t)(addr & 0xff));

    setPin(FLASH_E, HIGH);
}



void flash_read(int addr, int len, uint8_t *buf) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x3);
    spi_op((uint8_t)(((addr >> 16) & 0xff)));
    spi_op((uint8_t)(((addr >> 8) & 0xff)));
    spi_op((uint8_t)(addr & 0xff));
    for(int i = 0; i < len; i++) {
        buf[i] = spi_op(0x0);
    }

    setPin(FLASH_E, HIGH);
}

void flash_bytewrite(int addr, uint8_t val) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x2);
    spi_op((uint8_t)(((addr >> 16) & 0xff)));
    spi_op((uint8_t)(((addr >> 8) & 0xff)));
    spi_op((uint8_t)(addr & 0xff));
    spi_op(val);

    setPin(FLASH_E, HIGH);
}

#if 0
void dump_first_bytes(void) {
    uint8_t buf[4];
    dbg_putstr("00: ");
    flash_read(0x0, 4, buf);
    dbg_put_hex(buf[0]);
    dbg_putstr(" ");
    dbg_put_hex(buf[1]);
    dbg_putstr(" ");
    dbg_put_hex(buf[2]);
    dbg_putstr(" ");
    dbg_put_hex(buf[3]);
    dbg_putstr("\r\n");
}
#endif

void main(void) {
    vreg_init();
    flash_init();
    flash_readid();

    uint8_t testbuf[] = { 
        0xa8, 0x03, 0x99, 0x41, 0xe2, 
        0xc7, 0x21, 0xc8, 0xf2, 0x21 
    };
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    setPin(LED_RED, 0);
    setPin(LED_GREEN, 0);
    delaycs(300);
    uint8_t buf[10];
    flash_read(0x0, 10, buf);
    for(unsigned int i = 0; i < sizeof(testbuf); i++) {
        if(buf[i] == testbuf[i]) {
            setPin(LED_GREEN, 1);
            delaycs(50);
            setPin(LED_GREEN, 0);
        } else {
            setPin(LED_RED, 1);
            delaycs(50);
            setPin(LED_RED, 0);
        }
        delaycs(50);
    }

    dbg_putstr("status register:");
    dbg_put_hex(flash_rdsr());
    dbg_putstr("\r\n");
    delaycs(1);
    flash_wren();
    delaycs(1);
    uint8_t b = flash_rdsr();
    flash_wrsr(b & ~(0xc));
    //dump_first_bytes();

    delaycs(5);
    flash_wren();
    flash_secterase(0x0);
    // XXX: 50 ms
    delaycs(5);
    flash_wren();
    flash_bytewrite(0x0, testbuf[0]);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x1, testbuf[1]);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x2, testbuf[2]);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x3, testbuf[3]);

    while(1) {
    }
#if 0

    dbg_putstr("status register:");
    dbg_put_hex(flash_rdsr());
    dbg_putstr("\r\n");
    // XXX: .01 cs
    delaycs(1);
    flash_wren();
    // XXX: .01 cs
    delaycs(1);
    dbg_putstr("status register after WREN:");
    uint8_t b = flash_rdsr();
    dbg_put_hex(b);
    dbg_putstr("\r\n");
    dbg_putstr("status register after WRSR:");
    dbg_pustr("\r\n");
    flash_wrsr(b & ~(0xc));
    dbg_put_hex(flash_rdsr());
    dbg_putstr("\n");

    flash_wren();
    flash_secterase(0x0);
    // XXX: 50 ms
    delaycs(5);
    flash_wren();
    flash_bytewrite(0x0, 0xa1);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x1, 0xb2);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x2, 0x3c);
    // XXX: 50 us
    delaycs(2);
    flash_wren();
    flash_bytewrite(0x3, 0x4d);
    b = flash_rdsr();
    dbg_putstr("status register after last write:");
    dbg_put_hex(b);
    dbg_putstr("\n");
    // XXX: 50 us
    delaycs(5);
    dump_first_bytes();

    while(true) {
        delay(50);
    }
#endif

    return;
}
