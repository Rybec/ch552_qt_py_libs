#include <ch552.h>


// Set BOOT button to take input.
void bootload_button_init() {
	P3_MOD_OC = P3_MOD_OC & ~0b01000000;
	P3_DIR_PU = P3_DIR_PU & ~0b01000000;
	BOOT = 1;
}


// Call each loop to check BOOT button
// state and enter bootloader if pressed.
void bootload_button_poll() {
	if (BOOT) {
		EA = 0
		__asm__("lcall #0x3800");
	}
}

