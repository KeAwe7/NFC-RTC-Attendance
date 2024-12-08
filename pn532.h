#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include <stdbool.h>
#include "twi_master.h"

// PN532 I2C default 7-bit address
#define PN532_I2C_ADDR 0x24
#define PN532_PREAMBLE 0x00
#define PN532_STARTCODE1 0x00
#define PN532_STARTCODE2 0xFF
#define PN532_RESET_PIN PD4
#define LED_PIN PB5

// Function Prototypes
void pn532_init(void);
bool pn532_get_firmware_version(uint8_t *firmware_data, uint8_t len);
bool pn532_read_passive_target(uint8_t *uid, uint8_t *uid_len);

#endif // PN532_H
