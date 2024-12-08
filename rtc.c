// rtc.c
#include "rtc.h"
#include "twi_master.h"

#define DS1307_ADDR 0x68
#define DS1307_SECONDS_REG 0x00
#define DS1307_CONTROL_REG 0x07

void ds1307_init(void) {
    // Enable oscillator, disable square wave output
    uint8_t control_byte = 0x00;
    tw_master_transmit(DS1307_ADDR, &control_byte, 1, false);
}

bool ds1307_set_time(DS1307_Time* time) {
    uint8_t time_data[7];
    
    // Prepare time data in correct order
    time_data[0] = decimal_to_bcd(time->seconds);
    time_data[1] = decimal_to_bcd(time->minutes);
    time_data[2] = decimal_to_bcd(time->hours);
    time_data[3] = decimal_to_bcd(time->day);
    time_data[4] = decimal_to_bcd(time->date);
    time_data[5] = decimal_to_bcd(time->month);
    time_data[6] = decimal_to_bcd(time->year);

    // Write to RTC starting from seconds register
    uint8_t write_data[8] = {DS1307_SECONDS_REG};
    for (int i = 0; i < 7; i++) {
        write_data[i+1] = time_data[i];
    }

    return tw_master_transmit(DS1307_ADDR, write_data, 8, false) == SUCCESS;
}

bool ds1307_get_time(DS1307_Time* time) {
    // Set register pointer to seconds
    uint8_t reg = DS1307_SECONDS_REG;
    if (tw_master_transmit(DS1307_ADDR, &reg, 1, true) != SUCCESS) {
        return false;
    }

    // Read 7 bytes of time data
    uint8_t time_data[7];
    if (tw_master_receive(DS1307_ADDR, time_data, 7) != SUCCESS) {
        return false;
    }

    // Convert BCD to decimal
    time->seconds = bcd_to_decimal(time_data[0] & 0x7F);
    time->minutes = bcd_to_decimal(time_data[1]);
    time->hours = bcd_to_decimal(time_data[2] & 0x3F);
    time->day = bcd_to_decimal(time_data[3]);
    time->date = bcd_to_decimal(time_data[4]);
    time->month = bcd_to_decimal(time_data[5]);
    time->year = bcd_to_decimal(time_data[6]);

    return true;
}

uint8_t bcd_to_decimal(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10) << 4) | (decimal % 10);
}