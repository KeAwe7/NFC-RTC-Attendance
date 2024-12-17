// #include "pn532_i2c.h"
// #include <util/delay.h>
// #include "lcd.h"

// uint8_t pn532_firmware_ver[4];

// static bool wait_for_ack(void) {
//     uint8_t ack[6];
//     _delay_ms(50);  // Wait for response
    
//     if (tw_master_receive(PN532_I2C_ADDRESS, ack, 6) != SUCCESS) {
//         return false;
//     }
    
//     // Check ACK frame: 00 00 FF 00 FF 00
//     return (ack[0] == 0x00 && 
//             ack[1] == 0x00 && 
//             ack[2] == 0xFF && 
//             ack[3] == 0x00 &&
//             ack[4] == 0xFF && 
//             ack[5] == 0x00);
// }

// void pn532_init(void) {
//     lcd_set_cursor(0, 1);
//     lcd_write("Step 1...");
//     _delay_ms(300);

//     // Test basic I2C communication first
//     uint8_t test_byte = 0x55;
//     if (tw_master_transmit(PN532_I2C_ADDRESS, &test_byte, 1, false) != SUCCESS) {
//         lcd_write("I2C Failed");
//         return;
//     }
//     lcd_set_cursor(0, 1);
//     lcd_write("Step 2...");

//     // Reset sequence
//     uint8_t reset_cmd[] = {0x00, 0x00, 0xff};
//     if (tw_master_transmit(PN532_I2C_ADDRESS, reset_cmd, sizeof(reset_cmd), false) != SUCCESS) {
//         lcd_write("Reset Failed");
//         return;
//     }
//     _delay_ms(500);  // Longer delay after reset
//     lcd_set_cursor(0, 1);
//     lcd_write("Step 3...");

//     // Simplified SAM config
//     uint8_t sam_config[] = {
//         0x00, 0x00, 0xFF,   // Preamble
//         0x05,               // Length
//         0xFB,               // LCS
//         0xD4,               // Host to PN532
//         0x14,               // SAM Command
//         0x01,               // Normal Mode
//         0x01,               // Timeout 
//         0x00,              // IRQ off
//         0x00               // Checksum
//     };
    
//     if (tw_master_transmit(PN532_I2C_ADDRESS, sam_config, sizeof(sam_config), false) != SUCCESS) {
//         lcd_write("SAM Failed");
//         return;
//     }
//     lcd_write("Done Init");
// }

// bool get_firmware_version(void) {
//     uint8_t cmd[] = {
//         0x00, 0x00, 0xFF,   // Preamble
//         0x02,               // Length of following data
//         0xFE,               // Length checksum (255 - 2)
//         0xD4,               // PN532_HOSTTOPN532
//         0x02,               // Command GetFirmwareVersion
//         0x2A                // Checksum
//     };
    
//     if (tw_master_transmit(PN532_I2C_ADDRESS, cmd, sizeof(cmd), false) != SUCCESS) {
//         return false;
//     }

//     if (!wait_for_ack()) {
//         return false;
//     }

//     _delay_ms(50);

//     uint8_t response[12];
//     if (tw_master_receive(PN532_I2C_ADDRESS, response, sizeof(response)) != SUCCESS) {
//         return false;
//     }

//     // Check response format
//     if (response[0] != 0x00 || 
//         response[1] != 0x00 || 
//         response[2] != 0xFF || 
//         response[3] != 0x06) {
//         return false;
//     }

//     pn532_firmware_ver[0] = response[7];  // IC version
//     pn532_firmware_ver[1] = response[8];  // Version
//     pn532_firmware_ver[2] = response[9];  // Revision
//     pn532_firmware_ver[3] = response[10]; // Support

//     return true;
// }

// bool pn532_read_passive_target(uint8_t *uid, uint8_t *uid_len) {
//     uint8_t cmd[] = {
//         PN532_PREAMBLE,
//         PN532_STARTCODE1,
//         PN532_STARTCODE2,
//         0x04,                // Length
//         0xFC,                // LCS
//         PN532_HOSTTOPN532,
//         PN532_COMMAND_INLISTPASSIVETARGET,
//         0x01,                // MaxTg = 1 target
//         0x00,                // BrTy = ISO14443A
//         0x23                 // Checksum
//     };

//     if (tw_master_transmit(PN532_I2C_ADDRESS, cmd, sizeof(cmd), false) != SUCCESS) {
//         return false;
//     }
//     _delay_ms(150);

//     uint8_t response[20];
//     if (tw_master_receive(PN532_I2C_ADDRESS, response, sizeof(response)) != SUCCESS) {
//         return false;
//     }

//     // Check if a card was found
//     if (response[7] != 0x01) {  // No targets found
//         return false;
//     }

//     *uid_len = response[12];  // NFCID Length
//     if (*uid_len > 7) *uid_len = 7;  // Limit to buffer size

//     // Copy UID
//     for (uint8_t i = 0; i < *uid_len; i++) {
//         uid[i] = response[13 + i];
//     }

//     return true;
// }

#include <stddef.h>
#include "pn532_i2c.h"
#include <util/delay.h>

// Global variable to store last command
static uint8_t _command = 0;

// HW147 specific wakeup sequence
void pn532_wakeup(void)
{
    // Specific wakeup sequence for HW147
    uint8_t wakeup[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    tw_master_transmit(PN532_I2C_ADDRESS, wakeup, sizeof(wakeup), false);
    _delay_ms(10);
}

// Write command to PN532
uint8_t pn532_write_command(const uint8_t *header, uint8_t hlen, 
                            const uint8_t *body, uint8_t blen)
{
    uint8_t packet[PN532_FRAME_MAX_LENGTH];
    uint8_t packet_len = 0;

    // Packet start
    packet[packet_len++] = 0x00;   // Preamble
    packet[packet_len++] = 0x00;   // Preamble
    packet[packet_len++] = 0xFF;   // Start of packet

    // Length
    uint8_t length = hlen + blen + 1;  // +1 for TFI
    packet[packet_len++] = length;
    packet[packet_len++] = ~length + 1;  // Checksum of length

    // TFI (Direction: Host to PN532)
    packet[packet_len++] = 0xD4;

    // Header
    for (uint8_t i = 0; i < hlen; i++) {
        packet[packet_len++] = header[i];
    }

    // Body
    if (body && blen > 0) {
        for (uint8_t i = 0; i < blen; i++) {
            packet[packet_len++] = body[i];
        }
    }

    // Postamble
    packet[packet_len++] = 0x00;

    // Store the command for response matching
    _command = header[0];

    // Transmit the packet via I2C
    ret_code_t ret = tw_master_transmit(PN532_I2C_ADDRESS, packet, packet_len, false);
    
    return (ret == SUCCESS) ? 0 : -1;
}

// Read response from PN532
int16_t pn532_read_response(uint8_t *buf, uint8_t len, uint16_t timeout)
{
    uint8_t tmp[PN532_FRAME_MAX_LENGTH];
    
    // Wait for response with timeout
    uint16_t timer = 0;
    while (timer < timeout) {
        // Try to read response
        ret_code_t ret = tw_master_receive(PN532_I2C_ADDRESS, tmp, 7);
        
        if (ret == SUCCESS) {
            // Validate response packet
            if (tmp[0] == 0x00 && tmp[1] == 0x00 && tmp[2] == 0xFF) {
                uint8_t packet_len = tmp[3];
                uint8_t packet_len_checksum = tmp[4];
                
                // Verify length
                if ((uint8_t)(packet_len + packet_len_checksum) == 0xFF) {
                    // Read full response
                    ret = tw_master_receive(PN532_I2C_ADDRESS, buf, len);
                    
                    if (ret == SUCCESS) {
                        return len;
                    }
                }
            }
        }
        
        _delay_ms(1);
        timer++;
    }
    
    return -1;  // Timeout or error
}

// Read acknowledge frame
int8_t pn532_read_ack_frame(void)
{
    uint8_t ack_frame[6];
    ret_code_t ret = tw_master_receive(PN532_I2C_ADDRESS, ack_frame, 6);
    
    if (ret == SUCCESS) {
        // Check for valid ACK frame
        return (ack_frame[0] == 0x00 && 
                ack_frame[1] == 0x00 && 
                ack_frame[2] == 0xFF && 
                ack_frame[3] == 0x00 && 
                ack_frame[4] == 0xFF) ? 0 : -1;
    }
    
    return -1;
}

// Initialize PN532
bool pn532_i2c_init(void)
{
    // Multiple attempts to initialize
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        // Wakeup the module
        pn532_wakeup();
        
        // Specific SAM configuration for HW147
        uint8_t sam_config[] = {
            PN532_COMMAND_SAMCONFIGURATION, 
            0x01,   // Normal mode
            0x00,   // Timeout 0x00 = default
            0x00    // Use IRQ pin
        };
        
        // Get firmware version
        uint8_t cmd[] = {PN532_COMMAND_GETFIRMWAREVERSION};
        uint8_t response[5];
        
        // Set SAM configuration first
        if (pn532_write_command(sam_config, sizeof(sam_config), NULL, 0) == 0) {
            if (pn532_read_ack_frame() == 0) {
                // Delay after SAM config
                _delay_ms(10);
            }
        }
        
        // Try to get firmware version
        if (pn532_write_command(cmd, sizeof(cmd), NULL, 0) == 0) {
            if (pn532_read_ack_frame() == 0) {
                if (pn532_read_response(response, sizeof(response), 1000) > 0) {
                    return true;  // Successfully initialized
                }
            }
        }
        
        // Small delay between attempts
        _delay_ms(100);
    }
    
    return false;
}