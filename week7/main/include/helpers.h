#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>

//CONSTANT DEFINITIONS
#define TRIG_PIN 32
#define ECHO_PIN 33

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

#define LCD_ADDR             0x27
#define FLAG_BACKLIGHT_ON    0x08
#define FLAG_ENABLE          0x04
#define FLAG_RS_DATA         0x01
#define FLAG_RS_COMMAND      0x00

//SETUP FUNCTIONS
void setup_lcd();
void setup_ultrasonic();

//LCD FUNCTIONS
void lcd_print(const char *str);
void lcd_print_binary(const char* prefix, uint8_t val);
void lcd_set_cursor(bool row);

#endif