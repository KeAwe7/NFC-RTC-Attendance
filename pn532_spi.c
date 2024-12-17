// pn532_spi.c
#include "pn532_spi.h"
#include "spi.h"
#include "uart.h"
#include "lcd.h"
#include <stdio.h>

// bool pn532_spi_init(void) {
//     // Set SPI clock rate slower (similar to Arduino setClockDivider)
//     SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0); // f_osc/128
    
//     // Wake up PN532 like Arduino version
//     PN532_CS_HIGH();
//     _delay_ms(100);
//     PN532_CS_LOW();
//     _delay_ms(200);
//     PN532_CS_HIGH();
//     _delay_ms(100);
    
//     return pn532_spi_detect();
// }

bool pn532_spi_init(void) {
    uart_println("Initializing PN532 in SPI mode...");
    
    // Initial chip select state
    PN532_CS_HIGH();
    _delay_ms(100);
    
    // Hardware wakeup sequence
    PN532_CS_LOW();
    _delay_ms(2);
    PN532_CS_HIGH();
    _delay_ms(20);
    
    // Test SPI communication
    PN532_CS_LOW();
    _delay_ms(2);
    
    uint8_t test = SPI_transfer(STATUS_READ);
    char debug[32];
    sprintf(debug, "Initial status: 0x%02X", test);
    uart_println(debug);
    
    if (test == 0x00) {
        uart_println("No response - check SPI switch and connections:");
        uart_println("- Switch set to 'SPI'");
        uart_println("- MOSI -> Pin 11");
        uart_println("- MISO -> Pin 12");
        uart_println("- SCK  -> Pin 13");
        uart_println("- SS   -> Pin 10");
        return false;
    }
    
    // Send wakeup command
    SPI_transfer(DATA_WRITE);
    SPI_transfer(0x55); // Wakeup
    SPI_transfer(0x55);
    SPI_transfer(0x00);
    
    PN532_CS_HIGH();
    _delay_ms(50);
    
    return true;
}

bool pn532_spi_detect(void) {
    uart_println("Testing PN532 SPI connection...");
    
    PN532_CS_LOW();
    _delay_ms(2);
    
    // Match Arduino frame format exactly
    SPI_transfer(DATA_WRITE);
    SPI_transfer(PN532_PREAMBLE);
    SPI_transfer(PN532_STARTCODE1);
    SPI_transfer(PN532_STARTCODE2);
    SPI_transfer(0x02);              // Length
    SPI_transfer(0xFE);              // LCS
    SPI_transfer(PN532_HOSTTOPN532);
    SPI_transfer(PN532_CMD_GETFIRMWAREVERSION);
    SPI_transfer(0xD4);              // DCS
    SPI_transfer(PN532_POSTAMBLE);
    
    PN532_CS_HIGH();
    _delay_ms(50);
    
    // Add readAckFrame check from Arduino code
    if (pn532_read_ack() != 0) {
        uart_println("No ACK frame received");
        return false;
    }

    if (pn532_read_ack()) {
    uint8_t version[3];
    if (pn532_read_firmware_version(version)) {
        lcd_clear();
        lcd_set_cursor(0, 1);
        lcd_write("FW: ");
        for (uint8_t i = 0; i < 3; i++) {
            char hex[3];
            sprintf(hex, "%02X", version[i]);
            lcd_write(hex);
            if (i < 2) lcd_write(".");
        }
        return true;
    }
}
    
    // Rest of detection code remains similar
    return true;
}

bool pn532_read_ack(void) {
    uart_println("Reading ACK in SPI mode...");
    char debug[32];

    // Retry mechanism for status check
    uint8_t retries = 0;
    uint8_t status;
    
    do {
        PN532_CS_LOW();
        _delay_ms(2);
        status = SPI_transfer(STATUS_READ);
        PN532_CS_HIGH();
        
        sprintf(debug, "Status retry %d: 0x%02X", retries, status);
        uart_println(debug);
        
        if (status == 0xFF) {  // Ready in SPI mode
            break;
        }
        
        retries++;
        _delay_ms(10);
    } while (retries < 10);

    if (retries >= 10) {
        uart_println("Device not responding in SPI mode");
        return false;
    }

    // Read ACK frame
    SPI_transfer(DATA_READ);
    uint8_t ackbuff[6];
    for (uint8_t i = 0; i < 6; i++) {
        ackbuff[i] = SPI_transfer(0x00);
        sprintf(debug, "ACK[%d]: 0x%02X", i, ackbuff[i]);
        uart_println(debug);
    }

    PN532_CS_HIGH();

    // Check ACK frame format
    if ((ackbuff[0] == 0x00) && 
        (ackbuff[1] == 0x00) && 
        (ackbuff[2] == 0xFF) && 
        (ackbuff[3] == 0x00) && 
        (ackbuff[4] == 0xFF) && 
        (ackbuff[5] == 0x00)) {
        uart_println("Valid ACK received");
        return true;
    }

    uart_println("Invalid ACK format");
    return false;
}

bool pn532_read_firmware_version(uint8_t *version) {
    uart_println("Reading firmware version...");
    
    PN532_CS_LOW();
    _delay_ms(2);
    
    // Read response
    SPI_transfer(STATUS_READ);
    SPI_transfer(DATA_READ);
    
    // Read response frame
    uint8_t response[12];
    for (uint8_t i = 0; i < 12; i++) {
        response[i] = SPI_transfer(0x00);
    }
    
    PN532_CS_HIGH();
    
    // Check response format
    if ((response[0] == 0x00) && 
        (response[1] == 0x00) && 
        (response[2] == 0xFF) && 
        (response[5] == 0xD5) && 
        (response[6] == 0x03)) {
        
        // Store version data
        version[0] = response[7];  // IC version
        version[1] = response[8];  // Firmware ver
        version[2] = response[9];  // Firmware rev
        
        char buf[32];
        sprintf(buf, "FW: %02X.%02X.%02X", version[0], version[1], version[2]);
        uart_println(buf);
        return true;
    }
    
    uart_println("Invalid firmware response");
    return false;
}

void PN532_SPI_Init(PN532_SPI *pn532, uint8_t ss_pin) {
    pn532->ss_pin = ss_pin;
    pn532->command = 0;
}

void PN532_SPI_Begin(PN532_SPI *pn532) {
    // Configure SS pin as output
    SPI_DDR |= (1 << pn532->ss_pin);
    
    // Initialize SPI in master mode
    SPI_init_master();
}

void PN532_SPI_Wakeup(PN532_SPI *pn532) {
    SPI_set_SS_low();
    _delay_ms(2);
    SPI_set_SS_high();
}

int8_t PN532_SPI_WriteCommand(PN532_SPI *pn532, 
                               const uint8_t *header, uint8_t hlen, 
                               const uint8_t *body, uint8_t blen) {
    // Store command for later response checking
    pn532->command = header[0];
    
    // Write frame
    PN532_SPI_WriteFrame(pn532, header, hlen, body, blen);
    
    // Wait for ready with timeout
    uint8_t timeout = PN532_ACK_WAIT_TIME;
    while (!PN532_SPI_IsReady(pn532)) {
        _delay_ms(1);
        timeout--;
        if (0 == timeout) {
            return -2;  // Timeout
        }
    }
    
    // Check ACK frame
    if (PN532_SPI_ReadAckFrame(pn532)) {
        return PN532_INVALID_ACK;
    }
    
    return 0;
}

int16_t PN532_SPI_ReadResponse(PN532_SPI *pn532, 
                                uint8_t *buf, uint8_t len, 
                                uint16_t timeout) {
    uint16_t time = 0;
    
    // Wait for device to be ready
    while (!PN532_SPI_IsReady(pn532)) {
        _delay_ms(1);
        time++;
        if (timeout > 0 && time > timeout) {
            return PN532_TIMEOUT;
        }
    }
    
    SPI_set_SS_low();
    _delay_ms(1);
    
    int16_t result;
    do {
        SPI_transfer(DATA_READ);
        
        // Validate frame start
        if (PN532_PREAMBLE != SPI_transfer(0) ||
            PN532_STARTCODE1 != SPI_transfer(0) ||
            PN532_STARTCODE2 != SPI_transfer(0)) {
            result = PN532_INVALID_FRAME;
            break;
        }
        
        // Read length and validate
        uint8_t length = SPI_transfer(0);
        if (0 != (uint8_t)(length + SPI_transfer(0))) {
            result = PN532_INVALID_FRAME;
            break;
        }
        
        // Validate response
        uint8_t cmd = pn532->command + 1;
        if (PN532_PN532TOHOST != SPI_transfer(0) || 
            cmd != SPI_transfer(0)) {
            result = PN532_INVALID_FRAME;
            break;
        }
        
        // Adjust length
        length -= 2;
        if (length > len) {
            // Overflow protection
            for (uint8_t i = 0; i < length; i++) {
                SPI_transfer(0);  // Dump excess data
            }
            SPI_transfer(0);  // Checksum
            SPI_transfer(0);  // Postamble
            result = PN532_NO_SPACE;
            break;
        }
        
        // Read response
        uint8_t sum = PN532_PN532TOHOST + cmd;
        for (uint8_t i = 0; i < length; i++) {
            buf[i] = SPI_transfer(0);
            sum += buf[i];
        }
        
        // Validate checksum
        uint8_t checksum = SPI_transfer(0);
        if (0 != (uint8_t)(sum + checksum)) {
            result = PN532_INVALID_FRAME;
            break;
        }
        
        SPI_transfer(0);  // Postamble
        result = length;
    } while (0);
    
    SPI_set_SS_high();
    return result;
}

// bool pn532_poll_for_card(uint8_t *uid, uint8_t *uid_length) {
//     uart_println("Polling for card...");
    
//     PN532_CS_LOW();
//     _delay_ms(2);

//     // Debug status byte first
//     uint8_t status = SPI_transfer(STATUS_READ);
//     char status_str[32];
//     sprintf(status_str, "Status byte: 0x%02X", status);
//     uart_println(status_str);
    
//     // Only proceed if ready
//     if ((status & 0x01) == 0) {
//         PN532_CS_HIGH();
//         uart_println("PN532 not ready");
//         return false;
//     }
    
//     // Send InListPassiveTarget command
//     SPI_transfer(DATA_WRITE);
//     SPI_transfer(PN532_PREAMBLE);
//     SPI_transfer(PN532_STARTCODE1);
//     SPI_transfer(PN532_STARTCODE2);
//     SPI_transfer(0x04);              // Length
//     SPI_transfer(0xFC);              // LCS
//     SPI_transfer(PN532_HOSTTOPN532);
//     SPI_transfer(PN532_CMD_INLISTPASSIVETARGET);
//     SPI_transfer(0x01);              // Max 1 card
//     SPI_transfer(0x00);              // Baud rate (106 kbps type A)
//     SPI_transfer(0xE1);              // DCS
//     SPI_transfer(PN532_POSTAMBLE);
    
//     PN532_CS_HIGH();
//     _delay_ms(150);                  // Wait for card detection

//     // Check if response ready
//     PN532_CS_LOW();
//     _delay_ms(2);
//     status = SPI_transfer(STATUS_READ);
//     sprintf(status_str, "Response status: 0x%02X", status);
//     uart_println(status_str);
    
//     if ((status & 0x01) == 0) {
//         PN532_CS_HIGH();
//         uart_println("No response ready");
//         return false;
//     }
    
//     if (!pn532_read_ack()) {
//         return false;
//     }
    
//     // Read response
//     uint8_t response[20];
//     PN532_CS_LOW();
    
//     if (SPI_transfer(STATUS_READ) != 0x01) {
//         PN532_CS_HIGH();
//         return false;
//     }
    
//     SPI_transfer(DATA_READ);
//     for (uint8_t i = 0; i < 20; i++) {
//         response[i] = SPI_transfer(0x00);
//     }
    
//     PN532_CS_HIGH();
    
//     // Check if card was found
//     if (response[7] != 0x01) {
//         return false;    // No card found
//     }
    
//     // Extract UID
//     *uid_length = response[12];
//     for (uint8_t i = 0; i < *uid_length; i++) {
//         uid[i] = response[13 + i];
//     }
    
//     return true;
// }

bool pn532_poll_for_card(uint8_t *uid, uint8_t *uid_length) {
    uart_println("Polling for MIFARE card...");
    
    PN532_CS_HIGH();
    _delay_ms(2);    
    PN532_CS_LOW();
    _delay_ms(2);
    
    // Clear any pending data
    SPI_transfer(STATUS_READ);
    SPI_transfer(0x00);
    
    // Send InListPassiveTarget for MIFARE
    SPI_transfer(DATA_WRITE);
    SPI_transfer(PN532_PREAMBLE);
    SPI_transfer(PN532_STARTCODE1);
    SPI_transfer(PN532_STARTCODE2);
    SPI_transfer(0x04);              // Length
    SPI_transfer(0xFC);              // LCS
    SPI_transfer(PN532_HOSTTOPN532);
    SPI_transfer(PN532_CMD_INLISTPASSIVETARGET);
    SPI_transfer(0x01);              // Max 1 card
    SPI_transfer(MIFARE_ISO14443A);  // MIFARE Type A
    SPI_transfer(0xE1);              // DCS
    SPI_transfer(PN532_POSTAMBLE);
    
    PN532_CS_HIGH();
    _delay_ms(500);  // Increased delay for MIFARE
    
    PN532_CS_LOW();
    _delay_ms(2);
    
    // Check ready status
    uint8_t status = SPI_transfer(STATUS_READ);
    char debug[32];
    sprintf(debug, "Status byte: 0x%02X", status);
    uart_println(debug);
    
    // Read response
    SPI_transfer(DATA_READ);  // Start read
    
    uint8_t response[20];
    for (uint8_t i = 0; i < 20; i++) {
        response[i] = SPI_transfer(0x00);
    }
    
    PN532_CS_HIGH();
    
    // Debug response bytes
    uart_println("Response bytes:");
    for (uint8_t i = 0; i < 20; i++) {
        sprintf(debug, "byte[%d]: 0x%02X", i, response[i]);
        uart_println(debug);
    }
    
    // Check response format
    if (response[0] != 0x00 || response[1] != 0x00 || response[2] != 0xFF) {
        uart_println("Invalid response header");
        return false;
    }
    
    if (response[7] == 0x01) {  // Card detected
        *uid_length = response[12];
        memcpy(uid, &response[13], *uid_length);
        
        uart_println("Card found!");
        sprintf(debug, "UID Length: %d", *uid_length);
        uart_println(debug);
        return true;
    }
    
    uart_println("No card detected");
    return false;
}

uint8_t PN532_SPI_IsReady(PN532_SPI *pn532) {
    SPI_set_SS_low();
    SPI_transfer(STATUS_READ);
    uint8_t status = SPI_transfer(0) & 1;
    SPI_set_SS_high();
    return status;
}

void PN532_SPI_WriteFrame(PN532_SPI *pn532, 
                           const uint8_t *header, uint8_t hlen, 
                           const uint8_t *body, uint8_t blen) {
    SPI_set_SS_low();
    _delay_ms(2);  // Wake up PN532
    
    SPI_transfer(DATA_WRITE);
    SPI_transfer(PN532_PREAMBLE);
    SPI_transfer(PN532_STARTCODE1);
    SPI_transfer(PN532_STARTCODE2);
    
    // Calculate and send length
    uint8_t length = hlen + blen + 1;
    SPI_transfer(length);
    SPI_transfer(~length + 1);
    
    // Write frame data
    SPI_transfer(PN532_HOSTTOPN532);
    uint8_t sum = PN532_HOSTTOPN532;
    
    // Send header
    for (uint8_t i = 0; i < hlen; i++) {
        SPI_transfer(header[i]);
        sum += header[i];
    }
    
    // Send body
    for (uint8_t i = 0; i < blen; i++) {
        SPI_transfer(body[i]);
        sum += body[i];
    }
    
    // Send checksum and postamble
    SPI_transfer(~sum + 1);
    SPI_transfer(PN532_POSTAMBLE);
    
    SPI_set_SS_high();
}

int8_t PN532_SPI_ReadAckFrame(PN532_SPI *pn532) {
    const uint8_t PN532_ACK[] = {0, 0, 0xFF, 0, 0xFF, 0};
    uint8_t ackBuf[sizeof(PN532_ACK)];
    
    SPI_set_SS_low();
    _delay_ms(1);
    SPI_transfer(DATA_READ);
    
    // Read ACK frame
    for (uint8_t i = 0; i < sizeof(PN532_ACK); i++) {
        ackBuf[i] = SPI_transfer(0);
    }
    
    SPI_set_SS_high();
    
    // Compare with expected ACK
    return memcmp(ackBuf, PN532_ACK, sizeof(PN532_ACK));
}