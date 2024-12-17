/* NFC TEST */

// main.c
// #include "lcd.h"
// #include "pn532.h"
// #include "twi_master.h"
// #include <util/delay.h>

// int main(void) {
//     tw_init(TW_FREQ_100K, true); // Initialize I2C with 100kHz frequency
//     _delay_ms(500);

//     lcd_init();
//     _delay_ms(500);
//     lcd_write("Initializing...");

//     pn532_init();
//     _delay_ms(500);

//     uint8_t firmware_data[8];
//     char firmware_str[32];

//     if (pn532_get_firmware_version(firmware_data, sizeof(firmware_data))) {
//         lcd_set_cursor(0, 1);
//         lcd_write("PN532 Ready");
//         // Configure the PN532
//         uint8_t samconfig_cmd[] = {0xD4, 0x14, 0x01, 0x14, 0x01};
//         tw_master_transmit(PN532_I2C_ADDR, samconfig_cmd, sizeof(samconfig_cmd), false);
//     } else {
//         // lcd_set_cursor(0, 1);
//         // lcd_write("PN532 Error");
//         while (1); // Halt if PN532 initialization fails
//     }

//     // uint8_t hours, minutes, seconds;
//     // ds1307_set_time(10, 30, 0); // Set time to 10:30:00
//     // ds1307_get_time(&hours, &minutes, &seconds);

//     // char time_str[16];
//     // snprintf(time_str, sizeof(time_str), "Time: %02d:%02d:%02d", hours, minutes, seconds);
//     // lcd_write(time_str);

//     // uint8_t nfc_data[16];
//     // pn532_read_data(nfc_data, sizeof(nfc_data));

//     while (1) {
//         // Your main application loop
//     }

//     return 0;
// }

// int main(void) {
//     tw_init(TW_FREQ_100K, true); // Initialize I2C with 100kHz frequency

//     lcd_init();
//     _delay_ms(500);

//     lcd_write("Hello, World!");
//     while (1) {
//         // Loop forever
//     }

//     return 0;
// }

/* NFC SPI TEST 2 */

#include "pn532_spi.h"
#include "spi.h"
#include "lcd.h"
#include "twi_master.h"
#include "uart.h"
#include <util/delay.h>
#include <stdio.h>

void spi_init(void) {
    SPI_init_master();
}

void debug_ack_frame(void) {
    PN532_CS_LOW();
    _delay_ms(2);
    
    uart_println("\nReading ACK frame bytes:");
    
    SPI_transfer(DATA_READ);
    char debug[32];
    
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t byte = SPI_transfer(0x00);
        sprintf(debug, "ACK[%d]: 0x%02X", i, byte);
        uart_println(debug);
    }
    
    PN532_CS_HIGH();
}

void pn532_hardware_reset(void) {
    uart_println("Performing hardware reset...");
    
    // Configure reset pin as output
    PN532_RESET_DDR |= (1 << PN532_RESET_PIN);
    
    // Reset sequence
    PN532_RESET_PORT &= ~(1 << PN532_RESET_PIN);  // Reset low
    _delay_ms(400);  // Hold reset
    PN532_RESET_PORT |= (1 << PN532_RESET_PIN);   // Reset high
    _delay_ms(100);  // Wait for chip ready
}

// Modify status check function
bool check_pn532_ready(void) {
    uint8_t retries = 0;
    uint8_t status;
    char debug[32];
    
    // Test power first
    PN532_CS_HIGH();
    _delay_ms(100);
    
    while (retries < 10) {
        PN532_CS_LOW();
        _delay_ms(2);
        
        // Send dummy byte to test connection
        SPI_transfer(0xFF);
        status = SPI_transfer(STATUS_READ);
        
        PN532_CS_HIGH();
        _delay_ms(2);
        
        sprintf(debug, "Status check %d: 0x%02X", retries, status);
        uart_println(debug);
        
        if (status == 0x00) {
            uart_println("No response - check connections");
            PORTB ^= (1 << PB5); // Toggle LED on error
            return false;
        }
        
        retries++;
        _delay_ms(50);
    }
    
    return false;
}
bool try_send_command(PN532_SPI *pn532, uint8_t *cmd, uint8_t cmd_len, uint8_t max_retries) {
    uint8_t tries = 0;
    
    while (tries < max_retries) {
        uart_println("\nCommand bytes:");
        for (uint8_t i = 0; i < cmd_len; i++) {
            char debug[32];
            sprintf(debug, "cmd[%d]: 0x%02X", i, cmd[i]);
            uart_println(debug);
        }
        
        if (!check_pn532_ready()) {
            uart_println("Device not ready, retrying...");
            tries++;
            continue;
        }
        
        // Proper frame construction
        PN532_CS_LOW();
        _delay_ms(2);
        
        // Frame structure:
        SPI_transfer(DATA_WRITE);         // Start write
        SPI_transfer(PN532_PREAMBLE);     // 0x00
        SPI_transfer(PN532_STARTCODE1);   // 0x00
        SPI_transfer(PN532_STARTCODE2);   // 0xFF
        
        uint8_t length = cmd_len + 1;     // Command length + TFI
        SPI_transfer(length);             // LEN
        SPI_transfer(~length + 1);        // LCS (Length checksum)
        
        SPI_transfer(PN532_HOSTTOPN532);  // TFI
        
        // Calculate data checksum
        uint8_t sum = PN532_HOSTTOPN532;
        for (uint8_t i = 0; i < cmd_len; i++) {
            SPI_transfer(cmd[i]);
            sum += cmd[i];
            _delay_us(100);
        }
        
        SPI_transfer(~sum + 1);           // DCS (Data checksum)
        SPI_transfer(PN532_POSTAMBLE);    // 0x00
        
        PN532_CS_HIGH();
        _delay_ms(5);
        
        // Check ACK
        PN532_CS_LOW();
        uint8_t ack = SPI_transfer(STATUS_READ);
        PN532_CS_HIGH();
        
        char debug[32];
        sprintf(debug, "ACK status: 0x%02X", ack);
        uart_println(debug);
        
        if (ack == 0x01) {
            uart_println("Command ACK received");
            return true;
        }
        
        tries++;
        _delay_ms(100);
    }
    
    uart_println("Command failed after max retries");
    return false;
}

int main(void) {
    PN532_SPI pn532;
    uart_init(9600);
    
    // Initialize LCD and SPI as before
    tw_init(TW_FREQ_400K, true);
    lcd_init();
    lcd_write("System Init...");
    _delay_ms(1000);

    // Initialize SPI with proper setup
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
    spi_init();
    
    // Initialize PN532 properly
    PN532_SPI_Init(&pn532, 10);  // SS on pin 10
    PN532_SPI_Begin(&pn532);
    PN532_SPI_Wakeup(&pn532);
    
    lcd_set_cursor(0, 1);
    lcd_write("PN532 Init...");
    _delay_ms(1000);
    
    // Main loop
    uint8_t uid[7];
    uint8_t uid_length;
    char debug[32];
    
    // while(1) {
    //     lcd_clear();
    //     lcd_write("Scanning...");
    //     uart_println("\nWaiting for card...");

    //     // Full wakeup sequence
    //     PN532_CS_HIGH();
    //     _delay_ms(100);
    //     PN532_CS_LOW();
    //     _delay_ms(2);
    //     PN532_CS_HIGH();
    //     _delay_ms(100);

    //     if (!check_pn532_ready()) {
    //         uart_println("PN532 not responding");
    //         continue;
    //     }        

    //     uint8_t cmd[] = {
    //         PN532_CMD_INLISTPASSIVETARGET,
    //         0x01,  // Max 1 card
    //         MIFARE_ISO14443A
    //     };
        
    //     uart_println("Sending card detect command...");
    //     int8_t status = PN532_SPI_WriteCommand(&pn532, cmd, sizeof(cmd), NULL, 0);
        
    //     sprintf(debug, "Command status: %d", status);
    //     uart_println(debug);
        
    //     if (status != 0) {
    //         uart_println("Command failed - resetting PN532");
    //         PN532_SPI_Wakeup(&pn532);
    //         _delay_ms(200);
    //         continue;
    //     }

    //     _delay_ms(200);
        
    //     char debug[32];
    //     sprintf(debug, "Command status: %d", status);
    //     uart_println(debug);
        
    //     if (status == 0) {
    //         // Wait for card with timeout
    //         uint16_t timeout = 0;
    //         while (!PN532_SPI_IsReady(&pn532)) {
    //             _delay_ms(10);
    //             timeout++;
    //             if (timeout > 100) { // 1 second timeout
    //                 uart_println("Response timeout");
    //                 break;
    //             }
    //         }
            
    //         uint8_t response[20];
    //         int16_t respLength = PN532_SPI_ReadResponse(&pn532, response, sizeof(response), 1000);
            
    //         sprintf(debug, "Response length: %d", respLength);
    //         uart_println(debug);
            
    //         if (respLength > 0) {
    //             uart_println("Response bytes:");
    //             for (uint8_t i = 0; i < respLength; i++) {
    //                 sprintf(debug, "byte[%d]: 0x%02X", i, response[i]);
    //                 uart_println(debug);
    //             }
                
    //             if (response[0] == 1) {  // Card found
    //                 uid_length = response[5];
    //                 memcpy(uid, &response[6], uid_length);
                    
    //                 lcd_clear();
    //                 lcd_write("Card Found!");
    //                 lcd_set_cursor(0, 1);
                    
    //                 // Print UID
    //                 for (uint8_t i = 0; i < uid_length; i++) {
    //                     char hex[3];
    //                     sprintf(hex, "%02X", uid[i]);
    //                     lcd_write(hex);
    //                     if (i < uid_length - 1) lcd_write(":");
    //                 }
    //                 uart_println("Card detected!");
    //                 _delay_ms(2000);  // Show UID longer
    //             }
    //         }
    //     }
        
    //     _delay_ms(200);  // Delay between scans
    // }

    while(1) {
    lcd_clear();
    lcd_write("Scanning...");
    uart_println("\nWaiting for card...");
    
    uint8_t cmd[] = {
        PN532_CMD_INLISTPASSIVETARGET,
        0x01,
        MIFARE_ISO14443A
    };
    
    if (try_send_command(&pn532, cmd, sizeof(cmd), 3)) {
        uart_println("Command sent successfully");
        
        // Wait for response
        uint8_t timeout = 0;
        while (!PN532_SPI_IsReady(&pn532) && timeout < 100) {
            _delay_ms(10);
            timeout++;
        }
        
        if (timeout >= 100) {
            uart_println("Response timeout");
            continue;
        }
        
        // Read response
        uint8_t response[20];
        int16_t respLength = PN532_SPI_ReadResponse(&pn532, response, sizeof(response), 1000);
        
        if (respLength > 0 && response[0] == 1) {
            uart_println("Card found!");
            // Process card data...
        }
    } else {
        uart_println("Command failed after retries");
        pn532_hardware_reset();
        _delay_ms(200);
    }
    
    _delay_ms(100);
}
    
    return 0;
}


/* NFC TEST NEW CODE */
// #include "lcd.h"
// #include "twi_master.h"
// #include "pn532_i2c.h"
// #include <util/delay.h>

// int main(void) {
//     // Initialize I2C
//     tw_init(TW_FREQ_100K, true);
//     _delay_ms(500);

//     // Initialize LCD
//     lcd_init();
//     _delay_ms(100);
//     lcd_write("NFC Init...");

//     // Initialize PN532
//     pn532_init();
//     _delay_ms(2000);

//     // Check communication by reading firmware version
//     lcd_set_cursor(0, 1);
//     if (!get_firmware_version()) {
//         lcd_clear();
//         lcd_write("PN532 Init Fail!");
//         while(1);
//     }

//     // Display firmware version
//     lcd_clear();
//     lcd_write("FW: ");
//     for (uint8_t i = 0; i < 4; i++) {
//         lcd_write_hex(pn532_firmware_ver[i]);
//         lcd_write(" ");
//     }
//     _delay_ms(2000);

//     lcd_clear();
//     lcd_write("Ready to scan");

//     uint8_t uid[7];
//     uint8_t uid_len;
//     bool card_detected = false;

//     while (1) {
//         if (pn532_read_passive_target(uid, &uid_len)) {
//             if (!card_detected) {
//                 lcd_clear();
//                 lcd_write("Card Found!");
//                 lcd_set_cursor(0, 1);
                
//                 // Display UID
//                 for (uint8_t i = 0; i < uid_len; i++) {
//                     lcd_write_hex(uid[i]);
//                     lcd_write(" ");
//                 }
//                 card_detected = true;
//             }
//         } else {
//             if (card_detected) {
//                 lcd_clear();
//                 lcd_write("Ready to scan");
//                 card_detected = false;
//             }
//         }
//         _delay_ms(100);
//     }

//     return 0;
// }

// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdio.h>
// #include "twi_master.h"
// #include "pn532_i2c.h"
// #include "lcd.h"

// int main(void)
// {
//     // Initialize I2C (TWI)
//     tw_init(TW_FREQ_100K, true);
    
//     // Initialize LCD
//     lcd_init();
    
//     // Clear LCD and show initial message
//     lcd_clear();
//     lcd_write("PN532 I2C Test");
//     _delay_ms(1000);
    
//     // Initialize PN532
//     lcd_clear();
//     if (pn532_init()) {
//         lcd_write("PN532 Init: OK");
//         _delay_ms(1000);
        
//         // Get firmware version
//         uint8_t cmd[] = {PN532_COMMAND_GETFIRMWAREVERSION};
//         uint8_t response[5];
        
//         if (pn532_write_command(cmd, sizeof(cmd), NULL, 0) == 0) {
//             if (pn532_read_ack_frame() == 0) {
//                 if (pn532_read_response(response, sizeof(response), 1000) > 0) {
//                     // Clear LCD and show firmware version
//                     lcd_clear();
//                     lcd_write("FW Ver:");
                    
//                     // Move to second line
//                     lcd_set_cursor(0, 1);
                    
//                     // Prepare buffer for version details
//                     char buffer[16];
//                     snprintf(buffer, sizeof(buffer), 
//                              "IC:0x%02X V:0x%02X", 
//                              response[1], response[2]);
                    
//                     // Write to LCD
//                     lcd_write(buffer);
                    
//                     // Optional: show revision on next screen
//                     _delay_ms(2000);
                    
//                     lcd_clear();
//                     lcd_write("Rev:");
//                     lcd_set_cursor(0, 1);
//                     snprintf(buffer, sizeof(buffer), "0x%02X", response[3]);
//                     lcd_write(buffer);
//                 }
//             }
//         } else {
//             lcd_clear();
//             lcd_write("Cmd Write Fail");
//         }
//     } else {
//         lcd_clear();
//         lcd_write("PN532 Init Fail");
//     }
    
//     // Infinite loop
//     while (1) {
//         _delay_ms(1000);
//     }
    
//     return 0;
// }

/* RTC TEST */

// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdio.h>
// #include "rtc.h"
// #include "lcd.h"
// #include "twi_master.h"

// int main(void) {
//     // Initialize TWI (I2C), UART for potential debugging, LCD, and RTC
//     tw_init(TW_FREQ_100K, true);
//     lcd_init();
//     ds1307_init();

//     // Set an initial time (optional)
//     DS1307_Time initial_time = {
//         .seconds = 0,
//         .minutes = 30,
//         .hours = 14,
//         .day = 5,     // Friday
//         .date = 6,    // 6th of the month
//         .month = 12,  // December
//         .year = 24    // 2024
//     };
//     ds1307_set_time(&initial_time);

//     // Main loop to continuously display time
//     while (1) {
//         DS1307_Time current_time;
        
//         // Clear previous display
//         lcd_clear();
        
//         // Get current time
//         if (ds1307_get_time(&current_time)) {
//             // Set cursor to first line
//             lcd_set_cursor(0, 0);
            
//             // Format and display time
//             char time_str[32];
//             snprintf(time_str, sizeof(time_str), 
//                      "%02d:%02d:%02d %02d/%02d/20%02d", 
//                      current_time.hours, 
//                      current_time.minutes, 
//                      current_time.seconds,
//                      current_time.date,
//                      current_time.month,
//                      current_time.year);
            
//             lcd_write(time_str);
//         }
//         else {
//             // Error handling
//             lcd_write("RTC Read Error");
//         }

//         // Wait before next update
//         _delay_ms(1000);
//     }

//     return 0;
// }

// #include "lcd.h"
// #include "pn532.h"
// #include "twi_master.h"
// #include <util/delay.h>

// int main(void) {
//     ret_code_t error_code;
    
//     // Initialize I2C at 250kHz - working speed
//     tw_init(TW_FREQ_250K, true);
//     _delay_ms(500);

//     lcd_init();
//     _delay_ms(500);
//     lcd_write("Init...");

//     pn532_init();
//     _delay_ms(500);

//     // Wake sequence - working
//     lcd_set_cursor(0, 1);
//     lcd_write("Wake..");
//     uint8_t dummy = 0x00;
//     error_code = tw_master_transmit(PN532_I2C_ADDR, &dummy, 1, true);
//     if (error_code != SUCCESS) {
//         lcd_write_hex(error_code);
//         while(1);
//     }
//     _delay_ms(100);

//     // Configure RF field
//     uint8_t rf_config[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x32, 0x01, 0x01, 0x00};
//     error_code = tw_master_transmit(PN532_I2C_ADDR, rf_config, sizeof(rf_config), false);
//     if (error_code != SUCCESS) {
//         lcd_write_hex(error_code);
//         while(1);
//     }
//     _delay_ms(100);

//     lcd_clear();
//     lcd_write("Ready to scan");

//     uint8_t card_data[8];
//     uint8_t card_data_len;
//     bool card_detected = false;
//     while (1) {
//         if (pn532_read_passive_target(card_data, &card_data_len)) {
//             if (!card_detected) {
//                 lcd_clear();
//                 lcd_write("Card Detected");
//                 lcd_set_cursor(0, 1);
//                 for (uint8_t i = 0; i < card_data_len; i++) {
//                     lcd_write_hex(card_data[i]);
//                     lcd_write(" ");
//                 }
//                 card_detected = true;
//             }
//         } else {
//             if (card_detected) {
//                 lcd_clear();
//                 lcd_write("Ready to scan");
//                 card_detected = false;
//             }
//         }
//         _delay_ms(1000); // Delay to avoid rapid polling
//     }

//     return 0;
// }

/* SD CARD TEST */

// #include <avr/io.h>
// #include <util/delay.h>
// #include <stdio.h>
// #include "spi.h"
// #include "sd_card.h"

// #define F_CPU 16000000UL

// // Define buffer size
// #define BUFFER_SIZE 128

// int main(void) {
//     char write_data[] = "Hello, SD Card!";
//     char read_data[BUFFER_SIZE] = {0};

//     // Initialize SPI
//     SPI_init_master();
    
//     // Initialize SD card
//     if (sd_card_init() != 0) {
//         // SD card initialization failed
//         while (1) {
//             // Blink LED or hang for debugging
//         }
//     }

//     // Create or open a file
//     if (sd_card_create_file("test.txt") != 0) {
//         // Failed to create/open the file
//         while (1) {
//             // Blink LED or hang for debugging
//         }
//     }

//     // Write data to the file
//     if (sd_card_write_file("test.txt", write_data) != 0) {
//         // Failed to write to the file
//         while (1) {
//             // Blink LED or hang for debugging
//         }
//     }

//     // Read data from the file
//     if (sd_card_read_file("test.txt", read_data, BUFFER_SIZE) != 0) {
//         // Failed to read the file
//         while (1) {
//             // Blink LED or hang for debugging
//         }
//     }

//     // At this point, `read_data` contains the content of `test.txt`.

//     // Main loop
//     while (1) {
//         // Implement further logic here, e.g., display data on an LCD, toggle an LED, etc.
//     }

//     return 0;
// }
