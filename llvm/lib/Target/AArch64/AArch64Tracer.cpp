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
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

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

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::NoVRegs);
  }

private:
  // FIX: Change signatures to accept pointers (MachineOperand*) instead of references (&)
  bool traceRegisterModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx);
  bool traceStackModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx);
  bool traceMemoryModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx);
};
}

char AArch64Tracer::ID = 0;

bool AArch64Tracer::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;
  for (auto &MBB : MF) {
    std::vector<std::tuple<MachineInstr *, union {MachineOperand *m_op, MachineMemOperand *m_mem}, uint32_t>> stack_modifications;
    std::vector<std::tuple<MachineInstr *, union {MachineOperand *m_op, MachineMemOperand *m_mem}, uint32_t>> memory_modifications;

    uint32_t instr_idx = 0;
    for (auto &MI : MBB) {
      bool mayPerformStore = MI.mayStore();

      // Loop 1: Iterate over all regular operands
      for (unsigned opIdx = 0; opIdx < MI.getNumOperands(); ++opIdx) {
        MachineOperand &Op = MI.getOperand(opIdx);

        if (Op.isReg() && Op.isDef() && !Op.isImplicit()) {
          if(traceRegisterModification(MI, MBB, &Op, instr_idx)) {
            Changed = true;
            stack_modifications.push_back({ &MI, &Op, instr_idx });
          }
          continue;
        }

        if (mayPerformStore) {
          if (Op.isFI()) {
            if(traceStackModification(MI, MBB, &Op, instr_idx)) {
              Changed = true;
              stack_modifications.push_back({ &MI, &Op, instr_idx });
            }
          } else if (Op.isGlobal() || Op.isSymbol() || Op.isCPI() || Op.isTargetIndex()) {
            if(traceMemoryModification(MI, MBB, &Op, instr_idx)) {
              Changed = true;
              memory_modifications.push_back({ &MI, &Op, instr_idx });
            }
          }
        }
      }

      // Loop 2: Iterate over all MachineMemOperands
      for (const MachineMemOperand *MemOp : MI.memoperands()) {
        if (MemOp->isStore()) {

          // FIX Classification: Use PseudoSourceValue instead of Address Space.
          bool isStack = false;
          if (const PseudoSourceValue *PSV = MemOp->getPseudoValue()) {
            if (PSV->isStack()) {
              isStack = true;
            }
          }
          // If PSV is null (like for the heap pointer), isStack remains false.

          if (isStack) {
            // Trace as stack modification.
            // FIX UB: Pass nullptr instead of attempting an invalid cast.
            if(traceStackModification(MI, MBB, nullptr, instr_idx)) {
              Changed = true;
              stack_modifications.push_back(std::make_tuple(&MI, nullptr, instr_idx));
            }
          } else {
            // Trace as general memory modification (Heap, Globals, etc.)
            if(traceMemoryModification(MI, MBB, nullptr, instr_idx)) {
              Changed = true;
              memory_modifications.push_back(std::make_tuple(&MI, nullptr, instr_idx));
            }
          }
        }
      }
      instr_idx++;
    }

    // insert trace instructions
    for (auto &Mod : stack_modifications) {
        // auto instr = std::get<0>(Mod);
        // auto op = std::get<1>(Mod); // NOTE: op might be nullptr now!
        // auto instr_idx = std::get<2>(Mod);
        // ...
    }

    for (auto &Mod : memory_modifications) {
        // ...
    }
  }
  return Changed;
}

// Updated implementations to handle potential nullptr
bool AArch64Tracer::traceRegisterModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx) {
  dbgs() << "Tracing register modification: " << MI;
  if (Op) {
      dbgs() << " (" << *Op << ")";
  }
  dbgs() << "\n";
  return true;
}

bool AArch64Tracer::traceStackModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx) {
  dbgs() << "Tracing stack modification: " << MI;
  if (Op) {
      dbgs() << " (" << *Op << ")";
  }
  dbgs() << "\n";
  return true;
}

bool AArch64Tracer::traceMemoryModification(MachineInstr &MI, MachineBasicBlock &MBB, const MachineOperand *Op, uint32_t instr_idx) {
  dbgs() << "Tracing memory modification: " << MI;
  if (Op) {
      dbgs() << " (" << *Op << ")";
  }
  dbgs() << "\n";
  return true;
}

FunctionPass *llvm::createAArch64TracerPass() {
  return new AArch64Tracer();
}

INITIALIZE_PASS(AArch64Tracer, "aarch64-tracer", "AArch64 Tracer", false, false)