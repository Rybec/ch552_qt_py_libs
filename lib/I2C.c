#include <stdint.h>
#include "delay.h"


/**********************************
Delay Timings
@ 16MHz, 1 cycle = 62.5ns
 100ns =  1.6 cycles = 2 nop
 300ns =  4.8 cycles
 600ns =  9.6 cycles
1300ns = 20.8 cycles

// Min times in cycles
START_HOLD    10  //  600ns  Min time from SDA drop to SCL drop
DATA_HOLD      5  //  300ns  Min time from SCL drop to SDA change
DATA_SETUP     2  //  100ns  Min time from SDA change to SCL rise
RESTART_SETUP 10  //  600ns  Min time to START on restart
STOP_SETUP    10  //  600ns  Min time from SCL rise to SDA rise
IDLE_HOLD     21  // 1300ns  Min time between STOP and START
CLOCK_HIGH    10  //  600ns
CLOCK_LOW     21  // 1300ns
**********************************/


// 10 cycle delay (625ns)
#define START_HOLD \
	__data uint8_t __at(0x00) start_hold_ctr = 2; \
	__asm__("00001$:\n\tdjnz r0, 00001$\n\tnop\n");

// 5 cycle delay (312.5ns)
#define DATA_HOLD \
	__asm__("\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n");

// 2 cycle delay (125ns)
#define DATA_SETUP \
	__asm__("	nop\n	nop\n");

// 10 cycle delay (625ns)
#define RESTART_SETUP \
	__data uint8_t __at(0x00) restart_setup_ctr = 2; \
	__asm__("00002$:\n\tdjnz r0, 00002$\n\tnop\n");

// 10 cycle delay (625ns)
#define STOP_SETUP \
	__data uint8_t __at(0x00) stop_setup_ctr = 2; \
	__asm__("00003$:\n\tdjnz r0, 00003$\n\tnop\n");

// 17 cycle delay + 4 from function `ret` (1312.5ns)
#define IDLE_HOLD \
	__data uint8_t __at(0x00) idle_hold_ctr = 4; \
	__asm__("00004$:\n	djnz r0, 00004$\n");

// 10 cycle delay (625ns)
#define CLOCK_HIGH \
	__data uint8_t __at(0x00) clock_high_ctr = 2; \
	__asm__("00005$:\n\tdjnz r0, 00005$\n\tnop\n");

// 21 cycle delay (1312.5ns)
#define CLOCK_LOW \
	__data uint8_t __at(0x00) clock_low_ctr = 5; \
	__asm__("00006$:\n	djnz r0, 00006$\n");

// 14 cycle delay (875ns) (Use with DATA_HOLD and DATA_SETUP)
#define CLOCK_LOW_EXTRA \
	__data uint8_t __at(0x00) clock_low_extra_ctr = 3; \
	__asm__("00007$:\n\tdjnz r0, 00007$\n\tnop\n");



__sfr __at(0x96) P3_MOD_OC;
__sfr __at(0x97) P3_DIR_PU;

__bit __at(0xB3) P3_3;
__bit __at(0xB4) P3_4;

__bit __at(0xB3) SCL;
__bit __at(0xB4) SDA;



void I2C_init(void) {
	P3_MOD_OC |=  0b00011000;
	P3_DIR_PU &= ~0b00011000;

	P3_3 = 1;
	P3_4 = 1;
}

void I2C_start(void) {
	SDA = 0;
	START_HOLD
	SCL = 0;
	CLOCK_LOW
}

void I2C_restart(void) {
	SDA = 1;
	SCL = 1;

	RESTART_SETUP

	SDA = 0;
	START_HOLD
	SCL = 0;
	CLOCK_LOW
}

void I2C_stop(void) {

	SCL = 0;
	SDA = 0;

	CLOCK_LOW

	SCL = 1;
	STOP_SETUP
	SDA = 1;

	IDLE_HOLD
}

void I2C_ack(void) {
	SDA = 0;
	DATA_SETUP
	SCL = 1;

	CLOCK_HIGH

	SCL = 0;
	DATA_HOLD
	SDA = 1;
	CLOCK_LOW
}

void I2C_nak(void) {
	SDA = 1;
	DATA_SETUP
	SCL = 1;

	CLOCK_HIGH

	SCL = 0;
	DATA_HOLD
	CLOCK_LOW_EXTRA
}

uint8_t I2C_send(uint8_t i2c_data) {
	uint8_t ack_bit;
	for (uint8_t i = 0; i < 8; i++) {
		if ((i2c_data >> 7) == 0)
			SDA = 0;
		else
			SDA = 1;

		DATA_SETUP

		SCL = 1;

		CLOCK_HIGH

		SCL = 0;
		i2c_data <<= 1;

		DATA_HOLD

		CLOCK_LOW_EXTRA
	}


	P3_DIR_PU |= 0b00010000;
	SDA = 1;
	SCL = 1;

	__asm__("	nop\n	nop\n");  // Data setup

	ack_bit = SDA;

	P3_DIR_PU &= ~0b00010000;
	SCL = 0;

	return ack_bit;
}


uint8_t I2C_read(void) {
	uint8_t data = 0;

	P3_DIR_PU |= 0b00010000;

	for (uint8_t i = 0; i < 8; i++) {
		data <<= 1;
		SCL = 1;
		if (SDA == 1)
			data |= 1;

		CLOCK_HIGH

		SCL = 0;

		CLOCK_LOW
	}

	P3_DIR_PU &= ~0b00010000;

	return data;
}

