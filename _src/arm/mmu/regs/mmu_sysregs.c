#include "mmu_sysregs.h"

#include "arm/mmu/mmu.h"
#include "boot/panic.h"
#include "lib/stdint.h"


static inline uint64 tg0_from_granularity_enum_(mmu_granularity g)
{
    switch (g) {
        case MMU_GRANULARITY_4KB:
            return 0b00ul;
        case MMU_GRANULARITY_16KB:
            return 0b10ul;
        case MMU_GRANULARITY_64KB:
            return 0b01ul;
    }

    PANIC();
}

static inline uint64 tg1_from_granularity_enum_(mmu_granularity g)
{
    switch (g) {
        case MMU_GRANULARITY_4KB:
            return 0b10ul;
        case MMU_GRANULARITY_16KB:
            return 0b01;
        case MMU_GRANULARITY_64KB:
            return 0b11;
    }

    PANIC();
}

void mmu_apply_cfg(mmu_cfg cfg)
{
    // TODO: configurable mair
    // AttrIdx 0: Normal memory, WB WA
    // AttrIdx 1: Device-nGnRE
    // https://df.lth.se/~getz/ARM/SysReg/AArch64-mair_el1.html
    uint64 mair = (0xFFUL << 0) | (0x04UL << 8);
    uint64 tcr = 0;
    uint64 sctlr = _mmu_get_SCTLR_EL1();


    ASSERT(cfg.lo_va_addr_bits >= 39 && cfg.lo_va_addr_bits <= 48);
    ASSERT(cfg.hi_va_addr_bits >= 39 && cfg.hi_va_addr_bits <= 48);

    // vaddress size
    tcr |= (uint64)(64 - cfg.lo_va_addr_bits) << 0;  // T0SZ
    tcr |= (uint64)(64 - cfg.hi_va_addr_bits) << 16; // T1SZ

    // enable/disable TTBRs
    tcr |= (uint64)(!cfg.lo_enable) << 7;  // EPD0
    tcr |= (uint64)(!cfg.hi_enable) << 23; // EPD1

    // granularity
    tcr |= tg0_from_granularity_enum_(cfg.lo_gran) << 14; // TG0
    tcr |= tg1_from_granularity_enum_(cfg.hi_gran) << 30; // TG1

    // cacheability shareability
    tcr |= 0b11ULL << 12; // SH0 = Inner Shareable
    tcr |= 0b01ULL << 10; // ORGN0 = WB WA
    tcr |= 0b01ULL << 8;  // IRGN0 = WB WA

    tcr |= 0b11ULL << 28; // SH1 = Inner Shareable
    tcr |= 0b01ULL << 26; // ORGN1 = WB WA
    tcr |= 0b01ULL << 24; // IRGN1 = WB WA


    // paddress size
    uint64 id_aa64mmfr0 = _mmu_get_ID_AA64MMFR0_EL1();
    uint64 pa_range = id_aa64mmfr0 & 0xFUL; // [3:0] DDI0500J_cortex_a53_trm.pdf p.104

    uint64 ips;
    switch (pa_range) {
        case 0b0000:
            ips = 0b000;
            break; // 32 bits
        case 0b0001:
            ips = 0b001;
            break; // 36 bits
        case 0b0010:
            ips = 0b010;
            break; // 40 bits
        case 0b0011:
            ips = 0b011;
            break; // 42 bits
        case 0b0100:
            ips = 0b100;
            break; // 44 bits
        case 0b0101:
            ips = 0b101;
            break; // 48 bits
        default:
            PANIC("Unsupported PA range");
    }

    tcr |= (uint64)(ips & 0b111ULL) << 32; // IPS


    sctlr &= ~(1ULL << 0);
    sctlr &= ~(1ULL << 1);
    sctlr &= ~(1ULL << 2);
    sctlr &= ~(1ULL << 12);

    sctlr |= ((uint64)cfg.i_cache << 12);   // I instruction cache
    sctlr |= ((uint64)cfg.d_cache << 2);    // D data cache
    sctlr |= ((uint64)cfg.align_trap << 1); // A alignment trap
    sctlr |= ((uint64)cfg.enable << 0);     // M MMU enable


    _mmu_set_MAIR_EL1(mair);
    _mmu_set_TCR_EL1(tcr);

    asm volatile("tlbi vmalle1\n"
                 "dsb ish\n"
                 "isb\n");
    _mmu_set_SCTLR_EL1(sctlr);
}
