#include "sd_card.h"
#include "spi.h"  // Include SPI helper functions

// Macro to control the CS pin
#define SD_SELECT()   (SD_CS_PORT &= ~(1 << SD_CS_PIN))
#define SD_DESELECT() (SD_CS_PORT |= (1 << SD_CS_PIN))

// Initialize the SD card
void sd_init(void) {
    // Set CS pin as output
    SD_CS_DDR |= (1 << SD_CS_PIN);

    // Deselect the SD card
    SD_DESELECT();

    // Initialize SPI
    spi_init();

    // Send 80 clock pulses with CS high
    for (uint8_t i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }

    // Send CMD0 to reset the card
    SD_SELECT();
    sd_send_command(CMD0, 0);
    SD_DESELECT();

    // Wait for the card to come out of idle state
    while (sd_send_command(CMD1, 0) != R1_READY) {
        _delay_ms(10);
    }
}

// Send a command to the SD card
uint8_t sd_send_command(uint8_t cmd, uint32_t arg) {
    uint8_t response;
    uint8_t crc = 0x95; // Predefined CRC for CMD0, CMD8 (others can be 0x00 in SPI mode)

    // Send command
    spi_transfer(cmd | 0x40); // Command token
    spi_transfer((arg >> 24) & 0xFF); // Argument MSB
    spi_transfer((arg >> 16) & 0xFF);
    spi_transfer((arg >> 8) & 0xFF);
    spi_transfer(arg & 0xFF); // Argument LSB
    spi_transfer(crc); // CRC

    // Wait for a response (R1)
    for (uint8_t i = 0; i < 8; i++) {
        response = spi_transfer(0xFF);
        if (response != 0xFF) {
            break;
        }
    }

    return response;
}

// Read a single block from the SD card
uint8_t sd_read_block(uint32_t block_addr, uint8_t *buffer, uint16_t len) {
    uint8_t response;

    // Send CMD17 to read a block
    response = sd_send_command(CMD17, block_addr);
    if (response != R1_READY) {
        return 1; // Error
    }

    // Wait for the start token (0xFE)
    while (spi_transfer(0xFF) != 0xFE);

    // Read the block data
    for (uint16_t i = 0; i < len; i++) {
        buffer[i] = spi_transfer(0xFF);
    }

    // Ignore CRC bytes
    spi_transfer(0xFF);
    spi_transfer(0xFF);

    return 0; // Success
}

// Write a single block to the SD card
uint8_t sd_write_block(uint32_t block_addr, const uint8_t *buffer, uint16_t len) {
    uint8_t response;

    // Send CMD24 to write a block
    response = sd_send_command(CMD24, block_addr);
    if (response != R1_READY) {
        return 1; // Error
    }

    // Send start token (0xFE)
    spi_transfer(0xFE);

    // Write the block data
    for (uint16_t i = 0; i < len; i++) {
        spi_transfer(buffer[i]);
    }

    // Send dummy CRC
    spi_transfer(0xFF);
    spi_transfer(0xFF);

    // Wait for data response
    response = spi_transfer(0xFF);
    if ((response & 0x1F) != 0x05) {
        return 2; // Error
    }

    // Wait for the card to finish programming
    while (spi_transfer(0xFF) == 0x00);

    return 0; // Success
}
