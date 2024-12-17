#ifndef SPI_H
#define SPI_H

#include <avr/io.h>

#define SPI_DDR  DDRB // Data Direction Register for SPI
#define SPI_PORT PORTB // Port for SPI
#define SPI_SCK  PB5  // SPI SCK Pin
#define SPI_MISO PB4  // SPI MISO Pin
#define SPI_MOSI PB3  // SPI MOSI Pin
#define SPI_SS   PB2  // SPI Slave Select Pin

void SPI_init_master(void);
void SPI_init_slave(void);
uint8_t SPI_transfer(uint8_t data);
void SPI_set_SS_high(void);
void SPI_set_SS_low(void);

#endif
