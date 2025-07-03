# ch552_qt_py_libs
A set of small, high performance libraries for Adafruit's CH552 QT Py.

This is a set of fairly streamlined libraries designed specifically for the CH552 QT Py.  It should work with any CH552, but certain things are hardcoded for Adafruit's board layout.

## Libraries included

### I2C

<details>
<summary>Details</summary>

This is a fairly complete, from scratch I2C driver, hardcoded to use the Stemma QT port on the CH552 QT Py.  The CH552 does not have hardware I2C, so this is a bitbanging driver.  This is mainly written in C, with delays written using inline assembly in macros.  I have not measured the timing, but at 16MHz, my math indicates it should run at around 333kbit/s.

#### Usage

`void I2C_init(void)`  
Sets up the pins for the I2C port.  This should be run once, before any other I2C function.

`void I2C_start(void)`  
Starts an I2C transaction.

`void I2C_restart(void)`  
Restarts a currently open I2C transaction.

`void I2C_stop(void)`  
Stops a currently open I2C transaction.

`void I2C_ack(void)`  
Sends I2C ack bit.  

`void I2C_nak(void)`  
Sends I2C nak bit.

`uint8_t I2C_send(uint8_t i2c_data)`  
Sends one byte of data to the I2C bus and returns ack.

`uint8_t I2C_read(void)`  
Reads one byte of data from the I2C bus and returns it.


#### Generic CH552 Use

This is hardcoded to use the Stemma QT port on Adafruit's CH552 QT Py.  If you want to use it with another CH552 system, and you are using the same pins (port 3 pins 3 and 4, for SCL and SDA respectively), this should work for you as is.  If you need to change the pins used, you will find the pin/port variable declarations after the delay macro definitions.  Note that these two pins are assigned to two sets of variables.  The ones named after port/pin number are only used in `I2C_init()`.  Everywhere else `SCL` and `SDA` are used instead.  You will also need to change the `MOD_OC` and `DIR_PU` port addresses, and you'll need to change their setup in `I2C_init()` to point to the right pins.  And lastly, these are also used in `I2C_read()` and `I2C_send()`, to change the direct of SDA.  Those will have to be adjusted to fit the new pin mapping as well.  Note that if performance is not a huge issue, ch55xduino has a more generic I2C driver for Arduino that is much easier to remap.

Also note that this is designed for the highest clock speed supported by the CH552 QT Py.  Due to the board voltage, this is 16MHz.  If you use a lower clock rate, I2C speed will be decreased proportionally.  If your CH552 board is supplying 5V, and you set the clock speed to 24MHz, I2C speed will be 50% faster, which is likely to exceed specifications for 400kbit/s peripherals.  You can adjust for this by modifying the delay macros, I can tell you that this is a huge pain.  This is another case where you might want to use ch55xduino, unless you need the speed so badly that you are willing to spend hours doing the math and assembly juggling to manually retune the delays.  (If you are running slower than 16MHz, and you need fast I2C, retuning is the only option.  This applies to the CH552 QT Py as well.)

#### Future Work

This currently sends one byte per function call.  This almost certainly reduces the bit rate significantly below the estimated maximum.  It needs a function that can send an entire buffer of arbitrary length all at once, to eliminate unnecessary function call overhead when sending large amounts of data.  It might also be worth adding a buffered multi-byte read function at some point as well.
</details>

---

### Delay

<details>
<summary>Details</summary>

This is a pair of simple assembly delay functions, designed for extremely high precision delays.  These are tuned for 16MHz.  They will be proportionally shorter or longer if the clock speed is higher or lower respectively.  These are also size optimized.

It should be noted that if you use interrupts, and one of these delays is interrupted, it won't know that it was interrupted, and the time spent in the interupt service routine will be added to the delay.  If you really need ultra high precision timing, and you are using interrupts, you should disable interrupts before starting the time sensitive code and reenable them after.  These functions will not do that for you!

One final note: These delay 1 to 256 units.  If you need a delay between 256 and the next unit up, you'll have to use multiple delay calls.  Function call overhead may introduce some inaccuracy, though it will never be enough to matter for the millisecond variant.  For the microsecond variant, 16 cycles is one microsecond, and the overhead of four function calls _will_ add up to 16 or more cycles.  Depending on pre-call register juggling, four calls could easily take several microseconds.  If this is a problem for your particular use case, you might want to consider writing your timing dependent code in assembly, so that you can control this and precisely calculate function call overhead to subtract it from the delays.  (Or you could just do trial and error...  _I_ wouldn't recommend that though.)

#### Usage

`void delay_us(uint8_t us)`  
Delays for the given number of microseconds.  Note that due to loop optimization, an argument of 0 will give a 256 microsecond delay.  This function delays exactly 1/16th or 1/8th of a microsecond extra (1 or 2 cycles, depending on alignment).  (This includes return overhead but not call overhead.  Call overhead is generally unlikely to be more than one microsecond.)

`void delay_ms(uint8_t ms)`  
Delays for the given number of milliseconds.  Again, an argument of 0 is treated as 256.  This function delays an extra 4 or 5 cycles (depending on alignment), which is roughly 1/4th of a microsecond (exactly 1/4000th to 1/3200th of a millisecond) extra.  Function call overhead is also negligable.

#### Generic CH552 Use

This should work as-is for any CH552 set to run at 16MHz.  It may also run as-is for other CH55x variants and possibly even for other 8051 variants, as there is nothing specific to the CH55x line that it uses or requires.  For other clock speeds, the delay will be inversely proportional.  At 24MHz, a 120 microsecond delay should take 80 microseconds.  At 8MHz, a 120 microsecond delay should end up taking 240 microseconds.  These are so highly tuned to 16MHz that if you need something with similar precision for a different clock speed, you'll be better off writing new delay functions from scratch than trying to adjust the timing on these.  (Though the assembly code for these functions might be good reference for doing this!)

#### Future Work

This is _not_ a high priority, but a delay function with a resolution of 1 second might be a good idea.  That said, this is probably better done with the built in timer peripherals than with assembly level delays.
</details>

---

### SSD1306

<details>
<summary>Details</summary>
  
This is a driver for the SSD1306 OLED driver chip.  This is designed specifically for [Adafruit's 128x32 I2C OLED Display](https://www.adafruit.com/product/4440), which uses this driver chip.  The CH552 has only 1KB of external RAM (and 256 bytes of internal RAM), which means that this is the biggest display that can be framebuffered on it, without using all of the external RAM and leaving too little free memory to do much that is useful.  This driver uses the top half (512 bytes) of XRAM for the framebuffer.  Significant portions of this are written in assembly, to maximize speed and to keep the code footprint small.  All rendering goes to the framebuffer, which can then be blitted to the display over I2C with a single function call.  The overall speed is easily fast enough for animations and even video games.  Support is provided for two kinds of sprites (one for speed, the other for flexibility), pixel by pixel drawing, printing grid aligned monospace text in 8x8 fonts, as well as for some special features of the SSD1306 driver chip.

Note that this is dependent on the I2C library.

#### Usage

##### General Functions

`uint8_t SSD1306_init(void)`  
Initialize the display.  Note that this calls I2C_init(), to ensure that it is initialized before sending the startup commands to the display over I2C.  It won't hurt anything if I2C_init() is called multiple times, so long as it is all during program initialization and not at random places in the program.  You do need to initialize the display before attempting to use any other functions that communicate with it.  (Technically you can get away with using rendering functions that write to the framebuffer before initializing the display, but it's probably best to develop a habit of initializing before using any other SSD1306 function.)

`void SSD1306_clear_display(void)`  
This clears the framebuffer to all black.  Note that it does not blit the framebuffer to the display, so it won't clear the display on its own.

`void SSD1306_display(void)`  
This is the function for blitting (***bl***ock ***i***mage ***t***ransfer) the framebuffer to the display.  Whatever has been rendered to the framebuffer will be sent to the display when this function is called.  This should generally be the last function called at the end of the render loop.

`void SSD1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color)`  
This draws a single pixel of the framebuffer.  If `color` is 0, it draws black, otherwise it draws white.  Note that pixel by pixel drawing is slow.

##### Sprite Rendering

```
void SSD1306_blit_tile_fb(SSD1306_CTiles *src,
                          uint8_t column,
                          uint8_t y_page,
                          uint8_t index)
```  
This function blits a tile graphic from program memory ("C" in "CTiles" is for "Code memory") to the framebuffer.  See `SSD1306.h` for the definition of the SSD1306_CTiles struct.  This type of struct holds an array of images that are all the same height and width, where the height is a multiple of 8 (byte aligned heights can be blitted much faster).  The `index` argument to the function selects which image in the array will be blitted.  `column` is the x coordinate of the left side of the image in pixels.  `y_page` is the vertical position of the image, in 8 pixel increments.  So `y_page` of 0 is the top of the screen, 1 is 8 pixels down from the top, 2 is 16 pixels down from the top, and 3 is 24 pixels down from the top.  So while the column can be set to a specific pixel, the row must be one of the four discrete 8 pixel tall rows.  This is due to how the display handles bytes as monochrome image data.  This function is implemented entirely in inline assembly for speed optimization and is _very_ fast!

```
void SSD_1306_blit_sprite_fb(SSD1306_CSprites *src,
                             uint8_t x,
                             uint8_t y,
                             uint8_t index)
```  
This function blits a sprite graphic from program memory to the framebuffer.  Sprites are different from tiles in several ways.  One difference is orientation.  Tiles are arranged with the bits in each byte being displayed vertically, with the lowest bit being at the bottom of the row and the highest being at the top, and then bytes are oriented left to right.  Sprites are purely linear, which each bit displaying to the right of the previous bit, and at the right edge starting over on the next line down.  This makes them slower to render but allows for fully arbitrary height, width, and placement.  All of the images in a Sprite struct must be the same dimensions, but they can have any height and width.  Similarly, they are displayed at an `x, y` location, where `x` and `y` are pixel measurements indicating the top left corner of the image.  This is also written entirely in inline assembly, but because it has to write bit by bit, rather than a byte at a time, it is much slower.  Ideally, the above function is used for tiles and other things that are always going to be byte aligned in size and placement, while this is used for sprites that need more flexibility.  This will optimize for speed while still providing enough flexibility for good visuals.

```
void SSD1306_blit_xbytes_fb(__xdata uint8_t *src,
                            uint8_t column,
                            uint8_t y_page,
                            uint8_t len)
```  
This function exists mainly to allow for modifying or generating images dynamically.  Program memory is read-only during program execution, so images stored there cannot be modified.  Images stored in XRAM can be modified though, and this function provides a simple way of blitting images store in XRAM to the framebuffer.  Note that all this does is calculate the address in the framebuffer where the image starts and then it writes sequentially until `len` bytes have been blitted.  This means it can write off the right side of the display data and go down to the beginning of the next line and finish there.  This is extremely fast (100% inline assembly), but it can produce unexpected results if you accidentally write past the right edge of the display area.  On the other hand, this could also be deliberately used to optimize writes, for example, if you need to draw a frame, you can combine 3 of the left edge writes with 3 of the right edge writes to reduce function call overhead, if you arrange the side frame elements in memory correctly.

##### Text Rendering

`void SSD1306_cls(void)`  
This function does two things.  It clears the screen to black, and it resets the text cursor position to the top left corner.  Text rendering uses a global cursor that is always positioned at an x and y position where each coordinate is 8 pixel aligned.  This makes the display essentially a 16x4 text display, when using these text rendering functions.

`void SSD1306_blit_char_fb(__code uint8_t *src)`  
This blits 8 bytes from code memory to the framebuffer at the current cursor location.  If you've put a font in code memory, `src` can be a pointer to the first byte of the character you want to display.  This function is another inline assembly one that is very fast.

`void SSD1306_locate(uint8_t col, uint8_t row)`  
The name of this function is based on an old QBasic function.  That function repositioned the text cursor to the specified position, which is exactly what this one does.  If you need to print some text in a specific location, use this to move the cursor there, then used one of the text printing functions.

`void SSD1306_print(char* s, uint8_t len, SSD1306_CTiles *font)`  
This function will print a string, advancing the cursor after each character is printed.  Because the framebuffer is linear, writing off the right edge will move down to the next line at the left side.  This function does not have protection for writing past the end of the framebuffer.  On the CH552, the framebuffer is at the end of XRAM, so writing past the end writes into the XRAM address space where no RAM actually exists.  This is considered undefined, but it will probably just eat whatever you've written without a complaint.  The font is a tile array that should be long enough to contain all of the indices for the characters you've used in your string.  Currently three fonts are provided, as header files.  For a basic, simple DOS style font, I've created and included `IBM_PC_BIOS_8x8.h`, which is a basic ASCII font.  This means it only has 128 characters.  This covers letters, symbols, and numbers that you can type on your keyboard without any special tricks, along with a handful of semi-graphical characters in the control region.  To use this font, `#include <SSD1308/IBM_PS_BIOS_8x8.h>`, then use the variable `ibm_pc_bios_font` for the `font` argument.  Just make sure not to use any characters past 127 with this font, or you'll be rendering whatever garbage happens to be in the memory past the end of the font!  Two other full extended ASCII (256 character) fonts are also included.  `Caverns_8x8.h` and `Megazeux_8x8.h` contain fonts based on the DOS game Megazeux, which modified the DOS character set to provide graphic-like characters.  These include items, monsters, and terrain tiles, as well as menu box graphic elements.  Look in the header files to find the name of the variables containing the fonts.  Note that the 128 character IBM font only uses ~1kb of program memory, while the 256 character fonts each use around 2kb.  The CH552 has 16kb of code memory, but only 14kb are available, as the rest is used by the bootloader.  If your program is small, you can get away with using all three fonts, but if it is large and you don't need the graphical glyphs, you might want to use the IBM font to save 1kb.

##### Fundamental Commands for the Display

`void SSD1306_set_contrast(uint8_t con)`  
The display itself has some features that can be controlled by sending commands over I2C.  This function sends the command to adjust the contrast.  The initialization code sets the contrast to 143, a little higher than the driver chip's default of 127.

`void SSD1306_all_on(uint8_t enable)`  
The display driver has a command that turns all of the OLEDs on (white).  This doesn't change the image data on the driver's video memory, so you can use this to turn the screen all white then disable it to turn it back to whatever it was before.  Or you could turn it all white, write new data, then turn it back to display the new image.  This might be useful for certain kinds of special effects.

`void SSD1306_invert(uint8_t enable)`  
When enabled, the screen colors are inverted.  Again this doesn't change the image data stored in the driver chip, and this might be useful for certain kinds of special effects.

`void SSD1306_sleep(uint8_t enable)`  
This does the opposite of `all on`.  It causes the display to go black, without modifying the image data.  Because OLED power draw is a function of how many OLEDs are currently lit, this should reduce display power consumption significantly.  Of course, it also might be useful for special effects.

##### Scrolling Commands for the Display

```
void SSD1306_h_scroll(uint8_t start_p, uint8_t stop_p,
                      uint8_t interval, uint8_t dir)
```  
The SSD1306 has some scrolling features.  I haven't played with them myself.  The datasheet has more details, but there's a lot of ambiguity and missing details.  I'll do my best to explain as well as I can.  `start_p` and `stop_p` define the top and bottom of the region that scrolls, in 8 pixel aligned pages.  `interval` controls the delay between each movement.  `dir` controls direction, were 0 is right and 1 is left.  I might be wrong about `start_p` and `stop_p`, which might be left and right sides?  The important part you should know about the scrolling feature is that it is pretty clearly designed for scrolling marquees and not for precision scrolling that you might need in a video game.

```
void SSD1306_hv_scroll(uint8_t start_p, uint8_t stop_p,
                      uint8_t interval, uint8_t dir,
                      uint8_t v_offset)
```
The first four arguments here do the same thing they do in the previous function.  `v_offset` defines a vertical element to the scrolling, presumably in pixels per movement event.  The datasheet seems to indicate that only upward scrolling is available, but if `v_offset` is 31, that should scroll down by one pixel per scroll event.

`void SSD1306_start_scroll(void)`  
The previous two functions merely setup how the scrolling is supposed to work.  This one starts it.  None of the setup command should be issued when scrolling is on, and no graphics should be written.  According to the datasheet, failure to follow this rule will corrupt the graphics memory.

`void SSD1306_stop_scroll(void)`  
This is how you stop scrolling.

`void SSD1306_set_v_scroll_area(uint8_t start_row, uint8_t height)`
This sets the starting row for vertical scrolling, and it sets the height of the vertical scroll area.  This is one of the places where the datasheet actually provides good information...mosty.  Everything above the starting row does not scroll vertically.  The height is the total height of the scroll region, and nothing below that scrolls.  What the datasheet is missing is any explanation of how this affects horizontal scrolling.  Horizontal scrolling appears to set the top and bottom of the scroll region by pages, but these appear to be by pixel.  It's _really_ unclear what this actually does.  I haven't had time to test any of this scrolling capability, and since I have no application where highly imprecise scrolling would be useful I have little motive test it, when I have more important things to spend my time on.  If you decide to try out this feature, good luck!

#### Generic CH552 Use

This should be usable with other CH552 boards and with other CH55x variants.  Note that the framebuffer is statically mapped to start at address 512 in XRAM.  If you are using a CH55x variant with more XRAM, you may want to move the framebuffer to a different position in XRAM.  It's position is defined in `SSD1306.c`, and there's a macro defining its end address as well.  Make sure to change both of those, such that the framebuffer maintains a size of 512 bytes.  If you use this with another CH55x variant with more XRAM though, you might find yourself considering getting  128x64 or 128x128 display (Adafruit sells both) instead.  Most of my code makes no assumptions about framebuffer size and leaves it to the user to stay within the lines.  That said, there are some parts where the 512 byte length is hardcoded, including some assembly code.  If you want it to work with a larger display that uses the same hardware driver, it shouldn't be terribly difficult to adjust it, but you will definitely have to change at least a little bit of assembly code.  Oh right, you'll also have to change some arcane stuff in the initialization code, so that the hardware driver knows it has more pixels to work with.  Let me suggest looking at Adafruit's driver initialization code for those displays, if you want to use them.  It's possible that the initialization sequence will need more tweaking than just the size stuff, depending on the specific properties of each size of OLED display.

I can guarantee 100% that this won't work with other 8051 variants.  To achieve extremely high blitting speeds for writes to the framebuffer, I took advantage of a special instruction present in the CH552 that is not present in the original 8051 architecture and that is likely not present in most modern variants either.

#### Future Work

It might be nice to have some vector drawing functions, maybe as a separate submodule.  Circles, lines, squares, and triangles can be drawn quite quickly, and it might even be possible to get some (dithered) interpolation or even some shader capabilities in there as well.  I'm not sure if the CH552 is fast enough to do all of that _and_ an application that uses it all at the same time though.  Basic 2D vector drawing is a very good idea though.  This is _not_ high priority though.

I _am_ planning on converting more Megazeux fonts though.  Megazeux has some 6 games or so that are part of the series.  The fonts I've provided are the default character set and the character set from Caverns, the second game in the series.  I've started on Zeux, but converting 8x14 to 8x8 is rather a challenge.  Then we've got the ones after Caverns, which I've got the character sets for as well.  (Note that a) in the U.S. font faces cannot be copyrighted, and b) I'm not downscaline, I'm completely remaking these fonts from scratch, using the originals only as reference.  I'm releasing the fonts themselves into the public domain, so you can do whatever you want with them.)
</details>

---

### SSD1306_stdio

<details>
<summary>Details</summary>

This adds full console support using the SSD1306 library for rendering.  It allows the use of `printf()` to print directly to the display.  This also means that you can use `printf()` formatting commands and number to string conversion.  Note, however, that doing this will interfere with using `print()` to print to the serial console, so you have to choose!

#### Usage

`void SSD1306_set_font(SSD1306_CTiles *font)`  
Sets the font used by `printf()`.

Beyond this, there's nothing else you need to do.  This module overrides/provides `putchar()` to send text to the display.  This is automatic.  It also handles the cursor automatically, and it uses the same cursor as SSD1306.  This means you can use `SSD1306_locate()`, and now `printf()` will print starting at that location.  This also adds one more feature: If you write a character past the end of the display, it will scroll the display up 8 pixels to make room for the new text, just like a typical console.

Note that currently this is included in the library by default, and SSDC will include it in the program automatically, because `putchar()` is used.  I'm going to add a `make` variable that can be set to disable this module, so you can avoid the extra code if you aren't using it, and so you can prevent it from clashing with code directing `printf()` to the serial console.  I'll replace this with instructions on using that variable once I've done this.

#### Generic CH552 Use

This should work anywhere the SSD1306 module works.  If you change the screen size, you'll have to make changes here as well.

#### Future Work

I actually want to completely decouple this from SSD1306, so it can be used alone when you don't want graphics but need console-like behavior.  The graphics code is kind of heavy, so including it all when you are only ever printing characters just doesn't make sense.  I will have to include some interoperability code to do that though, so that if you _do_ want both, they don't step on each other's toes.  This honestly shouldn't be that hard, but it's not a high priority right now.
</details>

---

### Adafruit Gamepad Stemma QT

<details>
<summary>Details</summary>

This is not complete.  In fact, it's not quite started either.  The next driver I'm working on is for [Adafruit's Stemma QT Gamepad](https://www.adafruit.com/product/5743).  This uses Adafruit's Seesaw driver, so I'm going to have to at least partially implement a driver for that for the CH552.  The name of this section will probably change to something reflecting that, and it might end up being two drivers, the Seesaw driver, and then the gamepad driver that is dependent on the Seesaw driver.  I don't know yet.  I haven't actually started!
</details>
