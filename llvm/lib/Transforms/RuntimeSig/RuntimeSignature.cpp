#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "RuntimeSignature.h"
#include <cstdlib>
#include "llvm/IR/Dominators.h"
#include "llvm/IR/CFG.h"

using namespace llvm;

static bool isInstructionSafe(const Instruction &I)
{
    // Only filter truly unsafe operations
    if (isa<FenceInst>(&I))
        return false;

    // Skip volatile memory operations as they might have special semantics
    if (auto* MI = dyn_cast<LoadInst>(&I))
        if (MI->isVolatile())
            return false;

    if (auto* MI = dyn_cast<StoreInst>(&I))
        if (MI->isVolatile())
            return false;

    // Skip vector operations that might be converted to NEON/SVE
    /*if (auto* VL = dyn_cast<VectorType>(I.getType())) {
        unsigned Width = VL->getPrimitiveSizeInBits();
        if (Width != 64 && Width != 128)
            return false;
    }*/

    return true;
}

// Define specific primes for each instruction type
struct InstructionPrimes {
    // Integer arithmetic - Unsigned 32-bit
    static constexpr uint64_t Add_u32 = 0xEDC1A7F47ACD2EE3;
    static constexpr uint64_t Sub_u32 = 0x87353D33C2976737;
    static constexpr uint64_t Mul_u32 = 0x91C6A1D9B4EDC1AB;
    static constexpr uint64_t Div_u32 = 0xFD9B02FDC35C72B3;
    static constexpr uint64_t URem_u32 = 0xB161AB7FB04F004D;
    
    // Integer arithmetic - Signed 32-bit
    static constexpr uint64_t Add_s32 = 0xE620952D37DDFF91;
    static constexpr uint64_t Sub_s32 = 0xC4FA795EB531A38B;
    static constexpr uint64_t Mul_s32 = 0x8F28A03020BF8813;
    static constexpr uint64_t Div_s32 = 0xD80622EB6110253F;
    static constexpr uint64_t SRem_s32 = 0xC56DD2999CD1234F;

    // Integer arithmetic - Unsigned 64-bit
    static constexpr uint64_t Add_u64 = 0xBE8098FE5CFCC7B5;
    static constexpr uint64_t Sub_u64 = 0xE155AD1E824B25E3;
    static constexpr uint64_t Mul_u64 = 0x9A16DFEE16D4BBF5;
    static constexpr uint64_t Div_u64 = 0xF5F2BCF367F68ED1;
    static constexpr uint64_t URem_u64 = 0x96E8220B9C5E8C97;

    // Integer arithmetic - Signed 64-bit
    static constexpr uint64_t Add_s64 = 0x9BD972EB4677C3F1;
    static constexpr uint64_t Sub_s64 = 0xF2D231CD2E1FBA23;
    static constexpr uint64_t Mul_s64 = 0xE511FE4AEA87A429;
    static constexpr uint64_t Div_s64 = 0xA2E613C950EA7F17;
    static constexpr uint64_t SRem_s64 = 0xF0FB61E6281360FD;

    // Floating point - Single precision
    static constexpr uint64_t Add_f32 = 0x90B2AD995AF897CD;
    static constexpr uint64_t Sub_f32 = 0xC256B33A0EF0745F;
    static constexpr uint64_t Mul_f32 = 0x9AF1495384190163;
    static constexpr uint64_t Div_f32 = 0x966C40D3884717FF;
    static constexpr uint64_t Rem_f32 = 0x918C3A31D782B0E5;
    static constexpr uint64_t Sqrt_f32 = 0xECE9FBD3A6D77023;
    static constexpr uint64_t FNeg_f32 = 0xC002087960B604D5;

    // Floating point - Double precision
    static constexpr uint64_t Add_f64 = 0xC3EA137E165AECED;
    static constexpr uint64_t Sub_f64 = 0x8C4AAED901B0472B;
    static constexpr uint64_t Mul_f64 = 0x883C081D0CDE3065;
    static constexpr uint64_t Div_f64 = 0xFED4DB903BB79241;
    static constexpr uint64_t Rem_f64 = 0xC6051FFCF3752D2B;
    static constexpr uint64_t Sqrt_f64 = 0xb79249404c3bc015;
    static constexpr uint64_t FNeg_f64 = 0xE94FDBF317D00AB7;

    // Vector operations - element-wise
    static constexpr uint64_t Add_vec_i8 = 0x83edeae76b4b0545;
    static constexpr uint64_t Add_vec_i16 = 0xbceb49f5ee123215;
    static constexpr uint64_t Add_vec_i32 = 0x9B8C768C1401BA7B;
    static constexpr uint64_t Add_vec_i64 = 0xBC49DE9B3A818899;
    static constexpr uint64_t Add_vec_f32 = 0xA17576DF232E7669;
    static constexpr uint64_t Add_vec_f64 = 0xD298F1C0BF940F95;
    static constexpr uint64_t Mul_vec_i8 = 0x5d1e44ab549dfc51;
    static constexpr uint64_t Mul_vec_i16 = 0x9abf77d2ec41ae40;
    static constexpr uint64_t Mul_vec_i32 = 0xAE6A290706DD70E7;
    static constexpr uint64_t Mul_vec_i64 = 0xD4FDC03C4401C533;
    static constexpr uint64_t Mul_vec_f32 = 0xA3E6942DA60A926F;
    static constexpr uint64_t Mul_vec_f64 = 0xBCB5CD76C9DC3253;

    // Bitwise operations
    static constexpr uint64_t And_i8 = 0xb140c30437087f6a;
    static constexpr uint64_t And_i16 = 0xc76462d73bbe7cbe;
    static constexpr uint64_t And_i32 = 0x80B429DFBFC318EB;
    static constexpr uint64_t And_i64 = 0xBDA9ECF782F0D3FB;
    static constexpr uint64_t And_vec_i8 = 0x4c350adc099a1bfc;
    static constexpr uint64_t And_vec_i16 = 0xe456007c0cf46ec7;
    static constexpr uint64_t And_vec_i32 = 0x99c157351c99736d;
    static constexpr uint64_t And_vec_i64 = 0xc5d66049960c766e;
    static constexpr uint64_t Or_i8 = 0xf34d856aed6bc4ad;
    static constexpr uint64_t Or_i16 = 0x4f9929353fc5f4d4;
    static constexpr uint64_t Or_i32 = 0xB8290AAC45720989;
    static constexpr uint64_t Or_i64 = 0xD0C39B467979E695;
    static constexpr uint64_t Or_vec_i8 = 0x25c5797689c9686d;
    static constexpr uint64_t Or_vec_i16 = 0xa2eac591940b98a7;
    static constexpr uint64_t Or_vec_i32 = 0x9417356a48354c1a;
    static constexpr uint64_t Or_vec_i64 = 0x49e64a1fad3da826;
    static constexpr uint64_t Xor_i8 = 0x689037a101804223;
    static constexpr uint64_t Xor_i16 = 0x0f118fc766622ba9;
    static constexpr uint64_t Xor_i32 = 0xE12B4AF73AAABEEF;
    static constexpr uint64_t Xor_i64 = 0xC1B8CDB5496BAFD7;
    static constexpr uint64_t Xor_vec_i8 = 0x6230a2fffd9b7cd5;
    static constexpr uint64_t Xor_vec_i16 = 0x51e5587912d014d8;
    static constexpr uint64_t Xor_vec_i32 = 0x5c92d5e3d8dec0a8;
    static constexpr uint64_t Xor_vec_i64 = 0x5b65623373691196;
    static constexpr uint64_t Shl_i8 = 0xcb71176a5fda261a;
    static constexpr uint64_t Shl_i16 = 0x4b3ba5cdbadb541e;
    static constexpr uint64_t Shl_i32 = 0x88330E6E1BFA5411;
    static constexpr uint64_t Shl_i64 = 0x8374F43D807A6F91;
    static constexpr uint64_t Shl_vec_i8 = 0xd376b899abe4c00d;
    static constexpr uint64_t Shl_vec_i16 = 0x38d3b8afe32bec91;
    static constexpr uint64_t Shl_vec_i32 = 0x6bbcf9501dbe1c6b;
    static constexpr uint64_t Shl_vec_i64 = 0x91a76a465de9d163;
    static constexpr uint64_t LShr_i8 = 0x554668ed91a7ec2d;
    static constexpr uint64_t LShr_i16 = 0xd094dd15a208d797;
    static constexpr uint64_t LShr_i32 = 0x21cf38cc21ec1aa8;
    static constexpr uint64_t LShr_i64 = 0xb8b42b0acea289f4;
    static constexpr uint64_t LShr_vec_i8 = 0x0e6b9540fead6e5e;
    static constexpr uint64_t LShr_vec_i16 = 0xe80025d702be8e2a;
    static constexpr uint64_t LShr_vec_i32 = 0x560da37e0a8108af;
    static constexpr uint64_t LShr_vec_i64 = 0x8afe29d9ed33cc71;
    static constexpr uint64_t AShr_i32 = 0xCEDC3D307D8D8683;
    static constexpr uint64_t AShr_i64 = 0x9DA4540AE2D7A161;

    // Memory operations
    static constexpr uint64_t Load_i32 = 0x9A9E961B54B29955;
    static constexpr uint64_t Load_i64 = 0x8B1B3D4D77BC7BB3;
    static constexpr uint64_t Load_f32 = 0xC161CEDAF256D5E5;
    static constexpr uint64_t Load_f64 = 0xD1E5DA2FDCD5E157;
    static constexpr uint64_t Store_i32 = 0x986CCCCFA9F5B10B;
    static constexpr uint64_t Store_i64 = 0x8CC7C690FF547D63;
    static constexpr uint64_t Store_f32 = 0x80FF2A3B14A4D19D;
    static constexpr uint64_t Store_f64 = 0xE0E9526F53C79197;

    // Comparison operations - Integer
    static constexpr uint64_t ICmp_eq = 0xB19319E767B773DF;
    static constexpr uint64_t ICmp_ne = 0x8AC38410037C32D5;
    static constexpr uint64_t ICmp_ugt = 0xAB0A8BAC52D5D76B;
    static constexpr uint64_t ICmp_uge = 0x9CDBFC00628D628B;
    static constexpr uint64_t ICmp_ult = 0x84378B096D13B6A3;
    static constexpr uint64_t ICmp_ule = 0xC04FBAAA56FA0DAB;
    static constexpr uint64_t ICmp_sgt = 0xCE9AA2EB4B22456B;
    static constexpr uint64_t ICmp_sge = 0xF165D7826D16DF47;
    static constexpr uint64_t ICmp_slt = 0xE95BA56F275D3EC5;
    static constexpr uint64_t ICmp_sle = 0x8C3F2F6F2EB0C34F;

    // Comparison operations - Floating point
    static constexpr uint64_t FCmp_oeq = 0xBF2A12007525FEED;
    static constexpr uint64_t FCmp_ogt = 0xF3A8D718FEC1B393;
    static constexpr uint64_t FCmp_oge = 0xF4692B3CA0566779;
    static constexpr uint64_t FCmp_olt = 0xADB80126D86C7295;
    static constexpr uint64_t FCmp_ole = 0xE6FD4C5BCBC70E3D;
    static constexpr uint64_t FCmp_one = 0xD89E33A78DBF9527;
    static constexpr uint64_t FCmp_ord = 0x8DB562EEC3BBD64F;
    static constexpr uint64_t FCmp_uno = 0xDA31E485E5E0C445;
    static constexpr uint64_t FCmp_ueq = 0xBFA9439C30B70919;
    static constexpr uint64_t FCmp_ugt = 0xDF9DC483E11B08A5;

    // Type conversions
    static constexpr uint64_t Trunc_i64_to_i32 = 0xc157d2bc1841b32d;
    static constexpr uint64_t ZExt_i32_to_i64 = 0x01c38c3e379c1db7;
    static constexpr uint64_t SExt_i32_to_i64 = 0xd69987668e5f1517;
    static constexpr uint64_t FPToUI_f32_to_i32 = 0x5654ebd597d15493;
    static constexpr uint64_t FPToUI_f32_to_i64 = 0xbea6044ca4f7e3d3;
    static constexpr uint64_t FPToUI_f64_to_i32 = 0xaf66598270b3937e;
    static constexpr uint64_t FPToUI_f64_to_i64 = 0x11853d813fe9d747;
    static constexpr uint64_t FPToSI_f32_to_i32 = 0xdeaf14c76e1b670e;
    static constexpr uint64_t FPToSI_f32_to_i64 = 0xfdf1eed19c4ba915;
    static constexpr uint64_t FPToSI_f64_to_i32 = 0x9bb1ac44de1f0299;
    static constexpr uint64_t FPToSI_f64_to_i64 = 0x9ccfd4dcc996c4dd;
    static constexpr uint64_t UIToFP_i32_to_f32 = 0x0d031923e7751ecf;
    static constexpr uint64_t UIToFP_i32_to_f64 = 0x1f03a9302f2654eb;
    static constexpr uint64_t UIToFP_i64_to_f32 = 0x09e828d0158ded05;
    static constexpr uint64_t UIToFP_i64_to_f64 = 0xa5508ff6a6b5d75d;
    static constexpr uint64_t SIToFP_i32_to_f32 = 0x08e0d8dfd70c8e28;
    static constexpr uint64_t SIToFP_i32_to_f64 = 0x464987723c37a486;
    static constexpr uint64_t SIToFP_i64_to_f32 = 0x2f1802507704db24;
    static constexpr uint64_t SIToFP_i64_to_f64 = 0x8e6e4de903862eb0;
    static constexpr uint64_t FPTrunc_f64_to_f32 = 0xd3a76d54e8c32463;
    static constexpr uint64_t FPExt_f32_to_f64 = 0xa3aced487c2b9330;

    // Vector operations - expanded set
    static constexpr uint64_t Sub_vec_i8 = 0xc5a4ba7d64c411da;
    static constexpr uint64_t Sub_vec_i16 = 0xb19dcef9643b439a;
    static constexpr uint64_t Sub_vec_i32 = 0xee0e7735e2767f66;
    static constexpr uint64_t Sub_vec_i64 = 0x2fbce2bc27f8586d;
    static constexpr uint64_t Sub_vec_f32 = 0x674330efde9e6e15;
    static constexpr uint64_t Sub_vec_f64 = 0x303b3241ee12230;
    static constexpr uint64_t Div_vec_i32 = 0xa61f9cfce45cc4a;
    static constexpr uint64_t Div_vec_i64 = 0x57ad025f97f19f8f;
    static constexpr uint64_t Div_vec_f32 = 0x303f9f52f0bea70c;
    static constexpr uint64_t Div_vec_f64 = 0x66bfad11704cb2da;

    // Atomic operations
    static constexpr uint64_t AtomicCmpXchg = 0x70eb96187f92b2fc;
    static constexpr uint64_t AtomicRMW_Add = 0x4c9e5d30f7d8424d;
    static constexpr uint64_t AtomicRMW_Sub = 0x0111dd2172836489;
    static constexpr uint64_t AtomicRMW_And = 0xad3e73a907240be0;
    static constexpr uint64_t AtomicRMW_Or = 0xde8d961405f75f8c;
    static constexpr uint64_t AtomicRMW_Xor = 0xd8c6cfa750f2925f;

    // Add call context primes
    static constexpr uint64_t DirectCall = 0x995e5c890782aab9;
    static constexpr uint64_t IndirectCall = 0xd24493956d09679c;
    static constexpr uint64_t TailCall = 0x1f52cad6fe2f6bcb;
    static constexpr uint64_t IntrinsicCall = 0x2ac34aedb01149f7;
};

struct FunctionContext {
    static constexpr uint64_t EntryBlock = 0x69c283054788c274;
    static constexpr uint64_t ExitBlock = 0xade60f68c74890a4;
    static constexpr uint64_t LoopHeader = 0x2f9e52dd35eb08fd;
    static constexpr uint64_t ExceptionHandler = 0xe87726bdac198e95;
    static constexpr uint64_t CallSite = 0xc0720e93e225a024;
    static constexpr uint64_t BranchTarget = 0xce461ea97423cc75;
    static constexpr uint64_t SwitchTarget = 0x140a55fc515ad16a;
    static constexpr uint64_t IndirectBranch = 0x7735c5c55b18dc5d;
    static constexpr uint64_t LoopBackEdge = 0xba646e190bfc39fd;
    static constexpr uint64_t CriticalEdge = 0x6ad18c93102b4361;
    static constexpr uint64_t InvokeTarget = 0x2aa086acbc74a62e;
    static constexpr uint64_t CallTarget = 0xf5624d5dfe14070f;
    static constexpr uint64_t CatchSwitchTarget = 0x1baa8e0dc432a39a;
    static constexpr uint64_t CleanupPadTarget = 0x2c34adfa542d7b97;
    static constexpr uint64_t CatchPadTarget = 0x13c0dc3c1fcf510e;
    static constexpr uint64_t CatchReturnTarget = 0xc5098a28e279e0b9;
    static constexpr uint64_t CleanupReturnTarget = 0x7ef4a8490515f992;
};

// Helper function to rotate a 64-bit value left
static uint64_t rotateLeft(uint64_t x, unsigned int n) {
    return (x << n) | (x >> (64 - n));
}

// Helper function to rotate a 64-bit value right
static uint64_t rotateRight(uint64_t x, unsigned int n) {
    return (x >> n) | (x << (64 - n));
}

// Modified position modifier to create rotation amounts
static std::pair<unsigned int, unsigned int> getPositionModifier(const Instruction* I, const Function* F)
{
    uint64_t BBIdx = 0;
    uint64_t InstIdx = 0;
    uint64_t FuncHash = hash_value(F->getName());
    
    for (auto &BB : *F)
    {
        if (&BB == I->getParent())
            break;
        BBIdx++;
    }
    
    for (auto &Inst : *I->getParent())
    {
        if (&Inst == I)
            break;
        InstIdx++;
    }
    
    const BasicBlock& BB = *I->getParent();
    bool isInEntryBlock = (&BB == &F->getEntryBlock());
    bool isInExitBlock = (BB.getTerminator() && BB.getTerminator()->getNumSuccessors() == 0);
    //bool isInLoop = DT.isReachableFromEntry(&BB) && LI.getLoopFor(&BB);
    bool isInExceptionHandler = BB.isEHPad();
    
    bool isBranchTarget = false;
    bool isSwitchTarget = false;
    bool isIndirectTarget = false;
    bool isLoopBackEdge = false;
    bool isCriticalEdge = false;
    bool isInvokeTarget = false;
    bool isCallTarget = false;
    bool isCatchSwitchTarget = false;
    bool isCleanupPadTarget = false;
    bool isCatchPadTarget = false;
    bool isCatchReturnTarget = false;
    bool isCleanupReturnTarget = false;
    for (const_pred_iterator PI = pred_begin(&BB), E = pred_end(&BB); PI != E; ++PI) {
        const BasicBlock &Pred = **PI;
        if (auto* BI = dyn_cast<BranchInst>(Pred.getTerminator()))
            isBranchTarget = true;
        if (auto* SI = dyn_cast<SwitchInst>(Pred.getTerminator()))
            isSwitchTarget = true;
        if (auto* IBI = dyn_cast<IndirectBrInst>(Pred.getTerminator()))
            isIndirectTarget = true;
        if (auto* II = dyn_cast<InvokeInst>(Pred.getTerminator()))
            isInvokeTarget = true;
        if (auto* LBI = dyn_cast<CallBrInst>(Pred.getTerminator()))
            isCallTarget = true;
        if (auto* LBI = dyn_cast<CatchSwitchInst>(Pred.getTerminator()))
            isCatchSwitchTarget = true;
        if (auto* LBI = dyn_cast<CleanupPadInst>(Pred.getTerminator()))
            isCleanupPadTarget = true;
        if (auto* LBI = dyn_cast<CatchPadInst>(Pred.getTerminator()))
            isCatchPadTarget = true;
        if (auto* LBI = dyn_cast<CatchReturnInst>(Pred.getTerminator()))
            isCatchReturnTarget = true;
        if (auto* LBI = dyn_cast<CleanupReturnInst>(Pred.getTerminator()))
            isCleanupReturnTarget = true;
    }

    uint64_t contextHash = 0;
    if (isInEntryBlock) contextHash = rotateLeft(contextHash ^ FunctionContext::EntryBlock, 43);
    if (isInExitBlock) contextHash = rotateLeft(contextHash ^ FunctionContext::ExitBlock, 43);
    if (isBranchTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::BranchTarget, 43);
    if (isSwitchTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::SwitchTarget, 43);
    if (isIndirectTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::IndirectBranch, 43);
    if (isLoopBackEdge) contextHash = rotateLeft(contextHash ^ FunctionContext::LoopBackEdge, 43);
    if (isCriticalEdge) contextHash = rotateLeft(contextHash ^ FunctionContext::CriticalEdge, 43);
    if (isInExceptionHandler) contextHash = rotateLeft(contextHash ^ FunctionContext::ExceptionHandler, 43);
    if (isInvokeTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::InvokeTarget, 43);
    if (isCallTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CallTarget, 43);
    if (isCatchSwitchTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CatchSwitchTarget, 43);
    if (isCleanupPadTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CleanupPadTarget, 43);
    if (isCatchPadTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CatchPadTarget, 43);
    if (isCatchReturnTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CatchReturnTarget, 43);
    if (isCleanupReturnTarget) contextHash = rotateLeft(contextHash ^ FunctionContext::CleanupReturnTarget, 43);
    
    unsigned int leftRot = ((BBIdx * 7 + InstIdx * 13 + FuncHash * 17 + contextHash * 23) & 0x3F);
    unsigned int rightRot = ((BBIdx * 11 + InstIdx * 17 + FuncHash * 23 + contextHash * 29) & 0x3F);
    
    return {leftRot, rightRot};
}

// Modified helper functions to use rotations
static uint64_t getBinaryOpPrime(const BinaryOperator* BO) {
    Type* Ty = BO->getType();
    bool isInt8 = Ty->isIntegerTy(8);
    bool isInt16 = Ty->isIntegerTy(16);
    bool isInt32 = Ty->isIntegerTy(32);
    bool isInt64 = Ty->isIntegerTy(64);
    bool isVector = Ty->isVectorTy();
    
    uint64_t prime = 0;
    switch (BO->getOpcode()) {
        case Instruction::Add:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Add_s32 : InstructionPrimes::Add_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Add_s64 : InstructionPrimes::Add_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Add_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Add_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Add_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Add_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Add_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Add_vec_i64;
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Add_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Add_vec_f64;
            }
            break;

        case Instruction::Sub:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Sub_s32 : InstructionPrimes::Sub_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Sub_s64 : InstructionPrimes::Sub_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Sub_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Sub_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Sub_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Sub_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Sub_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Sub_vec_i64;
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Sub_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Sub_vec_f64;
            }
            break;

        case Instruction::Mul:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Mul_s32 : InstructionPrimes::Mul_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Mul_s64 : InstructionPrimes::Mul_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Mul_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Mul_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Mul_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Mul_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Mul_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Mul_vec_i64;
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Mul_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Mul_vec_f64;
            }
            break;

        case Instruction::FAdd:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Add_s32 : InstructionPrimes::Add_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Add_s64 : InstructionPrimes::Add_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Add_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Add_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Add_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Add_vec_f64;
            }
            break;

        case Instruction::FSub:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Sub_s32 : InstructionPrimes::Sub_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Sub_s64 : InstructionPrimes::Sub_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Sub_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Sub_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Sub_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Sub_vec_f64;
            }
            break;

        case Instruction::FMul:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Mul_s32 : InstructionPrimes::Mul_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Mul_s64 : InstructionPrimes::Mul_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Mul_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Mul_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Mul_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Mul_vec_f64;
            }
            break;

        case Instruction::FDiv:
            if (isInt32) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Div_s32 : InstructionPrimes::Div_u32;
            if (isInt64) prime = BO->hasNoSignedWrap() ? InstructionPrimes::Div_s64 : InstructionPrimes::Div_u64;
            if (Ty->isFloatTy()) prime = InstructionPrimes::Div_f32;
            if (Ty->isDoubleTy()) prime = InstructionPrimes::Div_f64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Div_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Div_vec_i64;
                if (ElemTy->isFloatTy()) prime = InstructionPrimes::Div_vec_f32;
                if (ElemTy->isDoubleTy()) prime = InstructionPrimes::Div_vec_f64;
            }
            break;

        case Instruction::SDiv:
            if (isInt32) prime = InstructionPrimes::Div_s32;
            if (isInt64) prime = InstructionPrimes::Div_s64;
            break;

        case Instruction::UDiv:
            if (isInt32) prime = InstructionPrimes::Div_u32;
            if (isInt64) prime = InstructionPrimes::Div_u64;
            break;

        case Instruction::SRem:
            if (isInt32) prime = InstructionPrimes::SRem_s32;
            if (isInt64) prime = InstructionPrimes::SRem_s64;
            break;

        case Instruction::URem:
            if (isInt32) prime = InstructionPrimes::URem_u32;
            if (isInt64) prime = InstructionPrimes::URem_u64;
            break;

        case Instruction::And:
            if (isInt8) prime = InstructionPrimes::And_i8;
            if (isInt16) prime = InstructionPrimes::And_i16;
            if (isInt32) prime = InstructionPrimes::And_i32;
            if (isInt64) prime = InstructionPrimes::And_i64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::And_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::And_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::And_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::And_vec_i64;
            }
            break;
        case Instruction::Or:
            if (isInt8) prime = InstructionPrimes::Or_i8;
            if (isInt16) prime = InstructionPrimes::Or_i16;
            if (isInt32) prime = InstructionPrimes::Or_i32;
            if (isInt64) prime = InstructionPrimes::Or_i64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Or_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Or_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Or_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Or_vec_i64;
            }
            break;
        case Instruction::Xor:
            if (isInt8) prime = InstructionPrimes::Xor_i8;
            if (isInt16) prime = InstructionPrimes::Xor_i16;
            if (isInt32) prime = InstructionPrimes::Xor_i32;
            if (isInt64) prime = InstructionPrimes::Xor_i64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Xor_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Xor_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Xor_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Xor_vec_i64;
            }
            break;
        case Instruction::Shl:
            if (isInt8) prime = InstructionPrimes::Shl_i8;
            if (isInt16) prime = InstructionPrimes::Shl_i16;
            if (isInt32) prime = InstructionPrimes::Shl_i32;
            if (isInt64) prime = InstructionPrimes::Shl_i64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::Shl_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::Shl_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::Shl_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::Shl_vec_i64;
            }
            break;
        case Instruction::LShr:
            if (isInt8) prime = InstructionPrimes::LShr_i8;
            if (isInt16) prime = InstructionPrimes::LShr_i16;
            if (isInt32) prime = InstructionPrimes::LShr_i32;
            if (isInt64) prime = InstructionPrimes::LShr_i64;
            if (isVector) {
                auto* VT = cast<VectorType>(Ty);
                Type* ElemTy = VT->getElementType();
                if (ElemTy->isIntegerTy(8)) prime = InstructionPrimes::LShr_vec_i8;
                if (ElemTy->isIntegerTy(16)) prime = InstructionPrimes::LShr_vec_i16;
                if (ElemTy->isIntegerTy(32)) prime = InstructionPrimes::LShr_vec_i32;
                if (ElemTy->isIntegerTy(64)) prime = InstructionPrimes::LShr_vec_i64;
            }
            break;
        case Instruction::AShr:
            if (isInt32) prime = InstructionPrimes::AShr_i32;
            if (isInt64) prime = InstructionPrimes::AShr_i64;
            break;

        default:
            return 0;
    }
    
    return prime;
}

static uint64_t getCmpPrime(const CmpInst* CI)
{
    if (auto* ICmp = dyn_cast<ICmpInst>(CI))
    {
        switch (ICmp->getPredicate())
        {
            case ICmpInst::ICMP_EQ:  return InstructionPrimes::ICmp_eq;
            case ICmpInst::ICMP_NE:  return InstructionPrimes::ICmp_ne;
            case ICmpInst::ICMP_UGT: return InstructionPrimes::ICmp_ugt;
            case ICmpInst::ICMP_UGE: return InstructionPrimes::ICmp_uge;
            case ICmpInst::ICMP_ULT: return InstructionPrimes::ICmp_ult;
            case ICmpInst::ICMP_ULE: return InstructionPrimes::ICmp_ule;
            case ICmpInst::ICMP_SGT: return InstructionPrimes::ICmp_sgt;
            case ICmpInst::ICMP_SGE: return InstructionPrimes::ICmp_sge;
            case ICmpInst::ICMP_SLT: return InstructionPrimes::ICmp_slt;
            case ICmpInst::ICMP_SLE: return InstructionPrimes::ICmp_sle;
            default: return 0;
        }
    }
    else if (auto* FCmp = dyn_cast<FCmpInst>(CI))
    {
        switch (FCmp->getPredicate())
        {
            case FCmpInst::FCMP_OEQ: return InstructionPrimes::FCmp_oeq;
            case FCmpInst::FCMP_OGT: return InstructionPrimes::FCmp_ogt;
            case FCmpInst::FCMP_OLT: return InstructionPrimes::FCmp_olt;
            default: return 0;
        }
    }
    
    return 0;
}

static uint64_t getCastPrime(const CastInst* CI)
{
    Type* SrcTy = CI->getSrcTy();
    Type* DstTy = CI->getDestTy();
    
    switch (CI->getOpcode())
    {
        case Instruction::Trunc:
            /*if (SrcTy->isIntegerTy(64) && DstTy->isIntegerTy(32))
                return InstructionPrimes::Trunc_i64_to_i32;*/
            break;

        case Instruction::ZExt:
            /*if (SrcTy->isIntegerTy(32) && DstTy->isIntegerTy(64))
                return InstructionPrimes::ZExt_i32_to_i64;*/
            break;

        case Instruction::SExt:
            /*if (SrcTy->isIntegerTy(32) && DstTy->isIntegerTy(64))
                return InstructionPrimes::SExt_i32_to_i64;*/
            break;

        case Instruction::PtrToInt:
            /*if (DstTy->isIntegerTy(64))
                return InstructionPrimes::ZExt_i32_to_i64;*/
            break;

        case Instruction::IntToPtr:
            /*if (SrcTy->isIntegerTy(64))
                return InstructionPrimes::Trunc_i64_to_i32;*/
            break;

        case Instruction::FPToUI:
            if (SrcTy->isFloatTy() && DstTy->isIntegerTy(32))
                return InstructionPrimes::FPToUI_f32_to_i32;
            if (SrcTy->isFloatTy() && DstTy->isIntegerTy(64))
                return InstructionPrimes::FPToUI_f32_to_i64;
            if (SrcTy->isDoubleTy() && DstTy->isIntegerTy(32))
                return InstructionPrimes::FPToUI_f64_to_i32;
            if (SrcTy->isDoubleTy() && DstTy->isIntegerTy(64))
                return InstructionPrimes::FPToUI_f64_to_i64;
            break;

        case Instruction::FPToSI:
            if (SrcTy->isFloatTy() && DstTy->isIntegerTy(32))
                return InstructionPrimes::FPToSI_f32_to_i32;
            if (SrcTy->isFloatTy() && DstTy->isIntegerTy(64))
                return InstructionPrimes::FPToSI_f32_to_i64;
            if (SrcTy->isDoubleTy() && DstTy->isIntegerTy(32))
                return InstructionPrimes::FPToSI_f64_to_i32;
            if (SrcTy->isDoubleTy() && DstTy->isIntegerTy(64))
                return InstructionPrimes::FPToSI_f64_to_i64;
            break;

        case Instruction::UIToFP:
            if (SrcTy->isIntegerTy(32) && DstTy->isFloatTy())
                return InstructionPrimes::UIToFP_i32_to_f32;
            if (SrcTy->isIntegerTy(32) && DstTy->isDoubleTy())
                return InstructionPrimes::UIToFP_i32_to_f64;
            if (SrcTy->isIntegerTy(64) && DstTy->isFloatTy())
                return InstructionPrimes::UIToFP_i64_to_f32;
            if (SrcTy->isIntegerTy(64) && DstTy->isDoubleTy())
                return InstructionPrimes::UIToFP_i64_to_f64;
            break;

        case Instruction::SIToFP:
            if (SrcTy->isIntegerTy(32) && DstTy->isFloatTy())
                return InstructionPrimes::SIToFP_i32_to_f32;
            if (SrcTy->isIntegerTy(32) && DstTy->isDoubleTy())
                return InstructionPrimes::SIToFP_i32_to_f64;
            if (SrcTy->isIntegerTy(64) && DstTy->isFloatTy())
                return InstructionPrimes::SIToFP_i64_to_f32;
            if (SrcTy->isIntegerTy(64) && DstTy->isDoubleTy())
                return InstructionPrimes::SIToFP_i64_to_f64;
            break;

        case Instruction::FPTrunc:
            if (SrcTy->isDoubleTy() && DstTy->isFloatTy())
                return InstructionPrimes::FPTrunc_f64_to_f32;
            break;

        case Instruction::FPExt:
            if (SrcTy->isFloatTy() && DstTy->isDoubleTy())
                return InstructionPrimes::FPExt_f32_to_f64;
            break;

        default:
            return 0;
    }
    
    return 0;
}

static uint64_t getAtomicPrime(const Instruction* I)
{
    if (auto* CmpXchg = dyn_cast<AtomicCmpXchgInst>(I))
    {
        Type* ValTy = CmpXchg->getNewValOperand()->getType();
        if (ValTy->isVectorTy())
            return 0;

        return InstructionPrimes::AtomicCmpXchg;
    }
    else if (auto* RMW = dyn_cast<AtomicRMWInst>(I))
    {
        Type* ValTy = RMW->getValOperand()->getType();
        if (ValTy->isVectorTy())
            return 0;

        switch (RMW->getOperation())
        {
            case AtomicRMWInst::Add: return InstructionPrimes::AtomicRMW_Add;
            case AtomicRMWInst::Sub: return InstructionPrimes::AtomicRMW_Sub;
            case AtomicRMWInst::And: return InstructionPrimes::AtomicRMW_And;
            case AtomicRMWInst::Or:  return InstructionPrimes::AtomicRMW_Or;
            case AtomicRMWInst::Xor: return InstructionPrimes::AtomicRMW_Xor;
            default: return 0;
        }
    }
    return 0;
}

class OperandsRuntimeSignaturePass : public PassInfoMixin<OperandsRuntimeSignaturePass>
{
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
    {
        LLVMContext &Context = M.getContext();
        IRBuilder<> Builder(Context);

        const char* isFirstSrcStr = std::getenv("IS_FIRST_SRC");
        int isFirstSrc = isFirstSrcStr ? std::atoi(isFirstSrcStr) : 0;

        GlobalVariable *RuntimeSigGlobal;
        if (isFirstSrc)
        {
            RuntimeSigGlobal = new GlobalVariable(M, 
                Type::getInt64Ty(Context),
                false,
                GlobalValue::ExternalLinkage,
                ConstantInt::get(Type::getInt64Ty(Context), 0x4C4C4D564F4C4C4C),
                "__runtime_signature");
        }
        else
        {
            RuntimeSigGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__runtime_signature", Type::getInt64Ty(Context)));
            RuntimeSigGlobal->setLinkage(GlobalValue::ExternalLinkage);
        }

        MDNode *OpSigMD = MDNode::get(Context, {});

        for (auto &F : M)
        {
            if (F.empty() || F.isIntrinsic())
                continue;

            DominatorTree DT(F);
            SmallVector<BasicBlock*, 32> WorkList;

            for (auto &BB : F)
            {
                if (BB.hasAddressTaken())
                    continue;

                bool hasUnsafeInstr = false;
                for (auto &I : BB)
                {
                    if (I.getMetadata("mark_instr") || I.getMetadata("op_sig"))
                        continue;

                    if (auto* Call = dyn_cast<CallInst>(&I))
                    {
                        if (Call->isInlineAsm())
                            continue;
                        if (Call->getCalledFunction() && Call->getCalledFunction()->isIntrinsic())
                            continue;
                    }

                    if (!isInstructionSafe(I))
                    {
                        hasUnsafeInstr = true;
                        break;
                    }

                    /*if (auto* FP = dyn_cast<FPMathOperator>(&I))
                        if (FP->getFastMathFlags().any())
                        {
                            hasUnsafeInstr = true;
                            break;
                        }

                    if (I.mayReadOrWriteMemory() && !isa<LoadInst>(&I) && !isa<StoreInst>(&I))
                    {
                        hasUnsafeInstr = true;
                        break;
                    }*/
                }

                if (!hasUnsafeInstr)
                    WorkList.push_back(&BB);
            }

            std::stable_sort(WorkList.begin(), WorkList.end(),
                [&DT](BasicBlock* A, BasicBlock* B) {
                    return DT.dominates(A, B);
                });

            for (BasicBlock* BB : WorkList)
            {
                uint64_t BlockSig = 0;
                SmallVector<Instruction*, 32> InstrToMark;

                for (auto &I : *BB)
                {
                    if (I.getMetadata("mark_instr") || I.getMetadata("op_sig"))
                        continue;

                    uint64_t InstrSig = 0;
                    auto [leftRot, rightRot] = getPositionModifier(&I, &F);

                    if (auto* BO = dyn_cast<BinaryOperator>(&I))
                        InstrSig = rotateLeft(rotateRight(getBinaryOpPrime(BO), rightRot), leftRot);
                    else if (auto* CI = dyn_cast<CmpInst>(&I))
                        InstrSig = rotateLeft(rotateRight(getCmpPrime(CI), rightRot), leftRot);
                    else if (auto* Cast = dyn_cast<CastInst>(&I))
                        InstrSig = rotateLeft(rotateRight(getCastPrime(Cast), rightRot), leftRot);
                    /*else if (auto* Load = dyn_cast<LoadInst>(&I)) {
                        Type* LoadTy = Load->getType();
                        if (LoadTy->isIntegerTy(32))
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Load_i32, rightRot), leftRot);
                        else if (LoadTy->isIntegerTy(64))
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Load_i64, rightRot), leftRot);
                        else if (LoadTy->isFloatTy())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Load_f32, rightRot), leftRot);
                        else if (LoadTy->isDoubleTy())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Load_f64, rightRot), leftRot);
                    }
                    else if (auto* Store = dyn_cast<StoreInst>(&I)) {
                        Type* StoreTy = Store->getValueOperand()->getType();
                        if (StoreTy->isIntegerTy(32))
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Store_i32, rightRot), leftRot);
                        else if (StoreTy->isIntegerTy(64))
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Store_i64, rightRot), leftRot);
                        else if (StoreTy->isFloatTy())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Store_f32, rightRot), leftRot);
                        else if (StoreTy->isDoubleTy())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::Store_f64, rightRot), leftRot);
                    }*/
                    else if (auto* GEP = dyn_cast<GetElementPtrInst>(&I)) {
                        InstrSig = rotateLeft(rotateRight(InstructionPrimes::Load_i64, rightRot), leftRot);
                    }
                    else if (auto* Call = dyn_cast<CallInst>(&I)) {
                        if (Call->isTailCall())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::TailCall, rightRot), leftRot);
                        else if (Call->isIndirectCall())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::IndirectCall, rightRot), leftRot);
                        else if (Call->getCalledFunction() && Call->getCalledFunction()->isIntrinsic())
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::IntrinsicCall, rightRot), leftRot);
                        else
                            InstrSig = rotateLeft(rotateRight(InstructionPrimes::DirectCall, rightRot), leftRot);
                    }

                    if (InstrSig == 0)
                    {
                        if (isa<AtomicCmpXchgInst>(I) || isa<AtomicRMWInst>(I)) {
                            InstrSig = getAtomicPrime(&I);
                            InstrSig = rotateLeft(rotateRight(InstrSig, rightRot), leftRot);
                        }
                    }

                    if (InstrSig != 0)
                    {
                        BlockSig ^= InstrSig;
                        InstrToMark.push_back(&I);
                    }
                }

                if (BlockSig != 0)
                {
                    Builder.SetInsertPoint(&*BB->getFirstInsertionPt());
                    Value* NewSig = Builder.CreateAtomicRMW(
                        AtomicRMWInst::Xor,
                        RuntimeSigGlobal,
                        ConstantInt::get(Type::getInt64Ty(Context), BlockSig),
                        MaybeAlign(8),
                        AtomicOrdering::Monotonic
                    );

                    if (auto* NewSigInst = dyn_cast<Instruction>(NewSig))
                        NewSigInst->setMetadata("op_sig", OpSigMD);

                    for (Instruction* I : InstrToMark)
                        I->setMetadata("mark_instr", OpSigMD);
                }
            }
        }

        return PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
};

PassPluginLibraryInfo getRuntimeSignaturePluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "RuntimeSignaturePass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB)
        {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>)
                {
                    if (Name == "runtime-signature")
                    {
                        MPM.addPass(OperandsRuntimeSignaturePass());
                        return true;
                    }
                    
                    return false;
                }
            );

            const char* forceRunPassesStr = std::getenv("FORCE_RUN_PASSES");
            int forceRunPasses = forceRunPassesStr ? std::atoi(forceRunPassesStr) : 0;
            if (forceRunPasses)
            {
                PB.registerOptimizerLastEPCallback(
                    [](ModulePassManager &MPM, OptimizationLevel Level) 
                    {
                        MPM.addPass(OperandsRuntimeSignaturePass());
                    }
                );
            }
        }
    };
}

#ifndef LLVM_RUNTIMESIGNATURE_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getRuntimeSignaturePluginInfo();
}
#endif