#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <type_traits>

struct Snapshot {
    uint64_t fuel;
    uint64_t runtime_sig;
    uint64_t memory_usage;
    uint64_t instruction_count;
};

struct SnapshotX86 : public Snapshot {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rsp, rbp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rflags;
    
    struct {
        uint64_t low, high;
    } xmm[16];
    
    struct {
        uint64_t low, high;
    } ymm_upper[16];
    
    struct {
        uint64_t q0, q1, q2, q3;
    } zmm_upper[32];
    
    uint16_t cs, ds, es, fs, gs, ss;
    uint64_t cr0, cr2, cr3, cr4;
    uint64_t msr_efer, msr_star, msr_lstar;
};

struct SnapshotAArch64 : public Snapshot {
    uint64_t x[31];
    uint64_t sp;
    
    struct {
        uint64_t low, high;
    } v[32];
    
    uint64_t nzcv, fpcr, fpsr, tpidr_el0, tpidrro_el0;
    uint64_t elr_el1, spsr_el1;
    uint64_t ttbr0_el1, ttbr1_el1, tcr_el1, mair_el1, sctlr_el1;
};

namespace RegisterMasks {
    namespace X86 {
        enum Field : uint8_t {
            BASE_FIELDS = 0,
            RAX = 4, RBX, RCX, RDX, RSI, RDI, RSP, RBP,
            R8, R9, R10, R11, R12, R13, R14, R15,
            RFLAGS = 20,
            XMM0 = 21, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
            XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
            YMM0_UPPER = 37, YMM1_UPPER, YMM2_UPPER, YMM3_UPPER,
            YMM4_UPPER, YMM5_UPPER, YMM6_UPPER, YMM7_UPPER,
            YMM8_UPPER, YMM9_UPPER, YMM10_UPPER, YMM11_UPPER,
            YMM12_UPPER, YMM13_UPPER, YMM14_UPPER, YMM15_UPPER,
            SEGMENTS = 53,
            CR0 = 54, CR2, CR3, CR4,
            MSR_EFER = 58, MSR_STAR, MSR_LSTAR,
            ZMM_START = 61
        };
    }
    
    namespace AArch64 {
        enum Field : uint8_t {
            BASE_FIELDS = 0,
            X0 = 4, X1, X2, X3, X4, X5, X6, X7,
            X8, X9, X10, X11, X12, X13, X14, X15,
            X16, X17, X18, X19, X20, X21, X22, X23,
            X24, X25, X26, X27, X28, X29, X30, SP,
            V0 = 36, V1, V2, V3, V4, V5, V6, V7,
            V8, V9, V10, V11, V12, V13, V14, V15,
            V16, V17, V18, V19, V20, V21, V22, V23,
            V24, V25, V26, V27, V28, V29, V30, V31,
            NZCV = 68, FPCR, FPSR, TPIDR_EL0, TPIDRRO_EL0,
            ELR_EL1, SPSR_EL1, TTBR0_EL1, TTBR1_EL1, TCR_EL1, MAIR_EL1, SCTLR_EL1
        };
    }
}

template<typename SnapshotType>
struct DeltaSnapshot {
    uint64_t changed_registers_mask[2];
    std::vector<uint8_t> compressed_data;
    
    DeltaSnapshot() {
        changed_registers_mask[0] = 0;
        changed_registers_mask[1] = 0;
    }
};

template<typename SnapshotType>
class SnapshotDeltaCompressor {
private:
    static void setBit(DeltaSnapshot<SnapshotType>& delta, uint8_t bit) {
        uint8_t idx = bit / 64;
        uint8_t pos = bit % 64;
        delta.changed_registers_mask[idx] |= (1ULL << pos);
    }
    
    static void appendData(DeltaSnapshot<SnapshotType>& delta, const void* data, size_t size) {
        size_t current_size = delta.compressed_data.size();
        delta.compressed_data.resize(current_size + size);
        std::memcpy(delta.compressed_data.data() + current_size, data, size);
    }
    
    static DeltaSnapshot<SnapshotX86> createDeltaX86(const SnapshotX86& old_snap, const SnapshotX86& new_snap) {
        DeltaSnapshot<SnapshotX86> delta;
        
        if (std::memcmp(&old_snap.fuel, &new_snap.fuel, sizeof(Snapshot)) != 0) {
            setBit(delta, RegisterMasks::X86::BASE_FIELDS);
            appendData(delta, &new_snap.fuel, sizeof(Snapshot));
        }
        
        const uint64_t* old_gprs = &old_snap.rax;
        const uint64_t* new_gprs = &new_snap.rax;
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (old_gprs[idx] != new_gprs[idx]) {
                setBit(delta, RegisterMasks::X86::RAX + idx);
                appendData(delta, &new_gprs[idx], sizeof(uint64_t));
            }
        }
        
        if (old_snap.rflags != new_snap.rflags) {
            setBit(delta, RegisterMasks::X86::RFLAGS);
            appendData(delta, &new_snap.rflags, sizeof(uint64_t));
        }
        
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (std::memcmp(&old_snap.xmm[idx], &new_snap.xmm[idx], 16) != 0) {
                setBit(delta, RegisterMasks::X86::XMM0 + idx);
                appendData(delta, &new_snap.xmm[idx], 16);
            }
        }
        
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (std::memcmp(&old_snap.ymm_upper[idx], &new_snap.ymm_upper[idx], 16) != 0) {
                setBit(delta, RegisterMasks::X86::YMM0_UPPER + idx);
                appendData(delta, &new_snap.ymm_upper[idx], 16);
            }
        }
        
        if (std::memcmp(&old_snap.cs, &new_snap.cs, 12) != 0) {
            setBit(delta, RegisterMasks::X86::SEGMENTS);
            appendData(delta, &new_snap.cs, 12);
        }
        
        const uint64_t* old_crs = &old_snap.cr0;
        const uint64_t* new_crs = &new_snap.cr0;
        for (uint8_t idx = 0; idx < 4; ++idx) {
            if (old_crs[idx] != new_crs[idx]) {
                setBit(delta, RegisterMasks::X86::CR0 + idx);
                appendData(delta, &new_crs[idx], sizeof(uint64_t));
            }
        }
        
        const uint64_t* old_msrs = &old_snap.msr_efer;
        const uint64_t* new_msrs = &new_snap.msr_efer;
        for (uint8_t idx = 0; idx < 3; ++idx) {
            if (old_msrs[idx] != new_msrs[idx]) {
                setBit(delta, RegisterMasks::X86::MSR_EFER + idx);
                appendData(delta, &new_msrs[idx], sizeof(uint64_t));
            }
        }
        
        for (uint8_t idx = 0; idx < 32; ++idx) {
            if (std::memcmp(&old_snap.zmm_upper[idx], &new_snap.zmm_upper[idx], 32) != 0) {
                setBit(delta, RegisterMasks::X86::ZMM_START);
                appendData(delta, &idx, 1);
                appendData(delta, &new_snap.zmm_upper[idx], 32);
            }
        }
        
        return delta;
    }
    
    static DeltaSnapshot<SnapshotAArch64> createDeltaAArch64(const SnapshotAArch64& old_snap, const SnapshotAArch64& new_snap) {
        DeltaSnapshot<SnapshotAArch64> delta;
        
        if (std::memcmp(&old_snap.fuel, &new_snap.fuel, sizeof(Snapshot)) != 0) {
            setBit(delta, RegisterMasks::AArch64::BASE_FIELDS);
            appendData(delta, &new_snap.fuel, sizeof(Snapshot));
        }
        
        for (uint8_t idx = 0; idx < 31; ++idx) {
            if (old_snap.x[idx] != new_snap.x[idx]) {
                setBit(delta, RegisterMasks::AArch64::X0 + idx);
                appendData(delta, &new_snap.x[idx], sizeof(uint64_t));
            }
        }
        
        if (old_snap.sp != new_snap.sp) {
            setBit(delta, RegisterMasks::AArch64::SP);
            appendData(delta, &new_snap.sp, sizeof(uint64_t));
        }
        
        for (uint8_t idx = 0; idx < 32; ++idx) {
            if (std::memcmp(&old_snap.v[idx], &new_snap.v[idx], 16) != 0) {
                setBit(delta, RegisterMasks::AArch64::V0 + idx);
                appendData(delta, &new_snap.v[idx], 16);
            }
        }
        
        const uint64_t* old_sys = &old_snap.nzcv;
        const uint64_t* new_sys = &new_snap.nzcv;
        for (uint8_t idx = 0; idx < 10; ++idx) {
            if (old_sys[idx] != new_sys[idx]) {
                setBit(delta, RegisterMasks::AArch64::NZCV + idx);
                appendData(delta, &new_sys[idx], sizeof(uint64_t));
            }
        }
        
        return delta;
    }
    
public:
    static DeltaSnapshot<SnapshotType> createDelta(const SnapshotType& old_snapshot, const SnapshotType& new_snapshot) {
        if constexpr (std::is_same_v<SnapshotType, SnapshotX86>) {
            return createDeltaX86(old_snapshot, new_snapshot);
        } else if constexpr (std::is_same_v<SnapshotType, SnapshotAArch64>) {
            return createDeltaAArch64(old_snapshot, new_snapshot);
        }
    }
    
    static SnapshotType applyDelta(const SnapshotType& base_snapshot, const DeltaSnapshot<SnapshotType>& delta) {
        if constexpr (std::is_same_v<SnapshotType, SnapshotX86>) {
            return applyDeltaX86(base_snapshot, delta);
        } else if constexpr (std::is_same_v<SnapshotType, SnapshotAArch64>) {
            return applyDeltaAArch64(base_snapshot, delta);
        }
    }
    
private:
    static SnapshotX86 applyDeltaX86(const SnapshotX86& base, const DeltaSnapshot<SnapshotX86>& delta) {
        SnapshotX86 result = base;
        size_t data_offset = 0;
        
        auto checkBit = [&](uint8_t bit) -> bool {
            uint8_t idx = bit / 64;
            uint8_t pos = bit % 64;
            return delta.changed_registers_mask[idx] & (1ULL << pos);
        };
        
        auto readData = [&](void* dest, size_t size) {
            std::memcpy(dest, delta.compressed_data.data() + data_offset, size);
            data_offset += size;
        };
        
        if (checkBit(RegisterMasks::X86::BASE_FIELDS)) {
            readData(&result.fuel, sizeof(Snapshot));
        }
        
        uint64_t* gprs = &result.rax;
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (checkBit(RegisterMasks::X86::RAX + idx)) {
                readData(&gprs[idx], sizeof(uint64_t));
            }
        }
        
        if (checkBit(RegisterMasks::X86::RFLAGS)) {
            readData(&result.rflags, sizeof(uint64_t));
        }
        
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (checkBit(RegisterMasks::X86::XMM0 + idx)) {
                readData(&result.xmm[idx], 16);
            }
        }
        
        for (uint8_t idx = 0; idx < 16; ++idx) {
            if (checkBit(RegisterMasks::X86::YMM0_UPPER + idx)) {
                readData(&result.ymm_upper[idx], 16);
            }
        }
        
        if (checkBit(RegisterMasks::X86::SEGMENTS)) {
            readData(&result.cs, 12);
        }
        
        uint64_t* crs = &result.cr0;
        for (uint8_t idx = 0; idx < 4; ++idx) {
            if (checkBit(RegisterMasks::X86::CR0 + idx)) {
                readData(&crs[idx], sizeof(uint64_t));
            }
        }
        
        uint64_t* msrs = &result.msr_efer;
        for (uint8_t idx = 0; idx < 3; ++idx) {
            if (checkBit(RegisterMasks::X86::MSR_EFER + idx)) {
                readData(&msrs[idx], sizeof(uint64_t));
            }
        }
        
        if (checkBit(RegisterMasks::X86::ZMM_START)) {
            while (data_offset < delta.compressed_data.size()) {
                uint8_t zmm_idx;
                readData(&zmm_idx, 1);
                if (zmm_idx < 32) {
                    readData(&result.zmm_upper[zmm_idx], 32);
                }
            }
        }
        
        return result;
    }
    
    static SnapshotAArch64 applyDeltaAArch64(const SnapshotAArch64& base, const DeltaSnapshot<SnapshotAArch64>& delta) {
        SnapshotAArch64 result = base;
        size_t data_offset = 0;
        
        auto checkBit = [&](uint8_t bit) -> bool {
            uint8_t idx = bit / 64;
            uint8_t pos = bit % 64;
            return delta.changed_registers_mask[idx] & (1ULL << pos);
        };
        
        auto readData = [&](void* dest, size_t size) {
            std::memcpy(dest, delta.compressed_data.data() + data_offset, size);
            data_offset += size;
        };
        
        if (checkBit(RegisterMasks::AArch64::BASE_FIELDS)) {
            readData(&result.fuel, sizeof(Snapshot));
        }
        
        for (uint8_t idx = 0; idx < 31; ++idx) {
            if (checkBit(RegisterMasks::AArch64::X0 + idx)) {
                readData(&result.x[idx], sizeof(uint64_t));
            }
        }
        
        if (checkBit(RegisterMasks::AArch64::SP)) {
            readData(&result.sp, sizeof(uint64_t));
        }
        
        for (uint8_t idx = 0; idx < 32; ++idx) {
            if (checkBit(RegisterMasks::AArch64::V0 + idx)) {
                readData(&result.v[idx], 16);
            }
        }
        
        uint64_t* sys_regs = &result.nzcv;
        for (uint8_t idx = 0; idx < 10; ++idx) {
            if (checkBit(RegisterMasks::AArch64::NZCV + idx)) {
                readData(&sys_regs[idx], sizeof(uint64_t));
            }
        }
        
        return result;
    }
    
public:
    static double getCompressionRatio(const DeltaSnapshot<SnapshotType>& delta) {
        size_t compressed_size = delta.compressed_data.size() + 16;
        return static_cast<double>(sizeof(SnapshotType)) / compressed_size;
    }
    
    static size_t getChangedRegisterCount(const DeltaSnapshot<SnapshotType>& delta) {
        return __builtin_popcountll(delta.changed_registers_mask[0]) + 
               __builtin_popcountll(delta.changed_registers_mask[1]);
    }
};

using DeltaSnapshotX86 = DeltaSnapshot<SnapshotX86>;
using DeltaSnapshotAArch64 = DeltaSnapshot<SnapshotAArch64>;
using CompressorX86 = SnapshotDeltaCompressor<SnapshotX86>;
using CompressorAArch64 = SnapshotDeltaCompressor<SnapshotAArch64>;

template<typename SnapshotType>
class SnapshotStack {
private:
    SnapshotType base_snapshot;
    std::vector<DeltaSnapshot<SnapshotType>> delta_stack;
    bool has_base;
    
public:
    SnapshotStack() : has_base(false) {}
    
    void pushSnapshot(const SnapshotType& snapshot) {
        if (!has_base) {
            base_snapshot = snapshot;
            has_base = true;
        } else {
            SnapshotType current = getCurrentSnapshot();
            auto delta = SnapshotDeltaCompressor<SnapshotType>::createDelta(current, snapshot);
            delta_stack.push_back(std::move(delta));
        }
    }
    
    bool popSnapshot() {
        if (delta_stack.empty()) {
            if (has_base) {
                has_base = false;
                return true;
            }
            return false;
        }
        delta_stack.pop_back();
        return true;
    }
    
    SnapshotType getCurrentSnapshot() const {
        if (!has_base) {
            return SnapshotType{};
        }
        SnapshotType current = base_snapshot;
        for (const auto& delta : delta_stack) {
            current = SnapshotDeltaCompressor<SnapshotType>::applyDelta(current, delta);
        }
        return current;
    }
    
    size_t getDepth() const {
        return delta_stack.size() + (has_base ? 1 : 0);
    }
    
    size_t getTotalMemoryUsage() const {
        size_t total = has_base ? sizeof(SnapshotType) : 0;
        for (const auto& delta : delta_stack) {
            total += delta.compressed_data.size() + 16;
        }
        return total;
    }
    
    struct CompressionStats {
        size_t total_snapshots;
        size_t uncompressed_size;
        size_t compressed_size;
        double average_compression_ratio;
        size_t average_changed_registers;
    };
    
    CompressionStats getCompressionStats() const {
        CompressionStats stats = {};
        stats.total_snapshots = getDepth();
        stats.uncompressed_size = stats.total_snapshots * sizeof(SnapshotType);
        
        if (has_base) {
            stats.compressed_size += sizeof(SnapshotType);
        }
        
        double total_ratio = 0.0;
        size_t total_changed = 0;
        
        for (const auto& delta : delta_stack) {
            stats.compressed_size += delta.compressed_data.size() + 16;
            total_ratio += SnapshotDeltaCompressor<SnapshotType>::getCompressionRatio(delta);
            total_changed += SnapshotDeltaCompressor<SnapshotType>::getChangedRegisterCount(delta);
        }
        
        if (!delta_stack.empty()) {
            stats.average_compression_ratio = total_ratio / delta_stack.size();
            stats.average_changed_registers = total_changed / delta_stack.size();
        }
        
        return stats;
    }
};

using SnapshotStackX86 = SnapshotStack<SnapshotX86>;
using SnapshotStackAArch64 = SnapshotStack<SnapshotAArch64>;