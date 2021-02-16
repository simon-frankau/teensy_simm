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

// Data lines are B0-3.
#define DATA    PORTB
#define DATA_EN DDRB
static const char D_SHIFT = 0;
// Address lines are F4-7;
#define ADDR    PORTF
#define ADDR_EN DDRF
static const char A_SHIFT = 4;

void simm_init(void)
{
    // RAS, CAS and WE are all active low, so set them high...
    CONTROL |= RAS | CAS | WE;
    // And drive them.
    CONTROL_EN |= RAS | CAS | WE;

    // Drive address lines.
    ADDR_EN |= 0x0f << A_SHIFT;
    // Do not drive data lines.
    DATA_EN &= ~(0x0f << D_SHIFT);
}

void simm_write(char addr, char val)
{
    char row = (addr >> 4) & 0x0f;
    char col = addr & 0x0f;

    // Write row.
    ADDR = row << A_SHIFT;
    CONTROL &= ~RAS;

    // Set data.
    DATA = val << D_SHIFT;
    DATA_EN |= 0x0f << D_SHIFT;
    CONTROL &= ~WE;

    // Write col.
    ADDR = col << A_SHIFT;
    CONTROL &= ~CAS;

    // Release RAS and CAS first, then data.
    CONTROL |= RAS | CAS;
    DATA_EN &= ~(0x0f << D_SHIFT);
    CONTROL |= WE;
}

char simm_read(char addr)
{
    char row = (addr >> 4) & 0x0f;
    char col = addr & 0x0f;

    // Write row.
    ADDR = row << A_SHIFT;
    CONTROL &= ~RAS;

    // Write col.
    ADDR = col << A_SHIFT;
    CONTROL &= ~CAS;

    // Read the data.
    char val = (DATA >> D_SHIFT) & 0x0f;

    // Release RAS and CAS first.
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
    simm_init();

    // Initialise USB for debug, but don't wait.
    usb_init();

    while (1) {
        _delay_ms(250);
        led_on();
        _delay_ms(250);
        led_off();
        print("Boop\n");
        simm_write(0, 3);
        simm_write(1, 10);
        char r1 = simm_read(0);
        char r2 = simm_read(1);
        phex(r1);
        phex(r2);
        print("\n");
    }
}
