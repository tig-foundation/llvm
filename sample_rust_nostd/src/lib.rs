#![no_std]
#![feature(alloc_error_handler)]

use core::alloc::{GlobalAlloc, Layout};

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator::new();

struct BumpAllocator {
    _private: (),
}

impl BumpAllocator {
    const fn new() -> Self {
        BumpAllocator { _private: () }
    }
}

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, _layout: Layout) -> *mut u8 {
        core::ptr::null_mut()
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {}
}

#[alloc_error_handler]
fn alloc_error_handler(_layout: Layout) -> ! {
    loop {}
}
