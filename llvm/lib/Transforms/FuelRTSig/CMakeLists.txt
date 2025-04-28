add_llvm_library(LLVMFuelRTSig MODULE
  FuelRTSig.cpp
)

add_dependencies(LLVMFuelRTSig intrinsics_gen)