# Managing a SIMM from a Teensy 2.0

## What?

I have never built a circuit with DRAM before, and it's on my bucket
list to onstruct a 68K-based Mac clone, which would use DRAM as its
main memory. So, time to experiment with DRAM, and more specifically
with 30-pin SIMMs, just like I used to have in my PC of old.

The aim here is really just to exercise the DRAM functionality, play
with refresh cycles, etc.

## Software tooling

Same blurb as in my `teensy_oled` repo that I've copied the basics
from:

As I'm developing on a Mac, I installed [Crosspack
AVR](https://www.obdev.at/products/crosspack/index.html). I'm using
`[teensy_loader_cli](https://github.com/PaulStoffregen/teensy_loader_cli)`
to program the Teensy, and
`[hid_listen](https://www.pjrc.com/teensy/hid_listen.html)` from
pjrc.com to supply debug output. The utilities source and Makefile
come from pjrc's ["blinky"
example](https://www.pjrc.com/teensy/blinky.zip).

## Hardware configuration

| Pin # | Name  | Description           | Teensy pin |
| ----- | ----  | --------------------- | ---------- |
| 1     | VCC   | +5 VDC                | VCC        |
| 2     | /CAS  | Column Address Strobe | D1         |
| 3     | DQ0   | Data 0                | B7         |
| 4     | A0    | Address 0             | GND        |
| 5     | A1    | Address 1             | GND        |
| 6     | DQ1   | Data 1                | B6         |
| 7     | A2    | Address 2             | GND        |
| 8     | A3    | Address 3             | GND        |
| 9     | GND   | Ground                | GND        |
| 10    | DQ2   | Data 2                | B5         |
| 11    | A4    | Address 4             | F0         |
| 12    | A5    | Address 5             | F1         |
| 13    | DQ3   | Data 3                | B4         |
| 14    | A6    | Address 6             | F7         |
| 15    | A7    | Address 7             | F6         |
| 16    | DQ4   | Data 4                | B3         |
| 17    | A8    | Address 8             | F5         |
| 18    | A9    | Address 9             | F4         |
| 19    | A10   | Address 10            | GND        |
| 20    | DQ5   | Data 5                | B2         |
| 21    | /WE   | Write Enable          | D2         |
| 22    | GND   | Ground                | GND        |
| 23    | DQ6   | Data 6                | B1         |
| 24    | A11   | Address 11            | GND        |
| 25    | DQ7   | Data 7                | B0         |
| 26    | QP    | Data Parity Out       | N/C        |
| 27    | /RAS  | Row Address Strobe    | D0         |
| 28    | /CASP | CAS Parity            | Pull-up    |
| 29    | DP    | Data Parity In        | Pull-up    |
| 30    | VCC   | +5 VDC                | VCC        |

Only wiring up DQ4-DQ7, and A6-A9. Testing that the thing works in
principle, don't need to actually access the full memory. Why the
higher pins? They're the ones nearer the Teensy, so I can use more of
the shorter wires. :)

I avoided wiring up A11/A10, since my test SIMM is a 1MB (parity!)
SIMM, populated with 2x 422400-70, 1x 421000-70 DRAM chips. SIMMs.

Inverting the table for the Teensy's connections:

| Teensy pin | Usage |
| ---------- | ----- |
| D0         | /RAS  |
| D1         | /CAS  |
| D2         | /WE   |
| B0         | DQ7   |
| B1         | DQ6   |
| B2         | DQ5   |
| B3         | DQ4   |
| B4         | DQ3   |
| B5         | DQ2   |
| B6         | DQ1   |
| B7         | DQ0   |
| F4         | A9    |
| F5         | A8    |
| F6         | A7    |
| F7         | A6    |
| F1         | A5    |
| F0         | A4    |

## Results

### Old results

The little test that I've written right now write 256 bytes of data,
waits 2^n ms for n up to 15, and then reads the data back, writing
both 0xF and 0x0 (we only manage a nibble of data, remember?). The aim
is to see the decay creeping in, given there's no refresh.

Somewhat surprisingly to me (at the limited scale of writing 256
bytes), I'm seeing no decay, even when waiting 16s between write and
read. DRAM discharge is clearly much less of a thing than I thought!

I think the next steps are to wire up more data and address pins, and
see if I can observe decay over larger sample set.

### Current state

I'm wiring up more pins, and discovering new bugs. Hurrah!

## Simplified changelog

This project has been through a number of phases:

 * Simply getting read/write cycles to work, with 4 address lines and
   4 data lines. Getting AVR I/O right, basically.

 * Getting beyond "it seems to read/write, just incorrectly", by
   adding a delay on the read value to account for inputs going
   through two latches to avoid metastability.

 * Doing an initial test of 4 bits of data stored at 256 locations on
   what the DRAM decay characteristics look like, and finding the test
   DRAM decays slowly.

 * Wiring up more pins (2 more address lines, 4 more data lines), and
   doing a test, and discovering that the read value doesn't match the
   written value at 16MHz, but does at 8MHz and below.
