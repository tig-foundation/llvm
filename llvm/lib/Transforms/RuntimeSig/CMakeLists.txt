add_llvm_library(LLVMRuntimeSig MODULE
  RuntimeSignature.cpp
)

add_dependencies(LLVMRuntimeSig intrinsics_gen)
