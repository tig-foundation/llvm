#include "llvm/IR/IntrinsicsAArch64.h"

unsigned getIntrinsicCostAArch64(StringRef Name, IRBuilder<> &Builder, CallInst *CI, MDNode *OpSigMD)
{
    auto getVectorWidthMultiplier = [](StringRef Name) -> unsigned {
        if (Name.contains(".v16")) return 4;  // Wide NEON/SVE equivalent
        if (Name.contains(".v8")) return 3;
        if (Name.contains(".v4")) return 2;
        if (Name.contains(".v2")) return 1;   // Narrow
        if (Name.starts_with("llvm.aarch64.sve")) return 4;  // SVE scalable premium (static base; dynamic scaling handled separately)
        return 2;  // Default NEON
    };

    auto getDataTypeAdjustment = [](StringRef Name) -> unsigned {  // Additive; +2 for f64 (matching x86/NEON behavior)
        if (Name.ends_with("f64")) return 2;  // +2 only for FP64 (not i64)
        if (Name.ends_with("f32") || Name.ends_with("i32") || Name.ends_with("i64")) return 1;
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
            Name.contains(".reduce") ||  // Expanded for SVE reductions
            Name.contains(".uaddv") || Name.contains(".saddv") ||
            Name.contains(".umaxv") || Name.contains(".smaxv") ||
            Name.contains(".uminv") || Name.contains(".sminv") ||
            Name.contains(".faddv") || Name.contains(".fmaxv") ||
            Name.contains(".fminv") || Name.contains(".fmaxnmv") ||
            Name.contains(".fminnmv") || Name.contains(".fadda") ||
            Name.contains(".trn1") || Name.contains(".trn2") ||
            Name.contains(".uzp1") || Name.contains(".uzp2") ||
            Name.contains(".zip1") || Name.contains(".zip2") ||
            Name.contains(".qadd") || Name.contains(".qsub") ||
            Name.contains(".sqadd") || Name.contains(".sqsub") ||
            Name.contains(".uqadd") || Name.contains(".uqsub");
    };

    auto getRuntimeVectorLength = [](IRBuilder<>& Builder, Type* ElementType) -> Value* {
        unsigned ElementSizeBytes = ElementType->getPrimitiveSizeInBits() / 8;

        switch (ElementSizeBytes) {
            case 1:
                return Builder.CreateIntrinsic(Intrinsic::aarch64_sve_cntb, {}, {Builder.getInt32(31)});
            case 2:
                return Builder.CreateIntrinsic(Intrinsic::aarch64_sve_cnth, {}, {Builder.getInt32(31)});
            case 4:
                return Builder.CreateIntrinsic(Intrinsic::aarch64_sve_cntw, {}, {Builder.getInt32(31)});
            case 8:
                return Builder.CreateIntrinsic(Intrinsic::aarch64_sve_cntd, {}, {Builder.getInt32(31)});
            default:
                return Builder.getInt64(16);
        }
    };

    auto getDynamicVectorSizeFromType = [&getRuntimeVectorLength](IRBuilder<> &Builder, CallInst* CI, MDNode* OpSigMD) -> Value* {
        for (unsigned idx = 0; idx < CI->getNumOperands(); ++idx) {
            Type* ArgTy = CI->getOperand(idx)->getType();

            if (auto* SVTy = dyn_cast<ScalableVectorType>(ArgTy)) {
                Value* length = getRuntimeVectorLength(Builder, SVTy->getElementType());
                if (auto* Inst = dyn_cast<Instruction>(length))
                    Inst->setMetadata("op_sig", OpSigMD);
                return length;
            }
        }

        for (unsigned idx = 0; idx < CI->getNumOperands(); ++idx) {
            Type* ArgTy = CI->getOperand(idx)->getType();
            if (auto* VTy = dyn_cast<FixedVectorType>(ArgTy)) {
                Value* numElements = Builder.getInt64(VTy->getNumElements());
                if (auto* Inst = dyn_cast<Instruction>(numElements))
                    Inst->setMetadata("op_sig", OpSigMD);
                return numElements;
            }
        }

        Value* Default = Builder.getInt64(4); // Default to 4 elements (ConstantInt, no metadata possible/needed)
        if (auto* Inst = dyn_cast<Instruction>(Default))
            Inst->setMetadata("op_sig", OpSigMD);
        return Default;
    };

    unsigned widthMult = getVectorWidthMultiplier(Name);
    unsigned typeAdj = getDataTypeAdjustment(Name);
    bool isHorizontal = Name.contains(".addp") || Name.contains(".saddlp") ||
                        Name.contains(".uaddlp") || Name.contains(".vmaxv") ||
                        Name.contains(".vminv") || Name.contains(".faddv") ||
                        Name.contains(".fmaxv") || Name.contains(".fminv") ||
                        Name.contains(".uaddv") || Name.contains(".saddv") ||  
                        Name.contains(".umaxv") || Name.contains(".smaxv") ||
                        Name.contains(".uminv") || Name.contains(".sminv") ||
                        Name.contains(".fmaxnmv") || Name.contains(".fminnmv") ||
                        Name.contains(".fadda");

    if (Name.starts_with("llvm.arm.neon") || Name.starts_with("llvm.aarch64.neon"))
    {
        if (Name.contains(".add") || Name.contains(".sub") || Name.contains(".addp") ||
            Name.contains(".saddlp") || Name.contains(".uaddlp") || Name.contains(".saddl") ||
            Name.contains(".uaddl") || Name.contains(".ssubl") || Name.contains(".usubl") ||
            Name.contains(".saddl2") || Name.contains(".uaddl2") || Name.contains(".ssubl2") ||
            Name.contains(".usubl2") || Name.contains(".vpadd") || Name.contains(".hadd") ||
            Name.contains(".rhadd") || Name.contains(".shadd") || Name.contains(".uhadd") ||
            Name.contains(".srhadd") || Name.contains(".urhadd"))
        {
            unsigned base = 1;  // Align to Add/Sub in getFuelCost
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            return cost;
        }

        if (Name.contains(".mul") || Name.contains(".pmul") || Name.contains(".smull") ||
            Name.contains(".umull") || Name.contains(".vqdmull") || Name.contains(".vqdmlal") ||
            Name.contains(".vqdmlsl") || Name.contains(".vqdmulh") || Name.contains(".sqrdmulh") ||
            Name.contains(".sqdmulh") || Name.contains(".sqrdmlah") || Name.contains(".sqrdmlsh") ||
            Name.contains(".sdot") || Name.contains(".udot") || Name.contains(".pmull") ||
            Name.contains(".vmlal") || Name.contains(".vmlsl"))
        {
            unsigned base = 3;  // Align to Mul in getFuelCost
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".div"))
        {
            unsigned base = Name.contains("f") ? 17 : 12;  // Align to FDiv/UDiv/SDiv in getFuelCost, FP higher
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".fma") || Name.contains(".fms") || Name.contains(".fmla") ||
            Name.contains(".fmls") || Name.contains(".fnma") || Name.contains(".fnms") ||
            Name.contains(".mlal") || Name.contains(".mlsl"))
        {
            unsigned base = 4;  // Align to fused FMul (6) + FAdd (4) but average to 4 for efficiency
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".cmp") || Name.contains(".vcge") || Name.contains(".vcgt") ||
            Name.contains(".vcage") || Name.contains(".vcagt") || Name.contains(".vceq") ||
            Name.contains(".cmeq") || Name.contains(".cmge") || Name.contains(".cmgt") ||
            Name.contains(".cmle") || Name.contains(".cmlt") || Name.contains(".acge") ||
            Name.contains(".acgt"))
        {
            unsigned base = Name.contains("f") ? 3 : 1;  // Align to FCmp=3 / ICmp=1 in getFuelCost
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".min") || Name.contains(".max") || Name.contains(".vmin") ||
            Name.contains(".vmax") || Name.contains(".vpmin") || Name.contains(".vpmax") ||
            Name.contains(".vminv") || Name.contains(".vmaxv") || Name.contains(".smin") ||
            Name.contains(".smax") || Name.contains(".umin") || Name.contains(".umax"))
        {
            unsigned base = Name.contains("f") ? 3 : 2;  // Align to FCmp-like for FP, higher than int min/max
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            return cost;
        }

        if (Name.contains(".and") || Name.contains(".or") || Name.contains(".eor") ||
            Name.contains(".bic") || Name.contains(".orn") || Name.contains(".bsl"))
        {
            unsigned base = 1;  // Align to And/Or/Xor in getFuelCost
            unsigned cost = base * widthMult;
            return cost;
        }

        if (Name.contains(".shl") || Name.contains(".shr") || Name.contains(".asr") ||
            Name.contains(".lsl") || Name.contains(".lsr") || Name.contains(".rshrn") ||
            Name.contains(".sqshrn") || Name.contains(".uqshrn") || Name.contains(".sqshlu") ||
            Name.contains(".sqrshrun") || Name.contains(".srshl") || Name.contains(".urshl") ||
            Name.contains(".sqrshl") || Name.contains(".uqrshl"))
        {
            unsigned base = 1;  // Align to Shl/LShr/AShr in getFuelCost
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".abs") || Name.contains(".neg") || Name.contains(".qabs") ||
            Name.contains(".qneg") || Name.contains(".sqabs") || Name.contains(".sqneg"))
        {
            unsigned base = 1;  // Align to FNeg=1 or basic op
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".cvt") || Name.contains(".fcvt") || Name.contains(".scvtf") ||
            Name.contains(".ucvtf") || Name.contains(".fcvtzs") || Name.contains(".fcvtzu"))
        {
            unsigned base = 3;  // Align to custom conversion cost, midway between int<->fp
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".sqrt") || Name.contains(".frsqrte") || Name.contains(".frsqrts"))
        {
            unsigned base = 15;  // Align to custom sqrt=15 in generic
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".recpe") || Name.contains(".frecpe") || Name.contains(".vrecpe") ||
            Name.contains(".vrecps") || Name.contains(".frecps") || Name.contains(".vrecpx") ||
            Name.contains(".rsqrte") || Name.contains(".vrsqrte") || Name.contains(".vrsqrts") ||
            Name.contains(".frint"))
        {
            unsigned base = 4;  // Align to reciprocal approx, between Mul=3 and Div=12
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".trn1") || Name.contains(".trn2") || Name.contains(".uzp1") ||
            Name.contains(".uzp2") || Name.contains(".zip1") || Name.contains(".zip2") ||
            Name.contains(".ext") || Name.contains(".rev") || Name.contains(".vrev") ||
            Name.contains(".vext") || Name.contains(".vtbl") || Name.contains(".vtbx") ||
            Name.contains(".vdup_lane") || Name.contains(".vtrn") || Name.contains(".vzip") ||
            Name.contains(".vuzp"))
        {
            unsigned base = 3;  // Align to ShuffleVector/ExtractElement=3 in getFuelCost
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".qadd") || Name.contains(".qsub") || Name.contains(".sqadd") ||
            Name.contains(".sqsub") || Name.contains(".uqadd") || Name.contains(".uqsub") ||
            Name.contains(".abd") || Name.contains(".qabd") || Name.contains(".sqabd") ||
            Name.contains(".uqabd"))
        {
            unsigned base = 2;  // Align to saturating add/sub, between Add=1 and Mul=3
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0);
            return cost;
        }

        if (Name.contains(".reduce") || Name.contains(".uaddv") || Name.contains(".saddv") ||
            Name.contains(".umaxv") || Name.contains(".smaxv") || Name.contains(".uminv") ||
            Name.contains(".sminv") || Name.contains(".faddv") || Name.contains(".fmaxv") ||
            Name.contains(".fminv") || Name.contains(".fmaxnmv") || Name.contains(".fminnmv") ||
            Name.contains(".fadda"))
        {
            unsigned base = Name.contains("f") ? 8 : 6;  // Align to higher for reductions, FP premium
            unsigned cost = base * widthMult + (isElementSensitive(Name) ? typeAdj : 0) + (isHorizontal ? widthMult : 0);
            return cost;
        }

        if (Name.contains(".ld1") || Name.contains(".vld1") || Name.contains(".vld2") ||
            Name.contains(".vld3") || Name.contains(".vld4"))
        {
            unsigned base = 3;  // Align to Load in getFuelCost
            unsigned extra = Name.contains(".lane") ? 5 : 1;  // Lane variants more expensive
            if (Name.contains(".vld2")) base += 3;  // Structure loads higher
            if (Name.contains(".vld3")) base += 6;
            if (Name.contains(".vld4")) base += 9;
            unsigned cost = base + (widthMult / 2) + extra;
            return cost;
        }

        if (Name.contains(".st1") || Name.contains(".vst1") || Name.contains(".vst2") ||
            Name.contains(".vst3") || Name.contains(".vst4"))
        {
            unsigned base = 3;  // Align to Store in getFuelCost
            unsigned extra = Name.contains(".lane") ? 0 : 0;  // Lane variants less penalty for stores
            if (Name.contains(".vst2")) base += 3;
            if (Name.contains(".vst3")) base += 6;
            if (Name.contains(".vst4")) base += 9;
            unsigned cost = base + (widthMult / 2) + extra;
            return cost;
        }

        if (Name.contains(".get_lane") || Name.contains(".vget_lane") ||
            Name.contains(".set_lane") || Name.contains(".vset_lane"))
        {
            unsigned base = 3;  // Align to ExtractElement/InsertElement=3
            unsigned cost = base * widthMult + typeAdj;
            return cost;
        }

        // Default for unhandled NEON (align to generic vector op)
        return 4 * widthMult + typeAdj;
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

    if (Name.starts_with("llvm.aarch64.sve"))
    {
        Value* numElements = getDynamicVectorSizeFromType(Builder, CI, OpSigMD);
        IntegerType* I64Ty = Builder.getInt64Ty();
        Value* scaledCost = nullptr;

        // Determine element type from operands for accurate type adjustment
        Type* elemType = nullptr;
        for (unsigned idx = 0; idx < CI->getNumOperands(); ++idx) {
            Type* argTy = CI->getOperand(idx)->getType();
            if (auto* svTy = dyn_cast<ScalableVectorType>(argTy)) {
                elemType = svTy->getElementType();
                break;
            }
        }
        unsigned elemBits = elemType ? elemType->getPrimitiveSizeInBits() : 32;  // Default to 32-bit
        bool isFP = elemType && elemType->isFloatingPointTy();
        bool is64Bit = (elemBits == 64);

        // Compute typeAdj: +2 only for f64 (align to FP-specific, matching x86/NEON)
        unsigned typeAdj = 1;  // Default
        if (isFP && is64Bit) typeAdj = 2;  // +2 for f64 only (not i64)

        bool sensitive = isElementSensitive(Name);
        isHorizontal = Name.contains(".uaddv") || Name.contains(".saddv") || Name.contains(".umaxv") || Name.contains(".smaxv") || Name.contains(".uminv") || Name.contains(".sminv") || Name.contains(".faddv") || Name.contains(".fmaxv") ||
                       Name.contains(".fminv") || Name.contains(".fmaxnmv") || Name.contains(".fminnmv") || Name.contains(".fadda") || Name.contains(".orv") || Name.contains(".eorv") || Name.contains(".andv") || Name.contains(".orqv") ||
                       Name.contains(".eorqv") || Name.contains(".andqv") || Name.contains(".addqv") || Name.contains(".smaxqv") || Name.contains(".umaxqv") || Name.contains(".sminqv") || Name.contains(".uminqv");

        // Basic arithmetic operations (add, sub, etc.)
        if (Name.contains(".add") || Name.contains(".sub") || Name.contains(".saddlp") || Name.contains(".uaddlp"))
        {
            unsigned base = 1;  // Matches Add/Sub
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0) + (isHorizontal ? 1 : 0)));
        }

        if (Name.contains(".mul") || Name.contains(".pmul") || Name.contains(".smull") || Name.contains(".umull") || Name.contains(".pmull") || Name.contains(".sqdmulh") || Name.contains(".sqrdmulh"))
        {
            unsigned base = 3;  // Matches Mul
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        if (Name.contains(".div") || Name.contains(".sdiv") || Name.contains(".udiv") || Name.contains(".sdivr") || Name.contains(".udivr"))
        {
            unsigned base = isFP ? 17 : 12;  // Matches FDiv/UDiv
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Multiply-add operations (mla, mls, mad, msb, sqdmlalb, etc.)
        if (Name.contains(".mla") || Name.contains(".mls") || Name.contains(".mad") || Name.contains(".msb") || Name.contains(".sqdmlalb") || Name.contains(".sqdmlalt") || Name.contains(".sqdmlslb") || Name.contains(".sqdmlslt") || Name.contains(".sqdmlalbt") || Name.contains(".sqdmlslbt"))
        {
            unsigned base = 4; // Matches FAdd + Mul fused
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Floating-point multiply-add (fmla, fmls, etc.)
        if (Name.contains(".fmla") || Name.contains(".fmls") || Name.contains(".fmad") || Name.contains(".fmsb") || Name.contains(".fnmla") || Name.contains(".fnmls") || Name.contains(".fmlalb") || Name.contains(".fmlalt") || Name.contains(".fmlslb") || Name.contains(".fmlslt"))
        {
            unsigned base = 5; // Matches FMul + FAdd (~6+4)
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Comparisons (cmp, cmpeq, cmpge, etc.; facge, facgt, fcmpeq, etc. for FP)
        if (Name.contains(".cmp") || Name.contains(".cmpeq") || Name.contains(".cmpne") || Name.contains(".cmpge") || Name.contains(".cmpgt") || Name.contains(".cmphi") || Name.contains(".cmphs") || Name.contains(".cmple") || Name.contains(".cmplt") || Name.contains(".cmplo") || Name.contains(".cmpls") || Name.contains(".cmple_wide") || Name.contains(".cmplo_wide") || Name.contains(".cmpls_wide") || Name.contains(".cmplt_wide") || Name.contains(".cmpne_wide") || Name.contains(".fcmp") || Name.contains(".facge") || Name.contains(".facgt") || Name.contains(".fcmeq") || Name.contains(".fcmge") || Name.contains(".fcmgt") || Name.contains(".fcmle") || Name.contains(".fcmlt") || Name.contains(".fcmne") || Name.contains(".fcmpeq") || Name.contains(".fcmpeq_wide") || Name.contains(".fcmplo_wide") || Name.contains(".fcmpls_wide") || Name.contains(".fcmplt_wide") || Name.contains(".fcmple_wide"))
        {
            unsigned base = isFP ? 3 : 1;  // Matches FCmp/ICmp
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Reductions (uaddv, saddv, faddv, etc.)
        if (Name.contains(".uaddv") || Name.contains(".saddv") || Name.contains(".umaxv") || Name.contains(".smaxv") || Name.contains(".uminv") || Name.contains(".sminv") || Name.contains(".faddv") || Name.contains(".fmaxv") ||
            Name.contains(".fminv") || Name.contains(".fmaxnmv") || Name.contains(".fminnmv") || Name.contains(".fadda") || Name.contains(".orv") || Name.contains(".eorv") || Name.contains(".andv") || Name.contains(".orqv") || Name.contains(".eorqv") || Name.contains(".andqv") || Name.contains(".addqv") || Name.contains(".smaxqv") || Name.contains(".umaxqv") || Name.contains(".sminqv") || Name.contains(".uminqv"))
        {
            unsigned base = isFP ? 8 : 6;  // Higher for reductions, aligned with horizontal premium
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0) + (isHorizontal ? 1 : 0)));
        }

        // Bitwise operations (and, orr, eor, bic, orn, eor3, bcax, bsl, nbsl, etc.)
        if (Name.contains(".and") || Name.contains(".orr") || Name.contains(".eor") || Name.contains(".bic") || Name.contains(".orn") || Name.contains(".eor3") || Name.contains(".bcax") || Name.contains(".bsl") || Name.contains(".nbsl") || Name.contains(".and_z") || Name.contains(".bic_z") || Name.contains(".eor_z") || Name.contains(".orr_z") || Name.contains(".nand_z") || Name.contains(".nor_z") || Name.contains(".orn_z"))
        {
            unsigned base = 1;  // Matches And/Or/Xor
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Shifts (asr, lsl, lsr, xar, etc.)
        if (Name.contains(".lsl") || Name.contains(".lsr") || Name.contains(".asr") || Name.contains(".xar") || Name.contains(".asrd") || Name.contains(".srshr") || Name.contains(".urshr") || Name.contains(".srsra") || Name.contains(".ursra") || Name.contains(".ssra") || Name.contains(".usra") || Name.contains(".srshl") || Name.contains(".urshl") || Name.contains(".sqrshl") || Name.contains(".uqrshl") || Name.contains("srshl_single") || Name.contains("urshl_single"))
        {
            unsigned base = 1;  // Matches Shl/LShr/AShr
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Absolute value and negation (abs, neg, sqabs, sqneg, etc.)
        if (Name.contains(".abs") || Name.contains(".neg") || Name.contains(".fabs") || Name.contains(".fneg") || Name.contains(".sqabs") || Name.contains(".sqneg") || Name.contains(".cnot"))
        {
            unsigned base = 1;  // Matches FNeg
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Min/max operations (smax, smin, umax, umin, fmax, fmin, etc.)
        if (Name.contains(".smax") || Name.contains(".smin") || Name.contains(".umax") || Name.contains(".umin") || Name.contains(".fmax") || Name.contains(".fmin") || Name.contains(".fmaxnm") || Name.contains(".fminnm") || Name.contains(".smaxp") || Name.contains(".sminp") || Name.contains(".umaxp") || Name.contains(".uminp") || Name.contains(".smaxqv") || Name.contains(".umaxqv") || Name.contains(".sminqv") || Name.contains(".uminqv") || Name.contains(".fmaxp") || Name.contains(".fminp") || Name.contains(".fmaxnmp") || Name.contains(".fminnmp"))
        {
            unsigned base = isFP ? 3 : 2;  // Matches FCmp/ICmp + Select
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Saturating arithmetic (qadd, qsub, sqadd, sqsub, uqadd, uqsub, sqdech, sqdecw, etc.)
        if (Name.contains(".qadd") || Name.contains(".qsub") || Name.contains(".sqadd") || Name.contains(".sqsub") || Name.contains(".uqadd") || Name.contains(".uqsub") || Name.contains(".sqdech") || Name.contains(".sqdecw") || Name.contains(".sqdecd") || Name.contains(".sqdecp") || Name.contains(".sqinch") || Name.contains(".sqincw") || Name.contains(".sqincd") || Name.contains(".sqincp") || Name.contains(".uqdech") || Name.contains(".uqdecw") || Name.contains(".uqdecd") || Name.contains(".uqdecp") || Name.contains(".uqinch") || Name.contains(".uqincw") || Name.contains(".uqincd") || Name.contains(".uqincp") || Name.contains(".sqcvt") || Name.contains(".uqcvt") || Name.contains(".sqcvtu") || Name.contains(".sqcvtn") || Name.contains(".uqcvtn") || Name.contains(".sqcvtun") || Name.contains(".sqcvtu_x2") || Name.contains(".sqcvtu_x4") || Name.contains(".sqcvt_x2") || Name.contains(".sqcvt_x4") || Name.contains(".uqcvt_x2") || Name.contains(".uqcvt_x4") || Name.contains(".sqcvt_x2") || Name.contains(".sqcvt_x4") || Name.contains(".uqcvt_x2") || Name.contains(".uqcvt_x4") || Name.contains(".sqcvtn_x2") || Name.contains(".sqcvtn_x4") || Name.contains(".uqcvtn_x2") || Name.contains(".uqcvtn_x4") || Name.contains(".sqcvtun_x2") || Name.contains(".sqcvtun_x4"))
        {
            unsigned base = 3;  // Matches Mul-like saturation
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Conversions (fcvt, scvtf, ucvtf, fcvtzs, fcvtzu, scvtf, ucvtf, sxtb, uxtb, etc.)
        if (Name.contains(".fcvt") || Name.contains(".scvtf") || Name.contains(".ucvtf") || Name.contains(".fcvtzs") || Name.contains(".fcvtzu") || Name.contains(".sxtb") || Name.contains(".sxth") || Name.contains(".sxtw") || Name.contains(".uxtb") || Name.contains(".uxth") || Name.contains(".uxtw") || Name.contains(".fcvtl") || Name.contains(".fcvtn") || Name.contains(".bfcvtn") || Name.contains(".fcvtx") || Name.contains(".fcvtxnt") || Name.contains(".fcvtnt") || Name.contains(".fcvtlt") || Name.contains(".fcvt_bf16f32") || Name.contains(".fcvtnt_bf16f32") || Name.contains(".fcvt_f16f32") || Name.contains(".fcvt_f16f64") || Name.contains(".fcvt_f32f64") || Name.contains(".fcvt_f32f16") || Name.contains(".fcvt_f64f16") || Name.contains(".fcvt_f64f32"))
        {
            unsigned base = 4;  // Matches custom costs, aligned
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Predicate operations (fixed cost, not scaled)
        if (Name.contains(".ptrue"))
            return 1;
        if (Name.contains(".pfalse"))
            return 0;  // Can be optimized away

        if (Name.contains(".punpklo") || Name.contains(".punpkhi") || Name.contains(".punpkhi") || Name.contains(".punpklo") || Name.contains(".sunpkhi") || Name.contains(".sunpklo") || Name.contains(".uunpkhi") || Name.contains(".uunpklo"))
            return 2;

        if (Name.contains(".brka") || Name.contains(".brkb") ||
            Name.contains(".brkn") || Name.contains(".brkpa") ||
            Name.contains(".brkpb") || Name.contains(".brka_z") || Name.contains(".brkb_z") || Name.contains(".brkn_z"))
            return 3;

        if (Name.contains(".pfirst") || Name.contains(".pnext"))
            return 2;

        if (Name.contains(".ptest"))
            return 2;

        if (Name.contains(".cntp"))
        {
            // cntp counts active elements, fixed low cost (not scaled)
            return 3;
        }

        if (Name.contains(".ptest_any") || Name.contains(".ptest_first") || Name.contains(".ptest_last"))
            return 2;

        // While loop operations (fixed cost)
        if (Name.contains(".whilele") || Name.contains(".whilelo") ||
            Name.contains(".whilels") || Name.contains(".whilelt") ||
            Name.contains(".whilege") || Name.contains(".whilegt") ||
            Name.contains(".whilehs") || Name.contains(".whilehi") || Name.contains(".whilerw") || Name.contains(".whilewr"))
            return 3;

        // Memory operations - handle structure first to avoid overlap, then general with non-temporal adjustment
        if (Name.contains(".ld2") || Name.contains(".st2")) {
            unsigned base = Name.contains(".ld") ? 6 : 6;  // Approx 2x Load/Store=3
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        } else if (Name.contains(".ld3") || Name.contains(".st3")) {
            unsigned base = Name.contains(".ld") ? 9 : 9;  // Approx 3x
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        } else if (Name.contains(".ld4") || Name.contains(".st4")) {
            unsigned base = Name.contains(".ld") ? 12 : 12;  // Approx 4x
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        } else if (Name.contains(".ld1") || Name.contains(".ldff1") || Name.contains(".ldnt1") || Name.contains(".ldnf1") || Name.contains(".ld1rq") || Name.contains(".ld1ro") || Name.contains(".ld1_gather") || Name.contains(".ldff1_gather") || Name.contains(".ldnt1_gather") || Name.contains(".ld1_gather_index") || Name.contains(".ldff1_gather_index") || Name.contains(".ld1_gather_sxtw") || Name.contains(".ld1_gather_uxtw") || Name.contains(".ldff1_gather_sxtw") || Name.contains(".ldff1_gather_uxtw") || Name.contains(".ld1_gather_sxtw_index") || Name.contains(".ld1_gather_uxtw_index") || Name.contains(".ldff1_gather_sxtw_index") || Name.contains(".ldff1_gather_uxtw_index") || Name.contains(".ld1_gather_scalar_offset") || Name.contains(".ldff1_gather_scalar_offset") || Name.contains(".ldnt1_gather_scalar_offset") || Name.contains(".ld1q_gather") || Name.contains(".ld1q_gather_scalar_offset") || Name.contains(".ld1q_gather_index") || Name.contains(".ld1q_gather_vector_offset") || Name.contains(".ld2q_sret") || Name.contains(".ld3q_sret") || Name.contains(".ld4q_sret") || Name.contains(".ld1uwq") || Name.contains(".ld1udq") || Name.contains(".ld1_pn_x2") || Name.contains(".ld1_pn_x4") || Name.contains(".ldnt1_pn_x2") || Name.contains(".ldnt1_pn_x4") || Name.contains(".luti2_lane_zt") || Name.contains(".luti4_lane_zt") || Name.contains(".luti2_lane_zt_x2") || Name.contains(".luti4_lane_zt_x2") || Name.contains(".luti2_lane_zt_x4") || Name.contains(".luti4_lane_zt_x4")) {
            unsigned base = (Name.contains(".gather") || Name.contains(".ldff1") || Name.contains(".ldnf1")) ? 5 : 3;  // Base for loads
            if (Name.contains(".ldnt1")) base = 2;  // Non-temporal adjustment
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        } else if (Name.contains(".st1") || Name.contains(".stnt1") || Name.contains(".st1_scatter") || Name.contains(".stnt1_scatter") || Name.contains(".st1_scatter_index") || Name.contains(".st1_scatter_sxtw") || Name.contains(".st1_scatter_uxtw") || Name.contains(".st1_scatter_sxtw_index") || Name.contains(".st1_scatter_uxtw_index") || Name.contains(".st1_scatter_scalar_offset") || Name.contains(".stnt1_scatter_scalar_offset") || Name.contains(".st1q_scatter") || Name.contains(".st1q_scatter_scalar_offset") || Name.contains(".st1q_scatter_index") || Name.contains(".st1q_scatter_vector_offset") || Name.contains(".st2q") || Name.contains(".st3q") || Name.contains(".st4q") || Name.contains(".st1wq") || Name.contains(".st1dq") || Name.contains(".st1_pn_x2") || Name.contains(".st1_pn_x4") || Name.contains(".stnt1_pn_x2") || Name.contains(".stnt1_pn_x4")) {
            unsigned base = Name.contains(".scatter") ? 5 : 3;  // Base for stores
            if (Name.contains(".stnt1")) base = 2;  // Non-temporal adjustment
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Permutations and shuffles (rev, trn1, uzp1, zip1, tbl, tbx, compact, splice, lasta, lastb, ext, dup, etc.)
        if (Name.contains(".rev") || Name.contains(".revb") || Name.contains(".revh") || Name.contains(".revw") || Name.contains(".revd") || Name.contains(".trn1") || Name.contains(".trn2") || Name.contains(".uzp1") || Name.contains(".uzp2") || Name.contains(".zip1") || Name.contains(".zip2") || Name.contains(".tbl") || Name.contains(".tbx") || Name.contains(".compact") || Name.contains(".splice") || Name.contains(".lastb") || Name.contains(".lasta") || Name.contains(".ext") || Name.contains(".dup") || Name.contains(".dupq") || Name.contains(".extq") || Name.contains(".tblq") || Name.contains(".tbxq") || Name.contains(".trn1q") || Name.contains(".trn2q") || Name.contains(".uzp1q") || Name.contains(".uzp2q") || Name.contains(".zip1q") || Name.contains(".zip2q") || Name.contains(".zipq1") || Name.contains(".zipq2") || Name.contains(".uzpq1") || Name.contains(".uzpq2") || Name.contains(".trn1_b16") || Name.contains(".trn1_b32") || Name.contains(".trn1_b64") || Name.contains(".trn2_b16") || Name.contains(".trn2_b32") || Name.contains(".trn2_b64") || Name.contains(".uzp1_b16") || Name.contains(".uzp1_b32") || Name.contains(".uzp1_b64") || Name.contains(".uzp2_b16") || Name.contains(".uzp2_b32") || Name.contains(".uzp2_b64") || Name.contains(".zip1_b16") || Name.contains(".zip1_b32") || Name.contains(".zip1_b64") || Name.contains(".zip2_b16") || Name.contains(".zip2_b32") || Name.contains(".zip2_b64") || Name.contains(".zip_x2") || Name.contains(".zipq_x2") || Name.contains(".zip_x4") || Name.contains(".zipq_x4") || Name.contains(".uzp_x2") || Name.contains(".uzpq_x2") || Name.contains(".uzp_x4") || Name.contains(".uzpq_x4") || Name.contains(".tbl2") || Name.contains(".tbx2") || Name.contains(".extq"))
        {
            unsigned base = 3;  // Matches ShuffleVector/ExtractElement
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Index generation (index)
        if (Name.contains(".index"))
        {
            unsigned base = 1;
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // SVE2 specific operations (sqdmulh, sqrdmulh, sqdmlalb, etc.)
        if (Name.contains(".sqdmulh") || Name.contains(".sqrdmulh") || Name.contains(".sqdmullb") || Name.contains(".sqdmullt") || Name.contains(".sabalb") || Name.contains(".sabalt") ||
            Name.contains(".uabalb") || Name.contains(".uabalt") || Name.contains(".saddlb") || Name.contains(".saddlt") || Name.contains(".uaddlb") || Name.contains(".uaddlt") ||
            Name.contains(".ssublb") || Name.contains(".ssublt") || Name.contains(".usublb") || Name.contains(".usublt") || Name.contains(".sabdlb") || Name.contains(".sabdlt") ||
            Name.contains(".uabdlb") || Name.contains(".uabdlt") || Name.contains(".smullb") || Name.contains(".smullt") || Name.contains(".umullb") || Name.contains(".umullt") ||
            Name.contains(".pmullb") || Name.contains(".pmullt") || Name.contains(".pmullb_pair") || Name.contains(".pmullt_pair") || Name.contains(".eorbt") || Name.contains(".eortb") ||
            Name.contains(".bdep") || Name.contains(".bext") || Name.contains(".bgrp") || Name.contains(".cadd") || Name.contains(".sqcadd") || Name.contains(".cmla") || Name.contains(".sqrdcmlah") ||
            Name.contains(".histcnt") || Name.contains(".histseg") || Name.contains(".match") || Name.contains(".nmatch") || Name.contains(".eor3") || Name.contains(".bcax") || 
            Name.contains(".bsl") || Name.contains(".nbsl") || Name.contains(".xar") || Name.contains(".fmlalb") || Name.contains(".fmlalt") || Name.contains(".fmlslb") || Name.contains(".fmlslt") ||
            Name.contains(".addp") || Name.contains(".faddp") || Name.contains(".fmaxp") || Name.contains(".fmaxnmp") || Name.contains(".fminp") || Name.contains(".fminnmp") ||
            Name.contains(".smaxp") || Name.contains(".sminp") || Name.contains(".umaxp") || Name.contains(".uminp") || Name.contains(".sadalp") || Name.contains(".uadalp") ||
            Name.contains(".saddlbt") || Name.contains(".ssublbt") || Name.contains(".ssubltb") || Name.contains(".cdot") ||
            Name.contains(".shadd") || Name.contains(".shsub") || Name.contains(".shsubr") || Name.contains(".sli") || Name.contains(".sqabs") ||
            Name.contains(".sqadd") || Name.contains(".sqdmulh") || Name.contains(".sqneg") || Name.contains(".sqrdmlah") || Name.contains(".sqrdmlsh") ||
            Name.contains(".sqrdmulh") || Name.contains(".sqrshl") || Name.contains(".sqshl") || Name.contains(".sqshlu") || Name.contains(".sqsub") ||
            Name.contains(".sqsubr") || Name.contains(".srhadd") || Name.contains(".sri") || Name.contains(".srshl") || Name.contains(".srshr") ||
            Name.contains(".srsra") || Name.contains(".ssra") || Name.contains(".suqadd") || Name.contains(".uaba") || Name.contains(".uhadd") ||
            Name.contains(".uhsub") || Name.contains(".uhsubr") || Name.contains(".uqadd") || Name.contains(".uqrshl") || Name.contains(".uqshl") ||
            Name.contains(".uqsub") || Name.contains(".uqsubr") || Name.contains(".urecpe") || Name.contains(".urhadd") || Name.contains(".urshl") ||
            Name.contains(".urshr") || Name.contains(".ursqrte") || Name.contains(".ursra") || Name.contains(".usqadd") || Name.contains(".usra"))
        {
            unsigned base = Name.contains(".mul") || Name.contains(".qdmulh") || Name.contains(".qdmlal") || Name.contains(".qdmlsl") || Name.contains(".qdmull") || Name.contains(".qrdmulh") || Name.contains(".qrdmlah") || Name.contains(".qrdmlsh") || Name.contains(".mull") 
                ? 3 : (Name.contains(".add") || Name.contains(".sub") || Name.contains(".abd") || Name.contains(".adcl") || Name.contains(".sbcl") 
                ? 1 : (Name.contains(".shl") || Name.contains(".shr") || Name.contains(".sra") 
                ? 1 : (Name.contains(".hist") || Name.contains(".match") 
                ? 8 : 2)));  // Adjust bases based on op type, matching getFuelCost
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Floating-point special operations (frint, fsqrt, frecpe, frsqrte, fexpa, ftsmul, etc.)
        if (Name.contains(".frintn") || Name.contains(".frintp") ||
            Name.contains(".frintm") || Name.contains(".frinta") ||
            Name.contains(".frintx") || Name.contains(".frinti") ||
            Name.contains(".frintz") || Name.contains(".frint") || Name.contains(".frecpe") || Name.contains(".frecps") ||
            Name.contains(".frecpx") || Name.contains(".frsqrte") || Name.contains(".frsqrts") ||
            Name.contains(".fexpa") || Name.contains(".ftsmul") || Name.contains(".ftssel") ||
            Name.contains(".ftmad") || Name.contains(".fsqrt"))
        {
            unsigned base = Name.contains(".fsqrt") ? 20 : (Name.contains(".ftsmul") || Name.contains(".frsqrte") || Name.contains(".ftmad") ? 6 : 4);  // Aligned with custom costs
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Count operations (cnt, clz, cls, ctz, cntb, cnth, cntw, cntd, cntp, cntsb, cntsh, cntsw, cntsd)
        if (Name.contains(".cnt") || Name.contains(".clz") || Name.contains(".cls") || Name.contains(".ctz") || Name.contains(".cntb") || Name.contains(".cnth") || Name.contains(".cntw") || Name.contains(".cntd") || Name.contains(".cntp") || Name.contains(".cntsb") || Name.contains(".cntsh") || Name.contains(".cntsw") || Name.contains(".cntsd"))
        {
            unsigned base = 3;  // Matches ctpop=3
            if (Name.contains(".cntp") || Name.contains(".cntsb") || Name.contains(".cntsh") || Name.contains(".cntsw") || Name.contains(".cntsd")) {
                // Fixed cost for these
                return 3;
            } else {
                scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
            }
        }

        // Bit reversal (rbit, revd)
        if (Name.contains(".rbit") || Name.contains(".revd"))
        {
            unsigned base = 1;  // Matches bitwise=1
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // First Fault Register (FFR) operations (rdffr, wrffr, setffr)
        if (Name.contains(".rdffr") || Name.contains(".wrffr") || Name.contains(".setffr"))
        {
            unsigned base = 1;  // Low like basic ops
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        if (Name.contains(".sel") || Name.contains(".sel_x2") || Name.contains(".sel_x4") || Name.contains(".pmov_to_pred_lane") || Name.contains(".pmov_to_pred_lane_zero") || Name.contains(".pmov_to_vector_lane_merging") || Name.contains(".pmov_to_vector_lane_zeroing"))
        {
            unsigned base = 2;  // Matches Select=2
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Default for any unhandled SVE intrinsic (scale linearly with default base=4 like FMul approx)
        if (!scaledCost)
        {
            unsigned base = 4;
            scaledCost = Builder.CreateMul(numElements, ConstantInt::get(I64Ty, base + (sensitive ? typeAdj : 0)));
        }

        // Attach metadata to scaledCost once, if it's an instruction
        if (auto* Inst = dyn_cast<Instruction>(scaledCost))
            Inst->setMetadata("op_sig", OpSigMD);
    
        // Insert dynamic cost addition to fuel counter
        Module *M = CI->getModule();
        LLVMContext &Context = M->getContext();
        GlobalVariable *ThreadLocalFuelGlobal = cast<GlobalVariable>(M->getOrInsertGlobal("__thread_local_fuel_used", I64Ty));
        ThreadLocalFuelGlobal->setLinkage(GlobalValue::ExternalLinkage);
        ThreadLocalFuelGlobal->setThreadLocal(true);
    
        LoadInst *CurrentFuel = Builder.CreateLoad(I64Ty, ThreadLocalFuelGlobal);
        if (auto *Inst = dyn_cast<Instruction>(CurrentFuel))
            Inst->setMetadata("op_sig", OpSigMD);
    
        Value *NewFuel = Builder.CreateAdd(CurrentFuel, scaledCost);
        if (auto *Inst = dyn_cast<Instruction>(NewFuel))
            Inst->setMetadata("op_sig", OpSigMD);
    
        StoreInst *StoreFuel = Builder.CreateStore(NewFuel, ThreadLocalFuelGlobal);
        if (auto *Inst = dyn_cast<Instruction>(StoreFuel))
            Inst->setMetadata("op_sig", OpSigMD);
    
        return 0;  // Static cost handled dynamically
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
        return 3;
    }
    if (Name.starts_with("llvm.fmuladd")) {
        if (Name.contains(".v2")) return 8;
        if (Name.contains(".v4")) return 12;
        if (Name.contains(".v8")) return 16;
        return 7;
    }

    if (Name.starts_with("llvm.cache.flush") || Name.starts_with("llvm.cache.invalidate")) {
        return 15;
    }

    if (Name.starts_with("llvm.stackprobe")) {
        return 5;
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
                IRBuilder<> Builder(Call); 
                StringRef Name = Callee->getName();       
                if (Name.starts_with("llvm.memcpy") || Name.starts_with("llvm.memmove") || Name.starts_with("llvm.memset") || Name.starts_with("llvm.memcmp"))
                {
                    return getMemoryIntrinsicCostAArch64(Call, Name, Builder, OpSigMD);
                }

                if (unsigned cost = getIntrinsicCostAArch64(Callee->getName(), Builder, Call, OpSigMD))
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