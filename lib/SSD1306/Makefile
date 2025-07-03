
F_CPU=16000000
C_FLAGS=
INCLUDES=../../include/


all: SSD1306.rel SSD1306_stdio.rel

SSD1306.rel: SSD1306.c
	sdcc SSD1306.c -c -I $(INCLUDES) $(C_FLAGS)
#	sdcc SSD1306.c -c -DF_CPU=$(F_CPU) $(C_FLAGS)

SSD1306_stdio.rel: SSD1306_stdio.c
	sdcc SSD1306_stdio.c -c -I $(INCLUDES) $(C_FLAGS)
#	sdcc SSD1306_stdio.c -c -DF_CPU=$(F_CPU) $(C_FLAGS)


clean:
		-rm SSD1306.asm
		-rm SSD1306_stdio.asm
		-rm *.lst
		-rm *.rel
		-rm *.ihx
		-rm *.sym
		-rm *.map
		-rm *.mem
		-rm *.rst
		-rm *.lk
