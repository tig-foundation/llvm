struct InstructionPrimes {
    // Integer arithmetic - Unsigned 32-bit
    static constexpr uint64_t Add_u32 = 0xEDC1A7F47ACD2EE3;
    static constexpr uint64_t Sub_u32 = 0x87353D33C2976737;
    static constexpr uint64_t Mul_u32 = 0x91C6A1D9B4EDC1AB;
    static constexpr uint64_t Div_u32 = 0xFD9B02FDC35C72B3;
    static constexpr uint64_t URem_u32 = 0xB161AB7FB04F004D;
    
    // Integer arithmetic - Signed 32-bit
    static constexpr uint64_t Add_s32 = 0xE620952D37DDFF91;
    static constexpr uint64_t Sub_s32 = 0xC4FA795EB531A38B;
    static constexpr uint64_t Mul_s32 = 0x8F28A03020BF8813;
    static constexpr uint64_t Div_s32 = 0xD80622EB6110253F;
    static constexpr uint64_t SRem_s32 = 0xC56DD2999CD1234F;

    // Integer arithmetic - Unsigned 64-bit
    static constexpr uint64_t Add_u64 = 0xBE8098FE5CFCC7B5;
    static constexpr uint64_t Sub_u64 = 0xE155AD1E824B25E3;
    static constexpr uint64_t Mul_u64 = 0x9A16DFEE16D4BBF5;
    static constexpr uint64_t Div_u64 = 0xF5F2BCF367F68ED1;
    static constexpr uint64_t URem_u64 = 0x96E8220B9C5E8C97;

    // Integer arithmetic - Signed 64-bit
    static constexpr uint64_t Add_s64 = 0x9BD972EB4677C3F1;
    static constexpr uint64_t Sub_s64 = 0xF2D231CD2E1FBA23;
    static constexpr uint64_t Mul_s64 = 0xE511FE4AEA87A429;
    static constexpr uint64_t Div_s64 = 0xA2E613C950EA7F17;
    static constexpr uint64_t SRem_s64 = 0xF0FB61E6281360FD;

    // Floating point - Single precision
    static constexpr uint64_t Add_f32 = 0x90B2AD995AF897CD;
    static constexpr uint64_t Sub_f32 = 0xC256B33A0EF0745F;
    static constexpr uint64_t Mul_f32 = 0x9AF1495384190163;
    static constexpr uint64_t Div_f32 = 0x966C40D3884717FF;
    static constexpr uint64_t Rem_f32 = 0x918C3A31D782B0E5;
    static constexpr uint64_t Sqrt_f32 = 0xECE9FBD3A6D77023;
    static constexpr uint64_t FNeg_f32 = 0xC002087960B604D5;

    // Floating point - Double precision
    static constexpr uint64_t Add_f64 = 0xC3EA137E165AECED;
    static constexpr uint64_t Sub_f64 = 0x8C4AAED901B0472B;
    static constexpr uint64_t Mul_f64 = 0x883C081D0CDE3065;
    static constexpr uint64_t Div_f64 = 0xFED4DB903BB79241;
    static constexpr uint64_t Rem_f64 = 0xC6051FFCF3752D2B;
    static constexpr uint64_t Sqrt_f64 = 0xb79249404c3bc015;
    static constexpr uint64_t FNeg_f64 = 0xE94FDBF317D00AB7;

    // Vector operations - element-wise
    static constexpr uint64_t Add_vec_i8 = 0x83edeae76b4b0545;
    static constexpr uint64_t Add_vec_i16 = 0xbceb49f5ee123215;
    static constexpr uint64_t Add_vec_i32 = 0x9B8C768C1401BA7B;
    static constexpr uint64_t Add_vec_i64 = 0xBC49DE9B3A818899;
    static constexpr uint64_t Add_vec_f32 = 0xA17576DF232E7669;
    static constexpr uint64_t Add_vec_f64 = 0xD298F1C0BF940F95;
    static constexpr uint64_t Mul_vec_i8 = 0x5d1e44ab549dfc51;
    static constexpr uint64_t Mul_vec_i16 = 0x9abf77d2ec41ae40;
    static constexpr uint64_t Mul_vec_i32 = 0xAE6A290706DD70E7;
    static constexpr uint64_t Mul_vec_i64 = 0xD4FDC03C4401C533;
    static constexpr uint64_t Mul_vec_f32 = 0xA3E6942DA60A926F;
    static constexpr uint64_t Mul_vec_f64 = 0xBCB5CD76C9DC3253;

    // Bitwise operations
    static constexpr uint64_t And_i8 = 0xb140c30437087f6a;
    static constexpr uint64_t And_i16 = 0xc76462d73bbe7cbe;
    static constexpr uint64_t And_i32 = 0x80B429DFBFC318EB;
    static constexpr uint64_t And_i64 = 0xBDA9ECF782F0D3FB;
    static constexpr uint64_t And_vec_i8 = 0x4c350adc099a1bfc;
    static constexpr uint64_t And_vec_i16 = 0xe456007c0cf46ec7;
    static constexpr uint64_t And_vec_i32 = 0x99c157351c99736d;
    static constexpr uint64_t And_vec_i64 = 0xc5d66049960c766e;
    static constexpr uint64_t Or_i8 = 0xf34d856aed6bc4ad;
    static constexpr uint64_t Or_i16 = 0x4f9929353fc5f4d4;
    static constexpr uint64_t Or_i32 = 0xB8290AAC45720989;
    static constexpr uint64_t Or_i64 = 0xD0C39B467979E695;
    static constexpr uint64_t Or_vec_i8 = 0x25c5797689c9686d;
    static constexpr uint64_t Or_vec_i16 = 0xa2eac591940b98a7;
    static constexpr uint64_t Or_vec_i32 = 0x9417356a48354c1a;
    static constexpr uint64_t Or_vec_i64 = 0x49e64a1fad3da826;
    static constexpr uint64_t Xor_i8 = 0x689037a101804223;
    static constexpr uint64_t Xor_i16 = 0x0f118fc766622ba9;
    static constexpr uint64_t Xor_i32 = 0xE12B4AF73AAABEEF;
    static constexpr uint64_t Xor_i64 = 0xC1B8CDB5496BAFD7;
    static constexpr uint64_t Xor_vec_i8 = 0x6230a2fffd9b7cd5;
    static constexpr uint64_t Xor_vec_i16 = 0x51e5587912d014d8;
    static constexpr uint64_t Xor_vec_i32 = 0x5c92d5e3d8dec0a8;
    static constexpr uint64_t Xor_vec_i64 = 0x5b65623373691196;
    static constexpr uint64_t Shl_i8 = 0xcb71176a5fda261a;
    static constexpr uint64_t Shl_i16 = 0x4b3ba5cdbadb541e;
    static constexpr uint64_t Shl_i32 = 0x88330E6E1BFA5411;
    static constexpr uint64_t Shl_i64 = 0x8374F43D807A6F91;
    static constexpr uint64_t Shl_vec_i8 = 0xd376b899abe4c00d;
    static constexpr uint64_t Shl_vec_i16 = 0x38d3b8afe32bec91;
    static constexpr uint64_t Shl_vec_i32 = 0x6bbcf9501dbe1c6b;
    static constexpr uint64_t Shl_vec_i64 = 0x91a76a465de9d163;
    static constexpr uint64_t LShr_i8 = 0x554668ed91a7ec2d;
    static constexpr uint64_t LShr_i16 = 0xd094dd15a208d797;
    static constexpr uint64_t LShr_i32 = 0x21cf38cc21ec1aa8;
    static constexpr uint64_t LShr_i64 = 0xb8b42b0acea289f4;
    static constexpr uint64_t LShr_vec_i8 = 0x0e6b9540fead6e5e;
    static constexpr uint64_t LShr_vec_i16 = 0xe80025d702be8e2a;
    static constexpr uint64_t LShr_vec_i32 = 0x560da37e0a8108af;
    static constexpr uint64_t LShr_vec_i64 = 0x8afe29d9ed33cc71;
    static constexpr uint64_t AShr_i32 = 0xCEDC3D307D8D8683;
    static constexpr uint64_t AShr_i64 = 0x9DA4540AE2D7A161;

    // Memory operations
    static constexpr uint64_t Load_i32 = 0x9A9E961B54B29955;
    static constexpr uint64_t Load_i64 = 0x8B1B3D4D77BC7BB3;
    static constexpr uint64_t Load_f32 = 0xC161CEDAF256D5E5;
    static constexpr uint64_t Load_f64 = 0xD1E5DA2FDCD5E157;
    static constexpr uint64_t Store_i32 = 0x986CCCCFA9F5B10B;
    static constexpr uint64_t Store_i64 = 0x8CC7C690FF547D63;
    static constexpr uint64_t Store_f32 = 0x80FF2A3B14A4D19D;
    static constexpr uint64_t Store_f64 = 0xE0E9526F53C79197;

    // Comparison operations - Integer
    static constexpr uint64_t ICmp_eq = 0xB19319E767B773DF;
    static constexpr uint64_t ICmp_ne = 0x8AC38410037C32D5;
    static constexpr uint64_t ICmp_ugt = 0xAB0A8BAC52D5D76B;
    static constexpr uint64_t ICmp_uge = 0x9CDBFC00628D628B;
    static constexpr uint64_t ICmp_ult = 0x84378B096D13B6A3;
    static constexpr uint64_t ICmp_ule = 0xC04FBAAA56FA0DAB;
    static constexpr uint64_t ICmp_sgt = 0xCE9AA2EB4B22456B;
    static constexpr uint64_t ICmp_sge = 0xF165D7826D16DF47;
    static constexpr uint64_t ICmp_slt = 0xE95BA56F275D3EC5;
    static constexpr uint64_t ICmp_sle = 0x8C3F2F6F2EB0C34F;

    // Comparison operations - Floating point
    static constexpr uint64_t FCmp_oeq = 0xBF2A12007525FEED;
    static constexpr uint64_t FCmp_ogt = 0xF3A8D718FEC1B393;
    static constexpr uint64_t FCmp_oge = 0xF4692B3CA0566779;
    static constexpr uint64_t FCmp_olt = 0xADB80126D86C7295;
    static constexpr uint64_t FCmp_ole = 0xE6FD4C5BCBC70E3D;
    static constexpr uint64_t FCmp_one = 0xD89E33A78DBF9527;
    static constexpr uint64_t FCmp_ord = 0x8DB562EEC3BBD64F;
    static constexpr uint64_t FCmp_uno = 0xDA31E485E5E0C445;
    static constexpr uint64_t FCmp_ueq = 0xBFA9439C30B70919;
    static constexpr uint64_t FCmp_ugt = 0xDF9DC483E11B08A5;

    // Type conversions
    static constexpr uint64_t Trunc_i64_to_i32 = 0xc157d2bc1841b32d;
    static constexpr uint64_t ZExt_i32_to_i64 = 0x01c38c3e379c1db7;
    static constexpr uint64_t SExt_i32_to_i64 = 0xd69987668e5f1517;
    static constexpr uint64_t FPToUI_f32_to_i32 = 0x5654ebd597d15493;
    static constexpr uint64_t FPToUI_f32_to_i64 = 0xbea6044ca4f7e3d3;
    static constexpr uint64_t FPToUI_f64_to_i32 = 0xaf66598270b3937e;
    static constexpr uint64_t FPToUI_f64_to_i64 = 0x11853d813fe9d747;
    static constexpr uint64_t FPToSI_f32_to_i32 = 0xdeaf14c76e1b670e;
    static constexpr uint64_t FPToSI_f32_to_i64 = 0xfdf1eed19c4ba915;
    static constexpr uint64_t FPToSI_f64_to_i32 = 0x9bb1ac44de1f0299;
    static constexpr uint64_t FPToSI_f64_to_i64 = 0x9ccfd4dcc996c4dd;
    static constexpr uint64_t UIToFP_i32_to_f32 = 0x0d031923e7751ecf;
    static constexpr uint64_t UIToFP_i32_to_f64 = 0x1f03a9302f2654eb;
    static constexpr uint64_t UIToFP_i64_to_f32 = 0x09e828d0158ded05;
    static constexpr uint64_t UIToFP_i64_to_f64 = 0xa5508ff6a6b5d75d;
    static constexpr uint64_t SIToFP_i32_to_f32 = 0x08e0d8dfd70c8e28;
    static constexpr uint64_t SIToFP_i32_to_f64 = 0x464987723c37a486;
    static constexpr uint64_t SIToFP_i64_to_f32 = 0x2f1802507704db24;
    static constexpr uint64_t SIToFP_i64_to_f64 = 0x8e6e4de903862eb0;
    static constexpr uint64_t FPTrunc_f64_to_f32 = 0xd3a76d54e8c32463;
    static constexpr uint64_t FPExt_f32_to_f64 = 0xa3aced487c2b9330;

    // Vector operations - expanded set
    static constexpr uint64_t Sub_vec_i8 = 0xc5a4ba7d64c411da;
    static constexpr uint64_t Sub_vec_i16 = 0xb19dcef9643b439a;
    static constexpr uint64_t Sub_vec_i32 = 0xee0e7735e2767f66;
    static constexpr uint64_t Sub_vec_i64 = 0x2fbce2bc27f8586d;
    static constexpr uint64_t Sub_vec_f32 = 0x674330efde9e6e15;
    static constexpr uint64_t Sub_vec_f64 = 0x303b3241ee12230;
    static constexpr uint64_t Div_vec_i32 = 0xa61f9cfce45cc4a;
    static constexpr uint64_t Div_vec_i64 = 0x57ad025f97f19f8f;
    static constexpr uint64_t Div_vec_f32 = 0x303f9f52f0bea70c;
    static constexpr uint64_t Div_vec_f64 = 0x66bfad11704cb2da;

    // Atomic operations
    static constexpr uint64_t AtomicCmpXchg = 0x70eb96187f92b2fc;
    static constexpr uint64_t AtomicRMW_Add = 0x4c9e5d30f7d8424d;
    static constexpr uint64_t AtomicRMW_Sub = 0x0111dd2172836489;
    static constexpr uint64_t AtomicRMW_And = 0xad3e73a907240be0;
    static constexpr uint64_t AtomicRMW_Or = 0xde8d961405f75f8c;
    static constexpr uint64_t AtomicRMW_Xor = 0xd8c6cfa750f2925f;

    // Add call context primes
    static constexpr uint64_t DirectCall = 0x995e5c890782aab9;
    static constexpr uint64_t IndirectCall = 0xd24493956d09679c;
    static constexpr uint64_t TailCall = 0x1f52cad6fe2f6bcb;
    static constexpr uint64_t IntrinsicCall = 0x2ac34aedb01149f7;
};

struct FunctionContext {
    static constexpr uint64_t EntryBlock = 0x69c283054788c274;
    static constexpr uint64_t ExitBlock = 0xade60f68c74890a4;
    static constexpr uint64_t LoopHeader = 0x2f9e52dd35eb08fd;
    static constexpr uint64_t ExceptionHandler = 0xe87726bdac198e95;
    static constexpr uint64_t CallSite = 0xc0720e93e225a024;
    static constexpr uint64_t BranchTarget = 0xce461ea97423cc75;
    static constexpr uint64_t SwitchTarget = 0x140a55fc515ad16a;
    static constexpr uint64_t IndirectBranch = 0x7735c5c55b18dc5d;
    static constexpr uint64_t LoopBackEdge = 0xba646e190bfc39fd;
    static constexpr uint64_t CriticalEdge = 0x6ad18c93102b4361;
    static constexpr uint64_t InvokeTarget = 0x2aa086acbc74a62e;
    static constexpr uint64_t CallTarget = 0xf5624d5dfe14070f;
    static constexpr uint64_t CatchSwitchTarget = 0x1baa8e0dc432a39a;
    static constexpr uint64_t CleanupPadTarget = 0x2c34adfa542d7b97;
    static constexpr uint64_t CatchPadTarget = 0x13c0dc3c1fcf510e;
    static constexpr uint64_t CatchReturnTarget = 0xc5098a28e279e0b9;
    static constexpr uint64_t CleanupReturnTarget = 0x7ef4a8490515f992;
};