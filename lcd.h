// lcd.h
#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

#define LCD_ADDRESS 0x27

void lcd_init(void);
void lcd_write(char *message);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_command(uint8_t command);
void lcd_write_hex(uint8_t num);
void lcd_clear(void);

#endif /* LCD_H_ */