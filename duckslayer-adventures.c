#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "SMSlib/src/SMSlib.h"
#include "PSGlib/src/PSGlib.h"
#include "gfx.h"

int ply_frame_ctrl, ply_frame;

const unsigned int ply_frames[] = { 0, 4, 8, 4 };

void draw_ship(unsigned char x, unsigned char y, unsigned char base_tile) {
	SMS_addSprite(x, y, base_tile);
	SMS_addSprite(x + 8, y, base_tile + 2);
}

void draw_dragon(unsigned char x, unsigned char y, unsigned char base_tile) {
	unsigned char x1, y1, tile = base_tile;
	unsigned char i, j;

	for (i = 4, y1 = y; i; i--, y1 += 16) {
		for (j = 6, x1 = x; j; j--, x1 += 8) {
			SMS_addSprite(x1, y1, base_tile);
			base_tile += 2;
		}
		base_tile += 36;
	}
}

void main(void) {
	SMS_useFirstHalfTilesforSprites(true);
	SMS_setSpriteMode(SPRITEMODE_TALL);

	SMS_loadSpritePalette(all_sprites_palette_bin);
	SMS_loadPSGaidencompressedTiles(all_sprites_tiles_psgcompr, 2);
	SMS_setClippingWindow(0, 0, 255, 192);
	SMS_displayOn();
	
	ply_frame_ctrl = 16;
	ply_frame = 0;

	while (true) {
		SMS_initSprites();

		draw_ship(8, 8, 2 + ply_frames[ply_frame]);
		draw_ship(8, 24, 50 + ply_frames[ply_frame]);
		draw_ship(8, 40, 98 + ply_frames[ply_frame]);
		draw_ship(8, 56, 146 + ply_frames[ply_frame]);
		
		draw_dragon(32, 8, 14);
		draw_dragon(32, 72, 26);
		draw_dragon(80, 72, 38);
		
		ply_frame_ctrl--;
		if (!ply_frame_ctrl) {
			ply_frame_ctrl = 6;			
			ply_frame++;
			ply_frame &= 3;
		}

		SMS_finalizeSprites();

		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
	}

}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2018,2,20, "Haroldo-OK\\2018", "Duckslayer Adventures",
  "A homage to a classic.\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
