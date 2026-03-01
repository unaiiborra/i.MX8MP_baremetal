#pragma once

/* ================= DDR ================= */
#define IMX8MP_A53_DDR_LOW_BASE               0x100000000UL
#define IMX8MP_A53_DDR_HIGH_BASE              0x40000000UL

/* ================= Reserved / System ================= */
#define IMX8MP_A53_RESERVED_3F_BASE           0x3F000000UL
#define IMX8MP_A53_RESERVED_3E_BASE           0x3E000000UL
#define IMX8MP_A53_RESERVED_3D10_BASE         0x3D100000UL
#define IMX8MP_A53_RESERVED_3A_BASE           0x3A000000UL
#define IMX8MP_A53_RESERVED_3890_BASE         0x38900000UL
#define IMX8MP_A53_RESERVED_3870_BASE         0x38700000UL

/* ================= DDRC ================= */
#define IMX8MP_A53_DDRC_PHY_BROADCAST_BASE    0x3DC00000UL
#define IMX8MP_A53_DDRC_PERF_MON_BASE         0x3D800000UL
#define IMX8MP_A53_DDRC_CTL_BASE              0x3D400000UL
#define IMX8MP_A53_DDRC_BLK_CTRL_BASE         0x3D000000UL
#define IMX8MP_A53_DDRC_PHY_BASE              0x3C000000UL

/* ================= Audio / Multimedia ================= */
#define IMX8MP_A53_AUDIO_DSP_BASE             0x3B000000UL
#define IMX8MP_A53_GIC_BASE                   0x38800000UL

#define IMX8MP_A53_NPU_BASE                   0x38500000UL

#define IMX8MP_A53_VPU_BASE                   0x38340000UL
#define IMX8MP_A53_VPU_BLK_CTRL_BASE          0x38330000UL
#define IMX8MP_A53_VPU_VC8000E_BASE           0x38320000UL
#define IMX8MP_A53_VPU_G2_BASE                0x38310000UL
#define IMX8MP_A53_VPU_G1_BASE                0x38300000UL

/* ================= USB / GPU ================= */
#define IMX8MP_A53_USB2_BASE                  0x38200000UL
#define IMX8MP_A53_USB1_BASE                  0x38100000UL

#define IMX8MP_A53_GPU3D_BASE                 0x38000000UL
#define IMX8MP_A53_GPU2D_BASE                 0x38008000UL

/* ================= QSPI ================= */
#define IMX8MP_A53_QSPI_RX_BASE               0x36000000UL
#define IMX8MP_A53_QSPI1_RX_BASE              0x34000000UL
#define IMX8MP_A53_QSPI_TX_BASE               0x33008000UL
#define IMX8MP_A53_QSPI_BASE                  0x08000000UL

/* ================= PCIe ================= */
#define IMX8MP_A53_PCIE_BASE                  0x33800000UL
#define IMX8MP_A53_PCIE1_MEM_BASE             0x18000000UL

/* ================= DMA ================= */
#define IMX8MP_A53_APBH_DMA_BASE              0x33000000UL

/* ================= AIPS ================= */
#define IMX8MP_A53_AIPS4_BASE                 0x32C00000UL
#define IMX8MP_A53_AIPS5_BASE                 0x30C00000UL
#define IMX8MP_A53_AIPS3_BASE                 0x30800000UL
#define IMX8MP_A53_AIPS2_BASE                 0x30400000UL
#define IMX8MP_A53_AIPS1_BASE                 0x30000000UL

/* ================= Core / Debug ================= */
#define IMX8MP_A53_DAP_BASE                   0x28000000UL

/* ================= OCRAM / TCM ================= */
#define IMX8MP_A53_OCRAM_BASE                 0x00A00000UL
#define IMX8MP_A53_OCRAM_LOW_BASE             0x00900000UL
#define IMX8MP_A53_OCRAM_S_BASE               0x00180000UL

#define IMX8MP_A53_ITCM_BASE                  0x007E0000UL
#define IMX8MP_A53_DTCM_BASE                  0x00800000UL
#define IMX8MP_A53_TCM_BASE                   0x00820000UL

/* ================= Security ================= */
#define IMX8MP_A53_CAAM_BASE                  0x00100000UL

/* ================= Boot ================= */
#define IMX8MP_A53_BOOT_ROM_BASE              0x00000000UL
