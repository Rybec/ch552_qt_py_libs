#include <stdint.h>

#include <I2C.h>
#include <delay.h>
#include <SSD1306/SSD1306.h>

# define CTRL_CMD         0b00000000
# define CTRL_DATA        0b11000000
# define CTRL_DATA_STREAM 0b01000000

// Assembly rotate left instruction
#define RL(i) ((i >> 7) | (i << 1))

__xdata uint8_t __at(0x0200) SSD1306_framebuffer[512];
#define FB_END 0x0400

__xdata uint8_t * __data cursor = SSD1306_framebuffer;
uint8_t SSD1306_addr = 0x3C;


void SSD1306_clear_display(void) {
	__asm__("	.even\n"
			"   nop\n"                    // To even align label
			"	orl 0xA2, #0b00000001\n"  // Set XBUS to DPTR1
			"	mov dptr, #0x0200\n"      // Put address in DPTR1
			"	anl 0xA2, #0b11111110\n"  // Set XBUS to DPTR0
			"	mov a, #0x00\n"           // Value to write
			"	mov r1, #2\n"             // Outer counter (2)
			"	mov r0, #0\n"             // Inner counter (256)
			"00077$:\n"
			"	.db 0xA5\n"               // @DPTR1 = A, DPTR1++
			"	djnz r0, 00077$\n"
			"	djnz r1, 00077$\n");
			// (193.0625us @ 16MHz)
}


// Returns 1 on error, otherwise 0
uint8_t SSD1306_init(void) {
	I2C_init();

	SSD1306_clear_display();

	// Display startup commands
	I2C_start();
	uint8_t conn = I2C_send(RL(SSD1306_addr));

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xD5);  // Set display clock
	conn |= I2C_send(0x80);  // Default value

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xA8);  // Set multiplex
	conn |= I2C_send(0x1F);  // 0x1F = (HEIGHT - 1)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xD3);  // Set display offset
	conn |= I2C_send(0x00);  // No offset

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0x40);  // Set startline (0b01??????, 0x40 | 6-bit arg)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0x8D);  // Set charge pump
	conn |= I2C_send(0x14);  // Turn on (0x14=on, 0x10=off)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0x20);  // Set memory mode
	conn |= I2C_send(0x00);  // Horizontal Mode

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xA1);  // Segment remap (0b1010000?, 1 = flip screen)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xC8);  // Set COM Output Scan Direction
	                         // (0xC0 | 0b1000 decrement)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xDA);  // Set COM pins
	conn |= I2C_send(0x02);  // Horizontal Mode (0b00??0010; 00=Sequential)

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0x81);  // Set Contrast
	conn |= I2C_send(0x8F);  // 0x8F=143; 0x7F (127) is default

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xD9);  // Set Precharge
	conn |= I2C_send(0xF1);  // 0x01=Phase 1, 0xF0=Phase 2

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xDB);  // Set Vcomh Deselect Level
	conn |= I2C_send(0x40);  // 0x40 is higher than max datasheet value

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xA4);  // Resume display

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xA6);  // Disable invert

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0x2E);  // Deactivate scroll

	conn |= I2C_send(CTRL_CMD);
	conn |= I2C_send(0xAF);  // Display on (exit sleep mode)

	I2C_stop();

	return conn;
}

// No error handling, for performance
void SSD1306_display(void) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	// Set page range
	I2C_send(0x22);
	I2C_send(0x00); // Page 0 (Starts at row 0)
	I2C_send(0x03); // Page 3 (Ends at row 31)

	// Set column range
	I2C_send(0x21);
	I2C_send(0x00);  // Col 0
	I2C_send(0x7F);  // Col 127

	I2C_restart();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_DATA_STREAM);

	__xdata uint8_t *ptr = SSD1306_framebuffer;
	while ((uint16_t)ptr != FB_END) {
		I2C_send(*ptr++);
	}

	I2C_stop();
}


// Blits a tile image in program memory to
// the framebuffer at the specified location.
// Tile images are page aligned, requiring
// their height be a multiple of 8, and they
// are rendered page aligned.

#pragma disable_warning 85
void SSD1306_blit_tile_fb(SSD1306_CTiles *src,
				          __data uint8_t __at(0x00) column,
				          __data uint8_t __at(0x01) y_page,
						  __data uint8_t __at(0x02) index) {
	__asm__(
		// Setup col loop counter
		// Get (*src).w
		"	clr a            \n"
		"	movc a, @a+dptr  \n"
		"	mov r3, a        \n"  // r3 = (*src).w

		// Setup page loop counter
		// Get (*src).h_pages
		"	inc dptr         \n"
		"	clr a            \n"
		"	movc a, @a+dptr  \n"
		"	mov r4, a        \n"  // r4 = (*src).h_pages

		// Setup framebuffer pointer in DPTR1
		// SSD1306_framebuffer(0x0200) + (y_page(r1) * 128) + column(r0);
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov a, r1             \n"
		"	mov b, #128           \n"
		"	mul ab                \n"
		"	add a, r0             \n"
		"	mov dpl, a            \n"
		"	mov a, b              \n"
		"	addc a, #0x02         \n"
		"	mov dph, a            \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0

		// Set page advance
		"	mov a, #128 \n"
		"	clr c      \n"
		"	subb a, r3 \n"
		"	mov r5, a  \n"

		// Set DPTR to addr (src.data + (src.w * src.h_pages * index))
		"	inc dptr     \n"  // Pointed to (*src).len
		"	inc dptr     \n"  //    "       (*src).data
		"	mov a, r2    \n"  // r2 = index
		"	mov b, r3    \n"  // r3 = (*src).w
		"	mul ab       \n"  // r4 = (*src).h_pages
		"	push a       \n"
		"	mov a, r4    \n"
		"	mul ab       \n"
		"	mov r6, a    \n"  // temp (top byte)
		"	pop a        \n"
		"	mov b, r4    \n"
		"	mul ab       \n"
		"	add a, dpl   \n"
		"	mov dpl, a   \n"
		"	mov a, r6    \n"  // Free r6
		"	addc a, b    \n"
		"	add a, dph   \n"
		"	mov dph, a   \n"

		// Blit loop
		"00001$:         \n"  // pages loop
		"	mov a, r3    \n"
		"	mov r6, a    \n"

		"00002$:                  \n"  // columns loop
		"	clr a                 \n"
		"	movc a, @a+dptr       \n"
		"	.db 0xA5              \n"  // DPTR1 = DPTR, increment DPTR1
		"	inc dptr              \n"
		"	djnz r6, 00002$       \n"  // loop until end of page

		// Advance one page
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"   mov a, dpl            \n"
		"   add a, r5             \n"
		"	mov dpl, a            \n"
		"	mov a, dph            \n"
		"   addc a, #0            \n"
		"	mov dph, a            \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0
		"	djnz r4, 00001$       \n"  // loop end of image
	);
}


void SSD1306_blit_xbytes_fb(__xdata uint8_t *src,
				            __data uint8_t __at(0x00) column,
				            __data uint8_t __at(0x01) y_page,
						    __data uint8_t __at(0x02) len) {
	__asm__(
		// Setup framebuffer pointer in DPTR1
		// SSD1306_framebuffer(0x0200) + (y_page(r1) * 128) + column(r0);
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov a, r1             \n"
		"	mov b, #128           \n"
		"	mul ab                \n"
		"	add a, r0             \n"
		"	mov dpl, a            \n"
		"	mov a, b              \n"
		"	addc a, #0x02         \n"
		"	mov dph, a            \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0

		// Blit loop
		"00001$:                  \n"  // columns loop
		"	movx a, @dptr         \n"
		"	.db 0xA5              \n"  // DPTR1 = DPTR, increment DPTR1
		"	inc dptr              \n"
		"	djnz r2, 00001$       \n"  // loop until end of page
	);
}


void SSD1306_blit_char_fb(__code uint8_t *src) {
	__asm__(
		// Setup framebuffer pointer in DPTR1
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov dpl, _cursor      \n"
		"	mov dph, _cursor + 1  \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0

		// Blit loop
		"	mov r0, #8            \n"
		"00001$:                  \n"  // columns loop
		"	clr a                 \n"
		"	movc a, @a+dptr       \n"
		"	.db 0xA5              \n"  // DPTR1 = DPTR, increment DPTR1
		"	inc dptr              \n"
		"	djnz r0, 00001$       \n"  // loop until end of page
	);
}


void SSD1306_blit_sprite_fb(SSD1306_CSprites *src,
				          __data uint8_t __at(0x00) x,
				          __data uint8_t __at(0x01) y,
						  __data uint8_t __at(0x04) index) {
	__asm__(
		// Setup x loop counter
		// Get (*src).w
		"	clr a            \n"
		"	movc a, @a+dptr  \n"
		"	mov r2, a        \n"  // r2 = (*src).w

		// Setup y loop counter
		// Get (*src).h
		"	inc dptr         \n"  // Pointed to (*src).h
		"	clr a            \n"
		"	movc a, @a+dptr  \n"
		"	mov r3, a        \n"  // r3 = (*src).h

		// Set DPTR to addr (src.data + ((src.w * src.h) / 8 * index))
		"	inc dptr       \n"  // Pointed to (*src).len
		"	inc dptr       \n"  //    "       (*src).data
		// (*src).w * (*src).h
		"	mov a, r2      \n"  // r2 = (*src).w
		"	mov b, r3      \n"  // r3 = (*src).h
		"	mul ab         \n"
		// Check if round is necessary after div
		"   clr f0         \n"
		"	mov r6, a      \n"  // Using R6 for temp storage
		"	anl a, #0b111  \n"
		"	jz 00070$      \n"
		"	setb f0        \n"  // Save whether to round
		"00070$:           \n"
		"	mov a, r6      \n"  // Free R6
		// Divide by 8
		"	rlc a              \n"  // R5, R6, R7 are free
		"	mov r5, a          \n"  // Using R5 for temp storage
		"	mov a, b           \n"
		"	rlc a              \n"
		"	mov r6, #0         \n"  // Using R6 temporarily
		"	mov r7, 0x00       \n"  // Store R0 in R7 temporarily
		"	mov r0, #0x06      \n"
		"	xchd a, @r0        \n"
		"	swap a             \n"
		"	jnc 00071$         \n"
		"	orl a, #0b00010000 \n"  // Restore top bit if set
		"00071$:               \n"
		"	mov b, a           \n"
		"	mov a, r5          \n"  // Free R5
		"	xchd a, @r0        \n"  // Free R6
		"	swap a             \n"
		"	mov 0x00, r7       \n"  // Restore R0, Free R7
		// Round up if necessary
		"	jnb f0, 00072$ \n"  // R5, R6, and R7 are free
		"	add a, #1      \n"
		"   mov r5, a      \n"  // Using R5 for temp storage
		"	mov a, b       \n"
		"	addc a, #0     \n"
		"	mov b, a       \n"
		"	mov a, r5      \n"  // Free R5
		"00072$:           \n"
		// Multiply by index    // R5, R6, and R7 are free
		"	mov r5, b      \n"  // Using R5 for temp storage
		"	mov b, r4      \n"
		"	mul ab         \n"  // Multiply low byte by index
		"	mov r6, a      \n"  // Using R6 for temp storage (lower byte)
		"	mov r7, b      \n"  // Using R7 for temp storage (higher byte)
		"	mov a, r5      \n"  // Free R5
		"	mov b, r4      \n"  // Free R4 (no longer need index)
		"	mul ab         \n"  // Multiply high byte by index
		"	add a, r7      \n"  // Free R7
		"	mov b, a       \n"  // Put high byte in B
		"	mov a, r6      \n"  // Free R6
		// Add to DPTR
		"	add a, dpl     \n"
		"	mov dpl, a     \n"
		"	mov a, b       \n"
		"	addc a, dph    \n"
		"	mov dph, a     \n"	// R4, R5, R6, and R7 are free


		// Setup framebuffer pointer in DPTR1
		// SSD1306_framebuffer(0x0200) + ((y(r1) >> 3) * 128) + x(r0);
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov a, r1             \n"
		// y >> 3 (left rotate 5 then mask)
		"	swap a                \n"
		"	rl a                  \n"  // r0 = x
		"	anl a, #0b00011111    \n"  // r1 = y
		// * 128...
		"	mov b, #128           \n"
		"	mul ab                \n"
		"	add a, r0             \n"
		"	mov dpl, a            \n"
		"	mov a, b              \n"
		"	addc a, #0x02         \n"
		"	mov dph, a            \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0

// r0 = x countdown
// r1 = y pos counter
// r2 = w
// r3 = y countdown
// r4 = current image byte
// r5 = byte pos counter
// r6 = low DPTR1
// r7 = high DPTR1

		// Blit loop
		// Load first byte
		"	clr a            \n"
		"	movc a, @a+dptr  \n"
		"	inc dptr         \n"
		"	mov r4, a        \n"
		"	mov r5, #0x08    \n"
		// Store starting segment of current page
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov r6, dpl           \n"
		"	mov r7, dph           \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0
		// Start y loop
		"00001$:         \n"
		"	mov a, r2    \n"
		"	mov r0, a    \n"  // Load x countdown
		// Start x loop
		"00002$:                  \n"
		"	mov a, r4             \n"
		"	rlc a                 \n"  // Put next bit in C
		"	mov r4, a             \n"
		// Select bit to write
		"	mov a, r1             \n"
		"	anl a, #0b111         \n"
		"	mov b, a              \n"
		"	inc b                 \n"
		// Create write mask
		"	mov a, #1             \n"
		"	sjmp 00006$           \n"
		"00005$:                  \n"
		"	rl a                  \n"
		"00006$:                  \n"
		"	djnz b, 00005$        \n"
		"	mov b, a              \n"
		// Read byte to change
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	movx a, @dptr         \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0
		// if C is set (indicates white pixel)...
		"	jnc 00007$            \n"
		"	orl a, b              \n"  // ...write white bit at mask location
		"	sjmp 00010$           \n"
		"00007$:                  \n"  // Otherwise...
		"	xch a, b              \n"
		"	cpl a                 \n"
		"	anl a, b              \n"  // Write black bit at mask location
		"00010$:                  \n"
		// Write changed bit back to memory (and increment pointer)
		"	.db 0xA5              \n"  // DPTR1 = DPTR, increment DPTR1
		"	djnz r5, 00003$       \n"  // Decrement byte counter...
		"	clr a                 \n"
		"	movc a, @a+dptr       \n"  // ...and read new image byte if necessary.
		"	inc dptr              \n"
		"	mov r4, a             \n"
		"	mov r5, #0x08         \n"
		"00003$:                  \n"
		"	djnz r0, 00002$       \n"  // loop until end of row

		// Advance one page if needed
		"	inc r1                \n"
		"	mov a, r1             \n"
		"	anl a, #0b111         \n"
		"	jnz 00004$            \n"
		"   mov a, r6             \n"
		"   add a, #128           \n"
		"	mov r6, a             \n"
		"	mov a, r7             \n"
		"   addc a, #0            \n"
		"	mov r7, a             \n"
		"00004$:                  \n"
		"	orl 0xA2, #0b00000001 \n"  // Set XBUS to DPTR1
		"	mov dpl, r6           \n"
		"	mov dph, r7           \n"
		"	anl 0xA2, #0b11111110 \n"  // Set XBUS to DPTR0
		"	djnz r3, 00001$       \n"  // loop end of image

	);
}


void SSD1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
	__xdata uint8_t *segment = SSD1306_framebuffer + x + (y >> 3) * 128;

	uint8_t s = *segment;

	if (color == 0)
		s &= ~(1 << (y & 0b111));
	else
		s |=  (1 << (y & 0b111));

	*segment = s;
}


void SSD1306_cls(void) {
	SSD1306_clear_display();
	cursor = SSD1306_framebuffer;
}

void SSD1306_locate(uint8_t col, uint8_t row) {
	cursor = SSD1306_framebuffer + (8 * (col + 16 * row));
}

void SSD1306_print(char *s, uint8_t len, SSD1306_CTiles *font) {
	for (uint8_t i = 0; i < len; i++) {
		SSD1306_blit_char_fb(font->data + 8 * *s++);
		cursor += 8;
	}
}

/***** Fundamental Commands *****
*
*/

// Set contrast to 0-255, 127 is default
void SSD1306_set_contrast(uint8_t con) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);
	I2C_send(0x81);
	I2C_send(con);
	I2C_stop();
}

// Turn on all OLEDs
void SSD1306_all_on(uint8_t enable) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	uint8_t command;
	if (enable == 0)
		command = 0xA4;
	else
		command = 0xA5;

	I2C_send(command);
	I2C_stop();
}

// Invert black and white pixels
void SSD1306_invert(uint8_t enable) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	uint8_t command;
	if (enable == 0)
		command = 0xA6;
	else
		command = 0xA7;

	I2C_send(command);
	I2C_stop();
}

// Turn display off, putting it in sleep mode
void SSD1306_sleep(uint8_t enable) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	uint8_t command;
	if (enable == 0)
		command = 0xAF;
	else
		command = 0xAE;

	I2C_send(command);
	I2C_stop();
}

/*
*
********************************/



/***** Scrolling Commands *****
*
*/

void SSD1306_h_scroll(uint8_t start_p, uint8_t stop_p,
                      uint8_t interval, uint8_t dir) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	I2C_send(0x26 | (dir & 1));  // dir = 0: right, dir = 1: left
	I2C_send(0x00);
	I2C_send(start_p);
	I2C_send(interval);
	I2C_send(stop_p);
	I2C_send(0x00);
	I2C_send(0xFF);

	I2C_stop();
}

void SSD1306_hv_scroll(uint8_t start_p, uint8_t stop_p,
                       uint8_t interval, uint8_t dir,
                       uint8_t v_offset) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	I2C_send(0x29 | (dir & 1)); // dir = 0: right, dir = 1: left
	I2C_send(start_p);
	I2C_send(interval);
	I2C_send(stop_p);
	I2C_send(v_offset);

	I2C_stop();
}

void SSD1306_start_scroll(void) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	I2C_send(0x2F);

	I2C_stop();
}

void SSD1306_stop_scroll(void) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	I2C_send(0x2E);

	I2C_stop();
}

void SSD1306_set_v_scroll_area(uint8_t start_row, uint8_t height) {
	I2C_start();
	I2C_send(RL(SSD1306_addr));
	I2C_send(CTRL_CMD);

	I2C_send(0xA3);
	I2C_send(start_row);
	I2C_send(height);

	I2C_stop();
}


/*
*
******************************/









