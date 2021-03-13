/*
 * Teensy SIMM test demo program
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

// Don't forget to sync this with F_CPU in the Makefile.
//
// A macro as apparently you can't initialise one const with another?!
#define CLOCK_SPEED CPU_16MHz

static inline void cpu_prescale(char i)
{
    CLKPR = 0x80;
    CLKPR = i;
}

////////////////////////////////////////////////////////////////////////
// SIMM control.

// Control lines all on Port D.
#define CONTROL    PORTD
#define CONTROL_EN DDRD
static const char RAS = 1;
static const char CAS = 2;
static const char WE  = 4;

// Data lines are B0-7.
#define DATA_OUT PORTB
#define DATA_IN  PINB
#define DATA_EN  DDRB
// Address lines are F0-1 and 4-7;
#define ADDR    PORTF
#define ADDR_EN DDRF

void simm_init(void)
{
    // RAS, CAS and WE are all active low, so set them high...
    CONTROL |= RAS | CAS | WE;
    // And drive them.
    CONTROL_EN |= RAS | CAS | WE;

    // Drive address lines.
    ADDR_EN |= 0xf3; // Bits 0-1, 4-7.
    // Do not drive data lines.
    DATA_EN &= 0x00;

    // TODO: F0-F1 are address lines we're not using yet.
    DDRF |= 3;
    PORTF |= 3;
}

inline char addr_to_f(char c) {
    // Assemble bits 4-7 and bits 0-1.
    return ((c & 0x3c) << 2) | (c & 0x03);
}

void simm_write(char row, char col, char val)
{
    // Write row.
    ADDR = addr_to_f(row);
    CONTROL &= ~RAS;

    // Set data.
    DATA_OUT = val;
    DATA_EN |= 0xff;
    CONTROL &= ~WE;

    // Write col.
    ADDR = addr_to_f(col);
    CONTROL &= ~CAS;

    // Release RAS and CAS first, then data.
    // I'm not sure this strictly matters given the timing diagrams.
    CONTROL |= RAS | CAS;
    DATA_EN &= 0x00;
    DATA_OUT = 0;
    CONTROL |= WE;
}

char simm_read(char row, char col)
{
    // Write row.
    ADDR = addr_to_f(row);
    CONTROL &= ~RAS;

    // Write col.
    ADDR = addr_to_f(col);
    CONTROL &= ~CAS;

    // The input synchroniser has two flip-flops in series, delaying
    // the value being read, so we need to insert a NOP before the result of
    // the DRAM read will be available to an IN operation.
    if (CLOCK_SPEED == CPU_16MHz) {
        // One cycle seems to be insufficient at 16MHz, perhaps due to the
        // time it takes to perform the read.
        __builtin_avr_delay_cycles(2);
    } else {
        __builtin_avr_delay_cycles(1);
    }

    // Read the data.
    char val = DATA_IN;

    // Release RAS and CAS.
    CONTROL |= RAS | CAS;

    return val;
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
// And the main program itself...
//

void write_mem(char v)
{
    // Write 4K bytes in different rows and columns...
    for (int r = 0; r < 0x40; r++) {
        for (int c = 0; c < 0x40; c++) {
            simm_write(r, c, v);
        }
    }
}

void read_mem(void)
{
    // Read 4K bytes in different rows and columns...
    for (int r = 0; r < 0x40; r++) {
        for (int c = 0; c < 0x40; c++) {
            phex(simm_read(r, c));
        }
    }
}

// _delay_ms wants a constant, so... hack!
void delay(char power)
{
#define D(i) case i: _delay_ms(1L << i); break
    switch (power) {
        D(0);
        D(1);
        D(2);
        D(3);
        D(4);
        D(5);
        D(6);
        D(7);
        D(8);
        D(9);
        D(10);
        D(11);
        D(12);
        D(13);
        D(14);
        D(15);
    }
#undef D
}

int main(void)
{
    // Even at fastest speeds, a 70ns SIMM, like I have, can happily
    // keep up...
    //
    // CPU prescale must be set with interrupts disabled. They're off
    // when the CPU starts.
    cpu_prescale(CLOCK_SPEED);
    led_init();
    simm_init();

    // Initialise USB for debug, but don't wait.
    usb_init();

    // And give us some time to enable logging.
    _delay_ms(1000);

    // See how the memory decays without refresh.
    while (1) {
#if 1
        // Write, wait 2^i ms, read, and report the read data. Do this
        // having written 0s and 1s...
        for (int i = 0; i < 16; i++) {
            phex(i);
            print(",");

            led_on();
            write_mem(0xff);
            delay(i);
            read_mem();
            print(",");

            led_off();
            write_mem(0x00);
            delay(i);
            read_mem();

            print("\n");
        }
#else
        for (int r = 0; r < 0x40; r++) {
            for (int c = 0; c < 0x40; c++) {
                simm_write(r, c, r + (c << 1));
            }
        }
        for (int r = 0; r < 0x10; r++) {
            for (int c = 0; c < 0x10; c++) {
                unsigned char v = simm_read(r, c);
                if (v != (r + (c << 1))) {
                    print("??? ");
                    phex(v);
                    print(" - ");
                    phex(r + (c << 1));
                    print("\n");
                }
            }
        }
        print("DONE\n");
        _delay_ms(100);
#endif
    }
}
