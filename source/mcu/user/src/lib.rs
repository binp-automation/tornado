#![no_std]

#[macro_use]
mod io;
#[cfg(not(test))]
mod panic;

#[no_mangle]
pub extern "C" fn hello() {
    println!("Hello from Rust!\r\n");
    assert_eq!(1, 2);
}
