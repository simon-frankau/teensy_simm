# Managing a SIMM from a Teensy 2.0

## TODO

Yeah, TODO all the docs.

## Hardware configuration

| Pin # | Name  | Description           | Teensy pin |
| ----- | ----  | --------------------- | ---------- |
| 1     | VCC   | +5 VDC                | VCC        |
| 2     | /CAS  | Column Address Strobe | D1         |
| 3     | DQ0   | Data 0                | Pull-up    |
| 4     | A0    | Address 0             | GND        |
| 5     | A1    | Address 1             | GND        |
| 6     | DQ1   | Data 1                | Pull-up    |
| 7     | A2    | Address 2             | GND        |
| 8     | A3    | Address 3             | GND        |
| 9     | GND   | Ground                | GND        |
| 10    | DQ2   | Data 2                | Pull-up    |
| 11    | A4    | Address 4             | GND        |
| 12    | A5    | Address 5             | GND        |
| 13    | DQ3   | Data 3                | Pull-up    |
| 14    | A6    | Address 6             | GND        |
| 15    | A7    | Address 7             | F7         |
| 16    | DQ4   | Data 4                | B3         |
| 17    | A8    | Address 8             | F6         |
| 18    | A9    | Address 9             | F5         |
| 19    | A10   | Address 10            | F4         |
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

Only wiring up DQ4-DQ7, and A7-A10. Testing that the thing works in
principle, don't need to actually access the full memory. Why the
higher pins? They're the ones nearer the Teensy, so I can use more of
the shorter wires. :)

I avoided wiring up A11, since that's only used by the largest of
SIMMs.

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
| F4         | A10   |
| F5         | A9    |
| F6         | A8    |
| F7         | A7    |