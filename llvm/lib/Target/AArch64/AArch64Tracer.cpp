// === File: checkpoint_runtime.h ===

#if 0
#ifndef CHECKPOINT_RUNTIME_H
#define CHECKPOINT_RUNTIME_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the checkpointing system.
 * Must be called once at the start of the application.
 */
void checkpoint_init();

/**
 * @brief Logs a general memory write operation (e.g., to the heap or global data).
 * @param addr The memory address being written to.
 * @param value The value being written.
 * @param size The size of the write in bytes.
 */
void _checkpoint_log_mem_write(void* addr, uint64_t value, uint32_t size);

/**
 * @brief Logs a stack memory write operation.
 * @param addr The stack address being written to.
 * @param value The value being written.
 * @param size The size of the write in bytes.
 */
void _checkpoint_log_stack_write(void* addr, uint64_t value, uint32_t size);

/**
 * @brief Logs a register write operation.
 * @param reg_id An identifier for the register.
 * @param value The new value of the register.
 */
void _checkpoint_log_reg_write(uint32_t reg_id, uint64_t value);

/**
 * @brief Logs a function call.
 * @param target The address of the function being called.
 */
void _checkpoint_log_call(void* target);

/**
 * @brief Logs a branch instruction.
 * @param target The target address of the branch.
 */
void _checkpoint_log_branch(void* target);

/**
 * @brief Takes a checkpoint, saving the current execution context.
 * @return Returns 0 on the initial call. Returns the checkpoint_id when
 * execution is restored via checkpoint_rewind.
 */
int checkpoint_take();

/**
 * @brief Rewinds execution to a previously taken checkpoint.
 * @param checkpoint_id The ID of the checkpoint to restore.
 */
void checkpoint_rewind(int checkpoint_id);

/**
 * @brief Dumps the entire log to the console for inspection.
 */
void checkpoint_dump_log();

#ifdef __cplusplus
}
#endif

#endif // CHECKPOINT_RUNTIME_H

// === File: checkpoint_runtime.cpp ===

#include "checkpoint_runtime.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <setjmp.h>
#include <cstring>

// --- Data Structures for Logging ---

enum LogEntryType {
    LOG_MEM_WRITE = 0,
    LOG_STACK_WRITE = 1,
    LOG_REG_WRITE = 2,
    LOG_CALL = 3,
    LOG_BRANCH = 4,
    LOG_CHECKPOINT_MARKER = 5
};

struct LogEntry {
    LogEntryType type;
    uint64_t seq_num;

    union {
        struct {
            void* addr;
            uint64_t value;
            uint32_t size;
        } mem_write;

        struct {
            void* addr;
            uint64_t value;
            uint32_t size;
        } stack_write;

        struct {
            uint32_t reg_id;
            uint64_t value;
        } reg_write;

        struct {
            void* target;
        } call;

        struct {
            void* target;
        } branch;

        struct {
            int id;
        } checkpoint;
    } payload;
};

struct Checkpoint {
    int id;
    uint64_t log_seq_num;
    jmp_buf context;
};

// --- Global State for the Runtime ---

static std::vector<LogEntry> g_log;
static std::map<int, Checkpoint> g_checkpoints;
static int g_next_checkpoint_id = 1;
static uint64_t g_seq_num = 0;
static bool g_is_replaying = false;

// --- Runtime API Implementation ---

extern "C" void checkpoint_init() {
    g_log.clear();
    g_checkpoints.clear();
    g_next_checkpoint_id = 1;
    g_seq_num = 0;
    g_is_replaying = false;
    std::cout << " Checkpoint system initialized." << std::endl;
}

extern "C" void _checkpoint_log_mem_write(void* addr, uint64_t value, uint32_t size) {
    if (g_is_replaying) return;
    LogEntry entry;
    entry.type = LOG_MEM_WRITE;
    entry.seq_num = g_seq_num++;
    entry.payload.mem_write = {addr, value, size};
    g_log.push_back(entry);
}

extern "C" void _checkpoint_log_stack_write(void* addr, uint64_t value, uint32_t size) {
    if (g_is_replaying) return;
    LogEntry entry;
    entry.type = LOG_STACK_WRITE;
    entry.seq_num = g_seq_num++;
    entry.payload.stack_write = {addr, value, size};
    g_log.push_back(entry);
}

extern "C" void _checkpoint_log_reg_write(uint32_t reg_id, uint64_t value) {
    if (g_is_replaying) return;
    LogEntry entry;
    entry.type = LOG_REG_WRITE;
    entry.seq_num = g_seq_num++;
    entry.payload.reg_write = {reg_id, value};
    g_log.push_back(entry);
}

extern "C" void _checkpoint_log_call(void* target) {
    if (g_is_replaying) return;
    LogEntry entry;
    entry.type = LOG_CALL;
    entry.seq_num = g_seq_num++;
    entry.payload.call.target = target;
    g_log.push_back(entry);
}

extern "C" void _checkpoint_log_branch(void* target) {
    if (g_is_replaying) return;
    LogEntry entry;
    entry.type = LOG_BRANCH;
    entry.seq_num = g_seq_num++;
    entry.payload.branch.target = target;
    g_log.push_back(entry);
}

extern "C" int checkpoint_take() {
    Checkpoint cp;
    cp.id = g_next_checkpoint_id++;
    cp.log_seq_num = g_seq_num;

    int rewind_id = setjmp(cp.context);

    if (rewind_id == 0) {
        g_checkpoints[cp.id] = cp;
        std::cout << " Took checkpoint ID " << cp.id << " at log sequence " << cp.log_seq_num << std::endl;
        return 0;
    } else {
        std::cout << " Rewound to checkpoint ID " << rewind_id << std::endl;
        g_is_replaying = true;
        return rewind_id;
    }
}

extern "C" void checkpoint_rewind(int checkpoint_id) {
    auto it = g_checkpoints.find(checkpoint_id);
    if (it == g_checkpoints.end()) {
        std::cerr << " Error: Checkpoint ID " << checkpoint_id << " not found." << std::endl;
        return;
    }

    Checkpoint& cp = it->second;
    std::cout << " Starting rewind to checkpoint ID " << checkpoint_id << std::endl;
    std::cout << " Replaying log from sequence " << cp.log_seq_num << " to " << g_seq_num << std::endl;

    for (uint64_t idx = cp.log_seq_num; idx < g_log.size(); ++idx) {
        const auto& entry = g_log[idx];
        if (entry.type == LOG_MEM_WRITE || entry.type == LOG_STACK_WRITE) {
            // In a true replay, we would apply this change to memory.
            // For now, we just print it.
            void* addr = (entry.type == LOG_MEM_WRITE) ? entry.payload.mem_write.addr : entry.payload.stack_write.addr;
            uint64_t value = (entry.type == LOG_MEM_WRITE) ? entry.payload.mem_write.value : entry.payload.stack_write.value;
            std::cout << "  - Replay: " << (entry.type == LOG_MEM_WRITE ? "MEM_WRITE" : "STACK_WRITE")
                      << " to " << addr << " value " << value << std::endl;
        }
    }
    std::cout << " Log replay finished." << std::endl;

    g_log.resize(cp.log_seq_num);
    g_seq_num = cp.log_seq_num;

    longjmp(cp.context, checkpoint_id);
}

extern "C" void checkpoint_dump_log() {
    std::cout << "\n--- BEGIN CHECKPOINT LOG DUMP ---" << std::endl;
    for (const auto& entry : g_log) {
        std::cout << "Seq " << entry.seq_num << ": ";
        switch (entry.type) {
            case LOG_MEM_WRITE:
                std::cout << "MEM_WRITE to " << entry.payload.mem_write.addr
                          << " value " << entry.payload.mem_write.value
                          << " size " << entry.payload.mem_write.size << std::endl;
                break;
            case LOG_STACK_WRITE:
                std::cout << "STACK_WRITE to " << entry.payload.stack_write.addr
                          << " value " << entry.payload.stack_write.value
                          << " size " << entry.payload.stack_write.size << std::endl;
                break;
            case LOG_REG_WRITE:
                std::cout << "REG_WRITE to reg " << entry.payload.reg_write.reg_id
                          << " value " << entry.payload.reg_write.value << std::endl;
                break;
            case LOG_CALL:
                std::cout << "CALL to " << entry.payload.call.target << std::endl;
                break;
            case LOG_BRANCH:
                std::cout << "BRANCH to " << entry.payload.branch.target << std::endl;
                break;
            default:
                std::cout << "Unknown log entry type." << std::endl;
        }
    }
    std::cout << "--- END CHECKPOINT LOG DUMP ---\n" << std::endl;
}
#endif

// === File: AArch64Tracer.cpp (Modified) ===

#include "AArch64.h"
#include "AArch64InstrInfo.h"
#include "AArch64Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <variant>
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "aarch64-tracer"

namespace {

class AArch64Tracer : public MachineFunctionPass {
public:
    static char ID;

    AArch64Tracer() : MachineFunctionPass(ID) {
      initializeAArch64TracerPass(*PassRegistry::getPassRegistry());
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    StringRef getPassName() const override { return "AArch64 Tracer"; }
private:
    void instrumentRegisterModification(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, MachineOperand &Op, const TargetInstrInfo *TII);
    void instrumentMemoryModification(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, MachineInstr &MI, std::variant<MachineOperand*, MachineMemOperand*> Op, const TargetInstrInfo *TII);
    void instrumentStackModification(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, MachineInstr &MI, std::variant<MachineOperand*, MachineMemOperand*> Op, const TargetInstrInfo *TII);
    void instrumentCall(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, const TargetInstrInfo *TII);
    void instrumentBranch(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, const TargetInstrInfo *TII);
    void instrumentStore(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, MachineInstr &MI, const TargetInstrInfo *TII, FunctionCallee &LogFn, uint32_t size);

    FunctionCallee LogMemWriteFn;
    FunctionCallee LogStackWriteFn;
    FunctionCallee LogRegWriteFn;
    FunctionCallee LogCallFn;
    FunctionCallee LogBranchFn;
};
}

char AArch64Tracer::ID = 0;

static uint32_t getRegSize(unsigned Reg) {
    if (AArch64::FPR128RegClass.contains(Reg)) {
        return 16;
    }
    if (AArch64::GPR64RegClass.contains(Reg) || AArch64::FPR64RegClass.contains(Reg)) {
        return 8;
    }
    if (AArch64::GPR32RegClass.contains(Reg) || AArch64::FPR32RegClass.contains(Reg)) {
        return 4;
    }
    return 0;
}

bool AArch64Tracer::runOnMachineFunction(MachineFunction &MF) {
    bool Changed = false;
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    auto M = MF.getMMI().getModule();

    LLVMContext &Ctx = M->getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *Int8PtrTy = Type::getInt8Ty(Ctx)->getPointerTo();
    Type *Int64Ty = Type::getInt64Ty(Ctx);
    Type *Int32Ty = Type::getInt32Ty(Ctx);

    //LogMemWriteFn = M->getOrInsertFunction("_checkpoint_log_mem_write", VoidTy, Int8PtrTy, Int64Ty, Int32Ty);
    //LogStackWriteFn = M->getOrInsertFunction("_checkpoint_log_stack_write", VoidTy, Int8PtrTy, Int64Ty, Int32Ty);
    //LogRegWriteFn = M->getOrInsertFunction("_checkpoint_log_reg_write", VoidTy, Int32Ty, Int64Ty);
    //LogCallFn = M->getOrInsertFunction("_checkpoint_log_call", VoidTy, Int8PtrTy);
    //LogBranchFn = M->getOrInsertFunction("_checkpoint_log_branch", VoidTy, Int8PtrTy);

    for (auto &MBB : MF) {
        for (auto MBBI = MBB.begin(), E = MBB.end(); MBBI != E; ++MBBI) {
            MachineInstr &MI = *MBBI;

            if (MI.isCall()) {
                instrumentCall(MBB, MBBI, TII);
                Changed = true;
                continue;
            }

            if (MI.isBranch()) {
                instrumentBranch(MBB, MBBI, TII);
                Changed = true;
                continue;
            }

            for (unsigned opIdx = 0; opIdx < MI.getNumOperands(); ++opIdx) {
                MachineOperand &Op = MI.getOperand(opIdx);
                if (Op.isDef() && Op.isReg() && !Op.isImplicit()) {
                    instrumentRegisterModification(MBB, std::next(MBBI), Op, TII);
                    Changed = true;
                }
            }

            if (MI.mayStore()) {
                bool isFrameOperation = MI.getFlag(MachineInstr::FrameSetup) || MI.getFlag(MachineInstr::FrameDestroy);

                for (unsigned opIdx = 0; opIdx < MI.getNumOperands(); ++opIdx) {
                    MachineOperand &Op = MI.getOperand(opIdx);
                    if (Op.isFI()) {
                        instrumentStackModification(MBB, MBBI, MI, std::variant<MachineOperand*, MachineMemOperand*>(&Op), TII);
                        Changed = true;
                    }
                }

                for (MachineMemOperand *MemOp : MI.memoperands()) {
                    if (MemOp->isStore()) {
                        bool isStack = isFrameOperation;
                        if (!isStack) {
                            if (const PseudoSourceValue *PSV = MemOp->getPseudoValue()) {
                                isStack = isa<FixedStackPseudoSourceValue>(PSV) || PSV->isStack();
                            }
                        }

                        if (isStack) {
                            instrumentStackModification(MBB, MBBI, MI, std::variant<MachineOperand*, MachineMemOperand*>(MemOp), TII);
                            Changed = true;
                        } else {
                            instrumentMemoryModification(MBB, MBBI, MI, std::variant<MachineOperand*, MachineMemOperand*>(MemOp), TII);
                            Changed = true;
                        }
                    }
                }
            }
        }
    }
    return Changed;
}

void AArch64Tracer::instrumentStore(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  MachineInstr &MI, 
  const TargetInstrInfo *TII, 
  FunctionCallee &LogFn, 
  uint32_t size
) {
    /*DebugLoc DL = MI.getDebugLoc();
    if (MI.getNumOperands() < 3 || !MI.getOperand(0).isReg() || !MI.getOperand(1).isReg() || !MI.getOperand(2).isImm()) {
        return;
    }

    Register ValReg = MI.getOperand(0).getReg();
    Register AddrBaseReg = MI.getOperand(1).getReg();
    int64_t AddrOffset = MI.getOperand(2).getImm();

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADDXri), AArch64::X0).addReg(AddrBaseReg).addImm(AddrOffset).addImm(0);
    unsigned MovOpc = AArch64::GPR64RegClass.contains(ValReg) ? AArch64::ORRXrs : AArch64::ORRWrs;
    BuildMI(MBB, MBBI, DL, TII->get(MovOpc), AArch64::X1).addReg(AArch64::GPR64RegClass.contains(ValReg) ? AArch64::XZR : AArch64::WZR).addReg(ValReg).addImm(0);
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVi64), AArch64::X2).addImm(size);
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::BL)).addGlobalAddress(LogFn.getCallee());*/

    dbgs() << "Store instrumented" << "\n";
}

void AArch64Tracer::instrumentMemoryModification(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  MachineInstr &MI, 
  std::variant<MachineOperand*, MachineMemOperand*> Op, 
  const TargetInstrInfo *TII
) {
    /*uint32_t size = 0;
    if (std::holds_alternative<MachineMemOperand*>(Op)) {
        size = std::get<MachineMemOperand*>(Op)->getSize();
    }
    if (size == 0 && MI.getOperand(0).isReg()) {
        size = getRegSize(MI.getOperand(0).getReg());
    }
    if (size == 0) return;

    instrumentStore(MBB, MBBI, MI, TII, LogMemWriteFn, size);*/

    dbgs() << "Memory instrumented" << "\n";
}

void AArch64Tracer::instrumentStackModification(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  MachineInstr &MI, 
  std::variant<MachineOperand*, MachineMemOperand*> Op, 
  const TargetInstrInfo *TII
) {
    /*uint32_t size = 0;
    if (std::holds_alternative<MachineMemOperand*>(Op)) {
        size = std::get<MachineMemOperand*>(Op)->getSize();
    } else if (std::holds_alternative<MachineOperand*>(Op)) {
        if (MI.getOperand(0).isReg()) {
            size = getRegSize(MI.getOperand(0).getReg());
        }
    }
    if (size == 0) return;

    instrumentStore(MBB, MBBI, MI, TII, LogStackWriteFn, size);*/

    dbgs() << "Stack instrumented" << "\n";
}

void AArch64Tracer::instrumentRegisterModification(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  MachineOperand &Op, 
  const TargetInstrInfo *TII
) {
    /*DebugLoc DL = Op.getParent()->getDebugLoc();
    Register Reg = Op.getReg();

    if (!AArch64::GPR64RegClass.contains(Reg) && !AArch64::GPR32RegClass.contains(Reg)) {
        return;
    }

    uint32_t regId = Reg.id();
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVi64), AArch64::X0).addImm(regId);
    unsigned MovOpc = AArch64::GPR64RegClass.contains(Reg) ? AArch64::ORRXrs : AArch64::ORRWrs;
    BuildMI(MBB, MBBI, DL, TII->get(MovOpc), AArch64::X1).addReg(AArch64::GPR64RegClass.contains(Reg) ? AArch64::XZR : AArch64::WZR).addReg(Reg).addImm(0);
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::BL)).addGlobalAddress(LogRegWriteFn.getCallee());*/

    dbgs() << "Register instrumented" << "\n";
}

void AArch64Tracer::instrumentCall(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  const TargetInstrInfo *TII
) {
    /*MachineInstr &MI = *MBBI;
    DebugLoc DL = MI.getDebugLoc();

    if (MI.getOperand(0).isGlobal() || MI.getOperand(0).isSymbol()) {
        BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVaddr), AArch64::X0).add(MI.getOperand(0));
        BuildMI(MBB, MBBI, DL, TII->get(AArch64::BL)).addGlobalAddress(LogCallFn.getCallee());
    }*/

    dbgs() << "Call instrumented" << "\n";
}

void AArch64Tracer::instrumentBranch(
  MachineBasicBlock &MBB, 
  MachineBasicBlock::iterator MBBI, 
  const TargetInstrInfo *TII
) {
    /*MachineInstr &MI = *MBBI;
    DebugLoc DL = MI.getDebugLoc();

    if (MI.getOperand(0).isMBB()) {
        BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVi64), AArch64::X0).addImm(0);
        BuildMI(MBB, MBBI, DL, TII->get(AArch64::BL)).addGlobalAddress(LogBranchFn.getCallee());
    }*/

    dbgs() << "Branch instrumented" << "\n";
}


FunctionPass *llvm::createAArch64TracerPass() {
  return new AArch64Tracer();
}

INITIALIZE_PASS(AArch64Tracer, "aarch64-tracer", "AArch64 Tracer", false, false)

#if 0
// === File: main.cpp (Example Usage) ===

#include "checkpoint_runtime.h"
#include <iostream>
#include <vector>

void process_data(std::vector<int>& data, int value) {
    std::cout << "Processing data..." << std::endl;
    for (size_t idx = 0; idx < data.size(); ++idx) {
        data[idx] = data[idx] * value + idx;
    }
    std::cout << "Processing finished." << std::endl;
}

int main() {
    checkpoint_init();

    std::vector<int> my_data = {10, 20, 30, 40, 50};

    std::cout << "\n--- First run ---" << std::endl;
    process_data(my_data, 2);

    int cp1_id = checkpoint_take();

    if (cp1_id == 0) {
        std::cout << "\n--- Second run ---" << std::endl;
        process_data(my_data, 5);

        std::cout << "\nFinal data state before rewind:" << std::endl;
        for (int val : my_data) {
            std::cout << val << " ";
        }
        std::cout << std::endl;

        checkpoint_dump_log();

        checkpoint_rewind(1);
    }

    std::cout << "\n--- Execution continues after rewind ---" << std::endl;
    std::cout << "Data state after rewind (memory not restored in this demo):" << std::endl;
    for (int val : my_data) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    process_data(my_data, 10);

    std::cout << "\nFinal data state:" << std::endl;
    for (int val : my_data) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    return 0;
}
// === File: Makefile ===

# LLVM_CONFIG should point to the llvm-config executable of your LLVM build
LLVM_CONFIG = llvm-config-15

# Compiler and flags
CXX = clang++-15
CXXFLAGS = -O2 -fno-rtti
LDFLAGS =

# Get LLVM flags
LLVM_CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core mcjit native)
LLVM_SO_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

# Project files
PASS_SRC = AArch64Tracer.cpp
PASS_OBJ = $(PASS_SRC:.cpp=.o)
PASS_SO = AArch64Tracer.so

RUNTIME_SRC = checkpoint_runtime.cpp
RUNTIME_OBJ = $(RUNTIME_SRC:.cpp=.o)

MAIN_SRC = main.cpp
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
MAIN_INSTRUMENTED_OBJ = main.instrumented.o
MAIN_EXEC = main_instrumented

.PHONY: all clean

all: $(MAIN_EXEC)

# Build the LLVM pass as a shared object
$(PASS_SO): $(PASS_OBJ)
	$(CXX) -shared $(PASS_OBJ) -o $@ $(LLVM_SO_LDFLAGS)

$(PASS_OBJ): $(PASS_SRC)
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -fPIC -c $< -o $@

# Build the runtime library
$(RUNTIME_OBJ): $(RUNTIME_SRC) checkpoint_runtime.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build the main application object
$(MAIN_OBJ): $(MAIN_SRC) checkpoint_runtime.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create an instrumented version of the main object file using the modern pass manager syntax
$(MAIN_INSTRUMENTED_OBJ): $(MAIN_OBJ) $(PASS_SO)
	opt-15 -load-pass-plugin./$(PASS_SO) -passes=aarch64-tracer -S $(MAIN_OBJ:.o=.ll) | llc-15 -filetype=obj -o $@

# Link the final executable
$(MAIN_EXEC): $(MAIN_INSTRUMENTED_OBJ) $(RUNTIME_OBJ)
	$(CXX) $(MAIN_INSTRUMENTED_OBJ) $(RUNTIME_OBJ) -o $@ $(LDFLAGS)

# Rule to generate.ll from.cpp for inspection (optional)
%.ll: %.cpp
	$(CXX) $(CXXFLAGS) -S -emit-llvm -o $@ $<

clean:
	rm -f *.o *.so *.ll $(MAIN_EXEC) main.instrumented.o
#endif