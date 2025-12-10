#[repr(u32)]
#[derive(Clone, Copy)]
pub enum UART_ID {
    UART_ID_1 = 0,
    UART_ID_2,
    UART_ID_3,
    UART_ID_4,
}

unsafe extern "C" {
    fn UART_putc(id: u32, c: u8);
}

pub fn UART_put_str(id: UART_ID, s: &str) {
    for c in s.as_bytes().iter() {
        unsafe {
            UART_putc(id as u32, *c);
        }
    }
}

