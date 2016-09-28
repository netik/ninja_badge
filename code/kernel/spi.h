#ifndef SPI_H
#define SPI_H

#define SPI_TX_DATA     ((volatile uint32_t *) 0x80002000)
#define SPI_RX_DATA     ((volatile uint32_t *) 0x80002004)
#define SPI_CLK_CTRL    ((volatile uint32_t *) 0x80002008)
#define SPI_SETUP       ((volatile uint32_t *) 0x8000200c)
#define SPI_STATUS      ((volatile uint32_t *) 0x80002010)

#define SPI_START_MASK (0x80)

#endif /* SPI_H */
