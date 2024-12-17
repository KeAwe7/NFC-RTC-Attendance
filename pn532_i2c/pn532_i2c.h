// #ifndef PN532_I2C_H
// #define PN532_I2C_H

// #include <stdint.h>
// #include "twi_master.h"

// // PN532 Constants
// #define PN532_I2C_ADDRESS     0x48  // Default I2C address
// #define PN532_PREAMBLE       0x00
// #define PN532_STARTCODE1     0x00
// #define PN532_STARTCODE2     0xFF
// #define PN532_POSTAMBLE      0x00
// #define PN532_HOSTTOPN532    0xD4
// #define PN532_PN532TOHOST    0xD5

// // PN532 Commands
// #define PN532_COMMAND_GETFIRMWAREVERSION    0x02
// #define PN532_COMMAND_SAMCONFIGURATION      0x14
// #define PN532_COMMAND_INLISTPASSIVETARGET   0x4A

// // Return codes
// #define PN532_TIMEOUT        -3
// #define PN532_INVALID_FRAME  -2
// #define PN532_NO_SPACE      -1

// // Function prototypes
// void pn532_init(void);
// bool get_firmware_version(void);
// bool pn532_read_passive_target(uint8_t *uid, uint8_t *uid_len);

// // Global variables for firmware version
// extern uint8_t pn532_firmware_ver[4];

// #endif

#ifndef __PN532_I2C_H__
#define __PN532_I2C_H__

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "twi_master.h"

#define PN532_I2C_ADDRESS        0x24
#define PN532_FRAME_MAX_LENGTH   64

// PN532 Commands
#define PN532_COMMAND_GETFIRMWAREVERSION     0x02
#define PN532_COMMAND_SAMCONFIGURATION       0x14

// Function prototypes
uint8_t pn532_write_command(const uint8_t *header, uint8_t hlen, 
                            const uint8_t *body, uint8_t blen);
int16_t pn532_read_response(uint8_t *buf, uint8_t len, uint16_t timeout);
int8_t pn532_read_ack_frame(void);
void pn532_wakeup(void);
bool pn532_i2c_init(void);

#endif /* __PN532_I2C_H__ */