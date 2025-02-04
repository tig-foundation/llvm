add_llvm_library(LLVMFuel MODULE
  Fuel.cpp
)

add_dependencies(LLVMFuel intrinsics_gen)