// lcd.c
#include "lcd.h"
#include "twi_master.h"
#include <util/delay.h>

#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04
#define RW 0x02
#define RS 0x01

void lcd_send(uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;
    uint8_t data[4];

    data[0] = high_nibble | mode | LCD_BACKLIGHT | ENABLE;
    data[1] = high_nibble | mode | LCD_BACKLIGHT;
    data[2] = low_nibble | mode | LCD_BACKLIGHT | ENABLE;
    data[3] = low_nibble | mode | LCD_BACKLIGHT;

    for (uint8_t i = 0; i < 4; i++) {
        tw_master_transmit(LCD_ADDRESS, &data[i], 1, false);
        _delay_us(50);
    }
}

void lcd_command(uint8_t command) {
    lcd_send(command, 0);
}

void lcd_write_char(char data) {
    lcd_send(data, RS);
}

void lcd_init() {
    _delay_ms(50); // Wait for LCD to power up

    lcd_command(0x03);
    _delay_ms(5);
    lcd_command(0x03);
    _delay_us(150);
    lcd_command(0x03);
    lcd_command(0x02); // Set to 4-bit mode

    lcd_command(0x28); // Function set: 4-bit mode, 2 lines, 5x8 dots
    lcd_command(0x0C); // Display ON, cursor OFF
    lcd_command(0x06); // Entry mode: increment cursor
    lcd_command(0x01); // Clear display
    _delay_ms(2);
}

void lcd_write(char *message) {
    while (*message) {
        lcd_write_char(*message++);
    }
}

void lcd_clear(void) {
    lcd_command(0x01);  // Clear display command
    _delay_ms(2);       // Clear display needs longer delay
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 1) {
        row = 1; // We only have 2 rows
    }
    lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_write_hex(uint8_t num) {
    const char hex_chars[] = "0123456789ABCDEF";
    char hex_str[3];
    hex_str[0] = hex_chars[(num >> 4) & 0x0F];
    hex_str[1] = hex_chars[num & 0x0F];
    hex_str[2] = '\0';
    lcd_write(hex_str);
}