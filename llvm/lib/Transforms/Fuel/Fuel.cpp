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
    if (Name.starts_with("llvm.arm.neon") || Name.starts_with("llvm.aarch64.neon"))
    {
        if (Name.contains(".v2"))
        {
            if (Name.ends_with("f64") || Name.ends_with("i64"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
                if (Name.contains(".fmla") || Name.contains(".fmls"))
                    return 4;
                if (Name.contains(".frecpe") || Name.contains(".frint"))
                    return 4;
                if (Name.contains(".vrecpe") || Name.contains(".vrsqrte"))
                    return 3;
                if (Name.contains(".vrecps") || Name.contains(".vrsqrts"))
                    return 3;
                if (Name.contains(".vrecpx"))
                    return 5;
            }
        }
        else if (Name.contains(".v4"))
        {
            if (Name.ends_with("f32"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
                if (Name.contains(".fmla") || Name.contains(".fmls"))
                    return 4;
                if (Name.contains(".frecpe") || Name.contains(".frint"))
                    return 4;
                if (Name.contains(".vrecpe") || Name.contains(".vrsqrte"))
                    return 3;
                if (Name.contains(".vrecps") || Name.contains(".vrsqrts"))
                    return 3;
                if (Name.contains(".vrecpx"))
                    return 5;
            }
            else if (Name.ends_with("i32"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
                if (Name.contains(".vqdmull") || Name.contains(".vqdmlal") || 
                    Name.contains(".vqdmlsl"))
                    return 4;
                if (Name.contains(".vqdmulh") || Name.contains(".sqrdmulh"))
                    return 4;
            }
        }
        else if (Name.contains(".v8"))
        {
            if (Name.ends_with("i16") || Name.ends_with("f16"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vcage") || Name.contains(".vcagt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
                if (Name.contains(".vqdmull") || Name.contains(".vqdmlal") || 
                    Name.contains(".vqdmlsl"))
                    return 4;
                if (Name.contains(".vqdmulh") || Name.contains(".sqrdmulh"))
                    return 4;
            }
            else if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
            }
        }
        else if (Name.contains(".v16"))
        {
            if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
            }
        }
        else if (Name.contains(".v32"))
        {
            if (Name.ends_with("i8"))
            {
                if (Name.contains(".vcge") || Name.contains(".vcgt"))
                    return 3;
                if (Name.contains(".vceq"))
                    return 3;
                if (Name.contains(".cmeq") || Name.contains(".cmge") || 
                    Name.contains(".cmgt") || Name.contains(".cmle") || 
                    Name.contains(".cmlt"))
                    return 3;
                if (Name.contains(".vmins") || Name.contains(".vmaxs") || 
                    Name.contains(".umaxp") || Name.contains(".smaxp"))
                    return 3;
                if (Name.contains(".vminu") || Name.contains(".vmaxu"))
                    return 3;
                if (Name.contains(".vmaxv") || Name.contains(".vminv"))
                    return 3;
            }
        }

        if (Name.contains(".addp") || Name.contains(".saddlp") || 
            Name.contains(".uaddlp"))
        {
            if (Name.contains(".v4"))
                return 2;
            if (Name.contains(".v8"))
                return 2;
            if (Name.contains(".v16"))
                return 2;
            return 2;
        }

        if (Name.contains(".pmul") || Name.contains(".smull") || 
            Name.contains(".umull"))
        {
            if (Name.contains(".v4"))
                return 4;
            if (Name.contains(".v8"))
                return 4;
            return 4;
        }

        if (Name.contains(".vabs") || Name.contains(".vneg"))
            return 1;
        if (Name.contains(".vqabs") || Name.contains(".vqneg"))
            return 2;
        if (Name.contains(".vsqadd") || Name.contains(".vuqadd") || 
            Name.contains(".vqsub") || Name.contains(".vqadd"))
            return 3;
        if (Name.contains(".fcvt"))
            return 3;
        if (Name.contains(".rshrn") || Name.contains(".sqshrn") || 
            Name.contains(".uqshrn"))
            return 3;
        if (Name.contains(".sqshlu") || Name.contains(".sqrshrun"))
            return 4;

        if (Name.contains(".vext"))
            return 3;
        if (Name.contains(".vrev"))
            return 1;
        if (Name.contains(".vzip") || Name.contains(".vuzp"))
            return 3;
        if (Name.contains(".vtrn"))
            return 3;

        if (Name.contains(".vtbl") || Name.contains(".vtbx"))
        {
            if (Name.contains(".v8") || Name.contains(".v16"))
                return 6;
            return 3;
        }

        if (Name.contains(".vld1"))
        {
            if (Name.contains(".lane"))
                return 8;
            return 4;
        }
        if (Name.contains(".vst1"))
        {
            if (Name.contains(".lane"))
                return 3;
            return 1;
        }
        if (Name.contains(".vld2"))
        {
            if (Name.contains(".lane"))
                return 8;
            return 8;
        }
        if (Name.contains(".vst2"))
        {
            if (Name.contains(".lane"))
                return 3;
            return 3;
        }
        if (Name.contains(".vld3"))
        {
            if (Name.contains(".lane"))
                return 8;
            return 9;
        }
        if (Name.contains(".vst3"))
        {
            if (Name.contains(".lane"))
                return 3;
            return 3;
        }
        if (Name.contains(".vld4"))
        {
            if (Name.contains(".lane"))
                return 8;
            return 9;
        }
        if (Name.contains(".vst4"))
        {
            if (Name.contains(".lane"))
                return 3;
            return 4;
        }

        if (Name.contains(".vshl") || Name.contains(".vshr"))
            return 2;

        if (Name.contains(".vcvt"))
            return 3;

        if (Name.contains(".vset_lane"))
            return 2;
        if (Name.contains(".vget_lane"))
            return 2;

        if (Name.contains(".vadd_acc") || Name.contains(".vmla_acc"))
            return 4;

        if (Name.contains(".sdot") || Name.contains(".udot"))
            return 4;

        if (Name.contains(".vmul"))
        {
            if (Name.ends_with("f64") || Name.ends_with("f32"))
                return 4;
            return 3;
        }

        if (Name.contains(".vdiv"))
        {
            if (Name.ends_with("f64"))
                return 17;
            if (Name.ends_with("f32"))
                return 12;
        }

        if (Name.contains(".vdup_lane"))
            return 2;

        if (Name.contains(".vtbl2") || Name.contains(".vtbx2"))
            return 4;

        if (Name.contains(".vpadd"))
            return 2;

        if (Name.contains(".vpmax") || Name.contains(".vpmin"))
            return 2;
    }

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
        return 9;

    if (Name.starts_with("llvm.bswap"))
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

    if (Name.starts_with("llvm.masked.gather") || Name.starts_with("llvm.masked.scatter"))
        return 8;

    if (Name.starts_with("llvm.matrix.transpose"))
        return 6;

    if (Name.starts_with("llvm.matrix.multiply"))
        return 10;

    if (Name.starts_with("llvm.matrix"))
        return 8;

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

    if (Name.starts_with("llvm.aarch64.neon.aese") ||
        Name.starts_with("llvm.aarch64.neon.aesd"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.fcvt"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.rshrn") ||
        Name.starts_with("llvm.aarch64.neon.sqshrn") ||
        Name.starts_with("llvm.aarch64.neon.uqshrn"))
        return 3;

    if (Name.starts_with("llvm.aarch64.neon.sqshlu") ||
        Name.starts_with("llvm.aarch64.neon.sqrshrun"))
        return 4;

    if (Name.starts_with("llvm.vector.reduce.smax") || 
        Name.starts_with("llvm.vector.reduce.smin") || 
        Name.starts_with("llvm.vector.reduce.umax") || 
        Name.starts_with("llvm.vector.reduce.umin"))
        return 7;

    if (Name.starts_with("llvm.vector.reduce.fmax") || 
        Name.starts_with("llvm.vector.reduce.fmin"))
        return 10;

    if (Name.starts_with("llvm.vector.reduce.fadd"))
        return 9;

    if (Name.starts_with("llvm.vector.reduce.fmul"))
        return 10;
        
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

            if (Callee->getName() == "__check_fuel")
                return 0;

            unsigned Cost = 3;
            Cost += std::min(8u, Call->arg_size());
            
            return Cost;
        }
        else
        {
            unsigned Cost = 5;
            Cost += std::min(8u, Call->arg_size());
            
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
            baseCost = 5;
            break;

        case Instruction::FMul:
            isFloatingOp = true;
            baseCost = 7;
            break;

        case Instruction::FDiv:
            isFloatingOp = true;
            baseCost = 17;
            break;

        case Instruction::FRem:
            isFloatingOp = true;
            baseCost = 20;
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
            isFloatingOp = true;
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
            baseCost = 3;
            break;

        default:
            return 0;
    }

    if(!isFloatingOp)
    {
        if (isFloat && isVector)
            baseCost *= 3;
        else if (isFloat || isVector)
            baseCost *= 2;
    }
    else
    {
        if (isVector)
            baseCost = (baseCost * 5) / 2; // 2.5x
    }

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