SHELL=C:/Windows/System32/cmd.exe
CC	= sdcc -mz80

BINS	= duckslayer-adventures.sms


all:	$(BINS)

gfx.c: gfx/font.fnt
	folder2c gfx gfx

duckslayer-adventures.rel: gfx.c

%.rel:	%.c
	$(CC) -c --std-sdcc99 $< -o $@

%.sms:	%.ihx
	ihx2sms $< $@

duckslayer-adventures.ihx:	SMSlib/crt0_sms.rel duckslayer-adventures.rel SMSlib/SMSlib.lib PSGlib/PSGlib.rel gfx.rel
	$(CC) --no-std-crt0 --data-loc 0xC000 -o duckslayer-adventures.ihx $^

clean:
	rm -f *.ihx *.lk *.lst *.map *.noi *.rel *.sym *.asm *.sms gfx.*
