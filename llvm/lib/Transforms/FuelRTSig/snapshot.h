#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include <cstdint>
#include <vector>
#include <optional>

struct BBModifiedMemory {
    BBModifiedMemory() : addresses(), values() {}

    std::vector<uint64_t> addresses;
    std::vector<std::optional<uint64_t>> values;
};

struct BBModifiedRegisters {
    BBModifiedRegisters() : registers(), values() {}

    std::vector<uint16_t> registers;
    std::vector<std::optional<uint64_t>> values;
};

struct BBSnapshot {
    BBSnapshot() : memory(), registers() {}

    BBModifiedMemory memory;
    BBModifiedRegisters registers;
};

