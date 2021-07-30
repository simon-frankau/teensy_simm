# Managing a SIMM from a Teensy 2.0, to understand DRAM decay

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

I'm only wiring up A4-A9, since the Teensy has limited easy-to-access
pins, and I'm a little lazy for a proof-of-concept.

I avoided wiring up A11/A10, since my test SIMM is a 1MB (parity!)
SIMM, populated with 2x 422400-70, 1x 421000-70 DRAM chips.

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

I've written software to run a test cycle over 4KB of data (6 address
lines, multiplexed to give a 12-bit address), with 8 address lines.
The cycle writes data, waits *n* milliseconds, and reads it back. I've
observed that only 0s decay to 1s (presumably 0s are stored as a
charge in the DRAM cells), so I only write 0s and read back the
contents. I do this for *n = sqrt(2)^i* ms delays, which is like *2^i*
ms delays, only with extra intermediate steps.

As I've noticed decay rate depends on temperature (I was seeing no
noticeable decay at room temperature, but it really picked up when I
applied a hairdryer), I've run a bunch of test cycles at the
temperatures of 20, 25, 30, 35 and 40 degrees Celsius, using a
thermostat intended for reptiles. The raw output is in the "results"
directory, one file per temperature.

Then, using the tooling I've written in the "tools" directory, I've
generated CSVs of fraction of times a location decayed, based on
address and delay between reading and writing. These are stored in the
"processed_results" directory, and I've uploaded them to a [Google
sheet](https://docs.google.com/spreadsheets/d/17J4vXwe0mxszkWyo406M8UlAduA3VVvJ1ZQwqvPf7Q4/edit).

The results show more-or-less what you'd expect - certain locations
are more vulnerable to decay, and decay fairly consistently - if
they've decayed at time *n*, they'll also decay for times > *n*, and
they decay more quickly at higher temperatures.

Changing tack, I decided to look at the overall bit decay stats,
rather than individual memory locations. I built a model (described
below), and... it roughly fit, although the data was, quite frankly,
massively inadequate. See the section "Modelling decay rate".

I don't have the tools to extend the temperature range, but I could
collect data with longer time delays to improve the modeling, so
that's what I did. The results of this are in the "Modeling decay
rates #2" section.

You can read more there, but the summary is:

 * Despite demanding a 128ms refresh cycle, the median decay time at
   room temperature is nearly 3 minutes! This explains the result that
   surprises many, that you can power cycle a computer and (assuming
   the RAM isn't deliberately cleared) most of the memory contents
   will be intact.

 * There is a strong temperature effect, decreasing decay time. We
   only really see this kicking in hard at the edge of our data, at 40
   degrees, but it looks like the effect grows strongly with
   temperature.

### Future work

While the following are future possible extensions, without a
convenient way to extend the temperature range (no, I'm not allowed to
put circuitry in the oven), I'm going to call it done for now.

 * Consider collecting data over a wider temperature range (how?).
 * Demonstrate that refreshing the DRAM makes decay go away.

## Modeling decay rate

Rather than build an equation to fit the data out of absolutely
nothing, I wanted to construct a simple model and see how well it
fits.

We can start with the idea that a DRAM cell is a capacitor with some
RC constant, and that decay occurs if the voltage on it drops below
some threshold. In other words, the cell decays if *e^-RCt < X*, for
its *RC* constant and some threshold level *X*. *RC*, in turn, is going
to be some distribution across all the bits in the memory. A
temperature-dependent distribution, since the data clearly shows
faster decay at higher temperatures.

This distribution is probably going to be related to a normal
distribution, representing variability due to manufacturing. However,
the *RC* constant can never go below zero, so I'm gonna model the *RC*
value as a log normal distribution - *e^N(mu(T), sigma^2(T))*, where
the mean and variance are functions of temperature.

Without foundation, and almost certainly incorrectly, we assume the
statistics for the individual bits are i.i.d.

Putting it all together, the fraction of memory cells decaying is:

*E(I(e^-(e^N)t < X))*

where *E* is expectation, *I* is indicator function, and *N* our
normally distributed values. This can be rearranged:

*E(I((e^N)t > -ln X))*

*E(I(N > ln (-ln X) - ln t))*

*E(I(N < c + ln t)* (symmetry of *N*)

for some constant c.

By tweaking the mean and variance of the normal distribution, the
decay fraction expectation is the cumulative normal distribution up to
*ln(t)*.

If this model works, plotting the inverse CDF against log time should
give a nice, straight graph for each temperature. I did this for the
data I gathered in the "Decay Rates" tab of [this
sheet](https://docs.google.com/spreadsheets/d/17J4vXwe0mxszkWyo406M8UlAduA3VVvJ1ZQwqvPf7Q4/edit#gid=1629776164),
and got some pretty straight lines. Hurrah!

To be honest, though, based on this data, it's hardly conclusive.
(Boo.) Plotting inverse normal distribution against (non-log) time
looks like a slightly better fit, and simply plotting log fraction
decayed vs. time is a pretty good fit. I'm trying to fit a
distribution against the very tail of the distribution - most of the
points are at least 3 standard deviations from the mean. That's not
good.

Having confidence in the model, let alone trying to fit parameters,
seems extremely foolish given the data I'd collected. Moreover, we
know there's a strong temperature effect, but that data also looks
extremely dodgy to extrapolate. One slide deck from a researcher
suggested the decay rate was proportional to *T^4*, which makes the
curve very steep, but with my equipment going beyond 40 degrees is
tough.

## Modeling decay rates #2

In order to have a better go at validating the model
(unscientifically), I collected more data. Not wanting to set up a
thermostatic environment again, and wanting to take limited time, I
only did so at room temperature, but I extended the delays supported
up to 2896 seconds, finding the decay fraction for delays of powers of
sqrt(2) seconds.

For this run, the raw data is in `results/res_20_ext.txt`, and the
processed data is in `processed_results/res_20_ext.cs`. The data was
then copied into the "Extended decay rates" tab of the sheet.

Plotting raw decay fraction versus write-to-read delay gives a nice
sigmoid curve that suggests something normal-like for the underlying
distribution. I took the inverse normal distribution of the decay
fractions, and plotted it against linear time and log time, to get an
idea of whether a normal or a log-normal distribution works better. I
really wanted to see a nice linear region around the centre of the
distribution (0 standard deviations), where the data is least noisy (a
few bits more or less decaying won't change the standard deviation
numbers much), and fraction vs. log time looks pretty linear in that
region, certainly better than the linear-time plot. I feel my model,
using log normal, is validated! (I told you, didn't I, that this isn't
very scientific?!)

However, for the shorter time scale, and only a few bit decays, the
linear trend clearly doesn't extrapolate. I *could* put this down to
just a bit of noisiness - just a few bits difference (I estimate
roughly 10 out of 32k) would make the whole chart linear. However,
this feels like cheating. What I suspect is that, in the long tail,
there's something that means there's a few more weak bits than you'd
otherwise expect.

These fat tails mean the model I'm producing shouldn't be used for
deciding refresh rates. If you want to avoid unexpected single bit
flips across the whole of your memory, you need to understand the
fatness of the tails, and be very conservative.

For this exercise, I want to characterise the average cell (Why? No
practical reason, it's more to just demystify their behaviour. If I
use DRAM I'll be following the same refresh cycle as everyone else!).
In the sheet, I've done a linear regression on the region around the
median, and found the modal/median decay time to be 3 minutes, with
99.7% of cells (3 standard deviations) between 1.0 and 8.5 minutes.
Not bad when the documented max refresh time for a cell is something
like 128ms. Admittedly this is at room temperature, and decays happen
much faster at the temperatures running computers tend to see.

## Simplified changelog

This project has been through a number of phases:

 * Simply getting read/write cycles to work, with 4 address lines and
   4 data lines. Getting AVR I/O right, basically.
 * Getting beyond "it seems to read/write, just incorrectly", by
   adding a delay on the read value to account for inputs going
   through two latches to avoid metastability.
 * Doing an initial test of 4 bits of data stored at 256 locations on
   what the DRAM decay characteristics look like, and finding the test
   DRAM doesn't show decay at room temperature, even at the
   multi-second time horizon.
 * Wiring up more pins (2 more address lines, 4 more data lines), and
   doing a test, and discovering that the read value doesn't match the
   written value at 16MHz, but does at 8MHz and below. Discover the
   need to put in more delay.
 * Rerun earlier tests, still see no decay, try a hairdryer on the
   RAM, see decay!
 * Run tests at a variety of delays, and a range of temperatures from
   20 to 40 degrees Celsius.
 * Extract CSVs from the results, generated tables.
 * Run tests at room temperature with longer delays, in order to get
   more data to validate the model. Validate the model (time decay
   only, still no temperature modelling).