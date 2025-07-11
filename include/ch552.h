
#ifndef CH552_H
#define CH552_H

__sfr __at(0xA1) SAFE_MOD;
__sfr __at(0xB9) CLOCK_CFG;

__sfr __at(0x96) P3_MOD_OC;
__sfr __at(0x97) P3_DIR_PU;

__bit __at(0xB6) BOOT;

#endif
