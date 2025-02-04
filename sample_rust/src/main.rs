use std::sync::atomic::{compiler_fence, Ordering};
use std::ptr::{self, NonNull};

fn main() 
{
    let mut value1 = 42;
    let mut value2 = 10;
    let mut x: NonNull<i32> = unsafe { NonNull::new_unchecked(&mut value1) };
    let y: NonNull<i32> = unsafe { NonNull::new_unchecked(&mut value2) };

    let mut result = 0;
    unsafe {
        compiler_fence(Ordering::SeqCst);
        
        let val = ptr::read_volatile(x.as_ptr()) + ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());
        compiler_fence(Ordering::SeqCst);

        let val = ptr::read_volatile(x.as_ptr()) - ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) * ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) / ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) % ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) & ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) | ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) ^ ptr::read_volatile(y.as_ptr());
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) << 2;
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());

        let val = ptr::read_volatile(x.as_ptr()) >> 1;
        ptr::write_volatile(x.as_mut(), val);
        result ^= ptr::read_volatile(x.as_ptr());
    }

    let mut vec = Vec::with_capacity(8);
    unsafe {
        compiler_fence(Ordering::SeqCst);
        vec.extend_from_slice(&[1, 2, 3, 4]);
        let vec_ptr = vec.as_ptr() as *const u8;
        for idx in 0..vec.len() {
            result ^= ptr::read_volatile(vec_ptr.add(idx)) as i32;
        }
        compiler_fence(Ordering::SeqCst);
    }

    /*let mut buf = String::with_capacity(16);
    unsafe {
        compiler_fence(Ordering::SeqCst);
        buf.push_str("hello");
        let str_ptr = buf.as_ptr();
        for idx in 0..buf.len() {
            result ^= ptr::read_volatile(str_ptr.add(idx)) as i32;
        }
        compiler_fence(Ordering::SeqCst);
    }*/

    unsafe {
        #[cfg(target_arch = "aarch64")]
        std::arch::asm!(
            "mov x0, {0}",
            "mov x8, #93", 
            "svc #0",
            in(reg) result,
            options(noreturn)
        );

        #[cfg(target_arch = "x86_64")]
        std::arch::asm!(
            "mov rdi, {0}",
            "mov rax, 60",
            "syscall",
            in(reg) result,
            options(noreturn)
        );

        std::hint::unreachable_unchecked()
    }
}
