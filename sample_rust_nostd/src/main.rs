#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> !
{
    loop {}
}

#[unsafe(no_mangle)]
pub extern "C" fn main() -> i32
{
    let mut value1 = 42;
    let mut value2 = 10;
    let mut x: core::ptr::NonNull<i32> = unsafe { core::ptr::NonNull::new_unchecked(&mut value1) };
    let y: core::ptr::NonNull<i32> = unsafe { core::ptr::NonNull::new_unchecked(&mut value2) };

    let mut result = 0;
    unsafe {
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
        
        let val = core::ptr::read_volatile(x.as_ptr()) + core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);

        let val = core::ptr::read_volatile(x.as_ptr()) - core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) * core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) / core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) % core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) & core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) | core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) ^ core::ptr::read_volatile(y.as_ptr());
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) << 2;
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());

        let val = core::ptr::read_volatile(x.as_ptr()) >> 1;
        core::ptr::write_volatile(x.as_mut(), val);
        result ^= core::ptr::read_volatile(x.as_ptr());
    }

    let mut vec = heapless::Vec::<u8, 8>::new();
    unsafe {
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
        vec.extend_from_slice(&[1, 2, 3, 4]).unwrap();
        let vec_ptr = vec.as_ptr() as *const u8;
        for idx in 0..vec.len() {
            result ^= core::ptr::read_volatile(vec_ptr.add(idx)) as i32;
        }
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }

    let mut buf = heapless::String::<16>::new();
    unsafe {
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
        buf.push_str("hello").unwrap();
        let str_ptr = buf.as_ptr();
        for idx in 0..buf.len() {
            result ^= core::ptr::read_volatile(str_ptr.add(idx)) as i32;
        }
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }

    unsafe {
        #[cfg(target_arch = "aarch64")]
        core::arch::asm!(
            "mov x0, {0}",
            "mov x8, #93",
            "svc #0",
            in(reg) result,
            options(noreturn)
        );

        #[cfg(target_arch = "x86_64")]
        core::arch::asm!(
            "mov rdi, {0}",
            "mov rax, 60",
            "syscall",
            in(reg) result,
            options(noreturn)
        );

        core::hint::unreachable_unchecked()
    }
}
