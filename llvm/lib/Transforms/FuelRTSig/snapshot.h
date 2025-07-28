// note: ASLR might be a problem here
// note: use relative addresses for the snapshot stack
// figure out what to filter out from the snapshot
// might have to track memory pages, CoW
// should be fine without for just verifying calculations and detecting divergences

#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <type_traits>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/Triple.h"

using namespace llvm;

struct Snapshot {
    uint64_t fuel;
    uint64_t runtime_sig;
    uint64_t memory_usage;
    uint64_t instruction_count;
};

struct SnapshotX86 : public Snapshot {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rsp, rbp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rflags;
    
    struct {
        uint64_t low, high;
    } xmm[16];
    
    struct {
        uint64_t low, high;
    } ymm_upper[16];
    
    uint16_t cs, ds, es, fs, gs, ss;
    uint64_t cr0, cr2, cr3, cr4;
};

struct SnapshotAArch64 : public Snapshot {
    uint64_t x[31];
    uint64_t sp;
    
    struct {
        uint64_t low, high;
    } v[32];
    
    uint64_t nzcv, fpcr, fpsr, tpidr_el0, tpidrro_el0;
};

// Delta compression structures as LLVM types
struct DeltaHeader {
    uint64_t changed_mask[4];  // Bitmask for changed fields
    uint32_t compressed_size;  // Size of compressed data
    uint32_t padding;
};

class SnapshotPass {
private:
    Module *M;
    LLVMContext *Context;
    IRBuilder<> *Builder;
    
    // Global variables for snapshot state
    GlobalVariable *PreviousSnapshotGV;
    GlobalVariable *SnapshotStackGV;
    GlobalVariable *StackDepthGV;
    
    // LLVM types for snapshot structures
    StructType *BaseSnapshotTy;
    StructType *X86SnapshotTy;
    StructType *AArch64SnapshotTy;
    StructType *DeltaHeaderTy;
    
    // Runtime functions
    Function *CaptureRegistersFunc;
    Function *CreateDeltaFunc;
    Function *StoreSnapshotFunc;
    Function *CreateSnapshotFunc;

public:
    SnapshotPass(Module *M) : M(M), Context(&M->getContext()) {
        Builder = new IRBuilder<>(*Context);
        createSnapshotTypes();
        createGlobalState();
        createRuntimeFunctions();
    }
    
    ~SnapshotPass() {
        delete Builder;
    }
    
    void instrumentFunction(Function &F);

private:
    void createSnapshotTypes();
    void createGlobalState();
    void createRuntimeFunctions();
    
    // Architecture-specific implementations
    void createX86CaptureFunction();
    void createAArch64CaptureFunction();
    
    // Delta compression functions
    void createDeltaCompressionFunction();
    void createSnapshotStorageFunction();
    
    // Helper functions
    StructType* createX86SnapshotType();
    StructType* createAArch64SnapshotType();
    StructType* createBaseSnapshotType();
    
    Value* createInlineAsmCapture(const std::string &asmStr, 
                                  const std::string &constraints,
                                  ArrayRef<Type*> argTypes,
                                  ArrayRef<Value*> args);
};

void SnapshotPass::createSnapshotTypes() {
    // Base snapshot type
    BaseSnapshotTy = createBaseSnapshotType();
    
    // Architecture-specific types
    X86SnapshotTy = createX86SnapshotType();
    AArch64SnapshotTy = createAArch64SnapshotType();
    
    // Delta header type
    Type *I64Ty = Type::getInt64Ty(*Context);
    Type *I32Ty = Type::getInt32Ty(*Context);
    DeltaHeaderTy = StructType::create(*Context, {
        ArrayType::get(I64Ty, 4),  // changed_mask[4]
        I32Ty,                     // compressed_size
        I32Ty                      // padding
    }, "struct.DeltaHeader");
}

StructType* SnapshotPass::createBaseSnapshotType() {
    Type *I64Ty = Type::getInt64Ty(*Context);
    return StructType::create(*Context, {
        I64Ty,  // fuel
        I64Ty,  // runtime_sig
        I64Ty,  // memory_usage
        I64Ty   // instruction_count
    }, "struct.Snapshot");
}

StructType* SnapshotPass::createX86SnapshotType() {
    Type *I64Ty = Type::getInt64Ty(*Context);
    Type *I16Ty = Type::getInt16Ty(*Context);
    
    // XMM register type (128-bit)
    StructType *XMMTy = StructType::create(*Context, {I64Ty, I64Ty}, "struct.XMM");
    
    std::vector<Type*> fields;
    
    // Base snapshot fields
    fields.insert(fields.end(), BaseSnapshotTy->element_begin(), BaseSnapshotTy->element_end());
    
    // General purpose registers (16 x 64-bit)
    for (int i = 0; i < 16; i++) {
        fields.push_back(I64Ty);
    }
    
    // RFLAGS
    fields.push_back(I64Ty);
    
    // XMM registers (16 x 128-bit)
    for (int i = 0; i < 16; i++) {
        fields.push_back(XMMTy);
    }
    
    // YMM upper halves (16 x 128-bit)
    for (int i = 0; i < 16; i++) {
        fields.push_back(XMMTy);
    }
    
    // Segment registers (6 x 16-bit)
    for (int i = 0; i < 6; i++) {
        fields.push_back(I16Ty);
    }
    
    // Control registers (4 x 64-bit)
    for (int i = 0; i < 4; i++) {
        fields.push_back(I64Ty);
    }
    
    return StructType::create(*Context, fields, "struct.SnapshotX86");
}

StructType* SnapshotPass::createAArch64SnapshotType() {
    Type *I64Ty = Type::getInt64Ty(*Context);
    
    // Vector register type (128-bit)
    StructType *VecTy = StructType::create(*Context, {I64Ty, I64Ty}, "struct.Vector");
    
    std::vector<Type*> fields;
    
    // Base snapshot fields
    fields.insert(fields.end(), BaseSnapshotTy->element_begin(), BaseSnapshotTy->element_end());
    
    // X0-X30 registers (31 x 64-bit)
    for (int i = 0; i < 31; i++) {
        fields.push_back(I64Ty);
    }
    
    // SP register
    fields.push_back(I64Ty);
    
    // V0-V31 vector registers (32 x 128-bit)
    for (int i = 0; i < 32; i++) {
        fields.push_back(VecTy);
    }
    
    // System registers (5 x 64-bit)
    for (int i = 0; i < 5; i++) {
        fields.push_back(I64Ty);
    }
    
    return StructType::create(*Context, fields, "struct.SnapshotAArch64");
}

void SnapshotPass::createGlobalState() {
    Type *I64Ty = Type::getInt64Ty(*Context);
    Type *I32Ty = Type::getInt32Ty(*Context);
    
    // Previous snapshot for delta compression
    Triple TT(M->getTargetTriple());
    StructType *SnapshotTy = (TT.getArch() == Triple::x86_64) ? X86SnapshotTy : AArch64SnapshotTy;
    
    PreviousSnapshotGV = new GlobalVariable(
        *M, SnapshotTy, false, GlobalValue::InternalLinkage,
        Constant::getNullValue(SnapshotTy), "__previous_snapshot");
    

    // Snapshot stack (for storing delta-compressed snapshots)
    Type *StackElementTy = StructType::create(*Context, {
        DeltaHeaderTy,                           // Delta header
        ArrayType::get(Type::getInt8Ty(*Context), 4096)  // Compressed data
    }, "struct.StackElement");
    
    Type *StackTy = ArrayType::get(StackElementTy, 1000);  // Max 1000 snapshots
    SnapshotStackGV = new GlobalVariable(
        *M, StackTy, false, GlobalValue::InternalLinkage,
        Constant::getNullValue(StackTy), "__snapshot_stack");
    
    // Stack depth
    StackDepthGV = new GlobalVariable(
        *M, I32Ty, false, GlobalValue::InternalLinkage,
        ConstantInt::get(I32Ty, 0), "__stack_depth");
}

void SnapshotPass::createRuntimeFunctions() {
    Triple TT(M->getTargetTriple());
    
    if (TT.getArch() == Triple::x86_64) {
        createX86CaptureFunction();
    } else if (TT.getArch() == Triple::aarch64) {
        createAArch64CaptureFunction();
    }
    
    createDeltaCompressionFunction();
    createSnapshotStorageFunction();
    
    // Main snapshot creation function
    FunctionType *CreateSnapshotTy = FunctionType::get(Type::getVoidTy(*Context), {}, false);
    CreateSnapshotFunc = Function::Create(CreateSnapshotTy, Function::InternalLinkage, 
                                         "__create_snapshot", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(*Context, "entry", CreateSnapshotFunc);
    Builder->SetInsertPoint(EntryBB);
    
    // Call register capture
    Value *CurrentSnapshot = Builder->CreateCall(CaptureRegistersFunc);
    
    // Call delta compression
    Value *DeltaData = Builder->CreateCall(CreateDeltaFunc, {CurrentSnapshot});
    
    // Call storage
    Builder->CreateCall(StoreSnapshotFunc, {DeltaData});
    
    Builder->CreateRetVoid();
}

void SnapshotPass::createX86CaptureFunction() {
    // Create function that captures x86_64 registers
    FunctionType *CaptureTy = FunctionType::get(PointerType::get(X86SnapshotTy, 0), {}, false);
    CaptureRegistersFunc = Function::Create(CaptureTy, Function::InternalLinkage, 
                                           "__capture_x86_registers", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(*Context, "entry", CaptureRegistersFunc);
    Builder->SetInsertPoint(EntryBB);
    
    // Allocate snapshot structure
    Value *SnapshotPtr = Builder->CreateAlloca(X86SnapshotTy, nullptr, "snapshot");
    
    // Capture base fields (fuel, runtime_sig, etc.)
    // These would be loaded from global variables in a real implementation
    Value *BasePtr = Builder->CreateStructGEP(X86SnapshotTy, SnapshotPtr, 0);
    Value *FuelPtr = Builder->CreateStructGEP(BaseSnapshotTy, BasePtr, 0);
    Builder->CreateStore(ConstantInt::get(Type::getInt64Ty(*Context), 0), FuelPtr);
    
    // Capture general purpose registers using inline assembly
    std::string asmStr = 
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1\n\t"
        "movq %%rcx, %2\n\t"
        "movq %%rdx, %3\n\t"
        "movq %%rsi, %4\n\t"
        "movq %%rdi, %5\n\t"
        "movq %%rsp, %6\n\t"
        "movq %%rbp, %7\n\t"
        "movq %%r8, %8\n\t"
        "movq %%r9, %9\n\t"
        "movq %%r10, %10\n\t"
        "movq %%r11, %11\n\t"
        "movq %%r12, %12\n\t"
        "movq %%r13, %13\n\t"
        "movq %%r14, %14\n\t"
        "movq %%r15, %15";
    
    std::string constraints = "=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m,=*m";
    
    std::vector<Type*> argTypes(16, PointerType::get(Type::getInt64Ty(*Context), 0));
    std::vector<Value*> args;
    
    // Create GEPs for each GPR field
    for (int i = 0; i < 16; i++) {
        Value *GEP = Builder->CreateStructGEP(X86SnapshotTy, SnapshotPtr, 4 + i);
        args.push_back(GEP);
    }
    
    createInlineAsmCapture(asmStr, constraints, argTypes, args);
    
    // Capture RFLAGS
    Value *RFlagsPtr = Builder->CreateStructGEP(X86SnapshotTy, SnapshotPtr, 20);
    std::string rflagsAsm = "pushfq\n\tpopq %0";
    std::string rflagsConstraints = "=*m";
    createInlineAsmCapture(rflagsAsm, rflagsConstraints, 
                          {PointerType::get(Type::getInt64Ty(*Context), 0)}, {RFlagsPtr});
    
    // Capture XMM registers
    for (int i = 0; i < 16; i++) {
        Value *XMMPtr = Builder->CreateStructGEP(X86SnapshotTy, SnapshotPtr, 21 + i);
        std::string xmmAsm = "movdqu %%xmm" + std::to_string(i) + ", %0";
        std::string xmmConstraints = "=*m";
        createInlineAsmCapture(xmmAsm, xmmConstraints, 
                              {PointerType::get(StructType::getTypeByName(*Context, "struct.XMM"), 0)}, 
                              {XMMPtr});
    }
    
    Builder->CreateRet(SnapshotPtr);
}

void SnapshotPass::createAArch64CaptureFunction() {
    // Create function that captures AArch64 registers
    FunctionType *CaptureTy = FunctionType::get(PointerType::get(AArch64SnapshotTy, 0), {}, false);
    CaptureRegistersFunc = Function::Create(CaptureTy, Function::InternalLinkage, 
                                           "__capture_aarch64_registers", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(*Context, "entry", CaptureRegistersFunc);
    Builder->SetInsertPoint(EntryBB);
    
    // Allocate snapshot structure
    Value *SnapshotPtr = Builder->CreateAlloca(AArch64SnapshotTy, nullptr, "snapshot");
    
    // Capture base fields
    Value *BasePtr = Builder->CreateStructGEP(AArch64SnapshotTy, SnapshotPtr, 0);
    Value *FuelPtr = Builder->CreateStructGEP(BaseSnapshotTy, BasePtr, 0);
    Builder->CreateStore(ConstantInt::get(Type::getInt64Ty(*Context), 0), FuelPtr);
    
    // Capture X0-X30 registers
    std::vector<Value*> args;
    std::vector<Type*> argTypes;
    std::string asmStr = "";
    std::string constraints = "";
    
    for (int i = 0; i < 31; i++) {
        if (i > 0) {
            asmStr += "\n\t";
            constraints += ",";
        }
        asmStr += "str x" + std::to_string(i) + ", %";
        asmStr += std::to_string(i);
        constraints += "=*m";
        
        Value *RegPtr = Builder->CreateStructGEP(AArch64SnapshotTy, SnapshotPtr, 4 + i);
        args.push_back(RegPtr);
        argTypes.push_back(PointerType::get(Type::getInt64Ty(*Context), 0));
    }
    
    createInlineAsmCapture(asmStr, constraints, argTypes, args);
    
    // Capture SP register
    Value *SPPtr = Builder->CreateStructGEP(AArch64SnapshotTy, SnapshotPtr, 35);
    std::string spAsm = "mov %0, sp";
    std::string spConstraints = "=*m";
    createInlineAsmCapture(spAsm, spConstraints, 
                          {PointerType::get(Type::getInt64Ty(*Context), 0)}, {SPPtr});
    
    // Capture vector registers V0-V31
    for (int i = 0; i < 32; i++) {
        Value *VPtr = Builder->CreateStructGEP(AArch64SnapshotTy, SnapshotPtr, 36 + i);
        std::string vAsm = "str q" + std::to_string(i) + ", %0";
        std::string vConstraints = "=*m";
        createInlineAsmCapture(vAsm, vConstraints, 
                              {PointerType::get(StructType::getTypeByName(*Context, "struct.Vector"), 0)}, 
                              {VPtr});
    }
    
    // Capture system registers
    Value *NZCVPtr = Builder->CreateStructGEP(AArch64SnapshotTy, SnapshotPtr, 68);
    std::string nzcvAsm = "mrs %0, nzcv";
    std::string nzcvConstraints = "=*m";
    createInlineAsmCapture(nzcvAsm, nzcvConstraints, 
                          {PointerType::get(Type::getInt64Ty(*Context), 0)}, {NZCVPtr});
    
    Builder->CreateRet(SnapshotPtr);
}

Value* SnapshotPass::createInlineAsmCapture(const std::string &asmStr, 
                                            const std::string &constraints,
                                            ArrayRef<Type*> argTypes,
                                            ArrayRef<Value*> args) {
    FunctionType *asmFuncTy = FunctionType::get(Type::getVoidTy(*Context), argTypes, false);
    InlineAsm *inlineAsm = InlineAsm::get(asmFuncTy, asmStr, constraints, true);
    return Builder->CreateCall(inlineAsm, args);
}

void SnapshotPass::createDeltaCompressionFunction() {
    Triple TT(M->getTargetTriple());
    StructType *SnapshotTy = (TT.getArch() == Triple::x86_64) ? X86SnapshotTy : AArch64SnapshotTy;
    
    // Function that creates delta between current and previous snapshot
    FunctionType *DeltaTy = FunctionType::get(
        PointerType::get(Type::getInt8Ty(*Context), 0),  // Returns compressed data
        {PointerType::get(SnapshotTy, 0)},               // Takes current snapshot
        false);
    
    CreateDeltaFunc = Function::Create(DeltaTy, Function::InternalLinkage, 
                                      "__create_delta", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(*Context, "entry", CreateDeltaFunc);
    Builder->SetInsertPoint(EntryBB);
    
    Value *CurrentSnapshot = CreateDeltaFunc->getArg(0);
    
    // Allocate delta header
    Value *DeltaHeader = Builder->CreateAlloca(DeltaHeaderTy, nullptr, "delta_header");
    
    // Initialize changed mask to zero
    Value *ChangedMaskPtr = Builder->CreateStructGEP(DeltaHeaderTy, DeltaHeader, 0);
    Builder->CreateStore(Constant::getNullValue(ArrayType::get(Type::getInt64Ty(*Context), 4)), 
                        ChangedMaskPtr);
    
    // Allocate compressed data buffer
    Value *CompressedData = Builder->CreateAlloca(ArrayType::get(Type::getInt8Ty(*Context), 4096), 
                                                  nullptr, "compressed_data");
    Value *CompressedSize = Builder->CreateAlloca(Type::getInt32Ty(*Context), nullptr, "compressed_size");
    Builder->CreateStore(ConstantInt::get(Type::getInt32Ty(*Context), 0), CompressedSize);
    
    // Load previous snapshot
    Value *PreviousSnapshot = Builder->CreateLoad(SnapshotTy, PreviousSnapshotGV);
    Value *PreviousPtr = Builder->CreateAlloca(SnapshotTy);
    Builder->CreateStore(PreviousSnapshot, PreviousPtr);
    
    // Compare each field and build delta
    uint32_t numFields = SnapshotTy->getNumElements();
    for (uint32_t i = 0; i < numFields; i++) {
        Value *CurrentFieldPtr = Builder->CreateStructGEP(SnapshotTy, CurrentSnapshot, i);
        Value *PreviousFieldPtr = Builder->CreateStructGEP(SnapshotTy, PreviousPtr, i);
        
        Value *CurrentField = Builder->CreateLoad(SnapshotTy->getElementType(i), CurrentFieldPtr);
        Value *PreviousField = Builder->CreateLoad(SnapshotTy->getElementType(i), PreviousFieldPtr);
        
        // Compare fields
        Value *FieldChanged;
        Type *FieldTy = SnapshotTy->getElementType(i);
        
        if (FieldTy->isIntegerTy()) {
            FieldChanged = Builder->CreateICmpNE(CurrentField, PreviousField);
        } else if (FieldTy->isStructTy()) {
            // For struct types, do a memcmp
            Value *CmpResult = Builder->CreateCall(
                Intrinsic::getDeclaration(M, Intrinsic::memcmp),
                {CurrentFieldPtr, PreviousFieldPtr, 
                 ConstantInt::get(Type::getInt64Ty(*Context), 
                                 M->getDataLayout().getTypeStoreSize(FieldTy))});
            FieldChanged = Builder->CreateICmpNE(CmpResult, ConstantInt::get(Type::getInt32Ty(*Context), 0));
        }
        
        // If field changed, set bit in mask and add to compressed data
        BasicBlock *FieldChangedBB = BasicBlock::Create(*Context, "field_changed", CreateDeltaFunc);
        BasicBlock *FieldUnchangedBB = BasicBlock::Create(*Context, "field_unchanged", CreateDeltaFunc);
        BasicBlock *NextFieldBB = BasicBlock::Create(*Context, "next_field", CreateDeltaFunc);
        
        Builder->CreateCondBr(FieldChanged, FieldChangedBB, FieldUnchangedBB);
        
        // Field changed: set bit and copy data
        Builder->SetInsertPoint(FieldChangedBB);
        
        // Set bit in changed mask
        uint32_t maskIndex = i / 64;
        uint32_t bitIndex = i % 64;
        Value *MaskPtr = Builder->CreateConstGEP2_32(ArrayType::get(Type::getInt64Ty(*Context), 4), 
                                                     ChangedMaskPtr, 0, maskIndex);
        Value *CurrentMask = Builder->CreateLoad(Type::getInt64Ty(*Context), MaskPtr);
        Value *BitMask = ConstantInt::get(Type::getInt64Ty(*Context), 1ULL << bitIndex);
        Value *NewMask = Builder->CreateOr(CurrentMask, BitMask);
        Builder->CreateStore(NewMask, MaskPtr);
        
        // Copy field data to compressed buffer
        Value *CurrentSize = Builder->CreateLoad(Type::getInt32Ty(*Context), CompressedSize);
        Value *DataPtr = Builder->CreateGEP(Type::getInt8Ty(*Context), CompressedData, 
                                           {ConstantInt::get(Type::getInt32Ty(*Context), 0), CurrentSize});
        
        uint64_t fieldSize = M->getDataLayout().getTypeStoreSize(FieldTy);
        Builder->CreateCall(
            Intrinsic::getDeclaration(M, Intrinsic::memcpy, 
                                     {Type::getInt8PtrTy(*Context), Type::getInt8PtrTy(*Context), 
                                      Type::getInt64Ty(*Context)}),
            {DataPtr, Builder->CreateBitCast(CurrentFieldPtr, Type::getInt8PtrTy(*Context)),
             ConstantInt::get(Type::getInt64Ty(*Context), fieldSize),
             ConstantInt::getFalse(*Context)});
        
        Value *NewSize = Builder->CreateAdd(CurrentSize, 
                                           ConstantInt::get(Type::getInt32Ty(*Context), fieldSize));
        Builder->CreateStore(NewSize, CompressedSize);
        
        Builder->CreateBr(NextFieldBB);
        
        // Field unchanged: do nothing
        Builder->SetInsertPoint(FieldUnchangedBB);
        Builder->CreateBr(NextFieldBB);
        
        Builder->SetInsertPoint(NextFieldBB);
    }
    
    // Update previous snapshot
    Value *CurrentSnapshotValue = Builder->CreateLoad(SnapshotTy, CurrentSnapshot);
    Builder->CreateStore(CurrentSnapshotValue, PreviousSnapshotGV);
    
    // Store compressed size in header
    Value *CompressedSizeValue = Builder->CreateLoad(Type::getInt32Ty(*Context), CompressedSize);
    Value *CompressedSizePtr = Builder->CreateStructGEP(DeltaHeaderTy, DeltaHeader, 1);
    Builder->CreateStore(CompressedSizeValue, CompressedSizePtr);
    
    // Return pointer to compressed data (header + data)
    Value *ResultPtr = Builder->CreateAlloca(ArrayType::get(Type::getInt8Ty(*Context), 4096 + 32));
    
    // Copy header
    Builder->CreateCall(
        Intrinsic::getDeclaration(M, Intrinsic::memcpy, 
                                 {Type::getInt8PtrTy(*Context), Type::getInt8PtrTy(*Context), 
                                  Type::getInt64Ty(*Context)}),
        {Builder->CreateBitCast(ResultPtr, Type::getInt8PtrTy(*Context)),
         Builder->CreateBitCast(DeltaHeader, Type::getInt8PtrTy(*Context)),
         ConstantInt::get(Type::getInt64Ty(*Context), 32),  // sizeof(DeltaHeader)
         ConstantInt::getFalse(*Context)});
    
    // Copy compressed data
    Value *DataDestPtr = Builder->CreateGEP(Type::getInt8Ty(*Context), ResultPtr, 
                                           {ConstantInt::get(Type::getInt32Ty(*Context), 0),
                                            ConstantInt::get(Type::getInt32Ty(*Context), 32)});
    Builder->CreateCall(
        Intrinsic::getDeclaration(M, Intrinsic::memcpy, 
                                 {Type::getInt8PtrTy(*Context), Type::getInt8PtrTy(*Context), 
                                  Type::getInt64Ty(*Context)}),
        {DataDestPtr, Builder->CreateBitCast(CompressedData, Type::getInt8PtrTy(*Context)),
         Builder->CreateZExt(CompressedSizeValue, Type::getInt64Ty(*Context)),
         ConstantInt::getFalse(*Context)});
    
    Builder->CreateRet(Builder->CreateBitCast(ResultPtr, Type::getInt8PtrTy(*Context)));
}

void SnapshotPass::createSnapshotStorageFunction() {
    // Function that stores compressed snapshot in the stack
    FunctionType *StoreTy = FunctionType::get(
        Type::getVoidTy(*Context),
        {PointerType::get(Type::getInt8Ty(*Context), 0)},  // Compressed data
        false);
    
    StoreSnapshotFunc = Function::Create(StoreTy, Function::InternalLinkage, 
                                        "__store_snapshot", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(*Context, "entry", StoreSnapshotFunc);
    Builder->SetInsertPoint(EntryBB);
    
    Value *CompressedData = StoreSnapshotFunc->getArg(0);
    
    // Load current stack depth
    Value *CurrentDepth = Builder->CreateLoad(Type::getInt32Ty(*Context), StackDepthGV);
    
    // Check if stack is full
    Value *MaxDepth = ConstantInt::get(Type::getInt32Ty(*Context), 1000);
    Value *StackFull = Builder->CreateICmpUGE(CurrentDepth, MaxDepth);
    
    BasicBlock *StoreBB = BasicBlock::Create(*Context, "store", StoreSnapshotFunc);
    BasicBlock *SkipBB = BasicBlock::Create(*Context, "skip", StoreSnapshotFunc);
    
    Builder->CreateCondBr(StackFull, SkipBB, StoreBB);
    
    // Store compressed data
    Builder->SetInsertPoint(StoreBB);
    
    // Get pointer to stack element
    Value *StackElementPtr = Builder->CreateGEP(
        SnapshotStackGV->getValueType(), SnapshotStackGV,
        {ConstantInt::get(Type::getInt32Ty(*Context), 0), CurrentDepth});
    
    // Copy compressed data to stack
    Builder->CreateCall(
        Intrinsic::getDeclaration(M, Intrinsic::memcpy, 
                                 {Type::getInt8PtrTy(*Context), Type::getInt8PtrTy(*Context), 
                                  Type::getInt64Ty(*Context)}),
        {Builder->CreateBitCast(StackElementPtr, Type::getInt8PtrTy(*Context)),
         CompressedData,
         ConstantInt::get(Type::getInt64Ty(*Context), 4096 + 32),  // Max size
         ConstantInt::getFalse(*Context)});
    
    // Increment stack depth
    Value *NewDepth = Builder->CreateAdd(CurrentDepth, ConstantInt::get(Type::getInt32Ty(*Context), 1));
    Builder->CreateStore(NewDepth, StackDepthGV);
    
    Builder->CreateBr(SkipBB);
    
    Builder->SetInsertPoint(SkipBB);
    Builder->CreateRetVoid();
}

void SnapshotPass::instrumentFunction(Function &F) {
    static int instructionCounter = 0;
    
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (isa<DbgInfoIntrinsic>(&I) || isa<PHINode>(&I))
                continue;
                
            if (++instructionCounter % 100 == 0) {
                Builder->SetInsertPoint(&I);
                
                // Call snapshot creation function
                auto Call = Builder->CreateCall(CreateSnapshotFunc);
                if (auto* Inst = dyn_cast<Instruction>(Call))
                    Inst->setMetadata("op_sig", MDNode::get(*Context, {}));
            }
        }
    }
}

template<typename SnapshotType>
void addSnapshotLogic(Function &F) {
    Module *M = F.getParent();
    LLVMContext &Context = M->getContext();
    IRBuilder<> Builder(Context);
    
    FunctionCallee CreateSnapshotFunc = M->getOrInsertFunction(
        "__create_snapshot",
        FunctionType::get(Type::getVoidTy(Context), {}, false)
    );
    
    static int instructionCounter = 0;
    
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (isa<DbgInfoIntrinsic>(&I) || isa<PHINode>(&I))
                continue;
                
            if (++instructionCounter % 100 == 0) {
                Builder.SetInsertPoint(&I);
                
                // Create register capture inline assembly call
                Value *SnapshotPtr = createRegisterCapture<SnapshotType>(Builder, M);
                
                // Call __create_snapshot with captured register state
                auto Call = Builder.CreateCall(CreateSnapshotFunc);
                if (auto* Inst = dyn_cast<Instruction>(Call))
                    Inst->setMetadata("op_sig", MDNode::get(Context, {}));
            }
        }
    }
}

// Fix the createSnapshotFunction to actually get the target triple
void createSnapshotFunction(Function &F) {
    Module *M = F.getParent();
    LLVMContext &Context = M->getContext();
    Triple TargetTriple(M->getTargetTriple());  // Add this line

    // Create the actual __create_snapshot function implementation
    FunctionType *SnapshotFuncType = FunctionType::get(Type::getVoidTy(Context), {}, false);
    Function *SnapshotFunc = Function::Create(SnapshotFuncType, Function::ExternalLinkage, "__create_snapshot", M);
    
    BasicBlock *EntryBB = BasicBlock::Create(Context, "entry", SnapshotFunc);
    IRBuilder<> Builder(Context);
    Builder.SetInsertPoint(EntryBB);
    
    // Add actual snapshot creation logic here
    if (TargetTriple.getArch() == Triple::ArchType::x86_64) {
        // Call x86 register capture
        Builder.CreateCall(M->getOrInsertFunction("__capture_x86_state", 
                          FunctionType::get(Type::getVoidTy(Context), {}, false)));
    } else if (TargetTriple.getArch() == Triple::ArchType::aarch64) {
        // Call AArch64 register capture  
        Builder.CreateCall(M->getOrInsertFunction("__capture_aarch64_state",
                          FunctionType::get(Type::getVoidTy(Context), {}, false)));
    }
    
    Builder.CreateRetVoid();

    // Now instrument the function based on architecture
    if (TargetTriple.getArch() == Triple::ArchType::x86_64) {
        addSnapshotLogic<SnapshotX86>(F);
    } else if (TargetTriple.getArch() == Triple::ArchType::aarch64) {
        addSnapshotLogic<SnapshotAArch64>(F);
    }
}


// Actually create the missing struct types in LLVM IR
template<typename SnapshotType>
StructType* createSnapshotType(LLVMContext &Context) {
    if constexpr (std::is_same_v<SnapshotType, SnapshotX86>) {
        Type *I64Ty = Type::getInt64Ty(Context);
        Type *I16Ty = Type::getInt16Ty(Context);
        
        std::vector<Type*> fields;
        // Base fields: fuel, runtime_sig, memory_usage, instruction_count
        fields.insert(fields.end(), 4, I64Ty);
        // GPRs: rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp, r8-r15
        fields.insert(fields.end(), 16, I64Ty);
        // RFLAGS
        fields.push_back(I64Ty);
        // XMM registers (16 * 128-bit = 16 * 2 * 64-bit)
        fields.insert(fields.end(), 32, I64Ty);
        // YMM upper (16 * 128-bit = 16 * 2 * 64-bit)  
        fields.insert(fields.end(), 32, I64Ty);
        // Segment registers
        fields.insert(fields.end(), 6, I16Ty);
        // Control registers
        fields.insert(fields.end(), 4, I64Ty);
        
        return StructType::create(Context, fields, "struct.SnapshotX86");
    } else if constexpr (std::is_same_v<SnapshotType, SnapshotAArch64>) {
        Type *I64Ty = Type::getInt64Ty(Context);
        
        std::vector<Type*> fields;
        // Base fields: fuel, runtime_sig, memory_usage, instruction_count
        fields.insert(fields.end(), 4, I64Ty);
        // X0-X30 registers
        fields.insert(fields.end(), 31, I64Ty);
        // SP register
        fields.push_back(I64Ty);
        // V0-V31 vector registers (32 * 128-bit = 32 * 2 * 64-bit)
        fields.insert(fields.end(), 64, I64Ty);
        // System registers: nzcv, fpcr, fpsr, tpidr_el0, tpidrro_el0
        fields.insert(fields.end(), 5, I64Ty);
        
        return StructType::create(Context, fields, "struct.SnapshotAArch64");
    }
}

template<typename SnapshotType>
Value* createRegisterCapture(IRBuilder<> &Builder, Module *M) {
    LLVMContext &Context = M->getContext();
    
    // Create the appropriate snapshot type
    StructType *SnapshotTy = createSnapshotType<SnapshotType>(Context);
    Value *SnapshotPtr = Builder.CreateAlloca(SnapshotTy);
    
    if constexpr (std::is_same_v<SnapshotType, SnapshotX86>) {
        captureX86Registers(Builder, M, SnapshotPtr);
    } else if constexpr (std::is_same_v<SnapshotType, SnapshotAArch64>) {
        captureAArch64Registers(Builder, M, SnapshotPtr);
    }
    
    return SnapshotPtr;
}

// Fix the X86 register capture to use proper struct GEP
void captureX86Registers(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    LLVMContext &Context = M->getContext();
    Type *I64Ty = Type::getInt64Ty(Context);
    StructType *SnapshotTy = cast<StructType>(SnapshotPtr->getType()->getPointerElementType());
    
    // Capture GPRs using inline assembly
    FunctionType *AsmFuncType = FunctionType::get(Type::getVoidTy(Context), {}, false);
    std::string AsmString = 
        "movq %%rax, 32(%0)\n\t"   // offset 4*8 = 32 (after base fields)
        "movq %%rbx, 40(%0)\n\t"
        "movq %%rcx, 48(%0)\n\t"
        "movq %%rdx, 56(%0)\n\t"
        "movq %%rsi, 64(%0)\n\t"
        "movq %%rdi, 72(%0)\n\t"
        "movq %%rsp, 80(%0)\n\t"
        "movq %%rbp, 88(%0)\n\t"
        "movq %%r8, 96(%0)\n\t"
        "movq %%r9, 104(%0)\n\t"
        "movq %%r10, 112(%0)\n\t"
        "movq %%r11, 120(%0)\n\t"
        "movq %%r12, 128(%0)\n\t"
        "movq %%r13, 136(%0)\n\t"
        "movq %%r14, 144(%0)\n\t"
        "movq %%r15, 152(%0)";
    
    std::string Constraints = "r";
    
    InlineAsm *IA = InlineAsm::get(AsmFuncType, AsmString, Constraints, true);
    Builder.CreateCall(IA, {SnapshotPtr});
    
    // Capture RFLAGS
    std::string RFlagsAsm = "pushfq\n\tpopq 160(%0)";  // offset after GPRs
    InlineAsm *RFlagsIA = InlineAsm::get(AsmFuncType, RFlagsAsm, Constraints, true);
    Builder.CreateCall(RFlagsIA, {SnapshotPtr});
}

// Fix the AArch64 register capture 
void captureAArch64Registers(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    LLVMContext &Context = M->getContext();
    Type *I64Ty = Type::getInt64Ty(Context);
    
    FunctionType *AsmFuncType = FunctionType::get(Type::getVoidTy(Context), {}, false);
    std::string AsmString = "";
    
    // Capture X0-X30
    for (int i = 0; i < 31; i++) {
        int offset = 32 + (i * 8);  // 32 bytes for base fields + i*8
        AsmString += "str x" + std::to_string(i) + ", " + std::to_string(offset) + "(%0)\n\t";
    }
    
    // Capture SP
    AsmString += "mov x30, sp\n\tstr x30, 280(%0)";  // 32 + 31*8 = 280
    
    std::string Constraints = "r";
    
    InlineAsm *IA = InlineAsm::get(AsmFuncType, AsmString, Constraints, true);
    Builder.CreateCall(IA, {SnapshotPtr});
}

// Remove the duplicate function definitions and add the missing ones
void captureRFlags(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    // This is now integrated into captureX86Registers
}

void captureXMMRegisters(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    // This can be added later for full XMM support
}

void captureSP(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    // This is now integrated into captureAArch64Registers  
}

void captureVectorRegisters(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    // This can be added later for full vector register support
}

void captureSystemRegisters(IRBuilder<> &Builder, Module *M, Value *SnapshotPtr) {
    // This can be added later for full system register support
}