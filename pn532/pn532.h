// #ifndef PN532_H
// #define PN532_H

// #include <stdint.h>
// #include <stdbool.h>
// #include "twi_master.h"

// // PN532 I2C default 7-bit address
// #define PN532_I2C_ADDR 0x24
// #define PN532_PREAMBLE 0x00
// #define PN532_STARTCODE1 0x00
// #define PN532_STARTCODE2 0xFF
// #define PN532_RESET_PIN PD4
// #define LED_PIN PB5
// #define PN532_IRQ_PIN PB0  // IRQ pin connected to PCINT0

// extern volatile bool nfc_data_ready;

// // Function Prototypes
// void pn532_init(void);
// bool pn532_get_firmware_version(uint8_t *firmware_data, uint8_t len);
// bool pn532_read_passive_target(uint8_t *uid, uint8_t *uid_len);

// #endif // PN532_H

// #ifndef PN532_H
// #define PN532_H

// #include <stdint.h>
// #include <stdbool.h>

// // Initializes the PN532 module
// void pn532_init(void);

// // Checks if data is ready from the PN532
// bool pn532_data_ready(void);

// // Reads data from the PN532
// int16_t pn532_read_passive_target_id(uint8_t *uid, uint8_t uid_len);
// void pn532_start_card_detection(void);

// #endif // PN532_H

#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include <stdbool.h>

void pn532_i2c_scan(void);

bool pn532_init(void);
bool pn532_get_firmware_version(uint8_t* version_data);
bool pn532_start_passive_target_detection(uint8_t baudrate);
bool pn532_read_passive_target_id(uint8_t* uid, uint8_t* uid_len);

// Add to pn532.h
#define PN532_MIFARE_ISO14443A 0x00
#define PN532_FIRMWARE_VERSION_CMD 0x02

bool pn532_start_card_detection(void);
bool pn532_data_ready(void);
bool pn532_read_card_uid(uint8_t* uid, uint8_t* uid_length);
bool pn532_mifare_auth_block(uint8_t* uid, uint8_t uid_length, uint8_t block, const uint8_t* key);
bool pn532_mifare_read_block(uint8_t block, uint8_t* data);
bool pn532_mifare_write_block(uint8_t block, const uint8_t* data);

#endif
