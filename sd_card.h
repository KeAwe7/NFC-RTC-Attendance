#ifndef SD_CARD_H
#define SD_CARD_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// SD card commands
#define CMD0  0x40  // GO_IDLE_STATE
#define CMD1  0x41  // SEND_OP_COND
#define CMD17 0x51  // READ_SINGLE_BLOCK
#define CMD24 0x58  // WRITE_SINGLE_BLOCK
#define CMD55 0x77  // APP_CMD
#define CMD58 0x7A  // READ_OCR

// SPI pins
#define SD_CS_PIN     PB2   // Chip Select (CS)
#define SD_CS_PORT    PORTB
#define SD_CS_DDR     DDRB

// SD card response types
#define R1_IDLE_STATE 0x01
#define R1_READY      0x00

// Functions
void sd_init(void);
uint8_t sd_send_command(uint8_t cmd, uint32_t arg);
uint8_t sd_read_block(uint32_t block_addr, uint8_t *buffer, uint16_t len);
uint8_t sd_write_block(uint32_t block_addr, const uint8_t *buffer, uint16_t len);

#endif
