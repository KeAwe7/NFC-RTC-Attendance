#include "spi.h"

/**
 * Initialize the SPI as Master.
 */
void SPI_init_master(void) {
    SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS); // Set MOSI, SCK, SS as output
    SPI_DDR &= ~(1 << SPI_MISO);                                // Set MISO as input
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);              // Enable SPI, Set Master mode, f_osc/16
    SPI_set_SS_high();                                          // Set SS high to deselect slave
}

/**
 * Initialize the SPI as Slave.
 */
void SPI_init_slave(void) {
    SPI_DDR |= (1 << SPI_MISO);                                 // Set MISO as output
    SPI_DDR &= ~((1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS)); // Set MOSI, SCK, SS as input
    SPCR = (1 << SPE);                                          // Enable SPI
}

/**
 * Perform an SPI data transfer.
 * @param data Byte to send
 * @return Received byte
 */
uint8_t SPI_transfer(uint8_t data) {
    SPDR = data;                                                // Load data into SPI Data Register
    while (!(SPSR & (1 << SPIF)));                              // Wait until transfer completes
    return SPDR;                                                // Return received data
}

/**
 * Set Slave Select (SS) high to deselect the slave.
 */
void SPI_set_SS_high(void) {
    SPI_PORT |= (1 << SPI_SS);                                  // Set SS high
}

/**
 * Set Slave Select (SS) low to select the slave.
 */
void SPI_set_SS_low(void) {
    SPI_PORT &= ~(1 << SPI_SS);                                 // Set SS low
}
