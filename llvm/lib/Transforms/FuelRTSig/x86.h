unsigned getIntrinsicCostX86(StringRef Name)
{
    // SSE intrinsics
    if (Name.starts_with("llvm.x86.sse"))
    {
        // SSE arithmetic operations
        if (Name.contains(".add") || Name.contains(".sub"))
            return 1;
        if (Name.contains(".mul"))
            return Name.contains(".ps") ? 4 : 3; // FP mul vs integer
        if (Name.contains(".div"))
            return Name.contains(".ps") ? 12 : 17; // SP vs DP division
        if (Name.contains(".sqrt"))
            return Name.contains(".ps") ? 12 : 17;
            
        // SSE horizontal operations - more expensive
        if (Name.contains(".hadd") || Name.contains(".hsub"))
            return 3;
            
        // SSE comparison operations
        if (Name.contains(".cmp") || Name.contains(".comieq") || 
            Name.contains(".comige") || Name.contains(".comigt") ||
            Name.contains(".comile") || Name.contains(".comilt") ||
            Name.contains(".comineq"))
            return 1;
            
        // SSE logical operations
        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1;
            
        // SSE shuffle and permute operations
        if (Name.contains(".shuf") || Name.contains(".pshufd") ||
            Name.contains(".pshufb") || Name.contains(".pshufw"))
            return 1;
        if (Name.contains(".unpack"))
            return 1;
        if (Name.contains(".movmsk"))
            return 2;
            
        // SSE conversion operations
        if (Name.contains(".cvt"))
            return 3;
        if (Name.contains(".cvtsi2ss") || Name.contains(".cvtsi2sd"))
            return 4;
        if (Name.contains(".cvttss2si") || Name.contains(".cvttsd2si"))
            return 3;
            
        // SSE min/max operations
        if (Name.contains(".min") || Name.contains(".max"))
            return 1;
            
        // SSE load/store operations
        if (Name.contains(".loadu") || Name.contains(".load"))
            return 1;
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 1;
        if (Name.contains(".movnt"))
            return 1;
            
        // SSE2 integer operations
        if (Name.contains("sse2"))
        {
            if (Name.contains(".padd") || Name.contains(".psub"))
                return 1;
            if (Name.contains(".pmul"))
                return 3;
            if (Name.contains(".pcmp"))
                return 1;
            if (Name.contains(".ppack") || Name.contains(".punpck"))
                return 1;
            if (Name.contains(".psll") || Name.contains(".psrl") || 
                Name.contains(".psra"))
                return 1;
            if (Name.contains(".sad"))
                return 4; // Sum of absolute differences
        }
    }
    
    // AVX intrinsics
    if (Name.starts_with("llvm.x86.avx"))
    {
        // AVX arithmetic operations
        if (Name.contains(".add") || Name.contains(".sub"))
            return 1;
        if (Name.contains(".mul"))
            return Name.contains(".ps") ? 4 : 3;
        if (Name.contains(".div"))
            return Name.contains(".ps") ? 12 : 17;
        if (Name.contains(".sqrt"))
            return Name.contains(".ps") ? 12 : 17;
            
        // AVX horizontal operations - expensive on x86
        if (Name.contains(".hadd") || Name.contains(".hsub"))
            return 4;
            
        // AVX fused multiply-add
        if (Name.contains(".fma") || Name.contains(".fmsub") ||
            Name.contains(".fnmadd") || Name.contains(".fnmsub"))
            return 4;
            
        // AVX comparison operations
        if (Name.contains(".cmp"))
            return 1;
            
        // AVX logical operations
        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1;
            
        // AVX shuffle and permute operations
        if (Name.contains(".vperm") || Name.contains(".vinsert") ||
            Name.contains(".vextract"))
            return 1;
        if (Name.contains(".vpermil"))
            return 1;
        if (Name.contains(".vbroadcast"))
            return 1;
        if (Name.contains(".shuffle"))
            return 1;
        if (Name.contains(".blend"))
            return 1;
            
        // AVX conversion operations
        if (Name.contains(".cvt"))
            return 3;
            
        // AVX min/max operations
        if (Name.contains(".min") || Name.contains(".max"))
            return 1;
            
        // AVX load/store operations
        if (Name.contains(".loadu") || Name.contains(".load"))
            return 1;
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 1;
        if (Name.contains(".maskload") || Name.contains(".maskstore"))
            return 3; // Slightly more expensive
        if (Name.contains(".gather"))
            return 8; // Much more expensive - x86 weakness
            
        // AVX2 operations
        if (Name.contains("avx2"))
        {
            if (Name.contains(".padd") || Name.contains(".psub"))
                return 1;
            if (Name.contains(".pmul"))
                return 3;
            if (Name.contains(".pcmp"))
                return 1;
            if (Name.contains(".ppack") || Name.contains(".punpck"))
                return 1;
            if (Name.contains(".psll") || Name.contains(".psrl") || 
                Name.contains(".psra"))
                return 1;
            if (Name.contains(".perm") || Name.contains(".shuf"))
                return 1;
            if (Name.contains(".gather"))
                return 8; // Expensive on AVX2
            if (Name.contains(".hadd") || Name.contains(".hsub"))
                return 4; // Horizontal operations
            if (Name.contains(".sad"))
                return 4;
        }
    }
    
    // AVX-512 intrinsics
    if (Name.starts_with("llvm.x86.avx512"))
    {
        // AVX-512 arithmetic operations
        if (Name.contains(".add") || Name.contains(".sub"))
            return 1;
        if (Name.contains(".mul"))
            return Name.contains(".ps") ? 4 : 3;
        if (Name.contains(".div"))
            return Name.contains(".ps") ? 14 : 20; // Slightly reduced from original
        if (Name.contains(".sqrt"))
            return Name.contains(".ps") ? 14 : 20;
            
        // AVX-512 horizontal operations
        if (Name.contains(".hadd") || Name.contains(".hsub"))
            return 5; // Even more expensive for 512-bit
            
        // AVX-512 fused multiply-add
        if (Name.contains(".fma") || Name.contains(".fmsub") ||
            Name.contains(".fnmadd") || Name.contains(".fnmsub"))
            return 4;
            
        // AVX-512 comparison operations
        if (Name.contains(".cmp") || Name.contains(".pcmp"))
            return 1;
            
        // AVX-512 logical operations
        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1;
            
        // AVX-512 shuffle and permute operations
        if (Name.contains(".perm") || Name.contains(".shuf"))
            return 1;
        if (Name.contains(".insert") || Name.contains(".extract"))
            return 1;
        if (Name.contains(".broadcast"))
            return 1;
        if (Name.contains(".align"))
            return 1;
            
        // AVX-512 mask operations
        if (Name.contains(".mask"))
            return 1;
        if (Name.contains(".cmp") && Name.contains(".mask"))
            return 1;
            
        // AVX-512 conversion operations
        if (Name.contains(".cvt"))
            return 3;
        if (Name.contains(".cvtps2ph") || Name.contains(".cvtph2ps"))
            return 4; // Half precision conversions
            
        // AVX-512 min/max operations
        if (Name.contains(".min") || Name.contains(".max"))
            return 1;
            
        // AVX-512 load/store operations
        if (Name.contains(".loadu") || Name.contains(".load"))
            return 2; // 512-bit loads can stress memory bandwidth
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 2; // 512-bit stores
        if (Name.contains(".gather") || Name.contains(".scatter"))
            return 10; // Still expensive but better than AVX2
            
        // AVX-512 specialized operations
        if (Name.contains(".conflict") || Name.contains(".lzcnt"))
            return 3;
        if (Name.contains(".reduce"))
            return 5; // Horizontal reductions are expensive
        if (Name.contains(".range"))
            return 4;
        if (Name.contains(".fixupimm"))
            return 4;
        if (Name.contains(".getexp") || Name.contains(".getmant"))
            return 4;
        if (Name.contains(".scalef"))
            return 4;
            
        // AVX-512 integer operations
        if (Name.contains(".padd") || Name.contains(".psub"))
            return 1;
        if (Name.contains(".pmul"))
            return 3;
        if (Name.contains(".pmadd"))
            return 3;
        if (Name.contains(".psll") || Name.contains(".psrl") || 
            Name.contains(".psra"))
            return 1;
        if (Name.contains(".pabs"))
            return 1;
        if (Name.contains(".pmin") || Name.contains(".pmax"))
            return 1;
        if (Name.contains(".sad"))
            return 4; // Sum of absolute differences
        if (Name.contains(".dbpsadbw"))
            return 5; // Double block SAD
    }
    
    // Other x86 intrinsics
    if (Name.starts_with("llvm.x86"))
    {
        // BMI/BMI2 bit manipulation - these are efficient
        if (Name.contains(".bmi") || Name.contains(".andn") ||
            Name.contains(".bextr") || Name.contains(".blsi") ||
            Name.contains(".blsmsk") || Name.contains(".blsr"))
            return 1;
        if (Name.contains(".bzhi") || Name.contains(".pdep") ||
            Name.contains(".pext"))
            return 1;
        if (Name.contains(".lzcnt") || Name.contains(".tzcnt"))
            return 1;
        if (Name.contains(".popcnt"))
            return 1;
            
        // PCLMUL - polynomial multiplication
        if (Name.contains(".pclmul"))
            return 7;
            
        // AES - hardware accelerated
        if (Name.contains(".aes"))
            return 4;
            
        // SHA - hardware accelerated
        if (Name.contains(".sha"))
            return 4;
            
        // RDRAND/RDSEED - variable latency
        if (Name.contains(".rdrand") || Name.contains(".rdseed"))
            return 10; // Variable, but typically expensive
            
        // Memory fence operations
        if (Name.contains(".sfence") || Name.contains(".lfence") ||
            Name.contains(".mfence"))
            return 1;
            
        // Cache control
        if (Name.contains(".clflush") || Name.contains(".clwb"))
            return 50; // Very expensive cache operations
        if (Name.contains(".prefetch"))
            return 1;
            
        // Special CPU instructions
        if (Name.contains(".pause"))
            return 10; // Pause instruction for spin loops
        if (Name.contains(".rdtsc"))
            return 25; // Time stamp counter
    }
    
    return 0; // Unknown intrinsic
}

unsigned getGenericIntrinsicCostX86(StringRef Name)
{
    // Generic intrinsics with x86-specific adjustments
    if (Name.starts_with("llvm.vector.reduce"))
    {
        // x86 horizontal operations are genuinely more expensive
        if (Name.contains(".add") || Name.contains(".mul"))
        {
            if (Name.contains(".v4"))
                return 7;  // Slightly higher than AArch64's 5
            if (Name.contains(".v8"))
                return 10; // Moderately higher than AArch64's 7  
            if (Name.contains(".v16"))
                return 15; // Higher for AVX-512 due to complexity
        }
        if (Name.contains(".fadd") || Name.contains(".fmul"))
        {
            if (Name.contains(".v4"))
                return 11; // Slightly higher than AArch64's 9
            if (Name.contains(".v8"))
                return 15; // Moderately higher than AArch64's 11
            if (Name.contains(".v16"))
                return 20; // Higher for AVX-512
        }
    }

    if (Name.starts_with("llvm.masked.load"))
    {
        // x86 masked loads are more expensive, but not drastically
        if (Name.contains(".v4"))
            return 7;  // vs AArch64's 5
        if (Name.contains(".v8"))
            return 8;  // vs AArch64's 6
        if (Name.contains(".v16"))
            return 12; // vs AArch64's 8 - AVX-512 penalty
    }

    if (Name.starts_with("llvm.masked.store"))
    {
        // x86 masked stores are more expensive
        if (Name.contains(".v4"))
            return 4;  // vs AArch64's 3
        if (Name.contains(".v8"))
            return 6;  // vs AArch64's 4
        if (Name.contains(".v16"))
            return 9;  // vs AArch64's 6
    }

    if (Name.starts_with("llvm.fma"))
    {
        // x86 FMA is actually quite good - similar costs
        if (Name.contains(".v2"))
            return 8;  // Same as AArch64
        if (Name.contains(".v4"))
            return 12; // Same as AArch64
        if (Name.contains(".v8"))
            return 16; // Same as AArch64
        return 4;      // Same as AArch64
    }

    if (Name.starts_with("llvm.bswap"))
    {
        // x86 BSWAP is very efficient
        if (Name.contains("i128"))
            return 2; // Same as AArch64
        if (Name.contains(".v"))
            return 3; // Same as AArch64
        return 1;     // Same as AArch64
    }

    if (Name.starts_with("llvm.ctpop"))
        return 3; // Same as AArch64 - POPCNT is efficient

    if (Name.starts_with("llvm.ctlz") || Name.starts_with("llvm.cttz"))
        return 2; // Slightly higher than AArch64's 1 due to BSR/BSF + correction

    if (Name.starts_with("llvm.fshl") || Name.starts_with("llvm.fshr"))
        return 2; // Same as AArch64

    if (Name.starts_with("llvm.umin") || Name.starts_with("llvm.umax") || 
        Name.starts_with("llvm.smin") || Name.starts_with("llvm.smax"))
        return 2; // Same as AArch64

    if (Name.starts_with("llvm.abs"))
        return 2; // Same as AArch64

    if (Name.starts_with("llvm.fabs"))
        return 1; // Same as AArch64

    if (Name.starts_with("llvm.sqrt"))
        return 15; // Same as AArch64 - modern x86 sqrt is quite good

    if (Name.starts_with("llvm.powi"))
        return 25; // Same as AArch64

    if(Name.starts_with("llvm.exp2"))
        return 25; // Same as AArch64

    if(Name.starts_with("llvm.exp10"))
        return 35; // Same as AArch64

    if (Name.starts_with("llvm.exp")
        && !Name.starts_with("llvm.expect") && !Name.starts_with("llvm.experimental"))
        return 35; // Same as AArch64

    // Math functions - similar performance on modern x86
    if (Name.starts_with("llvm.round") || Name.starts_with("llvm.roundeven"))
        return 6; // Same as AArch64

    if (Name.starts_with("llvm.floor") || Name.starts_with("llvm.ceil"))
        return 5; // Same as AArch64

    if (Name.starts_with("llvm.smul.with.overflow") || 
        Name.starts_with("llvm.umul.with.overflow"))
        return 3; // Same as AArch64

    if (Name.starts_with("llvm.sadd.with.overflow") || 
        Name.starts_with("llvm.uadd.with.overflow") ||
        Name.starts_with("llvm.ssub.with.overflow") || 
        Name.starts_with("llvm.usub.with.overflow"))
        return 2; // Same as AArch64

    if (Name.starts_with("llvm.sadd.sat") || Name.starts_with("llvm.uadd.sat") ||
        Name.starts_with("llvm.ssub.sat") || Name.starts_with("llvm.usub.sat"))
        return 2; // Same as AArch64

    if(Name.starts_with("llvm.atan2"))
        return 70; // vs AArch64's 65

    if(Name.starts_with("llvm.atan"))
        return 55; // vs AArch64's 50

    if (Name.starts_with("llvm.cosh") || 
        Name.starts_with("llvm.sinh") || 
        Name.starts_with("llvm.tanh"))
        return 45; // vs AArch64's 40

    if (Name.starts_with("llvm.tan"))
        return 50; // vs AArch64's 45

    if(Name.starts_with("llvm.sincospi"))
        return 80; // vs AArch64's 75

    if(Name.starts_with("llvm.sincos"))
        return 65; // vs AArch64's 60

    if (Name.starts_with("llvm.asin") || Name.starts_with("llvm.acos"))
        return 55; // vs AArch64's 50

    if (Name.starts_with("llvm.modf"))
    {
        if (Name.contains(".f64"))
            return 10; // vs AArch64's 9
        if (Name.contains(".f32"))
            return 6;  // vs AArch64's 5
    }

    if (Name.starts_with("llvm.sin") || Name.starts_with("llvm.cos"))
        return 38; // vs AArch64's 35

    if (Name.starts_with("llvm.pow"))
        return 42; // vs AArch64's 40

    if (Name.starts_with("llvm.log2"))
        return 24; // vs AArch64's 23
    if (Name.starts_with("llvm.log10"))
        return 26; // vs AArch64's 25
    if (Name.starts_with("llvm.log"))
        return 25; // vs AArch64's 24

    if (Name.starts_with("llvm.masked.gather") || Name.starts_with("llvm.masked.scatter"))
        return 20; // vs AArch64's 8 - x86 gather/scatter still more expensive

    if (Name.starts_with("llvm.matrix.transpose"))
        return 7; // vs AArch64's 6

    if (Name.starts_with("llvm.matrix.multiply"))
    {
        if (Name.contains(".v2"))
            return 16; // vs AArch64's 15
        if (Name.contains(".v4"))
            return 27; // vs AArch64's 25
        if (Name.contains(".v8"))
            return 42; // vs AArch64's 40
        return 22;     // vs AArch64's 20
    }

    if (Name.starts_with("llvm.matrix"))
        return 8; // Same as AArch64

    // Vector reductions - the main area where x86 is genuinely less efficient
    if (Name.starts_with("llvm.vector.reduce.smax") || 
        Name.starts_with("llvm.vector.reduce.smin") || 
        Name.starts_with("llvm.vector.reduce.umax") || 
        Name.starts_with("llvm.vector.reduce.umin"))
    {
        if (Name.contains(".v2"))
            return 5;  // vs AArch64's 4
        if (Name.contains(".v4"))
            return 8;  // vs AArch64's 6
        if (Name.contains(".v8"))
            return 12; // vs AArch64's 8
        return 9;      // vs AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.or") || 
        Name.starts_with("llvm.vector.reduce.xor") ||
        Name.starts_with("llvm.vector.reduce.and") ||
        Name.starts_with("llvm.vector.reduce.not"))
    {
        if (Name.contains(".v2"))
            return 5;  // vs AArch64's 4
        if (Name.contains(".v4"))
            return 8;  // vs AArch64's 6
        if (Name.contains(".v8"))
            return 12; // vs AArch64's 8
        return 9;      // vs AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.fmax") || 
        Name.starts_with("llvm.vector.reduce.fmin"))
    {
        if (Name.contains(".v2"))
            return 7;  // vs AArch64's 6
        if (Name.contains(".v4"))
            return 10; // vs AArch64's 8
        if (Name.contains(".v8"))
            return 14; // vs AArch64's 10
        return 12;     // vs AArch64's 10
    }

    if (Name.starts_with("llvm.vector.reduce.add"))
    {
        if (Name.contains(".v2"))
            return 4;  // vs AArch64's 3
        if (Name.contains(".v4"))
            return 7;  // vs AArch64's 6
        if (Name.contains(".v8"))
            return 11; // vs AArch64's 9
        return 8;      // vs AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.mul"))
    {
        if (Name.contains(".v2"))
            return 9;  // vs AArch64's 8
        if (Name.contains(".v4"))
            return 14; // vs AArch64's 12
        if (Name.contains(".v8"))
            return 18; // vs AArch64's 16
        return 16;     // vs AArch64's 14
    }

    if (Name.starts_with("llvm.vector.reduce.sub"))
    {
        if (Name.contains(".v2"))
            return 4;  // vs AArch64's 3
        if (Name.contains(".v4"))
            return 6;  // vs AArch64's 5
        if (Name.contains(".v8"))
            return 9;  // vs AArch64's 7
        return 10;     // vs AArch64's 9
    }

    if (Name.starts_with("llvm.minnum") || Name.starts_with("llvm.maxnum"))
    {
        if (Name.contains(".v2"))
            return 6; // Same as AArch64
        if (Name.contains(".v4"))
            return 8; // Same as AArch64
        if (Name.contains(".v8"))
            return 10; // Same as AArch64
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }   

    if(Name.starts_with("llvm.lround") || Name.starts_with("llvm.llround"))
    {
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }

    if (Name.starts_with("llvm.minimumnum") || Name.starts_with("llvm.maximumnum"))
    {
        if (Name.contains(".v2"))
            return 6; // Same as AArch64
        if (Name.contains(".v4"))
            return 8; // Same as AArch64
        if (Name.contains(".v8"))
            return 10; // Same as AArch64
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }   
    
    if (Name.starts_with("llvm.minimum") || Name.starts_with("llvm.maximum"))
    {
        if (Name.contains(".v2"))
            return 6; // Same as AArch64
        if (Name.contains(".v4"))
            return 8; // Same as AArch64
        if (Name.contains(".v8"))
            return 10; // Same as AArch64
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }

    if(Name.starts_with("llvm.rint") || Name.starts_with("llvm.nearbyint"))
    {
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }

    if(Name.starts_with("llvm.lrint") || Name.starts_with("llvm.llrint"))
    {
        if (Name.contains(".f32"))
            return 3; // Same as AArch64
        if (Name.contains(".f64"))
            return 5; // Same as AArch64
        return 2;     // Same as AArch64
    }

    if(Name.starts_with("llvm.experimental.constrained."))
    {
        if (Name.contains(".fadd") || Name.contains(".fsub"))
            return 5; // Same as AArch64

        if (Name.contains(".fmul"))
            return 7; // Same as AArch64

        if (Name.contains(".fdiv") || Name.contains(".frem"))
            return 17; // Same as AArch64

        if (Name.contains(".fma") || Name.contains(".fmuladd"))
            return 8; // Same as AArch64

        if (Name.contains(".fcmp") || Name.contains(".fcmps"))
            return 3; // Same as AArch64

        if (Name.contains(".sqrt"))
            return 15; // Same as AArch64

        if (Name.contains(".powi"))
            return 25; // Same as AArch64

        if (Name.contains(".pow"))
            return 42; // Slightly higher than AArch64's 40

        if (Name.contains(".asin") || Name.contains(".acos"))
            return 55; // Slightly higher than AArch64's 50

        if (Name.contains(".atan2"))
            return 70; // Slightly higher than AArch64's 65

        if (Name.contains(".atan"))
            return 55; // Slightly higher than AArch64's 50

        if (Name.contains(".sinh") || Name.contains(".cosh") || Name.contains(".tanh"))
            return 45; // Slightly higher than AArch64's 40

        if (Name.contains(".sin") || Name.contains(".cos"))
            return 38; // Slightly higher than AArch64's 35

        if (Name.contains(".tan"))
            return 50; // Slightly higher than AArch64's 45

        if (Name.contains(".exp") && !Name.contains(".exp2"))
            return 35; // Same as AArch64

        if (Name.contains(".exp2"))
            return 25; // Same as AArch64

        if (Name.contains(".log2"))
            return 24; // Slightly higher than AArch64's 23

        if (Name.contains(".log10"))
            return 26; // Slightly higher than AArch64's 25

        if (Name.contains(".log") && !Name.contains(".log10") && !Name.contains(".log2"))
            return 25; // Slightly higher than AArch64's 24

        if (Name.contains(".rint") || Name.contains(".nearbyint"))
        {
            if (Name.contains(".f32"))
                return 3; // Same as AArch64
            if (Name.contains(".f64"))
                return 5; // Same as AArch64
            return 2;     // Same as AArch64
        }

        if (Name.contains(".lrint") || Name.contains(".llrint"))
        {
            if (Name.contains(".f32"))
                return 3; // Same as AArch64
            if (Name.contains(".f64"))
                return 5; // Same as AArch64
            return 2;     // Same as AArch64
        }

        if (Name.contains(".maxnum") || Name.contains(".minnum") ||
            Name.contains(".maximum") || Name.contains(".minimum"))
        {
            if (Name.contains(".v2"))
                return 6; // Same as AArch64
            if (Name.contains(".v4"))
                return 8; // Same as AArch64
            if (Name.contains(".v8"))
                return 10; // Same as AArch64
            if (Name.contains(".f32"))
                return 3; // Same as AArch64
            if (Name.contains(".f64"))
                return 5; // Same as AArch64
            return 2;     // Same as AArch64
        }

        if (Name.contains(".ceil") || Name.contains(".floor"))
            return 5; // Same as AArch64

        if (Name.contains(".round") || Name.contains(".roundeven"))
            return 6; // Same as AArch64

        if (Name.contains(".lround") || Name.contains(".llround"))
        {
            if (Name.contains(".f32"))
                return 3; // Same as AArch64
            if (Name.contains(".f64"))
                return 5; // Same as AArch64
            return 2;     // Same as AArch64
        }
    }

    if (Name.starts_with("llvm.bitreverse"))
        return 2; 
    
    if (Name.starts_with("llvm.byteswap"))
        return 1;

    if (Name.starts_with("llvm.smul.fix.sat") || Name.starts_with("llvm.umul.fix.sat"))
        return 4;

    if (Name.starts_with("llvm.sdiv.fix.sat") || Name.starts_with("llvm.udiv.fix.sat"))
        return 15;

    if (Name.starts_with("llvm.prefetch"))
        return 1;

    if (Name.starts_with("llvm.assume"))
        return 0;

    if (Name.starts_with("llvm.atomic"))
        return 8;

    if (Name.starts_with("llvm.cmpxchg"))
        return 6;

    if (Name.starts_with("llvm.trunc"))
        return 3;

    if (Name.starts_with("llvm.copysign"))
        return 2;

    if (Name.starts_with("llvm.ldexp"))
        return 5;

    if (Name.starts_with("llvm.frexp"))
        return 7;

    return 0;
}

Value *createMemcpyCostCalculationX86(Value *Size, unsigned BaseCost, 
    IRBuilder<> &Builder, MDNode *OpSigMD) 
{
    IntegerType *I64Ty = Builder.getInt64Ty();

    Value *Size8 = ConstantInt::get(I64Ty, 8);
    Value *Size32 = ConstantInt::get(I64Ty, 32);
    Value *Size128 = ConstantInt::get(I64Ty, 128);
    Value *Size1024 = ConstantInt::get(I64Ty, 1024);

    // x86-64 specific costs
    Value *Cost1 = ConstantInt::get(I64Ty, BaseCost + 1);   // <= 8: Single MOV
    Value *Cost3 = ConstantInt::get(I64Ty, BaseCost + 3);   // <= 32: Unrolled MOVs  
    Value *Cost6 = ConstantInt::get(I64Ty, BaseCost + 6);   // <= 128: SSE/AVX
    Value *Cost20 = ConstantInt::get(I64Ty, BaseCost + 20); // <= 1024: Loop + overhead
    Value *Cost35 = ConstantInt::get(I64Ty, BaseCost + 35); // > 1024: Library call

    Value *Cmp8 = Builder.CreateICmpULE(Size, Size8);
    if (auto *Inst = dyn_cast<Instruction>(Cmp8))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp32 = Builder.CreateICmpULE(Size, Size32);
    if (auto *Inst = dyn_cast<Instruction>(Cmp32))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp128 = Builder.CreateICmpULE(Size, Size128);
    if (auto *Inst = dyn_cast<Instruction>(Cmp128))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp1024 = Builder.CreateICmpULE(Size, Size1024);
    if (auto *Inst = dyn_cast<Instruction>(Cmp1024))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel4 = Builder.CreateSelect(Cmp1024, Cost20, Cost35);
    if (auto *Inst = dyn_cast<Instruction>(Sel4))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel3 = Builder.CreateSelect(Cmp128, Cost6, Sel4);
    if (auto *Inst = dyn_cast<Instruction>(Sel3))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel2 = Builder.CreateSelect(Cmp32, Cost3, Sel3);
    if (auto *Inst = dyn_cast<Instruction>(Sel2))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *FinalCost = Builder.CreateSelect(Cmp8, Cost1, Sel2);
    if (auto *Inst = dyn_cast<Instruction>(FinalCost))
        Inst->setMetadata("op_sig", OpSigMD);

    return FinalCost;
}

Value *createMemsetCostCalculationX86(Value *Size, IRBuilder<> &Builder, MDNode *OpSigMD) 
{
    IntegerType *I64Ty = Builder.getInt64Ty();

    Value *Size8 = ConstantInt::get(I64Ty, 8);
    Value *Size32 = ConstantInt::get(I64Ty, 32);  
    Value *Size128 = ConstantInt::get(I64Ty, 128);
    Value *Size1024 = ConstantInt::get(I64Ty, 1024);

    // x86-64 memset costs
    Value *Cost2 = ConstantInt::get(I64Ty, 2);   // <= 8: Single store
    Value *Cost4 = ConstantInt::get(I64Ty, 4);   // <= 32: Unrolled stores
    Value *Cost8 = ConstantInt::get(I64Ty, 8);   // <= 128: SSE/AVX stores  
    Value *Cost25 = ConstantInt::get(I64Ty, 25); // <= 1024: Loop
    Value *Cost45 = ConstantInt::get(I64Ty, 45); // > 1024: Library optimized

    Value *Cmp8 = Builder.CreateICmpULE(Size, Size8);
    if (auto *Inst = dyn_cast<Instruction>(Cmp8))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp32 = Builder.CreateICmpULE(Size, Size32);
    if (auto *Inst = dyn_cast<Instruction>(Cmp32))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp128 = Builder.CreateICmpULE(Size, Size128);
    if (auto *Inst = dyn_cast<Instruction>(Cmp128))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Cmp1024 = Builder.CreateICmpULE(Size, Size1024);
    if (auto *Inst = dyn_cast<Instruction>(Cmp1024))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel4 = Builder.CreateSelect(Cmp1024, Cost25, Cost45);
    if (auto *Inst = dyn_cast<Instruction>(Sel4))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel3 = Builder.CreateSelect(Cmp128, Cost8, Sel4);
    if (auto *Inst = dyn_cast<Instruction>(Sel3))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel2 = Builder.CreateSelect(Cmp32, Cost4, Sel3);
    if (auto *Inst = dyn_cast<Instruction>(Sel2))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *FinalCost = Builder.CreateSelect(Cmp8, Cost2, Sel2);
    if (auto *Inst = dyn_cast<Instruction>(FinalCost))
        Inst->setMetadata("op_sig", OpSigMD);

    return FinalCost;
}


unsigned insertDynamicMemoryCostX86(CallInst *Call, StringRef Name, Value *SizeArg, 
                                        IRBuilder<> &Builder, MDNode *OpSigMD) 
{
    IntegerType *I64Ty = Builder.getInt64Ty();
    
    Value *Size = SizeArg;
    if (Size->getType() != I64Ty) {
        Size = Builder.CreateZExtOrTrunc(Size, I64Ty);
        if (auto *Inst = dyn_cast<Instruction>(Size))
            Inst->setMetadata("op_sig", OpSigMD);
    }
    
    unsigned StaticBaseCost;
    if (Name.starts_with("llvm.memmove")) {
        StaticBaseCost = 3;
    } else if (Name.starts_with("llvm.memcpy")) {
        StaticBaseCost = 2;
    } else if (Name.starts_with("llvm.memset")) {
        StaticBaseCost = 0; 
    } else {
        return 5; 
    }
    
    Value *DynamicCost;
    
    if (Name.starts_with("llvm.memset")) {
        DynamicCost = createMemsetCostCalculationX86(Size, Builder, OpSigMD);
    } else {
        DynamicCost = createMemcpyCostCalculationX86(Size, StaticBaseCost, Builder, OpSigMD);
    }
    
    if (DynamicCost->getType() != I64Ty) {
        DynamicCost = Builder.CreateZExtOrTrunc(DynamicCost, I64Ty);
        if (auto *Inst = dyn_cast<Instruction>(DynamicCost))
            Inst->setMetadata("op_sig", OpSigMD);
    }
    
    Module *M = Call->getModule();
    LLVMContext &Context = M->getContext();
    GlobalVariable *ThreadLocalFuelGlobal = cast<GlobalVariable>(M->getOrInsertGlobal("__thread_local_fuel_used", Type::getInt64Ty(Context)));
    ThreadLocalFuelGlobal->setLinkage(GlobalValue::ExternalLinkage);
    ThreadLocalFuelGlobal->setThreadLocal(true);
    
    LoadInst *CurrentFuel = Builder.CreateLoad(I64Ty, ThreadLocalFuelGlobal);
    CurrentFuel->setMetadata("op_sig", OpSigMD);
        
    Value *NewFuel = Builder.CreateAdd(CurrentFuel, DynamicCost);
    if (auto *Inst = dyn_cast<Instruction>(NewFuel))
        Inst->setMetadata("op_sig", OpSigMD);
        
    StoreInst *StoreFuel = Builder.CreateStore(NewFuel, ThreadLocalFuelGlobal);
    StoreFuel->setMetadata("op_sig", OpSigMD);
    
    return 0; 
}

unsigned getMemoryIntrinsicCostX86(CallInst *Call, StringRef Name, IRBuilder<> &Builder, MDNode *OpSigMD) 
{
    if (Call->arg_size() >= 3) 
    {
        Value *SizeArg = Call->getArgOperand(2);
        
        if (auto *SizeConst = dyn_cast<ConstantInt>(SizeArg)) 
        {
            uint64_t Size = SizeConst->getZExtValue();
            
            if (Name.starts_with("llvm.memcpy") || Name.starts_with("llvm.memmove")) 
            {
                unsigned BaseCost = Name.starts_with("llvm.memmove") ? 3 : 2;
                
                if (Size <= 8)         return BaseCost + 1;  // Single MOV instruction
                if (Size <= 32)        return BaseCost + 3;  // Unrolled MOV instructions
                if (Size <= 128)       return BaseCost + 6;  // SSE/AVX vector operations
                if (Size <= 1024)      return BaseCost + 20; // Loop with good cache locality
                return BaseCost + 35;  // Library call overhead
            }
            
            if (Name.starts_with("llvm.memset")) 
            {
                if (Size <= 8)         return 2;  // Single store instruction
                if (Size <= 32)        return 4;  // Unrolled stores
                if (Size <= 128)       return 8;  // SSE/AVX broadcast stores
                if (Size <= 1024)      return 25; // Loop overhead
                return 45;            // Library optimized routine
            }
        }
        else 
        {
            return insertDynamicMemoryCostX86(Call, Name, SizeArg, Builder, OpSigMD);
        }
    }
    
    if (Name.starts_with("llvm.memcpy"))   return 6; 
    if (Name.starts_with("llvm.memmove"))  return 8;
    if (Name.starts_with("llvm.memset"))   return 5;
    
    return 0;
}

unsigned getFuelCostX86(Instruction &I)
{
    Type *Ty = I.getType();
    bool isFloat = Ty->isFloatingPointTy();
    bool isVector = Ty->isVectorTy();
    unsigned baseCost = 0;

    MDNode *OpSigMD = MDNode::get(I.getContext(), {});

    if (auto *Call = dyn_cast<CallInst>(&I))
    {
        if (Function *Callee = Call->getCalledFunction())
        {
            if (Callee->isIntrinsic())
            {
                StringRef Name = Callee->getName();       
                if (Name.starts_with("llvm.memcpy") || Name.starts_with("llvm.memmove") || Name.starts_with("llvm.memset"))
                {
                    IRBuilder<> Builder(Call); 
                    return getMemoryIntrinsicCostX86(Call, Name, Builder, OpSigMD);
                }

                if (unsigned cost = getIntrinsicCostX86(Callee->getName()))
                    return cost;

                if (unsigned cost = getGenericIntrinsicCostX86(Callee->getName()))
                    return cost;

                const char* outputUnhandledIntrinsicsStr = std::getenv("OUTPUT_UNHANDLED_INTRINSICS");
                int outputUnhandledIntrinsics = outputUnhandledIntrinsicsStr ? std::atoi(outputUnhandledIntrinsicsStr) : 0;
                if (outputUnhandledIntrinsics)
                    errs() << "Unhandled intrinsic: " << Callee->getName() << "\n";

                return 0;
            } 

            if (Callee->getName() == "__check_fuel")
                return 0;

            // Higher base cost for x86 function calls due to calling convention complexity
            unsigned Cost = 4;
            Cost += std::min(8u, Call->arg_size());
            
            return Cost;
        }
        else
        {
            // Indirect calls more expensive on x86
            unsigned Cost = 7;
            Cost += std::min(8u, Call->arg_size());
            
            return Cost;
        }
    }

    bool isFloatingOp = false;
    switch (I.getOpcode())
    {
        case Instruction::Add:
        case Instruction::Sub:
            baseCost = 1;  // Still single cycle on modern x86
            break;

        case Instruction::Mul:
            baseCost = 4;  // Higher than AArch64 due to complexity
            break;

        case Instruction::UDiv:
        case Instruction::SDiv:
            baseCost = 25; // x86 division is notoriously expensive
            break;

        case Instruction::URem:
        case Instruction::SRem:
            baseCost = 30; // Even worse than division
            break;

        case Instruction::FAdd:
        case Instruction::FSub:
            isFloatingOp = true;
            baseCost = 4;  // SSE adds some overhead
            break;

        case Instruction::FMul:
            isFloatingOp = true;
            baseCost = 6;  // x86 FP multiply
            break;

        case Instruction::FDiv:
            isFloatingOp = true;
            baseCost = 20; // x86 FP division is expensive
            break;

        case Instruction::FRem:
            isFloatingOp = true;
            baseCost = 25; // Even more expensive
            break;

        case Instruction::Load:
            baseCost = 3;  // Account for complex addressing modes
            if (auto *LI = dyn_cast<LoadInst>(&I))
            {
                if (LI->isVolatile())
                    baseCost *= 2;
                
                // Add penalty for complex addressing
                if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand()))
                {
                    if (GEP->getNumIndices() > 2)
                        baseCost += 1;
                }
            }
            break;

        case Instruction::Store:
            baseCost = 4;  // Stores slightly more expensive
            if (auto *SI = dyn_cast<StoreInst>(&I))
            {
                if (SI->isVolatile())
                    baseCost *= 2;
                
                // Add penalty for complex addressing
                if (auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand()))
                {
                    if (GEP->getNumIndices() > 2)
                        baseCost += 1;
                }
            }
            break;

        case Instruction::FNeg:
            isFloatingOp = true;
            baseCost = 1;
            break;

        case Instruction::Shl:
        case Instruction::LShr:
        case Instruction::AShr:
            baseCost = 1;  // x86 is actually good at shifts
            break;

        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor:
            baseCost = 1;  // Basic bitwise ops are fast
            break;

        case Instruction::ICmp:
            baseCost = 1;  // CMP instruction
            break;

        case Instruction::FCmp:
            isFloatingOp = true;
            baseCost = 3;  // FP compare with flags
            break;

        case Instruction::ExtractElement:
        case Instruction::InsertElement:
            baseCost = 4;  // SSE extract/insert can be expensive
            break;

        case Instruction::ShuffleVector:
            baseCost = 5;  // x86 shuffles vary widely in cost
            break;

        case Instruction::Select:
            baseCost = 2;  // CMOV or branch
            break;

        case Instruction::GetElementPtr:
            baseCost = 1;  // Usually folds into addressing mode
            // But add cost for complex computations
            if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
            {
                if (GEP->getNumIndices() > 2)
                    baseCost += 2;
            }
            break;

        default:
            return 0;
    }

    // x86 vector operations scaling
    if(!isFloatingOp)
    {
        if (isFloat && isVector)
            baseCost *= 3;  // SSE/AVX FP vectors
        else if (isVector)
        {
            // Scale based on vector width for x86
            if (auto *VT = dyn_cast<VectorType>(Ty))
            {
                unsigned elements = VT->getElementCount().getKnownMinValue();
                if (elements <= 4)
                    baseCost *= 2;      // SSE 128-bit
                else if (elements <= 8)
                    baseCost = (baseCost * 5) / 2;  // AVX 256-bit
                else
                    baseCost *= 3;      // AVX-512 or larger
            }
            else
            {
                baseCost *= 2;  // Default vector penalty
            }
        }
        else if (isFloat)
            baseCost += 1;  // Scalar FP slight penalty
    }
    else
    {
        if (isVector)
        {
            // x86 vector FP operations
            if (auto *VT = dyn_cast<VectorType>(Ty))
            {
                unsigned elements = VT->getElementCount().getKnownMinValue();
                if (elements <= 4)
                    baseCost = (baseCost * 3) / 2;      // SSE FP
                else if (elements <= 8)
                    baseCost = (baseCost * 7) / 3;      // AVX FP
                else
                    baseCost = (baseCost * 5) / 2;      // AVX-512 FP
            }
            else
            {
                baseCost = (baseCost * 3) / 2;  // Default vector FP penalty
            }
        }
    }

    return baseCost;
}