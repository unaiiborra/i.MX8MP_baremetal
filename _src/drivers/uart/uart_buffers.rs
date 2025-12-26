use crate::{
    drivers::uart::UART_ID,
    lib::{buffer::pow2_ring::RsPow2RingBuffer, mutex::RsMutex},
};

const RX1_SIZE: usize = 32;
const TX1_SIZE: usize = 32;

const RX2_SIZE: usize = 4096;
const TX2_SIZE: usize = 32768;

const RX3_SIZE: usize = 32;
const TX3_SIZE: usize = 32;

const RX4_SIZE: usize = 32;
const TX4_SIZE: usize = 32;

static UART1_RX_RING: RsMutex<RsPow2RingBuffer<u8, RX1_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; RX1_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data
static UART1_TX_RING: RsMutex<RsPow2RingBuffer<u8, TX1_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; TX1_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data

static UART2_RX_RING: RsMutex<RsPow2RingBuffer<u8, RX2_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; RX2_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data
static UART2_TX_RING: RsMutex<RsPow2RingBuffer<u8, TX2_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; TX2_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data

static UART3_RX_RING: RsMutex<RsPow2RingBuffer<u8, RX3_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; RX3_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data
static UART3_TX_RING: RsMutex<RsPow2RingBuffer<u8, TX3_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; TX3_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data

static UART4_RX_RING: RsMutex<RsPow2RingBuffer<u8, RX4_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; RX4_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data
static UART4_TX_RING: RsMutex<RsPow2RingBuffer<u8, TX4_SIZE, false>> =
    RsMutex::new(RsPow2RingBuffer::new([0; TX4_SIZE])); // While not initialized to any other value than 0, it goes to bss and not data

// Push
#[unsafe(no_mangle)]
extern "C" fn UART_rxbuf_push(id: UART_ID, v: u8) -> bool {
    match id {
        UART_ID::UART_ID_1 => UART1_RX_RING.lock(|rx| rx.push(v)),
        UART_ID::UART_ID_2 => UART2_RX_RING.lock(|rx| rx.push(v)),
        UART_ID::UART_ID_3 => UART3_RX_RING.lock(|rx| rx.push(v)),
        UART_ID::UART_ID_4 => UART4_RX_RING.lock(|rx| rx.push(v)),
    }
}

#[unsafe(no_mangle)]
extern "C" fn UART_txbuf_push(id: UART_ID, v: u8) -> bool {
    match id {
        UART_ID::UART_ID_1 => UART1_TX_RING.lock(|tx| tx.push(v)),
        UART_ID::UART_ID_2 => UART2_TX_RING.lock(|tx| tx.push(v)),
        UART_ID::UART_ID_3 => UART3_TX_RING.lock(|tx| tx.push(v)),
        UART_ID::UART_ID_4 => UART4_TX_RING.lock(|tx| tx.push(v)),
    }
}

// Pop
#[unsafe(no_mangle)]
extern "C" fn UART_txbuf_pop(id: UART_ID, v: *mut u8) -> bool {
    if let Some(ptr) = unsafe { v.as_mut() } {
        if let Some(c) = UART_rs_tx_pop(id) {
            *ptr = c;

            return true;
        }
        return false;
    }

    panic!("invalid ref");
}

#[unsafe(no_mangle)]
extern "C" fn UART_rxbuf_pop(id: UART_ID, v: *mut u8) -> bool {
    if let Some(ptr) = unsafe { v.as_mut() } {
        if let Some(c) = UART_rs_rx_pop(id) {
            *ptr = c;

            return true;
        }
        return false;
    }

    panic!("invalid ref");
}

#[inline(always)]
pub fn UART_rs_rx_pop(id: UART_ID) -> Option<u8> {
    match id {
        UART_ID::UART_ID_1 => UART1_RX_RING.lock(|rx| rx.pop()),
        UART_ID::UART_ID_2 => UART2_RX_RING.lock(|rx| rx.pop()),
        UART_ID::UART_ID_3 => UART3_RX_RING.lock(|rx| rx.pop()),
        UART_ID::UART_ID_4 => UART4_RX_RING.lock(|rx| rx.pop()),
    }
}

#[inline(always)]
pub fn UART_rs_tx_pop(id: UART_ID) -> Option<u8> {
    match id {
        UART_ID::UART_ID_1 => UART1_TX_RING.lock(|tx| tx.pop()),
        UART_ID::UART_ID_2 => UART2_TX_RING.lock(|tx| tx.pop()),
        UART_ID::UART_ID_3 => UART3_TX_RING.lock(|tx| tx.pop()),
        UART_ID::UART_ID_4 => UART4_TX_RING.lock(|tx| tx.pop()),
    }
}
