#include "pn532.h"
#include <util/delay.h>
#include "lcd.h"

// void pn532_init(void) {
//     tw_init(TW_FREQ_100K, true); // Initialize I2C at 100kHz
// }

// bool pn532_get_firmware_version(uint8_t *firmware_data, uint8_t len) {
//     uint8_t cmd[] = {0xD4, 0x02}; // PN532 'Get Firmware Version' command

//     // Send command
//     ret_code_t error_code = tw_master_transmit(PN532_I2C_ADDR, cmd, sizeof(cmd), false);
//     if (error_code != SUCCESS) {
//         lcd_set_cursor(0, 1);
//         lcd_write("TX");
//         lcd_write_hex(error_code);
//         return false; // Transmission failed
//     }

//     // Delay for PN532 processing
//     _delay_ms(100);

//     // Read response
//     error_code = tw_master_receive((PN532_I2C_ADDR << 1), firmware_data, len);
//     if (error_code != SUCCESS) {
//         lcd_set_cursor(0, 1);
//         lcd_write("RX");
//         lcd_write_hex(error_code);
//         return false; // Reception failed
//     }

//     return true; // Command successful
// }

void pn532_init(void) {
    // Configure reset pin
    DDRD |= (1 << PN532_RESET_PIN);
    
    // Hardware reset sequence
    PORTD &= ~(1 << PN532_RESET_PIN);  // Reset low
    _delay_ms(100);
    PORTD |= (1 << PN532_RESET_PIN);   // Reset high
    _delay_ms(100);                     // Wait for chip init

    DDRB |= (1 << LED_PIN);
    PORTB &= ~(1 << LED_PIN); // LED off initially
}

bool pn532_get_firmware_version(uint8_t *firmware_data, uint8_t len) {
    // Command sequence for GetFirmwareVersion
    uint8_t cmd[] = {
        PN532_PREAMBLE,    // Preamble
        PN532_STARTCODE1,  // Start code 1
        PN532_STARTCODE2,  // Start code 2
        0x02,              // Length of data (2 bytes)
        0xFE,              // LCS (Length Checksum)
        0xD4,              // TFI (Direction: Host to PN532)
        0x02,              // Command: GetFirmwareVersion
        0x2A              // DCS (Data Checksum)
    };

    // Send command
    ret_code_t error_code = tw_master_transmit(PN532_I2C_ADDR, cmd, sizeof(cmd), false);
    if (error_code != SUCCESS) return false;
    _delay_ms(50);

    // Read ACK frame
    uint8_t ack[6];
    error_code = tw_master_receive(PN532_I2C_ADDR, ack, sizeof(ack));
    if (error_code != SUCCESS) return false;
    _delay_ms(50);

    // Read response
    error_code = tw_master_receive(PN532_I2C_ADDR, firmware_data, len);
    if (error_code != SUCCESS) return false;

    return true;
}


#include "lcd.h"  // Include the LCD header for lcd_write functions

bool pn532_read_passive_target(uint8_t *uid, uint8_t *uid_len) {
    // Wake up sequence first
    uint8_t dummy = 0x00;
    tw_master_transmit(PN532_I2C_ADDR, &dummy, 1, true);
    _delay_ms(100);

    // Command for ISO14443A type cards
    uint8_t cmd[] = {
        0x00, 0x00, 0xFF,    // Preamble
        0x05,                // Length
        0xFB,                // LCS
        0xD4,                // TFI
        0x4A,                // InListPassiveTarget
        0x01,                // MaxTg
        0x00,                // BrTy
        0x00                 // Initiator
    };

    // Wait for TWI slave ready
    uint8_t counter = 0;
    while ((TWSR & 0xF8) != TW_MT_SLA_ACK && counter < 100) {
        tw_master_transmit(PN532_I2C_ADDR, &dummy, 1, true);
        _delay_ms(10);
        counter++;
    }

    if (counter >= 100) {
        lcd_clear();
        lcd_write("Timeout waiting");
        lcd_set_cursor(0, 1);
        lcd_write("for slave");
        return false;  // Timeout waiting for slave
    }

    // Send command with START condition
    ret_code_t error_code = tw_master_transmit(PN532_I2C_ADDR, cmd, sizeof(cmd), true);
    if (error_code != SUCCESS) {
        lcd_clear();
        lcd_write("Error sending");
        lcd_set_cursor(0, 1);
        lcd_write("command");
        return false;
    }
    _delay_ms(100);

    // Read ACK
    uint8_t ack[6];
    error_code = tw_master_receive(PN532_I2C_ADDR, ack, sizeof(ack));
    if (error_code != SUCCESS) {
        lcd_clear();
        lcd_write("Error reading");
        lcd_set_cursor(0, 1);
        lcd_write("ACK");
        return false;
    }
    _delay_ms(100);

    // Read response
    uint8_t response[25];  // Response can be up to 25 bytes
    error_code = tw_master_receive(PN532_I2C_ADDR, response, sizeof(response));
    if (error_code != SUCCESS) {
        lcd_clear();
        lcd_write("Error reading");
        lcd_set_cursor(0, 1);
        lcd_write("response");
        return false;
    }

    // Check if a card was found
    if (response[7] != 0x01) {  // NbTg (number of targets found)
        PORTB &= ~(1 << LED_PIN);
        lcd_clear();
        lcd_write("No card found");
        return false;
    }

    // Card found! Get UID
    *uid_len = response[12];  // NFCID length
    if (*uid_len > 7) *uid_len = 7;  // Limit to buffer size
    
    // Copy UID
    for (uint8_t i = 0; i < *uid_len; i++) {
        uid[i] = response[13 + i];
    }

    PORTB |= (1 << LED_PIN);  // Turn LED on
    lcd_clear();
    lcd_write("Card found");
    lcd_set_cursor(0, 1);
    for (uint8_t i = 0; i < *uid_len; i++) {
        lcd_write_hex(uid[i]);
        lcd_write(" ");
    }
    return true;
}