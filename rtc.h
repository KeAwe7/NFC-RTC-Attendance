#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>
#include <stdbool.h>

// RTC time structure
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} DS1307_Time;

// Initialize DS1307
void ds1307_init(void);

// Set time
bool ds1307_set_time(DS1307_Time* time);

// Get time
bool ds1307_get_time(DS1307_Time* time);

// Convert BCD to decimal
uint8_t bcd_to_decimal(uint8_t bcd);

// Convert decimal to BCD
uint8_t decimal_to_bcd(uint8_t decimal);

#endif // DS1307_H