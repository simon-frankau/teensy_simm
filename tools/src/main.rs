//
// simm_analyse: Take a set of test run logs at a variety of
// temperatures, and convert them into stats about the decay of memory
// cells.
//

use std::env;
use std::fs;

#[derive(Clone, Debug)]
struct Entry {
}

fn to_entry(s: &str) -> Entry {
    let entry = s.split('\n').collect::<Vec<_>>();
    eprintln!("Thing: {}", s);

    // Should be 3 lines, but allow an extra blank line at the end of
    // the file.
    assert!(entry.len() == 3 || (entry.len() == 4 && entry[3] == ""));

    Entry{}
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
