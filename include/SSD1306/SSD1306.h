#include <stdint.h>

#ifndef SSD1306_H
#define SSD1306_H

// Scroll intervals
#define SCRL_INT_11_4   0x07  //    11.4 ms +/- 10%
#define SCRL_INT_17_1   0x04  //    17.1 ms +/- 10%
#define SCRL_INT_22_8   0x05  //    22.8 ms +/- 10%
#define SCRL_INT_28_5   0x00  //    28.5 ms +/- 10%
#define SCRL_INT_142_5  0x06  //   142.5 ms +/- 10%
#define SCRL_INT_364_8  0x01  //   364.8 ms +/- 10%
#define SCRL_INT_729_6  0x02  //   729.6 ms +/- 10%
#define SCRL_INT_1459_2 0x03  // 1,459.2 ms +/- 10%


// Page aligned Image tiles in
// program memory (__code)
typedef struct SSD1306_CTiles {
	uint8_t w;
	uint8_t h_pages;
	uint8_t len;
	uint8_t data[];
} const __code SSD1306_CTiles;


// Sprites in program memory
typedef struct SSD1306_CSprites {
	uint8_t w;
	uint8_t h;
	uint8_t len;
	uint8_t data[];
} const __code SSD1306_CSprites;



void SSD1306_clear_display(void);  // Clear framebuffer to black
uint8_t SSD1306_init(void);        // Returns 1 on error
void SSD1306_display(void);        // Blit framebuffer to display



void SSD1306_blit_tile_fb(SSD1306_CTiles *src,
                          __data uint8_t __at(0x00) column,
                          __data uint8_t __at(0x01) y_page,
						  __data uint8_t __at(0x02) index);

void SSD1306_blit_xbytes_fb(__xdata uint8_t *src,
				            __data uint8_t __at(0x00) column,
				            __data uint8_t __at(0x01) y_page,
						    __data uint8_t __at(0x02) len);

void SSD1306_blit_char_fb(__code uint8_t *src);

void SSD1306_blit_sprite_fb(SSD1306_CSprites *src,
				          __data uint8_t __at(0x00) x,
				          __data uint8_t __at(0x01) y,
						  __data uint8_t __at(0x04) index);

void SSD1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color);

void SSD1306_cls(void);
void SSD1306_locate(uint8_t col, uint8_t row);
void SSD1306_print(char *s, uint8_t len, SSD1306_CTiles *font);



/***** Fundamental Commands *****
*
*/

// Set contrast to 0-255, 127 is default
void SSD1306_set_contrast(uint8_t con);

// Turn on all OLEDs
void SSD1306_all_on(uint8_t enable);

// Invert black and white pixels
void SSD1306_invert(uint8_t enable);

// Turn display off, putting it in sleep mode
void SSD1306_sleep(uint8_t enable);

/*
*
********************************/


/***** Scrolling Commands *****
*
*/

void SSD1306_h_scroll(uint8_t start_p, uint8_t stop_p,
                      uint8_t interval, uint8_t dir);

void SSD1306_hv_scroll(uint8_t start_p, uint8_t stop_p,
                       uint8_t interval, uint8_t dir,
                       uint8_t v_offset);

void SSD1306_start_scroll(void);

void SSD1306_stop_scroll(void);

void SSD1306_set_v_scroll_area(uint8_t start_row, uint8_t height);

/*
*
******************************/
#endif
