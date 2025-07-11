#include "ch552.h"


void clock_init(void) {
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;

#if F_CPU == 16000000
	CLOCK_CFG = CLOCK_CFG & ~0b111 | 0x05; // 16MHz
#else
#warning This library will have undefined behaviors if F_CPU is not set to 16000000.
#endif

	SAFE_MOD = 0x00;

}
