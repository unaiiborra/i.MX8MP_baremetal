#![no_std]
#![allow(non_snake_case)]
#![allow(non_camel_case_types)]
mod drivers;
mod kernel;

use core::panic::PanicInfo;

use crate::drivers::uart::{UART_ID, UART_put_str};

// -- Panic handler ---

unsafe extern "C" {
    fn panic() -> !;

    static mut PANIC_MESSAGE_PTR: *mut u8;
    static mut PANIC_FILE_PTR: *mut u8;

    static mut PANIC_LINE: u32;
    static mut PANIC_COL: u32;

    static PANIC_MESSAGE_LEN: u64;
    static PANIC_FILE_LEN: u64;

}

#[unsafe(no_mangle)]
extern "C" fn rust_test_panic() -> ! {
    panic!("Test working :)")
}

#[panic_handler]
fn rust_panic(info: &PanicInfo) -> ! {
    UART_put_str(UART_ID::UART_ID_2, "\n\r[Rust panic!]");

    let msg_len: usize = unsafe { PANIC_MESSAGE_LEN } as usize;
    let file_len: usize = unsafe { PANIC_FILE_LEN } as usize;

    if msg_len == 0 || file_len == 0 {
        UART_put_str(UART_ID::UART_ID_2, "\n\r[PROBABLE LINKING ERROR]\n\r");
        loop {}
    }

    if let Some(msg) = info.message().as_str() {
        let copy_bytes = core::cmp::min(msg.len(), msg_len - 1);

        unsafe {
            core::ptr::copy_nonoverlapping(msg.as_ptr(), PANIC_MESSAGE_PTR, copy_bytes);

            *PANIC_MESSAGE_PTR.add(copy_bytes) = b'\0';
        }
    }

    if let Some(location) = info.location() {
        unsafe {
            PANIC_LINE = location.line();
            PANIC_COL = location.column();
        }

        let copy_bytes = core::cmp::min(location.file().len(), file_len - 1);

        unsafe {
            core::ptr::copy_nonoverlapping(location.file().as_ptr(), PANIC_FILE_PTR, copy_bytes);

            *PANIC_FILE_PTR.add(copy_bytes) = b'\0';
        }
    }

    unsafe { panic() };
}
