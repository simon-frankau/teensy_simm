/*
 * Teensy OLED demo program.
 *
 * Works with Teensy 2.0 connected to an SSD1780-driven 128x32 display
 * - specifically a "Geekcreit 0.91 Inch 128x32 IIC I2C Blue OLED LCD
 * Display DIY Module"
 *
 * (C) 2021 Simon Frankau
 */

#include <stdlib.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "usb_debug_only.h"
#include "print.h"

////////////////////////////////////////////////////////////////////////
// CPU prescaler
//

static const char CPU_16MHz  = 0x00;
static const char CPU_8MHz   = 0x01;
static const char CPU_4MHz   = 0x02;
static const char CPU_2MHz   = 0x03;
static const char CPU_1MHz   = 0x04;
static const char CPU_500kHz = 0x05;
static const char CPU_250kHz = 0x06;
static const char CPU_125kHz = 0x07;
static const char CPU_62kHz  = 0x08;

static inline void cpu_prescale(char i)
{
    CLKPR = 0x80;
    CLKPR = i;
}

////////////////////////////////////////////////////////////////////////
// LED
//

static inline void led_init(void)
{
    DDRD |= 1 << 6;
}

static inline void led_on(void)
{
    PORTD |= 1 << 6;
}

static inline void led_off(void)
{
    PORTD &= ~(1 << 6);
}

////////////////////////////////////////////////////////////////////////
// Low-level I2C config
//

// I2C on D0/D1: SCL on D0, SDA on D1
static const char SCL = 0;
static const char SDA = 1;

static void i2c_init(void)
{
    // In I2C the lines float high and are actively pulled low, so we
    // set them to output zero, and enable/disable driving it low.

    // SCL
    DDRD &= ~(1 << SCL);
    PORTD &= ~(1 << SCL);
    // SDA
    DDRD &= ~(1 << SDA);
    PORTD &= ~(1 << SDA);
}

static inline void i2c_release(char pin)
{
    DDRD &= ~(1 << pin);
}

static inline void i2c_pulldown(char pin)
{
    DDRD |= 1 << pin;
}

static inline char i2c_read(char pin)
{
    return PIND & (1 << pin);
}


////////////////////////////////////////////////////////////////////////
// And the main program itself...
//

char const message_1[] = "My little ssd1306+teensy 2.0 demo. ";
char const message_2[] = "Look... bendy text! :) ";
char const message_3[] = "Wobble!";

int main(void)
{
    // Set a low speed so that we're well within the timing
    // requirements of the DRAM
    //
    // CPU prescale must be set with interrupts disabled. They're off
    // when the CPU starts.
    //
    // Don't forget to sync this with F_CPU in the Makefile.
    cpu_prescale(CPU_250kHz);
    led_init();

    // Initialise USB for debug, but don't wait.
    usb_init();

    while (1) {
        _delay_ms(250);
        led_on();
        _delay_ms(250);
        led_off();
        print("Boop\n");
    }
}
