mod buddy;

type p_uintptr = usize;
type v_uintptr = usize;

#[repr(C)]
#[derive(Clone)]
pub struct mm_page {
    virt: v_uintptr,
}

impl mm_page {
    pub fn new() -> mm_page {
        mm_page { virt: 0 }
    }
}
