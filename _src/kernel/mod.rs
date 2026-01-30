use core::ffi::c_void;

pub mod init;

#[repr(C)]
pub struct DriverHandle {
    pub base: usize,
    pub state: *mut c_void,
}
