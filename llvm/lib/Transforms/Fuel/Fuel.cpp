#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "Fuel.h"

using namespace llvm;

PreservedAnalyses FuelPass::run(Function &F, FunctionAnalysisManager &AM)
{
    return PreservedAnalyses::all();
}

unsigned getIntrinsicCost(StringRef Name)
{
    //if we are going to benchmark these, we should consider the len
    /*if (Name.starts_with("llvm.memcpy") || Name.starts_with("llvm.memmove"))
        return 10;

    if (Name.starts_with("llvm.memset"))
        return 8;*/

    if (Name.starts_with("llvm.fma"))
        return 5;

    if (Name.starts_with("llvm.bswap"))
        return 2;

    if (Name.starts_with("llvm.ctpop"))
        return 4;

    if (Name.starts_with("llvm.ctlz") || Name.starts_with("llvm.cttz"))
        return 3;

    if (Name.starts_with("llvm.fshl") || Name.starts_with("llvm.fshr"))
        return 2;

    if (Name.starts_with("llvm.umin") || Name.starts_with("llvm.umax") || 
        Name.starts_with("llvm.smin") || Name.starts_with("llvm.smax"))
        return 2;

    if (Name.starts_with("llvm.arm.neon.vmins") || 
        Name.starts_with("llvm.arm.neon.vmaxs"))
        return 3;

    if (Name.starts_with("llvm.arm.neon.vminu") || 
        Name.starts_with("llvm.arm.neon.vmaxu"))
        return 2;

    if (Name.starts_with("llvm.arm.neon.vmaxv") || Name.starts_with("llvm.arm.neon.vminv"))
        return 3;

    if (Name.starts_with("llvm.arm.neon.cmeq") || Name.starts_with("llvm.arm.neon.cmge") ||
        Name.starts_with("llvm.arm.neon.cmgt") || Name.starts_with("llvm.arm.neon.cmle") ||
        Name.starts_with("llvm.arm.neon.cmlt"))
        return 3;

    if (Name.starts_with("llvm.abs"))
        return 2;

    if (Name.starts_with("llvm.fabs"))
        return 1;

    if (Name.starts_with("llvm.sqrt"))
        return 20;
    
    if (Name.starts_with("llvm.powi"))
        return 25;

    if (Name.starts_with("llvm.exp")
        && !Name.starts_with("llvm.expect") && !Name.starts_with("llvm.experimental"))
        return 30;

    if (Name.starts_with("llvm.round"))
        return 6;

    if (Name.starts_with("llvm.floor") ||
        Name.starts_with("llvm.ceil"))
        return 5;

    if (Name.starts_with("llvm.smul.with.overflow") || 
        Name.starts_with("llvm.umul.with.overflow"))
        return 3;

    if (Name.starts_with("llvm.sadd.with.overflow") || 
        Name.starts_with("llvm.uadd.with.overflow") ||
        Name.starts_with("llvm.ssub.with.overflow") || 
        Name.starts_with("llvm.usub.with.overflow"))
        return 2;

    if (Name.starts_with("llvm.sadd.sat") || Name.starts_with("llvm.uadd.sat") ||
        Name.starts_with("llvm.ssub.sat") || Name.starts_with("llvm.usub.sat"))
        return 2;

    if (Name.starts_with("llvm.aarch64.neon.umaxp"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.fmla") || 
        Name.starts_with("llvm.aarch64.neon.fmls"))
        return 4;

    if (Name.starts_with("llvm.aarch64.neon.vcge") || 
        Name.starts_with("llvm.aarch64.neon.vcgt"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.vceq"))
        return 2;

    if (Name.starts_with("llvm.aarch64.neon.vcage") ||
        Name.starts_with("llvm.aarch64.neon.vcagt"))
        return 2;

    if (Name.starts_with("llvm.aarch64.neon.frint") || Name.starts_with("llvm.aarch64.neon.frecpe"))
        return 3;

    if (Name.starts_with("llvm.arm.neon.vrecpe") ||
        Name.starts_with("llvm.arm.neon.vrsqrte"))
        return 4;

    if (Name.starts_with("llvm.arm.neon.vrecps") ||
        Name.starts_with("llvm.arm.neon.vrsqrts"))
        return 4;

    if (Name.starts_with("llvm.aarch64.neon.vabs") ||
        Name.starts_with("llvm.aarch64.neon.vneg"))
        return 1;

    if (Name.starts_with("llvm.aarch64.neon.vrecpx"))
        return 4;

    if (Name.starts_with("llvm.aarch64.neon.vqabs") ||
        Name.starts_with("llvm.aarch64.neon.vqneg"))
        return 2;

    if (Name.starts_with("llvm.aarch64.neon.vqdmull") ||
        Name.starts_with("llvm.aarch64.neon.vqdmlal") ||
        Name.starts_with("llvm.aarch64.neon.vqdmlsl"))
        return 5;

    if (Name.starts_with("llvm.aarch64.neon.vqdmulh"))
        return 4;

    if (Name.starts_with("llvm.aarch64.neon.vsqadd") ||
        Name.starts_with("llvm.aarch64.neon.vuqadd") ||
        Name.starts_with("llvm.arm.neon.vqsub") ||
        Name.starts_with("llvm.arm.neon.vqadd"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.addp") ||
        Name.starts_with("llvm.aarch64.neon.saddlp") ||
        Name.starts_with("llvm.aarch64.neon.uaddlp"))
        return 2;

    if (Name.starts_with("llvm.aarch64.neon.pmul") ||
        Name.starts_with("llvm.aarch64.neon.smull") ||
        Name.starts_with("llvm.aarch64.neon.umull"))
        return 4;

    if (Name.starts_with("llvm.aarch64.neon.sqdmull") ||
        Name.starts_with("llvm.aarch64.neon.sqrdmulh"))
        return 5;

    if (Name.starts_with("llvm.aarch64.neon.sha1"))
        return 8;

    if (Name.starts_with("llvm.aarch64.neon.sha256"))
        return 10;

    if (Name.starts_with("llvm.aarch64.neon.aese") ||
        Name.starts_with("llvm.aarch64.neon.aesd"))
        return 12;

    if (Name.starts_with("llvm.aarch64.neon.fcvt"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.rshrn") ||
        Name.starts_with("llvm.aarch64.neon.sqshrn") ||
        Name.starts_with("llvm.aarch64.neon.uqshrn"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.sqshlu") ||
        Name.starts_with("llvm.aarch64.neon.sqrshrun"))
        return 4;

    if (Name.starts_with("llvm.vector.reduce.add") || 
        Name.starts_with("llvm.vector.reduce.and") || 
        Name.starts_with("llvm.vector.reduce.or") || 
        Name.starts_with("llvm.vector.reduce.xor"))
        return 3;

    if (Name.starts_with("llvm.vector.reduce.smax") || 
        Name.starts_with("llvm.vector.reduce.smin") || 
        Name.starts_with("llvm.vector.reduce.umax") || 
        Name.starts_with("llvm.vector.reduce.umin"))
        return 4;

    if (Name.starts_with("llvm.vector.reduce.fmax") || 
        Name.starts_with("llvm.vector.reduce.fmin"))
        return 4;

    if (Name.starts_with("llvm.vector.reduce.fadd"))
        return 5;

    if (Name.starts_with("llvm.vector.reduce.fmul"))
        return 6;

    if (Name.starts_with("llvm.tan"))
        return 45;

    if (Name.starts_with("llvm.cosh") || 
        Name.starts_with("llvm.sinh") || 
        Name.starts_with("llvm.tanh"))
        return 40;

    if (Name.starts_with("llvm.sin") || Name.starts_with("llvm.cos"))
        return 35;

    if (Name.starts_with("llvm.pow"))
        return 40;

    if (Name.starts_with("llvm.log") || Name.starts_with("llvm.log2") ||
        Name.starts_with("llvm.log10"))
        return 25;

    if (Name.starts_with("llvm.masked.load") || Name.starts_with("llvm.masked.store"))
        return 4;

    if (Name.starts_with("llvm.masked.gather") || Name.starts_with("llvm.masked.scatter"))
        return 6;

    if (Name.starts_with("llvm.matrix.transpose"))
        return 6;

    if (Name.starts_with("llvm.matrix.multiply"))
        return 10;

    if (Name.starts_with("llvm.matrix"))
        return 8;
        
    return 0;
}

unsigned getFuelCost(Instruction &I)
{
    Type *Ty = I.getType();
    bool isFloat = Ty->isFloatingPointTy();
    bool isVector = Ty->isVectorTy();
    unsigned baseCost = 0;

    if (auto *Call = dyn_cast<CallInst>(&I))
    {
        if (Function *Callee = Call->getCalledFunction())
        {
            if (Callee->isIntrinsic())
                return getIntrinsicCost(Callee->getName());
        }
    }

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
            baseCost = 8;
            break;

        case Instruction::URem:
        case Instruction::SRem:
            baseCost = 10;
            break;

        case Instruction::FAdd:
        case Instruction::FSub:
            baseCost = 2;
            break;

        case Instruction::FMul:
            baseCost = 4;
            break;

        case Instruction::FDiv:
            baseCost = 12;
            break;

        case Instruction::FRem:
            baseCost = 15;
            break;

        /*case Instruction::Load:
            if (auto *LI = dyn_cast<LoadInst>(&I))
            {
                baseCost = 2;
                if (LI->isVolatile())
                    baseCost *= 2;
            }
            break;

        case Instruction::Store:
            if (auto *SI = dyn_cast<StoreInst>(&I))
            {
                baseCost = 3;
                if (SI->isVolatile())
                    baseCost *= 2;
            }
            break;*/

        case Instruction::FNeg:
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
            baseCost = 2;
            break;

        case Instruction::ExtractElement:
        case Instruction::InsertElement:
            baseCost = 2;
            break;

        case Instruction::ShuffleVector:
            baseCost = 3;
            break;

        case Instruction::Select:
            baseCost = 2;
            break;

        default:
            return 0;
    }

    if (isFloat && isVector)
        baseCost *= 3;
    else if (isFloat || isVector)
        baseCost *= 2;

    return baseCost;
}

PreservedAnalyses FuelPass::run(Module &M, ModuleAnalysisManager &AM)
{
    LLVMContext &Context = M.getContext();
    IRBuilder<> Builder(Context);

    const char* isFirstSrcStr = std::getenv("IS_FIRST_SRC");
    int isFirstSrc = isFirstSrcStr ? std::atoi(isFirstSrcStr) : 0;

    if (isFirstSrc)
    {
        const char* fuelStr = std::getenv("FUEL");
        unsigned fuel = fuelStr ? std::atoi(fuelStr) : 10000;

        GlobalVariable *FuelGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), fuel),
            "__fuel_remaining");
        FuelGlobal->setAlignment(Align(8));
        FuelGlobal->setDSOLocal(true);
        
        GlobalVariable *CurrMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__curr_memory_usage");
        CurrMemoryUsageGlobal->setAlignment(Align(8));
        CurrMemoryUsageGlobal->setDSOLocal(true);

        GlobalVariable *TotalMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__total_memory_usage");
        TotalMemoryUsageGlobal->setAlignment(Align(8));
        TotalMemoryUsageGlobal->setDSOLocal(true);

        GlobalVariable *MaxMemoryUsageGlobal = new GlobalVariable(M,
            Type::getInt64Ty(Context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt64Ty(Context), 0),
            "__max_memory_usage");
        MaxMemoryUsageGlobal->setAlignment(Align(8));
        MaxMemoryUsageGlobal->setDSOLocal(true);

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
        BasicBlock *OutOfFuelBB = BasicBlock::Create(Context, "out_of_fuel", CheckFuelFunc);
        BasicBlock *ContinueBB = BasicBlock::Create(Context, "continue", CheckFuelFunc);

        Builder.SetInsertPoint(EntryBB);
        Value *FuelArg = CheckFuelFunc->getArg(0);
        Value *CurrentFuel = Builder.CreateAtomicRMW(
            AtomicRMWInst::Sub,
            FuelGlobal,
            FuelArg,
            MaybeAlign(8),
            AtomicOrdering::Monotonic
        );

        Value *ShouldAbort = Builder.CreateICmpSLT(
            Builder.CreateSub(CurrentFuel, FuelArg),
            ConstantInt::get(Type::getInt64Ty(Context), 0)
        );
        Builder.CreateCondBr(ShouldAbort, OutOfFuelBB, ContinueBB);

        Builder.SetInsertPoint(OutOfFuelBB);
        GlobalVariable *RuntimeSigGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__runtime_signature", Type::getInt64Ty(Context)));
        RuntimeSigGlobal->setLinkage(GlobalValue::ExternalLinkage);
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
    }
   
    FunctionCallee CheckFuelFunc = M.getOrInsertFunction(
        "__check_fuel",
        FunctionType::get(
            Type::getVoidTy(Context),
            {Type::getInt64Ty(Context)},
            false
        )
    );

    GlobalVariable *CurrMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__curr_memory_usage", Type::getInt64Ty(Context)));
    CurrMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

    GlobalVariable *TotalMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__total_memory_usage", Type::getInt64Ty(Context)));
    TotalMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

    GlobalVariable *MaxMemoryUsageGlobal = cast<GlobalVariable>(M.getOrInsertGlobal("__max_memory_usage", Type::getInt64Ty(Context)));
    MaxMemoryUsageGlobal->setLinkage(GlobalValue::ExternalLinkage);

    for (auto &F : M)
    {
        if (F.isDeclaration() || F.isIntrinsic() || F.getName().starts_with("llvm."))
            continue;

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

                if (auto *Call = dyn_cast<CallInst>(&I))
                {
                    if (Function *Callee = Call->getCalledFunction())
                    {
                        if (Callee->getName() == "__check_fuel")
                        {
                            hasRuntimeSignature = true;
                            continue;
                        }

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

                            Instruction *NewCurrMemoryUsage = Builder.CreateAtomicRMW(
                                AtomicRMWInst::Add,
                                CurrMemoryUsageGlobal,
                                SizeDiff,
                                MaybeAlign(8),
                                AtomicOrdering::Monotonic
                            );
                            NewCurrMemoryUsage->setMetadata("op_sig", MDNode::get(Context, {}));

                            Value *PositiveSizeDiff = Builder.CreateSelect(
                                Builder.CreateICmpSGT(NewSize, OldSize),
                                SizeDiff,
                                ConstantInt::get(Type::getInt64Ty(Context), 0)
                            );

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

                            continue;
                        }
                    }
                }

                if (!hasRuntimeSignature)
                    hasRuntimeSignature = F.getName() == "__check_fuel";

                if (!hasRuntimeSignature && I.getMetadata("op_sig"))
                    hasRuntimeSignature = true;

                if (!hasRuntimeSignature)
                {
                    unsigned FuelCost = getFuelCost(I);
                    BlockFuelCost += FuelCost;
                }
            }

            if (BlockFuelCost > 0)
            {
                Builder.SetInsertPoint(&*BB.getFirstInsertionPt());
                Builder.CreateCall(CheckFuelFunc, {
                    ConstantInt::get(Type::getInt64Ty(Context), BlockFuelCost)
                });
            }
        }
    }

    return PreservedAnalyses::none();
}

PassPluginLibraryInfo getFuelPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "FuelPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB)
        {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>)
                {
                    if (Name == "fuel")
                    {
                        MPM.addPass(FuelPass());
                        return true;
                    }
                    return false;
                }
            );
        }
    };
}

#ifndef LLVM_FUEL_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFuelPluginInfo();
}
#endif