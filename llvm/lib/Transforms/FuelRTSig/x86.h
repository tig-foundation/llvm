unsigned getIntrinsicCostX86(StringRef Name)
{
    // Softened helpers
    auto getVectorWidthMultiplier = [](StringRef Name) -> unsigned {
        if (Name.contains("avx512")) return 3;  // Soft premium for 512-bit
        if (Name.contains("avx")) return 2;     // Same as SSE (uop splitting)
        return 2;                               // SSE
    };

    auto getDataTypeAdjustment = [](StringRef Name) -> unsigned {  // Additive, not mult
        if (Name.contains(".pd") || Name.ends_with("f64")) return 2;  // +2 for FP64
        if (Name.contains(".ps") || Name.ends_with("f32")) return 1;  // +1 for FP32
        if (Name.ends_with("i64") || Name.contains("q")) return 1;
        if (Name.ends_with("i32") || Name.contains("d")) return 1;
        if (Name.ends_with("i16") || Name.contains("w")) return 1;
        if (Name.ends_with("i8") || Name.contains("b")) return 1;
        return 1;  // Default +1
    };

    auto isElementSensitive = [](StringRef Name) -> bool {
        return Name.contains(".div") || Name.contains(".sqrt") || 
               Name.contains(".cvt") || Name.contains(".gather") || 
               Name.contains(".scatter") || Name.contains(".sad") ||
               Name.contains(".reduce") || Name.contains(".mul") ||
               Name.contains(".fma") || Name.contains(".fmsub") ||
               Name.contains(".fnmadd") || Name.contains(".fnmsub");
    };

    unsigned widthMult = getVectorWidthMultiplier(Name);
    unsigned typeAdj = getDataTypeAdjustment(Name);
    bool isHorizontal = Name.contains(".hadd") || Name.contains(".hsub") ||
                        Name.contains(".sad") || Name.contains(".reduce");

    // SSE intrinsics (adjusted)
    if (Name.starts_with("llvm.x86.sse")) {
        if (Name.contains(".add") || Name.contains(".sub")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".mul")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".div")) {
            unsigned base = 15;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".sqrt")) {
            unsigned base = 15;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }

        if (Name.contains(".cmp") || Name.contains(".comieq") || 
            Name.contains(".comige") || Name.contains(".comigt") ||
            Name.contains(".comile") || Name.contains(".comilt") ||
            Name.contains(".comineq")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1 * widthMult;  // Low cost for bitwise

        if (Name.contains(".shuf") || Name.contains(".pshufd") ||
            Name.contains(".pshufb") || Name.contains(".pshufw"))
            return 1 * widthMult;  // Low for shuffles
        if (Name.contains(".unpack"))
            return 1 * widthMult;
        if (Name.contains(".movmsk"))
            return 2;  // Matches AArch64 extract=2

        if (Name.contains(".cvt")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".cvtsi2ss") || Name.contains(".cvtsi2sd"))
            return 3;
        if (Name.contains(".cvttss2si") || Name.contains(".cvttsd2si"))
            return 3;

        if (Name.contains(".min") || Name.contains(".max")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".loadu") || Name.contains(".load"))
            return 3 + (widthMult / 2);  // Softened
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 4 + (widthMult / 2);
        if (Name.contains(".movnt"))
            return 1;  // Non-temporal store

        if (Name.contains("sse2")) {
            if (Name.contains(".padd") || Name.contains(".psub")) {
                unsigned base = 1;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
            if (Name.contains(".pmul")) {
                unsigned base = 2;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
            if (Name.contains(".pcmp")) {
                unsigned base = 1;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            }
            if (Name.contains(".ppack") || Name.contains(".punpck"))
                return 1 * widthMult;
            if (Name.contains(".psll") || Name.contains(".psrl") || 
                Name.contains(".psra"))
                return 1 * widthMult;  // Low for shifts
            if (Name.contains(".sad")) {
                unsigned base = 3;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
        }
    }

    if (Name.starts_with("llvm.x86.avx512")) {
        if (Name.contains(".add") || Name.contains(".sub")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".mul")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".div")) {
            unsigned base = 12;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".sqrt")) {
            unsigned base = 12;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }

        if (Name.contains(".fma") || Name.contains(".fmsub") ||
            Name.contains(".fnmadd") || Name.contains(".fnmsub")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }

        if (Name.contains(".cmp") || Name.contains(".pcmp")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1 * widthMult;

        if (Name.contains(".perm") || Name.contains(".shuf"))
            return 1 * widthMult;
        if (Name.contains(".insert") || Name.contains(".extract"))
            return 1;
        if (Name.contains(".broadcast"))
            return 1;
        if (Name.contains(".align"))
            return 1 * widthMult;

        if (Name.contains(".mask"))
            return 1;
        if (Name.contains(".cmp") && Name.contains(".mask")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".cvt")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".cvtps2ph") || Name.contains(".cvtph2ps")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".min") || Name.contains(".max")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".loadu") || Name.contains(".load"))
            return 3 + (widthMult / 2);
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 4 + (widthMult / 2);
        if (Name.contains(".gather") || Name.contains(".scatter"))
            return 8 * widthMult / 2;  // Moderate

        if (Name.contains(".conflict") || Name.contains(".lzcnt"))
            return 2 * widthMult;
        if (Name.contains(".reduce")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".range")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".fixupimm")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".getexp") || Name.contains(".getmant")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".scalef")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".padd") || Name.contains(".psub")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".pmul")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".pmadd")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".psll") || Name.contains(".psrl") || 
            Name.contains(".psra"))
            return 1 * widthMult;
        if (Name.contains(".pabs"))
            return 1 * widthMult;
        if (Name.contains(".pmin") || Name.contains(".pmax")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".sad")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".dbpsadbw")) {
            unsigned base = 4;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".maskz"))
            return 2;

        if (Name.contains(".vp2intersect")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".vpmadd52")) {
            unsigned base = 4;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
        if (Name.contains(".vpopcnt")) return 2 * widthMult;
        if (Name.contains(".vpclmulqdq")) {
            unsigned base = 5;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }
    }

    // AVX intrinsics
    if (Name.starts_with("llvm.x86.avx")) {
        if (Name.contains(".add") || Name.contains(".sub")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".mul")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".div")) {
            unsigned base = 10;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }
        if (Name.contains(".sqrt")) {
            unsigned base = 10;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }

        if (Name.contains(".fma") || Name.contains(".fmsub") ||
            Name.contains(".fnmadd") || Name.contains(".fnmsub")) {
            unsigned base = 3;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
        }

        if (Name.contains(".cmp")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".and") || Name.contains(".or") || 
            Name.contains(".xor") || Name.contains(".andnot"))
            return 1 * widthMult;

        if (Name.contains(".vperm") || Name.contains(".vinsert") ||
            Name.contains(".vextract"))
            return 1 * widthMult;
        if (Name.contains(".vpermil"))
            return 1 * widthMult;
        if (Name.contains(".vbroadcast"))
            return 1;
        if (Name.contains(".shuffle"))
            return 1 * widthMult;
        if (Name.contains(".blend"))
            return 1 * widthMult;

        if (Name.contains(".cvt")) {
            unsigned base = 2;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".min") || Name.contains(".max")) {
            unsigned base = 1;
            return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
        }

        if (Name.contains(".loadu") || Name.contains(".load"))
            return 3 + (widthMult / 2);
        if (Name.contains(".storeu") || Name.contains(".store"))
            return 4 + (widthMult / 2);
        if (Name.contains(".maskload") || Name.contains(".maskstore"))
            return 2 * widthMult;
        if (Name.contains(".gather")) {
            return 8 * widthMult / 2;  // Moderate for gathers
        }

        if (Name.contains("avx2")) {
            if (Name.contains(".padd") || Name.contains(".psub")) {
                unsigned base = 1;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
            if (Name.contains(".pmul")) {
                unsigned base = 2;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
            if (Name.contains(".pcmp")) {
                unsigned base = 1;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            }
            if (Name.contains(".ppack") || Name.contains(".punpck"))
                return 1 * widthMult;
            if (Name.contains(".psll") || Name.contains(".psrl") || 
                Name.contains(".psra"))
                return 1 * widthMult;
            if (Name.contains(".perm") || Name.contains(".shuf"))
                return 1 * widthMult;
            if (Name.contains(".gather"))
                return 8 * widthMult / 2;
            if (Name.contains(".hadd") || Name.contains(".hsub")) {
                unsigned base = 4;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
            if (Name.contains(".sad")) {
                unsigned base = 3;
                return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            }
        }
    }

    if (Name.starts_with("llvm.x86")) {
        if (Name.contains(".bmi") || Name.contains(".andn") ||
            Name.contains(".bextr") || Name.contains(".blsi") ||
            Name.contains(".blsmsk") || Name.contains(".blsr"))
            return 1;  // Harmonized to AArch64 bitops=1
        if (Name.contains(".bzhi") || Name.contains(".pdep") ||
            Name.contains(".pext"))
            return 1;  // Harmonized to AArch64 bitops=1
        if (Name.contains(".lzcnt") || Name.contains(".tzcnt"))
            return 1;  // Harmonized to AArch64 ctlz=1
        if (Name.contains(".popcnt"))
            return 3;  // Harmonized to AArch64 ctpop=3

        if (Name.contains(".pclmul"))
            return 5;

        if (Name.contains(".aes"))
            return 3;  // Matches AArch64 crypto=3

        if (Name.contains(".sha"))
            return 3;  // Matches AArch64 crypto=3

        if (Name.contains(".rdrand") || Name.contains(".rdseed"))
            return 8;

        if (Name.contains(".sfence") || Name.contains(".lfence") ||
            Name.contains(".mfence"))
            return 1;  // Harmonized low

        if (Name.contains(".clflushopt"))
            return 15;  // Keep; expensive cache op

        if (Name.contains(".xsave") || Name.contains(".xrstor"))
            return 50;  // Keep; no AArch64 equiv, very expensive

        if (Name.contains(".clflush") || Name.contains(".clwb"))
            return 20;  // Keep; cache invalidate
        if (Name.contains(".prefetch"))
            return 1;  // Matches AArch64 prefetch=1

        if (Name.contains(".pause"))
            return 8;
        if (Name.contains(".rdtsc"))
            return 15;  // Keep; cycle counter
    }

    if (Name.starts_with("llvm.x86.xsaveopt") || Name.starts_with("llvm.x86.xsavec")) return 40;
    if (Name.starts_with("llvm.x86.monitor") || Name.starts_with("llvm.x86.mwait")) return 10;
    if (Name.starts_with("llvm.x86.pcommit")) return 15;
    if (Name.starts_with("llvm.x86.vzeroall") || Name.starts_with("llvm.x86.vzeroupper")) return 2;
    if (Name.starts_with("llvm.x86.flags.read")) return 2;
    if (Name.starts_with("llvm.x86.avx512.vcompress") || Name.starts_with("llvm.x86.avx512.vexpand")) {
        return 3 * widthMult;  // Harmonized to AArch64 permute/trn=3 equiv
    }

    if (Name.starts_with("llvm.x86.mmx")) {
        if (Name.contains(".add") || Name.contains(".sub")) return 2;  // Legacy; harmonized to AArch64 add=1 but penalize
        if (Name.contains(".mul")) return 4;  // Harmonized to AArch64 mul=3 but penalize legacy
        if (Name.contains(".psll") || Name.contains(".psrl")) return 2;  // Harmonized to AArch64 shift=2
        return 3;  // Default for MMX (legacy, penalize slightly)
    }

    if (Name.contains(".vpdpbusd") || Name.contains(".vpdpbusds") ||
        Name.contains(".vpdpwssd") || Name.contains(".vpdpwssds")) {
        unsigned base = 4;
        return base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);  // Harmonized to AArch64 sdot/udot=4
    }

    if (Name.starts_with("llvm.x86.xgetbv") || Name.starts_with("llvm.x86.xsetbv"))
        return 10;
    if (Name.starts_with("llvm.x86.amx"))
        return 6;

    return 0;
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
                return 5;  // Matches AArch64's 5
            if (Name.contains(".v8"))
                return 7; // Matches AArch64's 7  
            if (Name.contains(".v16"))
                return 9; // Matches AArch64's 9
        }
        if (Name.contains(".fadd") || Name.contains(".fmul"))
        {
            if (Name.contains(".v4"))
                return 9; // Matches AArch64's 9
            if (Name.contains(".v8"))
                return 11; // Matches AArch64's 11
            if (Name.contains(".v16"))
                return 13; // Matches AArch64's 13
        }
    }

    // Vector reductions - the main area where x86 is genuinely less efficient
    if (Name.starts_with("llvm.vector.reduce.smax") || 
        Name.starts_with("llvm.vector.reduce.smin") || 
        Name.starts_with("llvm.vector.reduce.umax") || 
        Name.starts_with("llvm.vector.reduce.umin"))
    {
        if (Name.contains(".v2"))
            return 4;  // Matches AArch64's 4
        if (Name.contains(".v4"))
            return 6;  // Matches AArch64's 6
        if (Name.contains(".v8"))
            return 9; // Slightly higher than AArch64's 8
        return 8;      // Slightly higher than AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.or") || 
        Name.starts_with("llvm.vector.reduce.xor") ||
        Name.starts_with("llvm.vector.reduce.and") ||
        Name.starts_with("llvm.vector.reduce.not"))
    {
        if (Name.contains(".v2"))
            return 4;  // Matches AArch64's 4
        if (Name.contains(".v4"))
            return 6;  // Matches AArch64's 6
        if (Name.contains(".v8"))
            return 9; // Slightly higher than AArch64's 8
        return 8;      // Slightly higher than AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.fmax") || 
        Name.starts_with("llvm.vector.reduce.fmin"))
    {
        if (Name.contains(".v2"))
            return 6;  // Matches AArch64's 6
        if (Name.contains(".v4"))
            return 8; // Matches AArch64's 8
        if (Name.contains(".v8"))
            return 11; // Slightly higher than AArch64's 10
        return 10;     // Matches AArch64's 10
    }

    if (Name.starts_with("llvm.vector.reduce.add"))
    {
        if (Name.contains(".v2"))
            return 3;  // Matches AArch64's 3
        if (Name.contains(".v4"))
            return 6;  // Matches AArch64's 6
        if (Name.contains(".v8"))
            return 9; // Matches AArch64's 9
        return 8;      // Slightly higher than AArch64's 7
    }

    if (Name.starts_with("llvm.vector.reduce.mul"))
    {
        if (Name.contains(".v2"))
            return 8;  // Matches AArch64's 8
        if (Name.contains(".v4"))
            return 12; // Matches AArch64's 12
        if (Name.contains(".v8"))
            return 16; // Matches AArch64's 16
        return 15;     // Slightly higher than AArch64's 14
    }

    if (Name.starts_with("llvm.vector.reduce.sub"))
    {
        if (Name.contains(".v2"))
            return 3;  // Matches AArch64's 3
        if (Name.contains(".v4"))
            return 5;  // Matches AArch64's 5
        if (Name.contains(".v8"))
            return 7;  // Matches AArch64's 7
        return 9;      // Matches AArch64's 9
    }

    if (Name.starts_with("llvm.masked.load"))
    {
        // x86 masked loads are more expensive, but not drastically
        if (Name.contains(".v4"))
            return 5;  // Matches AArch64's 5
        if (Name.contains(".v8"))
            return 6;  // Matches AArch64's 6
        if (Name.contains(".v16"))
            return 9; // Slightly higher than AArch64's 8 - AVX-512 penalty
    }

    if (Name.starts_with("llvm.masked.store"))
    {
        // x86 masked stores are more expensive
        if (Name.contains(".v4"))
            return 3;  // Matches AArch64's 3
        if (Name.contains(".v8"))
            return 4;  // Matches AArch64's 4
        if (Name.contains(".v16"))
            return 7;  // Slightly higher than AArch64's 6
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
        return 1; // Matches AArch64 (BSR/BSF efficient)

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
        return 65; // Same as AArch64

    if(Name.starts_with("llvm.atan"))
        return 50; // Same as AArch64

    if (Name.starts_with("llvm.cosh") || 
        Name.starts_with("llvm.sinh") || 
        Name.starts_with("llvm.tanh"))
        return 40; // Same as AArch64

    if (Name.starts_with("llvm.tan"))
        return 45; // Matches AArch64

    if(Name.starts_with("llvm.sincospi"))
        return 75; // Same as AArch64

    if(Name.starts_with("llvm.sincos"))
        return 60; // Same as AArch64

    if (Name.starts_with("llvm.asin") || Name.starts_with("llvm.acos"))
        return 50; // Same as AArch64

    if (Name.starts_with("llvm.modf"))
    {
        if (Name.contains(".f64"))
            return 9; // Same as AArch64
        if (Name.contains(".f32"))
            return 5;  // Same as AArch64
    }

    if (Name.starts_with("llvm.sin") || Name.starts_with("llvm.cos"))
        return 35; // Same as AArch64

    if (Name.starts_with("llvm.pow"))
        return 40; // Same as AArch64

    if (Name.starts_with("llvm.log2"))
        return 23; // Matches AArch64
    if (Name.starts_with("llvm.log10"))
        return 25; // Matches AArch64
    if (Name.starts_with("llvm.log"))
        return 24; // Matches AArch64

    if (Name.starts_with("llvm.masked.gather") || Name.starts_with("llvm.masked.scatter"))
        return 10; // Matches AArch64's 10

    if (Name.starts_with("llvm.matrix.transpose"))
        return 6; // Matches AArch64

    if (Name.starts_with("llvm.matrix.multiply"))
    {
        if (Name.contains(".v2"))
            return 15; // Matches AArch64
        if (Name.contains(".v4"))
            return 25; // Matches AArch64
        if (Name.contains(".v8"))
            return 40; // Matches AArch64
        return 20;     // Matches AArch64
    }

    if (Name.starts_with("llvm.matrix"))
        return 8; // Matches AArch64

    if (Name.starts_with("llvm.minnum") || Name.starts_with("llvm.maxnum"))
    {
        if (Name.contains(".v2"))
            return 6; // Matches AArch64
        if (Name.contains(".v4"))
            return 8; // Matches AArch64
        if (Name.contains(".v8"))
            return 10; // Matches AArch64
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }   

    if(Name.starts_with("llvm.lround") || Name.starts_with("llvm.llround"))
    {
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }

    if (Name.starts_with("llvm.minimumnum") || Name.starts_with("llvm.maximumnum"))
    {
        if (Name.contains(".v2"))
            return 6; // Matches AArch64
        if (Name.contains(".v4"))
            return 8; // Matches AArch64
        if (Name.contains(".v8"))
            return 10; // Matches AArch64
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }   
    
    if (Name.starts_with("llvm.minimum") || Name.starts_with("llvm.maximum"))
    {
        if (Name.contains(".v2"))
            return 6; // Matches AArch64
        if (Name.contains(".v4"))
            return 8; // Matches AArch64
        if (Name.contains(".v8"))
            return 10; // Matches AArch64
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }

    if(Name.starts_with("llvm.rint") || Name.starts_with("llvm.nearbyint"))
    {
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }

    if(Name.starts_with("llvm.lrint") || Name.starts_with("llvm.llrint"))
    {
        if (Name.contains(".f32"))
            return 3; // Matches AArch64
        if (Name.contains(".f64"))
            return 5; // Matches AArch64
        return 2;     // Matches AArch64
    }

    if(Name.starts_with("llvm.experimental.constrained."))
    {
        if (Name.contains(".fadd") || Name.contains(".fsub"))
            return 5; // Matches AArch64

        if (Name.contains(".fmul"))
            return 7; // Matches AArch64

        if (Name.contains(".fdiv") || Name.contains(".frem"))
            return 17; // Matches AArch64

        if (Name.contains(".fma") || Name.contains(".fmuladd"))
            return 8; // Matches AArch64

        if (Name.contains(".fcmp") || Name.contains(".fcmps"))
            return 3; // Matches AArch64

        if (Name.contains(".sqrt"))
            return 15; // Matches AArch64

        if (Name.contains(".pow"))
            return 40; // Matches AArch64

        if (Name.contains(".powi"))
            return 25; // Matches AArch64

        if (Name.contains(".asin") || Name.contains(".acos"))
            return 50; // Matches AArch64

        if (Name.contains(".atan2"))
            return 65; // Matches AArch64

        if (Name.contains(".atan"))
            return 50; // Matches AArch64

        if (Name.contains(".sinh") || Name.contains(".cosh") || Name.contains(".tanh"))
            return 40; // Matches AArch64

        if (Name.contains(".tan"))
            return 45;  // Matches AArch64

        if (Name.contains(".sin") || Name.contains(".cos"))
            return 35; // Matches AArch64

        if (Name.contains(".exp") && !Name.contains(".exp2"))
            return 35; // Matches AArch64

        if (Name.contains(".exp2"))
            return 25; // Matches AArch64

        if (Name.contains(".log2"))
            return 23; // Matches AArch64

        if (Name.contains(".log10"))
            return 25; // Matches AArch64

        if (Name.contains(".log") && !Name.contains(".log10") && !Name.contains(".log2"))
            return 24; // Matches AArch64

        if (Name.contains(".rint") || Name.contains(".nearbyint"))
        {
            if (Name.contains(".f32"))
                return 3; // Matches AArch64
            if (Name.contains(".f64"))
                return 5; // Matches AArch64
            return 2;     // Matches AArch64
        }

        if (Name.contains(".lrint") || Name.contains(".llrint"))
        {
            if (Name.contains(".f32"))
                return 3; // Matches AArch64
            if (Name.contains(".f64"))
                return 5; // Matches AArch64
            return 2;     // Matches AArch64
        }

        if (Name.contains(".maxnum") || Name.contains(".minnum") ||
            Name.contains(".maximum") || Name.contains(".minimum"))
        {
            if (Name.contains(".v2"))
                return 6; // Matches AArch64
            if (Name.contains(".v4"))
                return 8; // Matches AArch64
            if (Name.contains(".v8"))
                return 10; // Matches AArch64
            if (Name.contains(".f32"))
                return 3; // Matches AArch64
            if (Name.contains(".f64"))
                return 5; // Matches AArch64
            return 2;     // Matches AArch64
        }

        if (Name.contains(".ceil") || Name.contains(".floor"))
            return 5; // Matches AArch64

        if (Name.contains(".round") || Name.contains(".roundeven"))
            return 6; // Matches AArch64

        if (Name.contains(".lround") || Name.contains(".llround"))
        {
            if (Name.contains(".f32"))
                return 3; // Matches AArch64
            if (Name.contains(".f64"))
                return 5; // Matches AArch64
            return 2;     // Matches AArch64
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
    {
        if (Name.contains(".acquire") || Name.contains(".release"))
            return 10;
        if (Name.contains(".acq_rel") || Name.contains(".seq_cst"))
            return 12;
        return 8;
    }

    if (Name.starts_with("llvm.cmpxchg"))
        return 6;

    if (Name.starts_with("llvm.trunc"))
        return 3;

    if (Name.starts_with("llvm.copysign"))
        return 2;

    if (Name.starts_with("llvm.ldexp"))
        return 4;  // Matches AArch64 (reduced from 5)

    if (Name.starts_with("llvm.frexp"))
        return 6;  // Matches AArch64 (reduced from 7)

    if (Name.starts_with("llvm.stacksave"))
        return 2;
    if (Name.starts_with("llvm.stackrestore"))
        return 3;

    if (Name.starts_with("llvm.va_start") || 
        Name.starts_with("llvm.va_end") || 
        Name.starts_with("llvm.va_copy"))
        return 1;

    if (Name.starts_with("llvm.bitreverse"))
        return 2;

    if (Name.starts_with("llvm.vector.insert") || Name.starts_with("llvm.vector.extract")) {
        unsigned cost = 3;
        if (Name.contains(".v2")) cost = 4;
        if (Name.contains(".v4")) cost = 6;
        if (Name.contains(".v8")) cost = 8;
        if (Name.contains(".v16")) cost = 10;
        return cost;
    }
        
    if (Name.starts_with("llvm.canonicalize")) {
        return 3;  // FP normalization
    }
    if (Name.starts_with("llvm.fmuladd")) {  // Non-constrained version
        if (Name.contains(".v2")) return 8;
        if (Name.contains(".v4")) return 12;
        if (Name.contains(".v8")) return 16;
        return 7;  // Scalar or default
    }

    if (Name.starts_with("llvm.cache.flush") || Name.starts_with("llvm.cache.invalidate")) {
        return 15;  // Expensive cache operations
    }

    if (Name.starts_with("llvm.stackprobe")) {
        return 5;  // Windows-style stack probing
    }

    if (Name.starts_with("llvm.experimental.vector.reduce")) {
        StringRef Suffix = Name.substr(17); // Skip "llvm.experimental."
        std::string NewName = "llvm." + Suffix.str();
        return getGenericIntrinsicCostX86(NewName);
    }

    if (Name.starts_with("llvm.clear_cache")) return 15;

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

    // Harmonized to match AArch64 costs for fairness
    Value *Cost1 = ConstantInt::get(I64Ty, BaseCost + 1);   // <= 8
    Value *Cost2 = ConstantInt::get(I64Ty, BaseCost + 2);   // <= 32  
    Value *Cost4 = ConstantInt::get(I64Ty, BaseCost + 4);   // <= 128
    Value *Cost18 = ConstantInt::get(I64Ty, BaseCost + 18); // <= 1024
    Value *Cost30 = ConstantInt::get(I64Ty, BaseCost + 28); // > 1024

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

    Value *Sel4 = Builder.CreateSelect(Cmp1024, Cost18, Cost30);
    if (auto *Inst = dyn_cast<Instruction>(Sel4))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel3 = Builder.CreateSelect(Cmp128, Cost4, Sel4);
    if (auto *Inst = dyn_cast<Instruction>(Sel3))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel2 = Builder.CreateSelect(Cmp32, Cost2, Sel3);
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

    Value *Cost2 = ConstantInt::get(I64Ty, 2);   // <= 8
    Value *Cost3 = ConstantInt::get(I64Ty, 3);   // <= 32
    Value *Cost6 = ConstantInt::get(I64Ty, 6);   // <= 128  
    Value *Cost22 = ConstantInt::get(I64Ty, 22); // <= 1024
    Value *Cost40 = ConstantInt::get(I64Ty, 40); // > 1024

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

    Value *Sel4 = Builder.CreateSelect(Cmp1024, Cost22, Cost40);
    if (auto *Inst = dyn_cast<Instruction>(Sel4))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel3 = Builder.CreateSelect(Cmp128, Cost6, Sel4);
    if (auto *Inst = dyn_cast<Instruction>(Sel3))
        Inst->setMetadata("op_sig", OpSigMD);

    Value *Sel2 = Builder.CreateSelect(Cmp32, Cost3, Sel3);
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
    } else if (Name.starts_with("llvm.memcmp")) {
        StaticBaseCost = 1;
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

                if (Size <= 8)         return BaseCost + 1;
                if (Size <= 32)        return BaseCost + 3;
                if (Size <= 128)       return BaseCost + 6; 
                if (Size <= 1024)      return BaseCost + 18;
                return BaseCost + 28;
            }

            if (Name.starts_with("llvm.memset")) 
            {
                if (Size <= 8)         return 2;
                if (Size <= 32)        return 4;
                if (Size <= 128)       return 8;
                if (Size <= 1024)      return 22;
                return 40;
            }

            if (Name.starts_with("llvm.memcmp")) {
                if (Size <= 8)         return 2;
                if (Size <= 32)        return 4;
                if (Size <= 128)       return 8;
                if (Size <= 1024)      return 25;
                return 40;
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
            if (Callee->getName() == "__check_fuel" || Callee->getName() == "__commit_tls" || Callee->getName() == "__memory_check")
                return 0;

            if (Callee->isIntrinsic())
            {
                StringRef Name = Callee->getName();       
                if (Name.starts_with("llvm.memcpy") || Name.starts_with("llvm.memmove") || Name.starts_with("llvm.memset") || Name.starts_with("llvm.memcmp"))
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

            // Higher base cost for x86 function calls due to calling convention complexity
            unsigned Cost = 3;
            //Cost += std::min(8u, Call->arg_size());

            return Cost;
        }
        else
        {
            // Indirect calls more expensive on x86
            unsigned Cost = 5;
            //Cost += std::min(8u, Call->arg_size());

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
            baseCost = 3;  // Higher than AArch64 due to complexity
            break;

        case Instruction::UDiv:
        case Instruction::SDiv:
            baseCost = 12; // x86 division is notoriously expensive
            break;

        case Instruction::URem:
        case Instruction::SRem:
            baseCost = 15; // Even worse than division
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
            baseCost = 17; // x86 FP division is expensive
            break;

        case Instruction::FRem:
            isFloatingOp = true;
            baseCost = 20; // Even more expensive
            break;

        case Instruction::Load:
            baseCost = 2;  // Modern x86 L1=~3-4 cycles, but agile addressing
            if (auto *LI = dyn_cast<LoadInst>(&I))
            {
                if (LI->isVolatile())
                    baseCost += 1;

                // Add penalty for complex addressing
                if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand()))
                {
                    if (GEP->getNumIndices() > 2)
                        baseCost += 1;
                }
            }
            break;

        case Instruction::Store:
            baseCost = 2;  // Stores slightly more expensive, adjusted
            if (auto *SI = dyn_cast<StoreInst>(&I))
            {
                if (SI->isVolatile())
                    baseCost += 1;

                // Add penalty for complex addressing
                if (auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand()))
                {
                    if (GEP->getNumIndices() > 2)
                        baseCost += 1;
                }
            }
            break;

        case Instruction::FNeg:
            //isFloatingOp = true;
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
            baseCost = 3;  // SSE extract/insert can be expensive
            break;

        case Instruction::ShuffleVector:
            baseCost = 3;  // x86 shuffles vary widely in cost
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
                    baseCost += 1;
            }
            break;

        case Instruction::Br:
            baseCost = 1;
            break;

        case Instruction::IndirectBr:
            baseCost = 3;  // Penalize potential branch mispredicts more for DoS
            break;

        case Instruction::Alloca:
            baseCost = 3;  // Adjusted for x86 complex frame pointers/SP
            break;

        case Instruction::AtomicRMW:
        case Instruction::AtomicCmpXchg:
            baseCost = 12;  // Increased base for atomic ops; scale with ordering (acquire/release +2, seq_cst +4)
            if (auto *AI = dyn_cast<AtomicRMWInst>(&I)) {  // Or AtomicCmpXchgInst
                switch (AI->getOrdering()) {
                    case AtomicOrdering::Acquire:
                    case AtomicOrdering::Release:
                        baseCost += 2;
                        break;
                    case AtomicOrdering::AcquireRelease:
                    case AtomicOrdering::SequentiallyConsistent:
                        baseCost += 4;
                        break;
                    default:
                        break;
                }
            }
            break;

        default:
            return 0;
    }

    // x86 vector operations scaling
    if (!isFloatingOp) {
        if (isFloat && isVector) baseCost *= 3;  // Same as ARM
        else if (isVector) {
            if (auto *VT = dyn_cast<VectorType>(Ty)) {
                unsigned elements = VT->getElementCount().getKnownMinValue();
                if (elements <= 4) baseCost *= 2;
                else if (elements <= 8) baseCost = (baseCost * 5) / 2;
                else baseCost = std::min(baseCost * 5, baseCost * 3);  // Cap at x5 (>8 elements, adjusted for AVX512 thermal)
            } else {
                baseCost *= 2;
            }
        } else if (isFloat) {
            baseCost += 1;  // Same
        }
    } else {
        if (isVector) {
            if (auto *VT = dyn_cast<VectorType>(Ty)) {
                unsigned elements = VT->getElementCount().getKnownMinValue();
                baseCost = (baseCost * std::min(4u, (elements + 3) / 4) * 5) / 2;  // Scale like ARM (2.5x base), cap at x4
            } else {
                baseCost = (baseCost * 5) / 2;
            }
        }
    }

    return baseCost;
}