#include <SSD1306/SSD1306.h>


SSD1306_CTiles *_font;
extern __xdata uint8_t * __data cursor;
extern __xdata uint8_t SSD1306_framebuffer[512];


/********************************
Override for putchar(), to write
to the display when printf() and
other stdio output functions are
called.
********************************/
int putchar(int ch) {
	if (ch == '\n') {
		cursor += 128 - ((cursor - SSD1306_framebuffer) % 128);
	} else {
		SSD1306_blit_char_fb(_font->data + 8 * ch);
		cursor += 8;
	}

	if (cursor > (SSD1306_framebuffer + 511)) {
		SSD1306_blit_xbytes_fb(SSD1306_framebuffer + 128,  0, 0, 192);
		SSD1306_blit_xbytes_fb(SSD1306_framebuffer + 320, 64, 1, 192);
		for (__xdata uint8_t *p = SSD1306_framebuffer + 384;
		     p < SSD1306_framebuffer + 512; p++)
			*p = 0;

		cursor = SSD1306_framebuffer + 384;
	}

	return ch;
}


void SSD1306_set_font(SSD1306_CTiles *font) {
	_font = font;
}
