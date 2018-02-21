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

void main(void) {
	SMS_useFirstHalfTilesforSprites(true);
	SMS_setSpriteMode(SPRITEMODE_TALL);

	SMS_loadSpritePalette(gelatinous_cube_palette_bin);
	SMS_loadPSGaidencompressedTiles(gelatinous_cube_tiles_psgcompr, 2);
	SMS_setClippingWindow(0, 0, 255, 192);
	SMS_displayOn();
	
	ply_frame_ctrl = 16;
	ply_frame = 0;

	while (true) {
		SMS_initSprites();

		draw_ship(8, 8, 2 + ply_frames[ply_frame]);
		draw_ship(8, 24, 14 + ply_frames[ply_frame]);
		draw_ship(8, 40, 26 + ply_frames[ply_frame]);
		draw_ship(8, 56, 38 + ply_frames[ply_frame]);
		
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
