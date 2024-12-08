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
//     _delay_ms(100);

//     lcd_clear();
//     lcd_write("Ready to scan");

//     while(1) {
//         // Poll for NFC cards
//         uint8_t card_data[7];
//         if (pn532_read_passive_target(card_data, sizeof(card_data))) {
//             lcd_set_cursor(0, 1);
//             lcd_write("Card found!");
//             _delay_ms(1000);
//             lcd_set_cursor(0, 1);
//             lcd_write("          ");
//         }
//         _delay_ms(100);
//     }

//     return 0;
// }

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

#include "lcd.h"
#include "pn532.h"
#include "twi_master.h"
#include <util/delay.h>

int main(void) {
    ret_code_t error_code;
    
    // Initialize I2C at 250kHz - working speed
    tw_init(TW_FREQ_250K, true);
    _delay_ms(500);

    lcd_init();
    _delay_ms(500);
    lcd_write("Init...");

    pn532_init();
    _delay_ms(500);

    // Wake sequence - working
    lcd_set_cursor(0, 1);
    lcd_write("Wake..");
    uint8_t dummy = 0x00;
    error_code = tw_master_transmit(PN532_I2C_ADDR, &dummy, 1, true);
    if (error_code != SUCCESS) {
        lcd_write_hex(error_code);
        while(1);
    }
    _delay_ms(100);

    // Configure RF field
    uint8_t rf_config[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x32, 0x01, 0x01, 0x00};
    error_code = tw_master_transmit(PN532_I2C_ADDR, rf_config, sizeof(rf_config), false);
    if (error_code != SUCCESS) {
        lcd_write_hex(error_code);
        while(1);
    }
    _delay_ms(100);

    lcd_clear();
    lcd_write("Ready to scan");

    uint8_t card_data[8];
    uint8_t card_data_len;
    bool card_detected = false;
    while (1) {
        if (pn532_read_passive_target(card_data, &card_data_len)) {
            if (!card_detected) {
                lcd_clear();
                lcd_write("Card Detected");
                lcd_set_cursor(0, 1);
                for (uint8_t i = 0; i < card_data_len; i++) {
                    lcd_write_hex(card_data[i]);
                    lcd_write(" ");
                }
                card_detected = true;
            }
        } else {
            if (card_detected) {
                lcd_clear();
                lcd_write("Ready to scan");
                card_detected = false;
            }
        }
        _delay_ms(1000); // Delay to avoid rapid polling
    }

    return 0;
}