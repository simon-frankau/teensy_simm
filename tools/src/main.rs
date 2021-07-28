//
// simm_analyse: Take a set of test run logs at a variety of
// temperatures, and convert them into stats about the decay of memory
// cells.
//

#[macro_use] extern crate lazy_static;
extern crate regex;

use regex::Regex;
use std::collections::HashMap;
use std::collections::HashSet;
use std::env;
use std::fs;

// The testing was done over 4K bytes.
const TESTED_BITS: usize = 4096 * 8;

#[derive(Clone, Debug)]
struct Entry {
       delay: usize,
       corrupted: Vec<String>,
       bit_count: usize,
}

fn to_entry(s: &str) -> Entry {
    let entry = s.split('\n').collect::<Vec<_>>();

    // Should be 3 lines, but allow an extra blank line at the end of
    // the file.
    assert!(entry.len() == 3 || (entry.len() == 4 && entry[3] == ""));

    // First line pattern is "Delay: n, Pattern: m", only care about n.
    let delay = {
        lazy_static! {
            static ref RE: Regex = Regex::new(r"^Delay: ([0-9]*), Pattern: [0-9]*$").unwrap();
        }
        let captures = RE.captures(entry[0]).unwrap();
        captures.get(1).unwrap().as_str().parse::<usize>().unwrap()
    };

    // Second line is comma-separated list of corrupt locations.
    // Collect them all. Annoyingly, only the first 31 get recorded.
    let locations = {
        let mut locs = entry[1].split(",").map(String::from).collect::<Vec<String>>();
        // Locations are comma-terminated, so we can always drop the
        // last entry (empty string).
        let last = locs.pop().unwrap();
        assert!(last.is_empty());
        locs
    };

    // Third line is number of diffs. 'Diffs' is the number of bit
    // changes, so if multiple bits are corrupt in the same byte, the
    // counts may differ.
    //
    let num_diffs = {
        lazy_static! {
            static ref RE: Regex = Regex::new(r"^Diffs: ([0-9]*)$").unwrap();
        }
        let captures = RE.captures(entry[2]).unwrap();
        captures.get(1).unwrap().as_str().parse::<usize>().unwrap()
    };

    assert!(locations.len() == 31 || num_diffs == locations.len());

    Entry{ delay: delay, corrupted: locations, bit_count: num_diffs }
}

// Generate a table of fraction of time corrupted, vs. delay and
// location. The idea is to see if it's always the same locations that
// are most corruptable, and that there's some threshold time at which
// their RC constant is too low, and they just corrupt.
fn generate_corruptability(stats: &[Entry]) {
    // We're going to be inefficient because the data set is small,
    // and it's easier that way...

    // Start by collecting all known corrupted addresses:
    let corrupted: HashSet<String> = stats
        .iter()
        .map(|stat| stat.corrupted.iter().map(|s| s.to_string()))
        .flatten()
        .collect();

    // And collect all delays at which corruption occurs:
    let delays: HashSet<usize> = stats
        .iter()
        .filter(|e| !e.corrupted.is_empty())
        .map(|e| e.delay)
        .collect();

    // Build entries for all pairs.
    #[derive(Clone, Debug, Eq, Hash, PartialEq)]
    struct Key {
        delay: usize,
        location: String,
    };
    #[derive(Clone, Debug)]
    struct Value {
        numerator: usize,
        denominator: usize
    };

    let mut data: HashMap<Key, Value> = HashMap::new();
    for addr in corrupted.iter() {
        for delay in delays.iter() {
            data.insert(Key { delay: *delay, location: addr.to_string() },
                        Value { numerator: 0, denominator: 0 });
        }
    }

    // And populate the numerators and denominators.
    for entry in stats.iter() {
        // Numerators: Does the address occur in the corrupted list?
        for addr in entry.corrupted.iter() {
            data
                .get_mut(&Key { delay: entry.delay, location: addr.to_string() })
                .unwrap()
                .numerator += 1;
        }
        // Denominator: All addresses are included, unless they fall
        // off the upper end of the corrupted list, in which case the
        // numerator isn't bumped, so we shouldn't bump the denominator.
        let max_recorded: &str = if entry.corrupted.len() == 31 {
            &entry.corrupted[30]
        } else {
            "FFFFFFFF"
        };
        for (k, v) in data.iter_mut() {
            if k.delay == entry.delay && k.location.as_str() <= max_recorded {
                v.denominator += 1;
            }
        }
    }

    // Now we want to order the addresses by when they first appear.
    let addrs_in_order = {
        let mut addr_map = HashMap::new();
        for entry in stats.iter() {
            for addr in entry.corrupted.iter() {
                // Update the first usgae time if it's non-existent,
                // or greater than the currently-recorded value. Sort
                // secondarily on fraction of time corrupted.

                if addr_map.get(addr).map_or(true, |&(usage, _)| usage > entry.delay) {
                    let d = data.get(&Key { delay: entry.delay, location: addr.to_string() }).unwrap();
                    // Integer to keep sortable.
                    let fraction = 100 - d.numerator * 100 / d.denominator;
                    addr_map.insert(addr.to_string(), (entry.delay, fraction));
                }
            }
        }
        let mut sorted_addrs = addr_map
            .into_iter()
            .map(|(k, v)| (v, k))
            .collect::<Vec<((usize, usize), String)>>();
        sorted_addrs.sort();
        sorted_addrs.into_iter().map(|(_, v)| v).collect::<Vec<String>>()
    };

    // We've got the data, we've got the delays and addresses, let's
    // print the table!

    // Header row.
    let mut delays_vec = delays.iter().map(|x| *x).collect::<Vec<usize>>();
    delays_vec.sort();
    let delays_strings = delays_vec.iter().map(|&x| x.to_string()).collect::<Vec<String>>();
    println!(", {}", delays_strings.join(", "));

    // Row for each address.
    for addr in addrs_in_order.iter() {
        print!("{}", addr);
        for delay in delays_vec.iter() {
            let entry = data.get(&Key { delay: *delay, location: addr.to_string() }).unwrap();
            print!(", {}", entry.numerator as f64 / entry.denominator as f64);
        }
        println!();
    }
}

// Generate a very simple table of average bit flip rates per decay period.
fn generate_flip_rates(stats: &[Entry])
{
    // Store numerator and denominator, per delay.
    let mut flip_rates: HashMap<usize, (usize, usize)> = HashMap::new();

    // Collect data.
    for entry in stats.iter() {
        if !flip_rates.contains_key(&entry.delay) {
            flip_rates.insert(entry.delay, (0, 0));
        }
        let (num, denom) = flip_rates.get_mut(&entry.delay).unwrap();
        *num += entry.bit_count;
        *denom += TESTED_BITS;
    }

    // Sort it.
    let mut flip_rates_vec = flip_rates
        .iter()
        .map(|(&delay, (num, denom))| (delay, *num as f64 / *denom as f64))
        .collect::<Vec<(usize, f64)>>();
    flip_rates_vec.sort_by(|(a, _), (b, _)| a.cmp(b));

    // Print it.
    println!("{}", flip_rates_vec.iter().map(|(delay, _)| delay.to_string()).collect::<Vec<String>>().join(","));
    println!("{}", flip_rates_vec.iter().map(|(_, frac)| frac.to_string()).collect::<Vec<String>>().join(","));
}

fn main() {
    let mut args = env::args();
    assert_eq!(args.len(), 2);
    let file_name = args.nth(1).unwrap();

    let buffer = fs::read_to_string(file_name).expect("Read error");

    let entries: Vec<Entry> = buffer
        .split("\n--------------------------------\n")
        .map(to_entry)
        .collect();

    generate_corruptability(&entries);
    println!();
    generate_flip_rates(&entries);
}
