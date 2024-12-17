// pn532_spi.h
#ifndef PN532_SPI_H
#define PN532_SPI_H

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>

// SPI communication commands
#define STATUS_READ     0x02
#define DATA_WRITE      0x01
#define DATA_READ       0x03

// PN532 protocol constants
#define PN532_PREAMBLE      0x00
#define PN532_STARTCODE1    0x00
#define PN532_STARTCODE2    0xFF
#define PN532_POSTAMBLE     0x00
#define PN532_HOSTTOPN532   0xD4
#define PN532_PN532TOHOST   0xD5

#define PN532_CMD_GETFIRMWAREVERSION 0x02
#define PN532_CS_LOW()  SPI_set_SS_low()
#define PN532_CS_HIGH() SPI_set_SS_high()

#define PN532_CMD_INLISTPASSIVETARGET 0x4A
#define MIFARE_ISO14443A 0x00

#define PN532_RESET_PIN PB0
#define PN532_RESET_DDR DDRB
#define PN532_RESET_PORT PORTB

// Timeout and error definitions
#define PN532_ACK_WAIT_TIME 10
#define PN532_BAUDRATE      (F_CPU/4)
#define PN532_TIMEOUT       -1
#define PN532_INVALID_ACK   -3
#define PN532_INVALID_FRAME -4
#define PN532_NO_SPACE      -5

// PN532 SPI control structure
typedef struct {
    uint8_t ss_pin;     // Slave Select pin
    uint8_t command;    // Last command sent
} PN532_SPI;

// Function prototypes
void PN532_SPI_Init(PN532_SPI *pn532, uint8_t ss_pin);
void PN532_SPI_Begin(PN532_SPI *pn532);
void PN532_SPI_Wakeup(PN532_SPI *pn532);
int8_t PN532_SPI_WriteCommand(PN532_SPI *pn532, 
                               const uint8_t *header, uint8_t hlen, 
                               const uint8_t *body, uint8_t blen);
int16_t PN532_SPI_ReadResponse(PN532_SPI *pn532, 
                                uint8_t *buf, uint8_t len, 
                                uint16_t timeout);
uint8_t PN532_SPI_IsReady(PN532_SPI *pn532);
void PN532_SPI_WriteFrame(PN532_SPI *pn532, 
                           const uint8_t *header, uint8_t hlen, 
                           const uint8_t *body, uint8_t blen);
int8_t PN532_SPI_ReadAckFrame(PN532_SPI *pn532);
bool pn532_spi_detect(void);
bool pn532_read_ack(void);
bool pn532_read_firmware_version(uint8_t *version);
bool pn532_poll_for_card(uint8_t *uid, uint8_t *uid_length);


#endif // PN532_SPI_H