#ifndef LLVM_TRANSFORMS_RUNTIMESIG_FUELRTSIG_H
#define LLVM_TRANSFORMS_RUNTIMESIG_FUELRTSIG_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class FuelRTSigPass : public PassInfoMixin<FuelRTSigPass> 
{
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

    static bool isRequired() { return true; }
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_RUNTIMESIG_FUELRTSIG_H