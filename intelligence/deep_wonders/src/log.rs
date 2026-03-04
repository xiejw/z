#[macro_export]
macro_rules! panic_log {
    ($($arg:tt)*) => {{
        eprint!("\x1b[1;31m<-- PANIC -->\x1b[0m ");
        eprintln!($($arg)*);
        std::process::exit(1);
    }};
}

#[macro_export]
macro_rules! info {
    ($($arg:tt)*) => {{
        eprint!("\x1b[1;32m[INFO]\x1b[0m ");
        eprintln!($($arg)*);
    }};
}

#[macro_export]
macro_rules! warn {
    ($($arg:tt)*) => {{
        eprint!("\x1b[1;35m[WARN]\x1b[0m ");
        eprintln!($($arg)*);
    }};
}
