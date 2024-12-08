
#include "twi_master.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stddef.h> // Include this header for NULL definition
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define LED_PIN PB5 // Built-in LED on Arduino is connected to PB5 (pin 13)

void init_led(void) {
    // Set LED_PIN as output
    DDRB |= (1 << LED_PIN);
}

void toggle_led(void) {
    // Toggle LED_PIN
    PORTB ^= (1 << LED_PIN);
}

void init_uart(uint16_t baudrate) {
    uint16_t ubrr = F_CPU/16/baudrate-1;
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void uart_transmit(unsigned char data) {
    while (!( UCSR0A & (1<<UDRE0)));
    UDR0 = data;
}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void uart_print_hex(uint8_t num) {
    const char hex_chars[] = "0123456789ABCDEF";
    uart_transmit(hex_chars[(num >> 4) & 0x0F]);
    uart_transmit(hex_chars[num & 0x0F]);
}

int test_main(void) {
    init_led(); // Initialize LED
    init_uart(9600); // Initialize UART with 9600 baud rate
    tw_init(TW_FREQ_100K, true); // Initialize I2C with 100kHz frequency

    for (uint8_t address = 1; address < 128; address++) {
        if (tw_master_transmit(address, NULL, 0, false) == SUCCESS) {
            while (1) {
                uart_print("Detected I2C address: 0x");
                uart_print_hex(address);
                uart_print("\r\n");
                toggle_led(); // Toggle LED to indicate detected address
                _delay_ms(500); // Blink LED every 500ms
            }
        }
        _delay_ms(10);
    }

    while (1);
    return 0;
}

#ifdef TEST
int main(void) {
    return test_main();
}
#endif