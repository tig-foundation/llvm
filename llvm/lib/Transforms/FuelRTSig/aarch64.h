unsigned getIntrinsicCostAArch64(StringRef Name)
{
    // Softened helpers for consistency with X86
    auto getVectorWidthMultiplier = [](StringRef Name) -> unsigned {
        if (Name.contains(".v16")) return 4;  // Wide NEON/SVE equivalent
        if (Name.contains(".v8")) return 3;
        if (Name.contains(".v4")) return 2;
        if (Name.contains(".v2")) return 1;   // Narrow
        if (Name.starts_with("llvm.aarch64.sve")) return 4;  // SVE scalable premium
        return 2;  // Default NEON
    };

    auto getDataTypeAdjustment = [](StringRef Name) -> unsigned {  // Additive
        if (Name.ends_with("f64") || Name.ends_with("i64")) return 2;  // +2 for 64-bit
        if (Name.ends_with("f32") || Name.ends_with("i32")) return 1;
        if (Name.ends_with("f16") || Name.ends_with("i16")) return 1;
        if (Name.ends_with("i8")) return 1;
        return 1;  // Default +1
    };

    auto isElementSensitive = [](StringRef Name) -> bool {
        return Name.contains(".div") || Name.contains(".sqrt") || 
               Name.contains(".cvt") || Name.contains(".fcvt") || 
               Name.contains(".mul") || Name.contains(".fma") || 
               Name.contains(".fmla") || Name.contains(".fmls") ||
               Name.contains(".vqdmulh") || Name.contains(".sqrdmulh") ||
               Name.contains(".reduce");
    };

    unsigned widthMult = getVectorWidthMultiplier(Name);
    unsigned typeAdj = getDataTypeAdjustment(Name);
    bool isHorizontal = Name.contains(".addp") || Name.contains(".saddlp") ||
                        Name.contains(".uaddlp") || Name.contains(".vmaxv") ||
                        Name.contains(".vminv") || Name.contains(".faddv") ||
                        Name.contains(".fmaxv") || Name.contains(".fminv");

    if (Name.starts_with("llvm.arm.neon") || Name.starts_with("llvm.aarch64.neon"))
    {
        if (Name.contains(".v2"))
        {
            if (Name.ends_with("f64") || Name.ends_with("i64"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".fmla") || Name.contains(".fmls"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".frecpe") || Name.contains(".frint"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".vrecpe") || Name.contains(".vrsqrte"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vrecps") || Name.contains(".vrsqrts"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vrecpx"))
                    return 5 * widthMult + typeAdj;
            }
        }
        else if (Name.contains(".v4"))
        {
            if (Name.ends_with("f32"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".fmla") || Name.contains(".fmls"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".frecpe") || Name.contains(".frint"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".vrecpe") || Name.contains(".vrsqrte"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vrecps") || Name.contains(".vrsqrts"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vrecpx"))
                    return 5 * widthMult + typeAdj;
            }
            else if (Name.ends_with("i32"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vqdmull") || Name.contains(".vqdmlal") || 
                    Name.contains(".vqdmlsl"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".vqdmulh") || Name.contains(".sqrdmulh"))
                    return 4 * widthMult + typeAdj;
            }
        }
        else if (Name.contains(".v8"))
        {
            if (Name.ends_with("i16") || Name.ends_with("f16"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vqdmull") || Name.contains(".vqdmlal") || 
                    Name.contains(".vqdmlsl"))
                    return 4 * widthMult + typeAdj;
                if (Name.contains(".vqdmulh") || Name.contains(".sqrdmulh"))
                    return 4 * widthMult + typeAdj;
            }
            else if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
            }
        }
        else if (Name.contains(".v16"))
        {
            if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
            }
        }
        else if (Name.contains(".v32"))
        {
            if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vceq"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3 * widthMult + typeAdj;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
            }
        }

        if (Name.contains(".addp") || Name.contains(".saddlp") || 
            Name.contains(".uaddlp"))
        {
            unsigned base = 2;
            unsigned cost = base * widthMult + typeAdj + (isHorizontal ? widthMult : 0);
            if (Name.contains(".v4")) return cost;
            if (Name.contains(".v8")) return cost;
            if (Name.contains(".v16")) return cost;
            return cost;
        }

        if (Name.contains(".pmul") || Name.contains(".smull") || 
            Name.contains(".umull"))
        {
            unsigned base = 4;
            unsigned cost = base * widthMult + typeAdj;
            if (Name.contains(".v4")) return cost;
            if (Name.contains(".v8")) return cost;
            return cost;
        }

        if (Name.contains(".vabs") || Name.contains(".vneg"))
            return 1 * widthMult + typeAdj;
        if (Name.contains(".vqabs") || Name.contains(".vqneg"))
            return 2 * widthMult + typeAdj;
        if (Name.contains(".vsqadd") || Name.contains(".vuqadd") || 
            Name.contains(".vqsub") || Name.contains(".vqadd"))
            return 3 * widthMult + typeAdj;
        if (Name.contains(".fcvt"))
            return 3 * widthMult + typeAdj;
        if (Name.contains(".rshrn") || Name.contains(".sqshrn") || 
            Name.contains(".uqshrn"))
            return 3 * widthMult + typeAdj;
        if (Name.contains(".sqshlu") || Name.contains(".sqrshrun"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".vext"))
            return 3 * widthMult + typeAdj;
        if (Name.contains(".vrev"))
            return 1 * widthMult + typeAdj;
        if (Name.contains(".vzip") || Name.contains(".vuzp"))
            return 3 * widthMult + typeAdj;
        if (Name.contains(".vtrn"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".vld1"))
        {
            if (Name.contains(".lane"))
                return 8 + (widthMult / 2);
            return 4 + (widthMult / 2);
        }
        if (Name.contains(".vst1"))
        {
            if (Name.contains(".lane"))
                return 3 + (widthMult / 2);
            return 1 + (widthMult / 2);
        }
        if (Name.contains(".vld2"))
        {
            if (Name.contains(".lane"))
                return 8 + (widthMult / 2);
            return 8 + (widthMult / 2);
        }
        if (Name.contains(".vst2"))
        {
            if (Name.contains(".lane"))
                return 3 + (widthMult / 2);
            return 3 + (widthMult / 2);
        }
        if (Name.contains(".vld3"))
        {
            if (Name.contains(".lane"))
                return 8 + (widthMult / 2);
            return 9 + (widthMult / 2);
        }
        if (Name.contains(".vst3"))
        {
            if (Name.contains(".lane"))
                return 3 + (widthMult / 2);
            return 3 + (widthMult / 2);
        }
        if (Name.contains(".vld4"))
        {
            if (Name.contains(".lane"))
                return 8 + (widthMult / 2);
            return 9 + (widthMult / 2);
        }
        if (Name.contains(".vst4"))
        {
            if (Name.contains(".lane"))
                return 3 + (widthMult / 2);
            return 4 + (widthMult / 2);
        }

        if (Name.contains(".vshl") || Name.contains(".vshr"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".vcvt"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".vset_lane"))
            return 2 * widthMult + typeAdj;
        if (Name.contains(".vget_lane"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".vadd_acc") || Name.contains(".vmla_acc"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".sdot") || Name.contains(".udot"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".vmul"))
        {
            if (Name.ends_with("f64") || Name.ends_with("f32"))
                return 4 * widthMult + typeAdj;
            return 3 * widthMult + typeAdj;
        }

        if (Name.contains(".vdiv"))
        {
            if (Name.ends_with("f64"))
                return 17 * widthMult + typeAdj;
            if (Name.ends_with("f32"))
                return 12 * widthMult + typeAdj;
        }

        if (Name.contains(".vdup_lane"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".vtbl2") || Name.contains(".vtbx2"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".vpadd"))
            return 2 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

        if (Name.contains(".vpmax") || Name.contains(".vpmin"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".vtbl") || Name.contains(".vtbx"))
        {
            if (Name.contains(".v8") || Name.contains(".v16"))
                return 6 * widthMult + typeAdj;
            return 3 * widthMult + typeAdj;
        }
    }

    if (Name.starts_with("llvm.arm.neon.vmins") || 
        Name.starts_with("llvm.arm.neon.vmaxs"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.arm.neon.vminu") || 
        Name.starts_with("llvm.arm.neon.vmaxu"))
        return 2 * widthMult + typeAdj;

    if (Name.starts_with("llvm.arm.neon.vmaxv") || Name.starts_with("llvm.arm.neon.vminv"))
        return 3 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

    if (Name.starts_with("llvm.arm.neon.cmeq") || Name.starts_with("llvm.arm.neon.cmge") ||
        Name.starts_with("llvm.arm.neon.cmgt") || Name.starts_with("llvm.arm.neon.cmle") ||
        Name.starts_with("llvm.arm.neon.cmlt"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.umaxp"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.fmla") || 
        Name.starts_with("llvm.aarch64.neon.fmls"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vcge") || 
        Name.starts_with("llvm.aarch64.neon.vcgt"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vceq"))
        return 2 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vcage") ||
        Name.starts_with("llvm.aarch64.neon.vcagt"))
        return 2 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.frint") || Name.starts_with("llvm.aarch64.neon.frecpe"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.arm.neon.vrecpe") ||
        Name.starts_with("llvm.arm.neon.vrsqrte"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.arm.neon.vrecps") ||
        Name.starts_with("llvm.arm.neon.vrsqrts"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vabs") ||
        Name.starts_with("llvm.aarch64.neon.vneg"))
        return 1 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vrecpx"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vqabs") ||
        Name.starts_with("llvm.aarch64.neon.vqneg"))
        return 2 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vqdmull") ||
        Name.starts_with("llvm.aarch64.neon.vqdmlal") ||
        Name.starts_with("llvm.aarch64.neon.vqdmlsl"))
        return 5 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vqdmulh"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.vsqadd") ||
        Name.starts_with("llvm.aarch64.neon.vuqadd") ||
        Name.starts_with("llvm.arm.neon.vqsub") ||
        Name.starts_with("llvm.arm.neon.vqadd"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.addp") ||
        Name.starts_with("llvm.aarch64.neon.saddlp") ||
        Name.starts_with("llvm.aarch64.neon.uaddlp"))
        return 2 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

    if (Name.starts_with("llvm.aarch64.neon.pmul") ||
        Name.starts_with("llvm.aarch64.neon.smull") ||
        Name.starts_with("llvm.aarch64.neon.umull"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.sqdmull") ||
        Name.starts_with("llvm.aarch64.neon.sqrdmulh"))
        return 5 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.fcvt"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.rshrn") ||
        Name.starts_with("llvm.aarch64.neon.sqshrn") ||
        Name.starts_with("llvm.aarch64.neon.uqshrn"))
        return 3 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.neon.sqshlu") ||
        Name.starts_with("llvm.aarch64.neon.sqrshrun"))
        return 4 * widthMult + typeAdj;

    if (Name.starts_with("llvm.aarch64.tstart"))
        return 10;
    if (Name.starts_with("llvm.aarch64.tcommit"))
        return 8;

    if (Name.starts_with("llvm.aarch64.crypto.aes"))
        return 3;

    // SVE/SVE2 intrinsics (Scalable Vector Extension)
    if (Name.starts_with("llvm.aarch64.sve"))
    {
        // Basic arithmetic operations
        if (Name.contains(".add") || Name.contains(".sub"))
            return 1 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

        if (Name.contains(".mul"))
        {
            unsigned base = Name.contains(".i64") ? 4 : 3;
            return base * widthMult + typeAdj;
        }

        if (Name.contains(".div"))
        {
            unsigned base = Name.contains(".f64") ? 20 : (Name.contains(".f32") ? 15 : 25);
            return base * widthMult + typeAdj;
        }

        // Multiply-add operations
        if (Name.contains(".mla") || Name.contains(".mls") || 
            Name.contains(".mad") || Name.contains(".msb"))
            return 4 * widthMult + typeAdj;

        // Floating-point multiply-add
        if (Name.contains(".fmla") || Name.contains(".fmls") ||
            Name.contains(".fmad") || Name.contains(".fmsb") ||
            Name.contains(".fnmla") || Name.contains(".fnmls"))
            return 5 * widthMult + typeAdj;

        // Comparisons
        if (Name.contains(".cmp") || Name.contains(".cmpeq") ||
            Name.contains(".cmpne") || Name.contains(".cmpge") ||
            Name.contains(".cmpgt") || Name.contains(".cmple") ||
            Name.contains(".cmplt"))
            return 2 * widthMult + typeAdj;

        // Floating-point comparisons
        if (Name.contains(".fcmp") || Name.contains(".facge") ||
            Name.contains(".facgt") || Name.contains(".fcmeq") ||
            Name.contains(".fcmge") || Name.contains(".fcmgt") ||
            Name.contains(".fcmle") || Name.contains(".fcmlt") ||
            Name.contains(".fcmne"))
            return 3 * widthMult + typeAdj;

        // Reductions
        if (Name.contains(".uaddv") || Name.contains(".saddv") ||
            Name.contains(".umaxv") || Name.contains(".smaxv") ||
            Name.contains(".uminv") || Name.contains(".sminv"))
            return 6 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

        if (Name.contains(".faddv") || Name.contains(".fmaxv") ||
            Name.contains(".fminv") || Name.contains(".fmaxnmv") ||
            Name.contains(".fminnmv"))
            return 8 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

        if (Name.contains(".fadda"))
            return 10 * widthMult + typeAdj + (isHorizontal ? widthMult : 0);

        // Bitwise operations
        if (Name.contains(".and") || Name.contains(".orr") || 
            Name.contains(".eor") || Name.contains(".bic") ||
            Name.contains(".orn"))
            return 1 * widthMult;

        // Shifts
        if (Name.contains(".lsl") || Name.contains(".lsr") || 
            Name.contains(".asr"))
            return 2 * widthMult + typeAdj;

        // Absolute value and negation
        if (Name.contains(".abs") || Name.contains(".neg"))
            return 1 * widthMult + typeAdj;

        if (Name.contains(".fabs") || Name.contains(".fneg"))
            return 1 * widthMult + typeAdj;

        // Min/max operations
        if (Name.contains(".smax") || Name.contains(".smin") ||
            Name.contains(".umax") || Name.contains(".umin"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".fmax") || Name.contains(".fmin") ||
            Name.contains(".fmaxnm") || Name.contains(".fminnm"))
            return 3 * widthMult + typeAdj;

        // Saturating arithmetic
        if (Name.contains(".qadd") || Name.contains(".qsub") ||
            Name.contains(".sqadd") || Name.contains(".sqsub") ||
            Name.contains(".uqadd") || Name.contains(".uqsub"))
            return 3 * widthMult + typeAdj;

        // Conversions
        if (Name.contains(".fcvt") || Name.contains(".scvtf") ||
            Name.contains(".ucvtf"))
            return 4 * widthMult + typeAdj;

        // Sign/zero extension
        if (Name.contains(".sxtb") || Name.contains(".sxth") ||
            Name.contains(".sxtw") || Name.contains(".uxtb") ||
            Name.contains(".uxth") || Name.contains(".uxtw"))
            return 2 * widthMult + typeAdj;

        // Predicate operations
        if (Name.contains(".ptrue"))
            return 1;
        if (Name.contains(".pfalse"))
            return 0;  // Can be optimized away

        if (Name.contains(".punpklo") || Name.contains(".punpkhi"))
            return 2;

        if (Name.contains(".brka") || Name.contains(".brkb") ||
            Name.contains(".brkn") || Name.contains(".brkpa") ||
            Name.contains(".brkpb"))
            return 3;

        if (Name.contains(".pfirst") || Name.contains(".pnext"))
            return 2;

        if (Name.contains(".ptest"))
            return 2;

        if (Name.contains(".cntp"))
            return 3;

        // While loop operations
        if (Name.contains(".whilele") || Name.contains(".whilelo") ||
            Name.contains(".whilels") || Name.contains(".whilelt") ||
            Name.contains(".whilege") || Name.contains(".whilegt") ||
            Name.contains(".whilehs") || Name.contains(".whilehi"))
            return 3;

        if (Name.contains(".ld1rq") || Name.contains(".ld1rw"))
            return 4 + (widthMult / 2);

        // Memory operations
        if (Name.contains(".ld1") || Name.contains(".ldff1"))
        {
            if (Name.contains(".gather"))
                return 12 + (widthMult / 2);  // Gather loads are expensive
            return 4 + (widthMult / 2);
        }

        if (Name.contains(".st1"))
        {
            if (Name.contains(".scatter"))
                return 10 + (widthMult / 2);  // Scatter stores are expensive
            return 3 + (widthMult / 2);
        }

        // Non-temporal loads/stores
        if (Name.contains(".ldnt1") || Name.contains(".stnt1"))
            return 2 + (widthMult / 2);

        // Structure loads/stores (2/3/4 element)
        if (Name.contains(".ld2"))
            return 8 + (widthMult / 2);
        if (Name.contains(".ld3"))
            return 10 + (widthMult / 2);
        if (Name.contains(".ld4"))
            return 12 + (widthMult / 2);

        if (Name.contains(".st2"))
            return 6 + (widthMult / 2);
        if (Name.contains(".st3"))
            return 8 + (widthMult / 2);
        if (Name.contains(".st4"))
            return 10 + (widthMult / 2);

        // Contiguous first-faulting loads
        if (Name.contains(".ldff1"))
            return 5 + (widthMult / 2);

        // Permutations and shuffles
        if (Name.contains(".rev") || Name.contains(".revb") ||
            Name.contains(".revh") || Name.contains(".revw"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".trn1") || Name.contains(".trn2") ||
            Name.contains(".uzp1") || Name.contains(".uzp2") ||
            Name.contains(".zip1") || Name.contains(".zip2"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".tbl") || Name.contains(".tbx"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".compact"))
            return 5 * widthMult + typeAdj;

        if (Name.contains(".splice"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".lastb") || Name.contains(".lasta"))
            return 3 * widthMult + typeAdj;

        // Extract operations
        if (Name.contains(".ext"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".dupq") || Name.contains(".dup"))
            return 2 * widthMult + typeAdj;

        // Index generation
        if (Name.contains(".index"))
            return 1 * widthMult + typeAdj;

        // SVE2 specific operations
        if (Name.contains(".sqdmulh") || Name.contains(".sqrdmulh"))
            return 5 * widthMult + typeAdj;

        if (Name.contains(".sqdmlalb") || Name.contains(".sqdmlalt") ||
            Name.contains(".sqdmlslb") || Name.contains(".sqdmlslt"))
            return 6 * widthMult + typeAdj;

        if (Name.contains(".saddlb") || Name.contains(".saddlt") ||
            Name.contains(".uaddlb") || Name.contains(".uaddlt") ||
            Name.contains(".ssublb") || Name.contains(".ssublt") ||
            Name.contains(".usublb") || Name.contains(".usublt"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".sabdlb") || Name.contains(".sabdlt") ||
            Name.contains(".uabdlb") || Name.contains(".uabdlt"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".smullb") || Name.contains(".smullt") ||
            Name.contains(".umullb") || Name.contains(".umullt"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".sqdmullb") || Name.contains(".sqdmullt"))
            return 5 * widthMult + typeAdj;

        // Polynomial multiply
        if (Name.contains(".pmullb") || Name.contains(".pmullt"))
            return 5 * widthMult + typeAdj;

        // Bit manipulation (SVE2)
        if (Name.contains(".bdep") || Name.contains(".bext") ||
            Name.contains(".bgrp"))
            return 3 * widthMult + typeAdj;

        // Complex arithmetic (SVE2)
        if (Name.contains(".cadd") || Name.contains(".sqcadd"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".cmla") || Name.contains(".sqrdcmlah"))
            return 6 * widthMult + typeAdj;

        // Histogram operations (SVE2)
        if (Name.contains(".histcnt") || Name.contains(".histseg"))
            return 8 * widthMult + typeAdj;

        // Match operations (SVE2)
        if (Name.contains(".match") || Name.contains(".nmatch"))
            return 5 * widthMult + typeAdj;

        // Floating-point special operations
        if (Name.contains(".frintn") || Name.contains(".frintp") ||
            Name.contains(".frintm") || Name.contains(".frinta") ||
            Name.contains(".frintx") || Name.contains(".frinti") ||
            Name.contains(".frintz"))
            return 4 * widthMult + typeAdj;

        if (Name.contains(".fsqrt"))
            return 20 * widthMult + typeAdj;

        if (Name.contains(".frecpe") || Name.contains(".frecps") ||
            Name.contains(".frecpx"))
            return 5 * widthMult + typeAdj;

        if (Name.contains(".frsqrte") || Name.contains(".frsqrts"))
            return 6 * widthMult + typeAdj;

        // Exponential and trig approximations (if supported)
        if (Name.contains(".fexpa"))
            return 8 * widthMult + typeAdj;

        if (Name.contains(".ftsmul") || Name.contains(".ftssel") ||
            Name.contains(".ftmad"))
            return 6 * widthMult;

        // Count operations
        if (Name.contains(".cnt"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".clz") || Name.contains(".cls"))
            return 3 * widthMult + typeAdj;

        if (Name.contains(".ctz"))
            return 3 * widthMult + typeAdj;

        // Bit reversal
        if (Name.contains(".rbit"))
            return 2 * widthMult + typeAdj;

        // First Fault Register (FFR) operations
        if (Name.contains(".rdffr") || Name.contains(".wrffr") ||
            Name.contains(".setffr"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".sel"))
            return 2 * widthMult + typeAdj;

        if (Name.contains(".rbit") || Name.contains(".clz") || Name.contains(".cls")) return 3 * widthMult + typeAdj;  // Bit counts (efficient in SVE2)
        if (Name.contains(".ctz")) return 4 * widthMult + typeAdj;  // Slightly higher
        if (Name.contains(".histcnt") || Name.contains(".histseg")) return 8 * widthMult + typeAdj;  // Histogram (specialized, keep high)
        if (Name.contains(".match") || Name.contains(".nmatch")) return 5 * widthMult + typeAdj;  // Pattern matching
        if (Name.contains(".pmullb") || Name.contains(".pmullt")) return 5 * widthMult + typeAdj;  // Polynomial multiply
        if (Name.contains(".bdep") || Name.contains(".bext") || Name.contains(".bgrp")) return 3 * widthMult + typeAdj;  // Bit manipulation
        if (Name.contains(".cadd") || Name.contains(".sqcadd")) return 4 * widthMult + typeAdj;  // Complex add
        if (Name.contains(".cmla") || Name.contains(".sqrdcmlah")) return 6 * widthMult + typeAdj;  // Complex multiply-accumulate

        // Default for any unhandled SVE intrinsic
        return 4 * widthMult + typeAdj;
    }

    if (Name.starts_with("llvm.aarch64.sme")) return 7 * widthMult + typeAdj;  // Matrix extensions ~ SVE
    if (Name.starts_with("llvm.aarch64.ldxp") || Name.starts_with("llvm.aarch64.stxp")) return 8 * widthMult + typeAdj;

    return 0;
}

unsigned getGenericIntrinsicCostAArch64(StringRef Name)
{
    if (Name.starts_with("llvm.clear_cache")) return 15;

    if (Name.starts_with("llvm.vector.reduce"))
    {
        if (Name.contains(".add") || Name.contains(".mul"))
        {
            if (Name.contains(".v4"))
                return 5;
            if (Name.contains(".v8"))
                return 7;
            if (Name.contains(".v16"))
                return 9;
        }
        if (Name.contains(".fadd") || Name.contains(".fmul"))
        {
            if (Name.contains(".v4"))
                return 9;
            if (Name.contains(".v8"))
                return 11;
            if (Name.contains(".v16"))
                return 13;
        }
    }

    if (Name.starts_with("llvm.masked.load"))
    {
        if (Name.contains(".v4"))
            return 5;
        if (Name.contains(".v8"))
            return 6;
        if (Name.contains(".v16"))
            return 8;
    }

    if (Name.starts_with("llvm.masked.store"))
    {
        if (Name.contains(".v4"))
            return 3;
        if (Name.contains(".v8"))
            return 4;
        if (Name.contains(".v16"))
            return 6;
    }

    if (Name.starts_with("llvm.aarch64.crypto"))
    {
        if (Name.contains("aes"))
            return 3;
        if (Name.contains("sha1"))
            return 6;
        if (Name.contains("sha256"))
            return 6;
    }

    if (Name.starts_with("llvm.fma"))
    {
        if (Name.contains(".v2"))
            return 8;
        if (Name.contains(".v4"))
            return 12;
        if (Name.contains(".v8"))
            return 16;
        return 4;
    }

    if (Name.starts_with("llvm.bswap"))
    {
        if (Name.contains("i128"))
            return 2;
        if (Name.contains(".v"))
            return 3;
        return 1;
    }

    if (Name.starts_with("llvm.byteswap")) 
        return 1;

    if (Name.starts_with("llvm.ctpop"))
        return 3;

    if (Name.starts_with("llvm.ctlz") || Name.starts_with("llvm.cttz"))
        return 1;

    if (Name.starts_with("llvm.fshl") || Name.starts_with("llvm.fshr"))
        return 2;

    if (Name.starts_with("llvm.umin") || Name.starts_with("llvm.umax") || 
        Name.starts_with("llvm.smin") || Name.starts_with("llvm.smax"))
        return 2;

    if (Name.starts_with("llvm.abs"))
        return 2;

    if (Name.starts_with("llvm.fabs"))
        return 1;

    if (Name.starts_with("llvm.sqrt"))
        return 15;
    
    if (Name.starts_with("llvm.powi"))
        return 25;

    if(Name.starts_with("llvm.exp2"))
        return 25;

    if(Name.starts_with("llvm.exp10"))
        return 35;

    if (Name.starts_with("llvm.exp")
        && !Name.starts_with("llvm.expect") && !Name.starts_with("llvm.experimental"))
        return 35;

    if (Name.starts_with("llvm.round") || Name.starts_with("llvm.roundeven"))
        return 6;

    if (Name.starts_with("llvm.floor") ||
        Name.starts_with("llvm.ceil"))
        return 5;

    if (Name.starts_with("llvm.smul.with.overflow") || 
        Name.starts_with("llvm.umul.with.overflow"))
        return 3;

    if (Name.starts_with("llvm.smul.fix.sat") || Name.starts_with("llvm.umul.fix.sat"))
        return 4;
    
    if (Name.starts_with("llvm.sdiv.fix.sat") || Name.starts_with("llvm.udiv.fix.sat"))
        return 15;

    if (Name.starts_with("llvm.trunc"))
        return 3;

    if (Name.starts_with("llvm.copysign"))
        return 2;

    if (Name.starts_with("llvm.ldexp"))
        return 4;

    if (Name.starts_with("llvm.frexp"))
        return 6;

    if (Name.starts_with("llvm.sadd.with.overflow") || 
        Name.starts_with("llvm.uadd.with.overflow") ||
        Name.starts_with("llvm.ssub.with.overflow") || 
        Name.starts_with("llvm.usub.with.overflow"))
        return 2;

    if (Name.starts_with("llvm.sadd.sat") || Name.starts_with("llvm.uadd.sat") ||
        Name.starts_with("llvm.ssub.sat") || Name.starts_with("llvm.usub.sat"))
        return 2;

    if(Name.starts_with("llvm.atan2"))
        return 65;

    if(Name.starts_with("llvm.atan"))
        return 50;
        
    if (Name.starts_with("llvm.cosh") || 
        Name.starts_with("llvm.sinh") || 
        Name.starts_with("llvm.tanh"))
        return 40;

    if (Name.starts_with("llvm.tan"))
        return 45;

    if(Name.starts_with("llvm.sincospi"))
        return 75;

    if(Name.starts_with("llvm.sincos"))
        return 60;

    if (Name.starts_with("llvm.asin") || Name.starts_with("llvm.acos"))
        return 50;

    if (Name.starts_with("llvm.modf"))
    {
        if (Name.contains(".f64"))
            return 9;
        if (Name.contains(".f32"))
            return 5;
    }

    if (Name.starts_with("llvm.sin") || Name.starts_with("llvm.cos"))
        return 35;

    if (Name.starts_with("llvm.pow"))
        return 40;

    if (Name.starts_with("llvm.log2"))
        return 23;
    if (Name.starts_with("llvm.log10"))
        return 25;
    if (Name.starts_with("llvm.log"))
        return 24;

    if (Name.starts_with("llvm.masked.gather") || Name.starts_with("llvm.masked.scatter"))
        return 10;

    if (Name.starts_with("llvm.matrix.transpose"))
        return 6;

    if (Name.starts_with("llvm.matrix.multiply"))
    {
        if (Name.contains(".v2"))
            return 15;
        if (Name.contains(".v4"))
            return 25;
        if (Name.contains(".v8"))
            return 40;
        return 20;
    }

    if (Name.starts_with("llvm.matrix"))
        return 8;

    if (Name.starts_with("llvm.vector.reduce.smax") || 
        Name.starts_with("llvm.vector.reduce.smin") || 
        Name.starts_with("llvm.vector.reduce.umax") || 
        Name.starts_with("llvm.vector.reduce.umin"))
    {
        if (Name.contains(".v2"))
            return 4;
        if (Name.contains(".v4"))
            return 6;
        if (Name.contains(".v8"))
            return 8;
        return 7;
    }

    if (Name.starts_with("llvm.vector.reduce.or") || 
        Name.starts_with("llvm.vector.reduce.xor") ||
        Name.starts_with("llvm.vector.reduce.and") ||
        Name.starts_with("llvm.vector.reduce.not"))
    {
        if (Name.contains(".v2"))
            return 4;
        if (Name.contains(".v4"))
            return 6;
        if (Name.contains(".v8"))
            return 8;
        return 7;
    }

    if (Name.starts_with("llvm.vector.reduce.fmax") || 
        Name.starts_with("llvm.vector.reduce.fmin"))
    {
        if (Name.contains(".v2"))
            return 6;
        if (Name.contains(".v4"))
            return 8;
        if (Name.contains(".v8"))
            return 10;
        return 10;
    }

    if (Name.starts_with("llvm.vector.reduce.add"))
    {
        if (Name.contains(".v2"))
            return 3;
        if (Name.contains(".v4"))
            return 6;
        if (Name.contains(".v8"))
            return 9;
        return 7;
    }

    if (Name.starts_with("llvm.vector.reduce.mul"))
    {
        if (Name.contains(".v2"))
            return 8;
        if (Name.contains(".v4"))
            return 12;
        if (Name.contains(".v8"))
            return 16;
        return 14;
    }

    if (Name.starts_with("llvm.vector.reduce.sub"))
    {
        if (Name.contains(".v2"))
            return 3;
        if (Name.contains(".v4"))
            return 5;
        if (Name.contains(".v8"))
            return 7;
        return 9;
    }

    if (Name.starts_with("llvm.minnum") || Name.starts_with("llvm.maxnum"))
    {
        if (Name.contains(".v2"))
            return 6;
        if (Name.contains(".v4"))
            return 8;
        if (Name.contains(".v8"))
            return 10;
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }   

    if(Name.starts_with("llvm.lround") || Name.starts_with("llvm.llround"))
    {
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }

    if (Name.starts_with("llvm.minimumnum") || Name.starts_with("llvm.maximumnum"))
    {
        if (Name.contains(".v2"))
            return 6;
        if (Name.contains(".v4"))
            return 8;
        if (Name.contains(".v8"))
            return 10;
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }   
    
    if (Name.starts_with("llvm.minimum") || Name.starts_with("llvm.maximum"))
    {
        if (Name.contains(".v2"))
            return 6;
        if (Name.contains(".v4"))
            return 8;
        if (Name.contains(".v8"))
            return 10;
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }

    if(Name.starts_with("llvm.rint") || Name.starts_with("llvm.nearbyint"))
    {
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }

    if(Name.starts_with("llvm.lrint") || Name.starts_with("llvm.llrint"))
    {
        if (Name.contains(".f32"))
            return 3;
        if (Name.contains(".f64"))
            return 5;
        return 2;
    }

    if(Name.starts_with("llvm.experimental.constrained."))
    {
        if (Name.contains(".fadd") || Name.contains(".fsub"))
            return 5;

        if (Name.contains(".fmul"))
            return 7;

        if (Name.contains(".fdiv") || Name.contains(".frem"))
            return 17;

        if (Name.contains(".fma") || Name.contains(".fmuladd"))
            return 8;

        if (Name.contains(".fcmp") || Name.contains(".fcmps"))
            return 3;

        if (Name.contains(".sqrt"))
            return 15;

        if (Name.contains(".pow"))
            return 40;

        if (Name.contains(".powi"))
            return 25;

        if (Name.contains(".asin") || Name.contains(".acos"))
            return 50;

        if (Name.contains(".atan2"))
            return 65;

        if (Name.contains(".atan"))
            return 50;

        if (Name.contains(".sinh") || Name.contains(".cosh") || Name.contains(".tanh"))
            return 40;

        if (Name.contains(".tan"))
            return 45;

        if (Name.contains(".sin") || Name.contains(".cos"))
            return 35;

        if (Name.contains(".exp") && !Name.contains(".exp2"))
            return 35;

        if (Name.contains(".exp2"))
            return 25;

        if (Name.contains(".log2"))
            return 23;

        if (Name.contains(".log10"))
            return 25;

        if (Name.contains(".log") && !Name.contains(".log10") && !Name.contains(".log2"))
            return 24;

        if (Name.contains(".rint") || Name.contains(".nearbyint"))
        {
            if (Name.contains(".f32"))
                return 3;
            if (Name.contains(".f64"))
                return 5;
            return 2;
        }

        if (Name.contains(".lrint") || Name.contains(".llrint"))
        {
            if (Name.contains(".f32"))
                return 3;
            if (Name.contains(".f64"))
                return 5;
            return 2;
        }

        if (Name.contains(".maxnum") || Name.contains(".minnum") ||
            Name.contains(".maximum") || Name.contains(".minimum"))
        {
            if (Name.contains(".v2"))
                return 6;
            if (Name.contains(".v4"))
                return 8;
            if (Name.contains(".v8"))
                return 10;
            if (Name.contains(".f32"))
                return 3;
            if (Name.contains(".f64"))
                return 5;
            return 2;
        }

        if (Name.contains(".ceil") || Name.contains(".floor"))
            return 5;

        if (Name.contains(".round") || Name.contains(".roundeven"))
            return 6;

        if (Name.contains(".lround") || Name.contains(".llround"))
        {
            if (Name.contains(".f32"))
                return 3;
            if (Name.contains(".f64"))
                return 5;
            return 2;
        }
    }

    if (Name.starts_with("llvm.prefetch"))
        return 1;  // Cache prefetch hint
    
    if (Name.starts_with("llvm.assume"))
        return 0;  // Compiler assumption (no runtime cost)

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
        StringRef Suffix = Name.substr(17);
        std::string NewName = "llvm." + Suffix.str();
        return getGenericIntrinsicCostAArch64(NewName);
    }

    return 0;
}

Value *createMemcpyCostCalculationAArch64(Value *Size, unsigned BaseCost, 
    IRBuilder<> &Builder, MDNode *OpSigMD) 
{
    IntegerType *I64Ty = Builder.getInt64Ty();

    Value *Size8 = ConstantInt::get(I64Ty, 8);
    Value *Size32 = ConstantInt::get(I64Ty, 32);
    Value *Size128 = ConstantInt::get(I64Ty, 128);
    Value *Size1024 = ConstantInt::get(I64Ty, 1024);

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

Value *createMemsetCostCalculationAArch64(Value *Size, IRBuilder<> &Builder, MDNode *OpSigMD) 
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


unsigned insertDynamicMemoryCostAArch64(CallInst *Call, StringRef Name, Value *SizeArg, 
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
        DynamicCost = createMemsetCostCalculationAArch64(Size, Builder, OpSigMD);
    } else {
        DynamicCost = createMemcpyCostCalculationAArch64(Size, StaticBaseCost, Builder, OpSigMD);
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

unsigned getMemoryIntrinsicCostAArch64(CallInst *Call, StringRef Name, IRBuilder<> &Builder, MDNode *OpSigMD) 
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
            return insertDynamicMemoryCostAArch64(Call, Name, SizeArg, Builder, OpSigMD);
        }
    }

    if (Name.starts_with("llvm.memcpy"))   return 6; 
    if (Name.starts_with("llvm.memmove"))  return 8;
    if (Name.starts_with("llvm.memset"))   return 5;
    return 0;
}

unsigned getFuelCostAArch64(Instruction &I)
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
                    return getMemoryIntrinsicCostAArch64(Call, Name, Builder, OpSigMD);
                }

                if (unsigned cost = getIntrinsicCostAArch64(Callee->getName()))
                    return cost;

                if (unsigned cost = getGenericIntrinsicCostAArch64(Callee->getName()))
                    return cost;

                const char* outputUnhandledIntrinsicsStr = std::getenv("OUTPUT_UNHANDLED_INTRINSICS");
                int outputUnhandledIntrinsics = outputUnhandledIntrinsicsStr ? std::atoi(outputUnhandledIntrinsicsStr) : 0;
                if (outputUnhandledIntrinsics)
                    errs() << "Unhandled intrinsic: " << Callee->getName() << "\n";

                return 0;
            } 

            unsigned Cost = 3;
            //Cost += std::min(8u, Call->arg_size());

            return Cost;
        }
        else
        {
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
            baseCost = 1;
            break;

        case Instruction::Mul:
            baseCost = 3;
            break;

        case Instruction::UDiv:
        case Instruction::SDiv:
            baseCost = 12;
            break;

        case Instruction::URem:
        case Instruction::SRem:
            baseCost = 15;
            break;

        case Instruction::FAdd:
        case Instruction::FSub:
            isFloatingOp = true;
            baseCost = 4;
            break;

        case Instruction::FMul:
            isFloatingOp = true;
            baseCost = 6;
            break;

        case Instruction::FDiv:
            isFloatingOp = true;
            baseCost = 17;
            break;

        case Instruction::FRem:
            isFloatingOp = true;
            baseCost = 20;
            break;

        case Instruction::Load:
            baseCost = 3;  // Modern ARM L1 cache latency
            if (auto *LI = dyn_cast<LoadInst>(&I))
            {
                if (LI->isVolatile())
                    baseCost += 1;

                // ARM has simpler addressing modes than x86, smaller penalty
                if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand()))
                {
                    if (GEP->getNumIndices() > 2)
                        baseCost += 1;  // Complex addressing penalty
                }
            }
            break;

        case Instruction::Store:
            baseCost = 3;  // Store cost similar to load on modern ARM
            if (auto *SI = dyn_cast<StoreInst>(&I))
            {
                if (SI->isVolatile())
                    baseCost += 1;

                // Same addressing penalty as loads
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
            baseCost = 1;
            break;

        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor:
            baseCost = 1;
            break;

        case Instruction::ICmp:
            baseCost = 1;
            break;

        case Instruction::FCmp:
            isFloatingOp = true;
            baseCost = 3;
            break;

        case Instruction::ExtractElement:
        case Instruction::InsertElement:
            baseCost = 3;
            break;

        case Instruction::ShuffleVector:
            baseCost = 3;
            break;

        case Instruction::Select:
            baseCost = 2;
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
            baseCost = 2;  // Stack allocation
            break;

        case Instruction::AtomicRMW:
        case Instruction::AtomicCmpXchg:
            baseCost = 10;  // Base for atomic ops; scale with ordering (acquire/release +2, seq_cst +4)
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

    if (!isFloatingOp) {
        if (isFloat && isVector) baseCost *= 3;  // Same as X86
        else if (isVector) {
            if (auto *VT = dyn_cast<VectorType>(Ty)) {
                unsigned elements = VT->getElementCount().getKnownMinValue();
                if (elements <= 4) baseCost *= 2;
                else if (elements <= 8) baseCost = (baseCost * 5) / 2;
                else baseCost = std::min(baseCost * 5, baseCost * 4);  // Cap at x5 (>8 elements), adjusted to closer match X86
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
                baseCost = (baseCost * std::min(4u, (elements + 3) / 4) * 5) / 2;  // Scale like X86 (2.5x base), cap at x4
            } else {
                baseCost = (baseCost * 5) / 2;
            }
        }
    }

    return baseCost;
}