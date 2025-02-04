#ifndef LLVM_TRANSFORMS_RUNTIMESIG_RUNTIMESIGNATURE_H
#define LLVM_TRANSFORMS_RUNTIMESIG_RUNTIMESIGNATURE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class RuntimeSignaturePass : public PassInfoMixin<RuntimeSignaturePass> 
{
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

    static bool isRequired() { return true; }
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_HELLONEW_HELLOWORLD_H