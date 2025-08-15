#include "llvm/Pass.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "FuelRTSig.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include <cstdlib>
#include "llvm/IR/Dominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <string>
#include <vector>
#include <cctype>
#include <cstring>

using namespace llvm;

#include "x86.h"
#include "aarch64.h"
#include "rust.h"
#include "constants.h"
//#include "snapshot.h"

static bool isInstructionSafe(const Instruction &I)
{
    // Only filter truly unsafe operations
    /*if (isa<FenceInst>(&I))
        return false;

    // Skip volatile memory operations as they might have special semantics
    if (auto* MI = dyn_cast<LoadInst>(&I))
        if (MI->isVolatile())
            return false;

    if (auto* MI = dyn_cast<StoreInst>(&I))
        if (MI->isVolatile())
            return false;*/

    // Skip vector operations that might be converted to NEON/SVE
    /*if (auto* VL = dyn_cast<VectorType>(I.getType())) {
        unsigned Width = VL->getPrimitiveSizeInBits();
        if (Width != 64 && Width != 128)
            return false;
    }*/

    return true;
}



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

PreservedAnalyses FuelRTSigPass::run(Function &F, FunctionAnalysisManager &AM)
{
    return PreservedAnalyses::all();
}

enum Architecture {
    X86 = 0,
    AArch64,
};

unsigned getFuelCost(Instruction &I)
{
    // Get the module to check target triple
    Module *M = I.getModule();
    StringRef TargetTriple = M->getTargetTriple();
    
    // Check for x86 architectures
    if (TargetTriple.contains("x86_64") || 
        TargetTriple.contains("i386") || 
        TargetTriple.contains("i686")) {
        return getFuelCostX86(I);
    }
    
    // Check for ARM/AArch64 architectures  
    if (TargetTriple.contains("aarch64") || 
        TargetTriple.contains("arm64") ||
        TargetTriple.contains("arm")) {
        return getFuelCostAArch64(I);
    }

    errs() << "Unknown architecture: " << TargetTriple << "\n";
    
    // Fallback for unknown architectures
    return 0;//getFuelCostGeneric(I);
}

PreservedAnalyses FuelRTSigPass::run(Module &M, ModuleAnalysisManager &AM)
{
    LLVMContext &Context = M.getContext();
    IRBuilder<> Builder(Context);

    const char* isFirstSrcStr = std::getenv("IS_FIRST_SRC");
    int isFirstSrc = isFirstSrcStr ? std::atoi(isFirstSrcStr) : 0;

    const char *instrumentFuelStr = std::getenv("INSTRUMENT_FUEL");
    bool instrumentFuel = instrumentFuelStr ? std::atoi(instrumentFuelStr) : false;
    
    const char *instrumentMemoryStr = std::getenv("INSTRUMENT_MEMORY");
    bool instrumentMemory = instrumentMemoryStr ? std::atoi(instrumentMemoryStr) : false;

    const char *instrumentRTSigStr = std::getenv("INSTRUMENT_RTSIG");
    bool instrumentRTSig = instrumentRTSigStr ? std::atoi(instrumentRTSigStr) : false;

    const char *llFileBaseName = std::getenv("LL_FILE_BASENAME");

    // Setup globals
    GlobalVariable *FuelGlobal;
    GlobalVariable *RuntimeSigGlobal;
    GlobalVariable *ThreadLocalFuelGlobal;
    GlobalVariable *ThreadLocalRuntimeSigGlobal;
    GlobalVariable *CurrMemoryUsageGlobal;
    GlobalVariable *TotalMemoryUsageGlobal;
    GlobalVariable *MaxMemoryUsageGlobal;
    GlobalVariable *MaxAllowedMemoryUsageGlobal;    
    
    if (isFirstSrc)
    {
        // Create fuel globals
        /*FuelGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0x7FFFFFFFFFFFFFFF),
            "__fuel_remaining");
        FuelGlobal->setAlignment(Align(8));
        //FuelGlobal->setDSOLocal(true);*/

        // Add thread-local variable for fuel tracking
        ThreadLocalFuelGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__thread_local_fuel_used");
        ThreadLocalFuelGlobal->setAlignment(Align(8));
        //ThreadLocalFuelGlobal->setDSOLocal(true);
        ThreadLocalFuelGlobal->setThreadLocal(true); 

        // Create runtime signature globals
        /*RuntimeSigGlobal = new GlobalVariable(M, 
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0x4C4C4D564F4C4C4C),
            "__runtime_signature");*/

        FuelGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__fuel_remaining", Type::getInt64Ty(Context)));
        FuelGlobal->setLinkage(GlobalValue::ExternalLinkage);
        
        RuntimeSigGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__runtime_signature", Type::getInt64Ty(Context)));
        RuntimeSigGlobal->setLinkage(GlobalValue::ExternalLinkage);
        
        // Add thread-local variable for runtime signature tracking
        ThreadLocalRuntimeSigGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__thread_local_runtime_signature");
        ThreadLocalRuntimeSigGlobal->setAlignment(Align(8));
        //ThreadLocalRuntimeSigGlobal->setDSOLocal(true);
        ThreadLocalRuntimeSigGlobal->setThreadLocal(true);

        /*CurrMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__curr_memory_usage");
        CurrMemoryUsageGlobal->setAlignment(Align(8));
        //CurrMemoryUsageGlobal->setDSOLocal(true);

        TotalMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__total_memory_usage");
        TotalMemoryUsageGlobal->setAlignment(Align(8));
        //TotalMemoryUsageGlobal->setDSOLocal(true);

        MaxMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__max_memory_usage");
        MaxMemoryUsageGlobal->setAlignment(Align(8));
        //MaxMemoryUsageGlobal->setDSOLocal(true);*/

        MaxMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__max_memory_usage", Type::getInt64Ty(Context)));
        MaxMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        CurrMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__curr_memory_usage", Type::getInt64Ty(Context)));
        CurrMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        TotalMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__total_memory_usage", Type::getInt64Ty(Context)));
        TotalMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        /*MaxAllowedMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0xFFFFFFFFFFFFFFFF),
            "__max_allowed_memory_usage");
        MaxAllowedMemoryUsageGlobal->setAlignment(Align(8));
        MaxAllowedMemoryUsageGlobal->setDSOLocal(false);*/

        MaxAllowedMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__max_allowed_memory_usage", Type::getInt64Ty(Context)));
        MaxAllowedMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        // Create check fuel function
        FunctionType *CheckFuelType = FunctionType::get(
            Type::getVoidTy(Context),
            {Type::getInt64Ty(Context)},
            false
        );

        Function *CheckFuelFunc = Function::Create(
            CheckFuelType,
            GlobalValue::ExternalLinkage,
            "__check_fuel",
            M
        );

        BasicBlock *EntryBB = BasicBlock::Create(Context, "entry", CheckFuelFunc);
        Builder.SetInsertPoint(EntryBB);

        Value *FuelArg = CheckFuelFunc->getArg(0);
        Value *CurrentFuel = Builder.CreateAtomicRMW(
            AtomicRMWInst::Sub,
            FuelGlobal,
            FuelArg,
            MaybeAlign(8),
            AtomicOrdering::Monotonic
        );

        BasicBlock *ExitBB = BasicBlock::Create(Context, "exit", CheckFuelFunc);
        BasicBlock *ContinueBB = BasicBlock::Create(Context, "continue", CheckFuelFunc);

        Value *ShouldAbort = Builder.CreateICmpSLT(
            Builder.CreateSub(CurrentFuel, FuelArg),
            ConstantInt::get(Type::getInt64Ty(Context), 0)
        );

        Builder.CreateCondBr(ShouldAbort, ExitBB, ContinueBB);

        Builder.SetInsertPoint(ExitBB);
        Value *RuntimeSig = Builder.CreateLoad(Type::getInt64Ty(Context), RuntimeSigGlobal);

        FunctionCallee PrintfFunc = M.getOrInsertFunction(
            "printf",
            FunctionType::get(
                Type::getInt32Ty(Context),
                {PointerType::get(Type::getInt8Ty(Context), 0)},
                true
            )
        );

        Value *FormatStr = Builder.CreateGlobalStringPtr("\nRuntime signature: %lu\n");
        Builder.CreateCall(PrintfFunc, {FormatStr, RuntimeSig});

        FunctionCallee ExitFunc = M.getOrInsertFunction(
            "exit",
            FunctionType::get(Type::getVoidTy(Context), {Type::getInt32Ty(Context)}, false)
        );
        Builder.CreateCall(ExitFunc, {ConstantInt::get(Type::getInt32Ty(Context), 87)});
        Builder.CreateUnreachable();

        Builder.SetInsertPoint(ContinueBB);
        Builder.CreateRetVoid();
        
        // Create unified commit function that handles both fuel and runtime signature
        FunctionType *CommitType = FunctionType::get(
            Type::getVoidTy(Context),
            {},
            false
        );

        Function *CommitFunc = Function::Create(
            CommitType,
            GlobalValue::ExternalLinkage,
            "__commit_tls",
            M
        );

        BasicBlock *CommitEntryBB = BasicBlock::Create(Context, "entry", CommitFunc);
        Builder.SetInsertPoint(CommitEntryBB);
        
        // First handle fuel
        Value *ThreadFuelUsed = Builder.CreateLoad(Type::getInt64Ty(Context), ThreadLocalFuelGlobal);
        Value *HasFuelUsed = Builder.CreateICmpSGT(
            ThreadFuelUsed,
            ConstantInt::get(Type::getInt64Ty(Context), 0)
        );
        
        BasicBlock *FuelUpdateBB = BasicBlock::Create(Context, "update_fuel", CommitFunc);
        BasicBlock *AfterFuelBB = BasicBlock::Create(Context, "after_fuel", CommitFunc);
        
        Builder.CreateCondBr(HasFuelUsed, FuelUpdateBB, AfterFuelBB);
        
        Builder.SetInsertPoint(FuelUpdateBB);
        Builder.CreateCall(CheckFuelFunc, ThreadFuelUsed);
        Builder.CreateStore(ConstantInt::get(Type::getInt64Ty(Context), 0), ThreadLocalFuelGlobal);
        Builder.CreateBr(AfterFuelBB);
        
        // Then handle runtime signature
        Builder.SetInsertPoint(AfterFuelBB);
        Value *ThreadSig = Builder.CreateLoad(Type::getInt64Ty(Context), ThreadLocalRuntimeSigGlobal);
        Value *HasSig = Builder.CreateICmpNE(
            ThreadSig,
            ConstantInt::get(Type::getInt64Ty(Context), 0)
        );
        
        BasicBlock *SigUpdateBB = BasicBlock::Create(Context, "update_sig", CommitFunc);
        BasicBlock *EndBB = BasicBlock::Create(Context, "end", CommitFunc);
        
        Builder.CreateCondBr(HasSig, SigUpdateBB, EndBB);
        
        Builder.SetInsertPoint(SigUpdateBB);
        Builder.CreateAtomicRMW(
            AtomicRMWInst::Xor,
            RuntimeSigGlobal,
            ThreadSig,
            MaybeAlign(8),
            AtomicOrdering::Monotonic
        );
        Builder.CreateStore(ConstantInt::get(Type::getInt64Ty(Context), 0), ThreadLocalRuntimeSigGlobal);
        Builder.CreateBr(EndBB);
        
        Builder.SetInsertPoint(EndBB);
        Builder.CreateRetVoid();

        // memory check function    
        FunctionType *MemoryCheckType = FunctionType::get(
            Type::getVoidTy(Context),
            {},
            false
        );

        Function *MemoryCheckFunc = Function::Create(
            MemoryCheckType,
            GlobalValue::ExternalLinkage,
            "__memory_check",
            M
        );

        BasicBlock *MemoryCheckEntryBB = BasicBlock::Create(Context, "entry", MemoryCheckFunc);
        BasicBlock *ContinueBBMemoryCheck = BasicBlock::Create(Context, "continue", MemoryCheckFunc);
        BasicBlock *ExitBBMemoryCheck = BasicBlock::Create(Context, "exit", MemoryCheckFunc);
        
        Builder.SetInsertPoint(MemoryCheckEntryBB);

        Value *MaxAllowedMemoryUsage = Builder.CreateLoad(Type::getInt64Ty(Context), MaxAllowedMemoryUsageGlobal);
        Value *CurrMemoryUsage = Builder.CreateLoad(Type::getInt64Ty(Context), CurrMemoryUsageGlobal);
        Value *ShouldAbortMemoryCheck = Builder.CreateICmpUGT(
            CurrMemoryUsage,
            MaxAllowedMemoryUsage
        );

        Builder.CreateCondBr(ShouldAbortMemoryCheck, ExitBBMemoryCheck, ContinueBBMemoryCheck);

        Builder.SetInsertPoint(ExitBBMemoryCheck);
        Builder.CreateCall(ExitFunc, {ConstantInt::get(Type::getInt32Ty(Context), 83)});
        Builder.CreateUnreachable();

        Builder.SetInsertPoint(ContinueBBMemoryCheck);
        Builder.CreateRetVoid();
    }
    else
    {
        // Get globals if not first source file
        FuelGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__fuel_remaining", Type::getInt64Ty(Context)));
        FuelGlobal->setLinkage(GlobalValue::ExternalLinkage);
        
        RuntimeSigGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__runtime_signature", Type::getInt64Ty(Context)));
        RuntimeSigGlobal->setLinkage(GlobalValue::ExternalLinkage);

        CurrMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__curr_memory_usage", Type::getInt64Ty(Context)));
        CurrMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        TotalMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__total_memory_usage", Type::getInt64Ty(Context)));
        TotalMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        MaxMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__max_memory_usage", Type::getInt64Ty(Context)));
        MaxMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

        MaxAllowedMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__max_allowed_memory_usage", Type::getInt64Ty(Context)));
        MaxAllowedMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);
    }

    // Get or ensure thread-local variables and commit function for all source files
    ThreadLocalFuelGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__thread_local_fuel_used", Type::getInt64Ty(Context)));
    ThreadLocalFuelGlobal->setLinkage(GlobalValue::ExternalLinkage);
    ThreadLocalFuelGlobal->setThreadLocal(true);
    
    ThreadLocalRuntimeSigGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__thread_local_runtime_signature", Type::getInt64Ty(Context)));
    ThreadLocalRuntimeSigGlobal->setLinkage(GlobalValue::ExternalLinkage);
    ThreadLocalRuntimeSigGlobal->setThreadLocal(true);
    
    FunctionCallee CommitFunc = M.getOrInsertFunction(
        "__commit_tls",
        FunctionType::get(Type::getVoidTy(Context), {}, false)
    );

    FunctionCallee MemoryCheckFunc = M.getOrInsertFunction(
        "__memory_check",
        FunctionType::get(Type::getVoidTy(Context), {}, false)
    );

    MDNode *OpSigMD = MDNode::get(Context, {});

    for (auto &F : M)
    {
        if (F.isDeclaration() || F.isIntrinsic() || F.getName().starts_with("llvm.") ||
            F.getName() == "__check_fuel" || F.getName() == "__commit_tls" || F.getName() == "__memory_check" ||
            F.getName() == "__create_snapshot" || F.getName() == "__create_delta" || F.getName() == "__capture_x86_state" ||
            F.getName() == "__capture_aarch64_state" || F.getName() == "__copy_to_restore_region" || F.getName() == "__switch_stack_and_call")
            continue;

        //errs() << llFileBaseName << "::" << rustDemangle(F.getName().str()) << "\n";

        if (strcmp(llFileBaseName, "std") == 0 &&
            (strstr(rustDemangle(F.getName().str()).c_str(), "sync::") ||
            strstr(rustDemangle(F.getName().str()).c_str(), "::thread"))
        )
        {
            const char* outputSkippedFunctionsStr = std::getenv("OUTPUT_SKIPPED_FUNCTIONS");
            int outputSkippedFunctions = outputSkippedFunctionsStr ? std::atoi(outputSkippedFunctionsStr) : 0;
            if (outputSkippedFunctions)
            {
                uint64_t FuncHash = hash_value(F.getName());
                errs() << "skipping " << llFileBaseName << " :: " << F.getName() << " (" << rustDemangle(F.getName().str()) << ") [hash: " << FuncHash << "]\n";
            }
            continue;
        }

        /*if (strcmp(llFileBaseName, "tig_algorithms") == 0 &&
            (strstr(rustDemangle(F.getName().str()).c_str(), "rayon") ||
            strstr(rustDemangle(F.getName().str()).c_str(), "crossbeam"))
        )
        {
            uint64_t FuncHash = hash_value(F.getName());
            errs() << "skipping " << llFileBaseName << " :: " << F.getName() << " (" << rustDemangle(F.getName().str()) << ") [hash: " << FuncHash << "]\n";
            continue;
        }*/

        if (instrumentRTSig)
        {
            // Runtime signature instrumentation (simplified for brevity)
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
                    
                    // Update thread-local signature
                    Value* CurrentThreadSig = Builder.CreateLoad(Type::getInt64Ty(Context), ThreadLocalRuntimeSigGlobal);
                    ((Instruction *)(CurrentThreadSig))->setMetadata("op_sig", OpSigMD);
                    Value* NewThreadSig = Builder.CreateXor(CurrentThreadSig, ConstantInt::get(Type::getInt64Ty(Context), BlockSig));
                    ((Instruction *)(NewThreadSig))->setMetadata("op_sig", OpSigMD);
                    Instruction* StoreInst = Builder.CreateStore(NewThreadSig, ThreadLocalRuntimeSigGlobal);
                    (StoreInst)->setMetadata("op_sig", OpSigMD);

                    for (Instruction* I : InstrToMark)
                        I->setMetadata("mark_instr", OpSigMD);
                }
            }
        }

        for (auto &BB : F)
        {
            unsigned BlockFuelCost = 0;
            for (auto &I : BB)
            {
                bool hasRuntimeSignature = false;
                for (unsigned idx = 0; idx < I.getNumOperands(); idx++)
                {
                    if (auto *GV = dyn_cast<GlobalVariable>(I.getOperand(idx)))
                    {
                        if (GV->getName() == "__runtime_signature")
                        {
                            hasRuntimeSignature = true;
                            break;
                        }
                    }
                }

                auto *Call = dyn_cast<CallInst>(&I);
                if (instrumentMemory && Call)
                {
                    if (Function *Callee = Call->getCalledFunction())
                    {
                        if (Callee->getName() == "__check_fuel" || Callee->getName() == "__commit_tls" || Callee->getName() == "__memory_check")
                        {
                            hasRuntimeSignature = true;
                            continue;
                        }

                        // Memory allocation functions - keeping as is per instructions
                        if (Callee->getName() == "__rust_alloc")
                        {
                            Builder.SetInsertPoint(Call);
                            Value *Size = Call->getArgOperand(0);
                            Instruction *NewCurrMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                CurrMemoryUsageGlobal,
                                Size,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewCurrMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *NewTotalMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                TotalMemoryUsageGlobal,
                                Size,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewTotalMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *NewMaxMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Max,
                                MaxMemoryUsageGlobal,
                                NewCurrMemoryUsage,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic);
                            NewMaxMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *MemoryCheck = Builder.CreateCall(MemoryCheckFunc);
                            MemoryCheck->setMetadata("op_sig", MDNode::get(Context, {}));

                            continue;
                        }

                        if (Callee->getName() == "__rust_dealloc")
                        {
                            Builder.SetInsertPoint(Call);
                            Value *Size = Call->getArgOperand(1);

                            Instruction *NewCurrMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Sub,
                                CurrMemoryUsageGlobal,
                                Size,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewCurrMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            continue;
                        }

                        if (Callee->getName() == "__rust_realloc")
                        {
                            Builder.SetInsertPoint(Call);
                            Value *OldSize = Call->getArgOperand(1);
                            Value *NewSize = Call->getArgOperand(3);

                            Value *SizeDiff = Builder.CreateSub(NewSize, OldSize);
                            ((Instruction*)(SizeDiff))->setMetadata("op_sig", OpSigMD);

                            Instruction *NewCurrMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                CurrMemoryUsageGlobal,
                                SizeDiff,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewCurrMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Value *IsPositiveSizeDiff = Builder.CreateICmpSGT(NewSize, OldSize);
                            ((Instruction*)(IsPositiveSizeDiff))->setMetadata("op_sig", OpSigMD);

                            Value *PositiveSizeDiff = Builder.CreateSelect(
                                IsPositiveSizeDiff,
                                SizeDiff,
                                ConstantInt::get(Type::getInt64Ty(Context), 0)
                            );
                            ((Instruction*)(PositiveSizeDiff))->setMetadata("op_sig", OpSigMD);

                            Instruction *NewTotalMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                TotalMemoryUsageGlobal,
                                PositiveSizeDiff,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewTotalMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *NewMaxMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Max,
                                MaxMemoryUsageGlobal,
                                NewCurrMemoryUsage,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic);
                            NewMaxMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *MemoryCheck = Builder.CreateCall(MemoryCheckFunc);
                            MemoryCheck->setMetadata("op_sig", MDNode::get(Context, {}));

                            continue;
                        }

                        if (Callee->getName() == "__rust_alloc_zeroed")
                        {
                            Builder.SetInsertPoint(Call);
                            Value *Size = Call->getArgOperand(0);

                            Instruction *NewCurrMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                CurrMemoryUsageGlobal,
                                Size,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewCurrMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *NewTotalMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                TotalMemoryUsageGlobal,
                                Size,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewTotalMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));
                            
                            Instruction *NewMaxMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Max,
                                MaxMemoryUsageGlobal,
                                NewCurrMemoryUsage,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic);
                            NewMaxMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Instruction *MemoryCheck = Builder.CreateCall(MemoryCheckFunc);
                            MemoryCheck->setMetadata("op_sig", MDNode::get(Context, {}));

                            continue;
                        }
                    }
                }

                if (!hasRuntimeSignature)
                    hasRuntimeSignature = F.getName() == "__check_fuel" || F.getName() == "__commit_tls" || F.getName() == "__memory_check";

                if (!hasRuntimeSignature && I.getMetadata("op_sig"))
                    hasRuntimeSignature = true;

                if (!hasRuntimeSignature)
                {
                    unsigned FuelCost = getFuelCost(I);
                    BlockFuelCost += FuelCost;
                }
            }

            if (instrumentFuel && BlockFuelCost > 0)
            {
                Builder.SetInsertPoint(&*BB.getFirstInsertionPt());
                Value *CurrentThreadFuel = Builder.CreateLoad(Type::getInt64Ty(Context), ThreadLocalFuelGlobal);
                ((Instruction*)(CurrentThreadFuel))->setMetadata("op_sig", OpSigMD);

                Value *NewThreadFuel = Builder.CreateAdd(CurrentThreadFuel, 
                    ConstantInt::get(Type::getInt64Ty(Context), BlockFuelCost));
                ((Instruction*)(NewThreadFuel))->setMetadata("op_sig", OpSigMD);

                Instruction *StoreFuel = Builder.CreateStore(NewThreadFuel, ThreadLocalFuelGlobal);
                ((Instruction*)(StoreFuel))->setMetadata("op_sig", OpSigMD);
            }
        }

        // Add calls to commit at all function exit points
        if(instrumentRTSig || instrumentFuel)
        {
            for (auto &BB : F) {
                if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
                    Builder.SetInsertPoint(RI);
                    Instruction *Commit = Builder.CreateCall(CommitFunc);
                    Commit->setMetadata("op_sig", OpSigMD);
                }
            }
        }
    }

    return PreservedAnalyses::none();
}

PassPluginLibraryInfo getFuelPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "FuelRTSigPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB)
        {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>)
                {
                    if (Name == "fuel-rt-sig")
                    {
                        MPM.addPass(FuelRTSigPass());
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
                        MPM.addPass(FuelRTSigPass());
                    }
                );
            }
        }
    };
}

#ifndef LLVM_FUEL_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFuelPluginInfo();
}
#endif
