// #include "pn532.h"
// #include <util/delay.h>
// #include "lcd.h"

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

// #include "lcd.h"
// #include "pn532.h"
// #include "twi_master.h"
// #include <avr/io.h>
// #include <avr/interrupt.h>
// #include <util/delay.h>
// #include <stdbool.h>

// // Define PN532 I²C address
// #define PN532_I2C_ADDRESS   0x24  // Adjust as necessary

// // Define PN532 control pins
// #define PN532_IRQ_PIN       PB0  // Adjust as per your wiring
// #define PN532_RESET_PIN     PIND0  // Adjust as per your wiring

// // PN532 Command Codes
// #define PN532_COMMAND_INLISTPASSIVETARGET  0x4A

// // Variables to handle IRQ state
// volatile bool nfc_data_ready = false;

// // Function Prototype for internal function
// static void send_command(const uint8_t *command, uint8_t cmd_len);

// // Interrupt Service Routine for PN532 IRQ pin
// ISR(PCINT0_vect) {
//     if (!(PINB & (1 << PN532_IRQ_PIN))) {
//         nfc_data_ready = true;
//     }
// }

// // Implementation of public and internal functions follows...

// void pn532_init(void) {
//     // Initialize TWI (I2C) interface
//     // tw_init(TW_FREQ_100K, true);  // Initialize with 100kHz frequency and enable pull-up resistors

//     // Configure RESET pin
//     DDRD |= (1 << PN532_RESET_PIN);    // Set RESET pin as output
//     PORTD &= ~(1 << PN532_RESET_PIN);  // Pull RESET low
//     _delay_ms(100);
//     PORTD |= (1 << PN532_RESET_PIN);   // Pull RESET high
//     _delay_ms(100);

//     // Configure IRQ pin as input with pull-up resistor
//     DDRB &= ~(1 << PN532_IRQ_PIN);     // Set IRQ pin as input
//     PORTB |= (1 << PN532_IRQ_PIN);     // Enable pull-up on IRQ pin

//     // Enable pin change interrupt for IRQ pin
//     PCICR |= (1 << PCIE0);             // Enable pin change interrupt 0 (for PCINT[7:0])
//     PCMSK0 |= (1 << PCINT0);           // Enable interrupt on PCINT0 (PINB0)

//     // Enable global interrupts
//     sei();

//     // Send SAMConfiguration command to set PN532 to normal mode
//     uint8_t sam_config[] = {
//         0xD4, 0x14, 0x00, 0x01  // Command: SAMConfiguration, Mode: Normal
//     };
//     send_command(sam_config, sizeof(sam_config));
// }

// bool pn532_data_ready(void) {
//     if (nfc_data_ready) {
//         nfc_data_ready = false;
//         return true;
//     }
//     return false;
// }

// static void send_command(const uint8_t *command, uint8_t cmd_len) {
//     uint8_t frame[9 + cmd_len];  // Frame size: Preamble (1) + Start Codes (2) + Length (1) + LCS (1) + TFI (1) + Data (cmd_len) + DCS (1) + Postamble (1)
//     uint8_t idx = 0;

//     // Preamble and Start Code
//     frame[idx++] = 0x00;  // Preamble
//     frame[idx++] = 0x00;  // Start Code 1
//     frame[idx++] = 0xFF;  // Start Code 2

//     // Length and Length Checksum
//     uint8_t length = cmd_len + 1;  // Length of TFI + Data
//     frame[idx++] = length;
//     frame[idx++] = (uint8_t)(~length + 1);  // Length checksum

//     // TFI
//     frame[idx++] = 0xD4;  // Host-to-PN532

//     // Data
//     uint8_t checksum = 0xD4;  // Initialize checksum with TFI
//     for (uint8_t i = 0; i < cmd_len; i++) {
//         frame[idx++] = command[i];
//         checksum += command[i];
//     }

//     // Data Checksum and Postamble
//     frame[idx++] = (uint8_t)(~checksum + 1);  // Data checksum
//     frame[idx++] = 0x00;  // Postamble

//     // Send frame via TWI
//     ret_code_t error = tw_master_transmit(PN532_I2C_ADDRESS, frame, idx, false);
//     if (error != SUCCESS) {
//         // Handle error appropriately
//         lcd_set_cursor(0, 1);
//         lcd_write("TX Error");
//     }

//     _delay_ms(10);
// }

// int16_t pn532_read_passive_target_id(uint8_t *uid, uint8_t uid_len) {
//     uint8_t response[32];
//     ret_code_t error;

//     // Receive data from PN532
//     error = tw_master_receive(PN532_I2C_ADDRESS, response, sizeof(response));
//     if (error != SUCCESS) {
//         return -1;  // Error during receive
//     }

//     // Parse response frame
//     uint8_t idx = 0;

//     // Preamble and Start Code
//     if (response[idx++] != 0x01 ||  // Status byte
//         response[idx++] != 0x00 ||  // Preamble
//         response[idx++] != 0x00 ||  // Start Code 1
//         response[idx++] != 0xFF) {  // Start Code 2
//         return -2;  // Invalid frame
//     }

//     // Length and Length Checksum
//     uint8_t length = response[idx++];
//     uint8_t lcs = response[idx++];
//     if ((length + lcs) != 0x00) {
//         return -3;  // Invalid length checksum
//     }

//     // TFI
//     uint8_t tfi = response[idx++];
//     if (tfi != 0xD5) {
//         return -4;  // Unexpected TFI
//     }

//     // Response Code
//     uint8_t response_code = response[idx++];
//     if (response_code != 0x4B) {
//         return -5;  // Unexpected response code
//     }

//     // Number of Targets
//     uint8_t num_targets = response[idx++];
//     if (num_targets == 0) {
//         return 0;  // No card detected
//     }

//     // Target Data
//     idx++;  // Skip Target Number (1 byte)

//     // SENS_RES (2 bytes) and SEL_RES (1 byte)
//     idx += 3;

//     // UID Length
//     uint8_t uid_length = response[idx++];
//     if (uid_length > uid_len) {
//         return -6;  // Provided UID buffer is too small
//     }

//     // UID
//     for (uint8_t i = 0; i < uid_length; i++) {
//         uid[i] = response[idx++];
//     }

//     return uid_length;
// }

// void pn532_start_card_detection(void) {
//     // Send InListPassiveTarget command to start card detection
//     uint8_t inlist_passive_target[] = {
//         0xD4, 0x4A, 0x01, 0x00  // Command: InListPassiveTarget, MaxTg: 1, BaudRate: 106 kbps
//     };
//     send_command(inlist_passive_target, sizeof(inlist_passive_target));
// }

#include <string.h>
#include <stdio.h>
#include "pn532.h"
#include "twi_master.h"
#include "uart.h"
#include <util/delay.h>

#define PN532_I2C_ADDRESS 0x24

#define PN532_CMD_GETFIRMWAREVERSION 0x02
#define PN532_CMD_SAMCONFIGURATION   0x14
#define PN532_CMD_INLISTPASSIVETARGET 0x4A

#define PN532_ACK_PACKET    {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00}
#define PN532_ACK_PACKET_LEN 6

#define PN532_POSTAMBLE 0x00
#define PN532_PREAMBLE  0x00
#define PN532_STARTCODE 0xFF

static uint8_t pn532_write_command(uint8_t* cmd, uint8_t cmd_len);
static bool pn532_read_ack(void);
static bool pn532_read_response(uint8_t* buffer, uint8_t len);

void pn532_reset() {
    // Add hardware reset logic here if needed (GPIO-based reset pin control)
    _delay_ms(400); // Wait for PN532 to stabilize after reset
}

void pn532_i2c_scan(void) {
    uart_println("\r\n--- I2C Bus Scanner ---");
    uint8_t devices_found = 0;

    for (uint8_t address = 1; address < 128; address++) {
        // Use correct TWI function signature from your codebase
        ret_code_t result = tw_master_transmit(address, NULL, 0, false);
        if (result == SUCCESS) {
            char hex_str[8];
            sprintf(hex_str, "%02X", address);  // Convert to hex string
            
            uart_print("Device at 0x");
            uart_print(hex_str);
            uart_print("\r\n");
            
            devices_found++;
        }
        _delay_ms(5);
    }

    char count_str[16];
    sprintf(count_str, "Found %d devices", devices_found);
    uart_println(count_str);
}

// Modify pn532.c
bool pn532_init(void) {
    uart_println("Starting PN532 init sequence...");
    
    pn532_i2c_scan();
    
    // Hardware reset
    pn532_reset();
    _delay_ms(500);  // Increased delay after reset
    
    // Get firmware version as communication test
    uint8_t version_cmd[] = {PN532_CMD_GETFIRMWAREVERSION};
    uart_println("Sending firmware version command...");
    
    if (!pn532_write_command(version_cmd, sizeof(version_cmd))) {
        uart_println("Failed to send firmware version command");
        return false;
    }
    
    _delay_ms(50);  // Wait for processing
    
    if (!pn532_read_ack()) {
        uart_println("No ACK received for firmware version");
        return false;
    }
    
    uint8_t response[6];
    if (!pn532_read_response(response, sizeof(response))) {
        uart_println("Failed to read firmware response");
        return false;
    }
    
    char version_str[32];
    sprintf(version_str, "Version: %02X.%02X", response[2], response[3]);
    uart_println(version_str);
    
    // SAM Configuration
    uint8_t sam_config[] = {
        PN532_CMD_SAMCONFIGURATION,
        0x01,  // Normal mode
        0x14,  // Timeout 1s
        0x01   // Use IRQ pin
    };
    
    uart_println("Configuring SAM...");
    if (!pn532_write_command(sam_config, sizeof(sam_config))) {
        uart_println("SAM config command failed");
        return false;
    }
    
    _delay_ms(50);
    
    if (!pn532_read_ack()) {
        uart_println("SAM config ACK failed");
        return false;
    }
    
    uart_println("PN532 initialization complete");
    return true;
}

bool pn532_get_firmware_version(uint8_t* version_data) {
    uint8_t cmd[] = {PN532_CMD_GETFIRMWAREVERSION};
    if (!pn532_write_command(cmd, sizeof(cmd))) {
        return false;
    }

    return pn532_read_response(version_data, 6);
}

bool pn532_start_passive_target_detection(uint8_t baudrate) {
    uint8_t cmd[] = {PN532_CMD_INLISTPASSIVETARGET, 0x01, baudrate};
    return pn532_write_command(cmd, sizeof(cmd));
}

bool pn532_read_passive_target_id(uint8_t* uid, uint8_t* uid_len) {
    uint8_t response[24];
    if (!pn532_read_response(response, sizeof(response))) {
        return false;
    }

    if (response[7] != 1) { // Check number of targets detected
        return false;
    }

    *uid_len = response[12]; // Length of UID
    for (uint8_t i = 0; i < *uid_len; i++) {
        uid[i] = response[13 + i];
    }

    return true;
}

// Low-level functions
static uint8_t pn532_write_command(uint8_t* cmd, uint8_t cmd_len) {
    uint8_t buffer[8 + cmd_len];
    buffer[0] = PN532_PREAMBLE;
    buffer[1] = PN532_STARTCODE;
    buffer[2] = cmd_len + 1; // Data length
    buffer[3] = ~buffer[2] + 1; // Data length checksum
    buffer[4] = 0xD4; // Host to PN532 (TFI)
    
    uint8_t checksum = 0xD4;
    for (uint8_t i = 0; i < cmd_len; i++) {
        buffer[5 + i] = cmd[i];
        checksum += cmd[i];
    }
    
    buffer[5 + cmd_len] = ~checksum; // Checksum
    buffer[6 + cmd_len] = PN532_POSTAMBLE;

    return tw_master_transmit(PN532_I2C_ADDRESS, buffer, sizeof(buffer), false);
}

static bool pn532_read_ack() {
    uint8_t ack[PN532_ACK_PACKET_LEN];
    if (!pn532_read_response(ack, sizeof(ack))) {
        return false;
    }

    const uint8_t expected_ack[PN532_ACK_PACKET_LEN] = PN532_ACK_PACKET;
    for (uint8_t i = 0; i < PN532_ACK_PACKET_LEN; i++) {
        if (ack[i] != expected_ack[i]) {
            return false;
        }
    }

    return true;
}

static bool pn532_read_response(uint8_t* buffer, uint8_t len) {
    uint8_t data[len];
    if (tw_master_receive(PN532_I2C_ADDRESS, data, len) != SUCCESS) {
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = data[i];
    }

    return true;
}

// Add to pn532.c
bool pn532_start_card_detection(void) {
    uint8_t cmd[] = {
        PN532_CMD_INLISTPASSIVETARGET,
        0x01,  // Max targets
        PN532_MIFARE_ISO14443A
    };
    
    return pn532_write_command(cmd, sizeof(cmd)) && pn532_read_ack();
}

bool pn532_data_ready(void) {
    uint8_t status;
    if (!tw_master_receive(PN532_I2C_ADDRESS, &status, 1)) {
        return false;
    }
    return (status & 0x01) != 0;
}

bool pn532_read_card_uid(uint8_t* uid, uint8_t* uid_length) {
    uint8_t response[20];
    if (!pn532_read_response(response, sizeof(response))) {
        return false;
    }
    
    if (response[0] != 1) { // Number of targets found
        return false;
    }
    
    *uid_length = response[5];
    for (uint8_t i = 0; i < *uid_length; i++) {
        uid[i] = response[6 + i];
    }
    
    return true;
}

bool pn532_mifare_auth_block(uint8_t* uid, uint8_t uid_length, uint8_t block, const uint8_t* key) {
    uint8_t cmd[14] = {
        0x40,  // CMD MifareClassicAuthBlock
        block,
        0x60,  // Auth with KEY A
    };
    
    // Copy key
    for (uint8_t i = 0; i < 6; i++) {
        cmd[3 + i] = key[i];
    }
    
    // Copy UID
    for (uint8_t i = 0; i < uid_length; i++) {
        cmd[9 + i] = uid[i];
    }
    
    return pn532_write_command(cmd, sizeof(cmd)) && pn532_read_ack();
}

bool pn532_mifare_read_block(uint8_t block, uint8_t* data) {
    uint8_t cmd[] = {
        0x30,  // CMD MifareRead
        block
    };
    
    if (!pn532_write_command(cmd, sizeof(cmd)) || !pn532_read_ack()) {
        return false;
    }
    
    uint8_t response[18];
    if (!pn532_read_response(response, sizeof(response))) {
        return false;
    }
    
    memcpy(data, &response[1], 16);
    return true;
}

bool pn532_mifare_write_block(uint8_t block, const uint8_t* data) {
    uint8_t cmd[18] = {
        0xA0,  // CMD MifareWrite
        block
    };
    
    memcpy(&cmd[2], data, 16);
    
    return pn532_write_command(cmd, sizeof(cmd)) && pn532_read_ack();
}