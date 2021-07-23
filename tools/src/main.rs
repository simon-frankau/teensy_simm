//
// simm_analyse: Take a set of test run logs at a variety of
// temperatures, and convert them into stats about the decay of memory
// cells.
//

#[macro_use] extern crate lazy_static;
extern crate regex;

use regex::Regex;
use std::env;
use std::fs;

#[derive(Clone, Debug)]
struct Entry {
       delay: usize,
       corrupted: Vec<String>,
       bit_count: usize,
}

fn to_entry(s: &str) -> Entry {
    let entry = s.split('\n').collect::<Vec<_>>();
    eprintln!("Thing: {}", s);

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

fn main() {
    let mut args = env::args();
    println!("{:?}", args);
    assert_eq!(args.len(), 2);
    let file_name = args.nth(1).unwrap();

    let buffer = fs::read_to_string(file_name).expect("Read error");

    let entries: Vec<Entry> = buffer
        .split("\n--------------------------------\n")
        .map(to_entry)
        .collect();

    print!("Entries: {:?}", entries);
}
