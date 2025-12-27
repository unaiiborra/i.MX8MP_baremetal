/// Same struct as the defined in the uart.h header
#[repr(C, align(64))]
pub struct RingBuffer<const N: usize> {
    overwrite: bool,
    head: usize,
    tail: usize,
    buf: [u8; N],
}

impl<const T: usize> RingBuffer<T> {
    const MASK: usize = T - 1;

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.head == self.tail
    }

    #[inline]
    pub fn is_full(&self) -> bool {
        self.next(self.head) == self.tail
    }

    #[inline]
    fn next(&self, v: usize) -> usize {
        (v + 1) & Self::MASK
    }

    pub fn push(&mut self, v: u8) -> bool {
        if self.is_full() {
            if !self.overwrite {
                return false;
            }
            self.tail = (self.tail + 1) & Self::MASK;
        }

        self.buf[self.head] = v;
        self.head = self.next(self.head);

        true
    }

    pub fn pop(&mut self) -> Option<u8> {
        if self.is_empty() {
            return None;
        }

        let v = self.buf[self.tail];

        self.tail = (self.tail + 1) & Self::MASK;

        return Some(v);
    }
}
