CFLAGS=
INCLUDES=../include

rel_files=delay.rel I2C.rel SSD1306/SSD1306.rel SSD1306/SSD1306_stdio.rel clock.rel




mcs51.lib: $(rel_files)
	sdar rcs mcs51.lib $(rel_files)



delay.rel: delay.asm
	sdas8051.exe -plosffwv delay.rel delay.asm

I2C.rel: I2C.c
	sdcc I2C.c -c -I $(INCLUDES) $(CFLAGS)

clock.rel: clock.c
	echo $(CFLAGS)
	sdcc clock.c -c -I $(INCLUDES) $(CFLAGS)


SSD1306/SSD1306.rel:
	+$(MAKE) -C SSD1306 SSD1306.rel CFLAGS="$(CFLAGS)"

SSD1306/SSD1306_stdio.rel:
	+$(MAKE) -C SSD1306 SSD1306_stdio.rel CFLAGS="$(CFLAGS)"


clean:
		-rm I2C.asm
		-rm *.lst
		-rm *.rel
		-rm *.ihx
		-rm *.sym
		-rm *.map
		-rm *.mem
		-rm *.rst
		-rm *.lk
		$(MAKE) -C SSD1306 clean

purge: clean
	rm *.lib

