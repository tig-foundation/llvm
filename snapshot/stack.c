// Naked function for runtime stack switching
// arg0: stack_top_ptr - The TOP address of the new stack. It will be aligned internally.
// arg1: func_to_call - The function to call on the new stack.
// arg2: arg - The argument to pass to func_to_call.
__attribute__((naked)) __attribute__((visibility("default"))) __attribute__((externally_visible)
void __switch_stack_and_call(void* stack_top_ptr, void (*func_to_call)(void*), void* arg) {
    #ifdef __x86_64__
        __asm__ volatile (
            // Backup the caller's state
            "pushq %rbp\n\t"
            "movq %rsp, %rbp\n\t"
            "movq %rsp, %r11\n\t"     // Backup original stack pointer
    
            // Switch to the new stack provided in %rdi
            "movq %rdi, %rsp\n\t"
    
            // *** CRITICAL ALIGNMENT FIX ***
            // The ABI requires the stack to be 16-byte aligned before a call.
            // `andq $-16, %rsp` clears the low 4 bits, ensuring alignment.
            "andq $-16, %rsp\n\t"
    
            // *** CRITICAL RED ZONE FIX ***
            // Allocate 128 bytes to prevent the called function from using the
            // red zone and writing above our new stack buffer.
            "subq $128, %rsp\n\t"
    
            // Set up arguments for the target function and call it
            "movq %rdx, %rdi\n\t"     // Move arg into the first argument register
            "callq *%rsi\n\t"         // Call the target function
    
            // Restore the original stack and return
            "movq %r11, %rsp\n\t"
            "popq %rbp\n\t"
            "retq\n\t"
            :
            : // No output operands
            : "memory", "r11", "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
        );
    #elif defined(__aarch64__)
        __asm__ volatile (
            // Backup the caller's state
            "stp x29, x30, [sp, #-16]!\n\t"
            "mov x29, sp\n\t"
            "mov x9, sp\n\t"                 // Backup original stack pointer
    
            // Switch to the new stack provided in x0
            "mov x10, x0\n\t"           // Copy to general purpose register
    
            // *** CRITICAL ALIGNMENT FIX ***
            // The AArch64 ABI also requires a 16-byte aligned stack.
            // `bic` is "Bit Clear" and is the idiomatic way to do this on ARM.
            // `bic x10, x10, #15` clears the lowest 4 bits.
            "bic x10, x10, #15\n\t"
            "mov sp, x10\n\t"           // Move aligned value to sp
    
            // Set up arguments for the target function and call it
            "mov x0, x2\n\t"                 // Move arg into the first argument register
            "blr x1\n\t"                     // Call the target function
    
            // Restore the original stack and return
            "mov sp, x9\n\t"
            "ldp x29, x30, [sp], #16\n\t"
            "ret\n\t"
            :
            : // No output operands
            : "memory", "x9", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17"
        );
    #else
        #error "Unsupported architecture for stack switching"
    #endif
    }