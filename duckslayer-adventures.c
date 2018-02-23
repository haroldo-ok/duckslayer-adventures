#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "SMSlib/src/SMSlib.h"
#include "PSGlib/src/PSGlib.h"
#include "gfx.h"

#define MAX_ACTORS 7

typedef struct _actor {
	int x, y;
	unsigned char base_tile;
	void (*draw)(void *);
} actor;

int ply_frame_ctrl, ply_frame;
int flicker_ctrl;

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

void draw_actor_player(void *p) {
	actor *act = p;
	draw_ship(act->x, act->y, act->base_tile);
}

void draw_actor_dragon(void *p) {
	actor *act = p;
	draw_dragon(act->x, act->y, act->base_tile);
}

const actor actors[MAX_ACTORS] = {
	{8, 8, 2, draw_actor_player},
	{8, 24, 50, draw_actor_player},
	{8, 40, 98, draw_actor_player},
	{8, 56, 146, draw_actor_player},
	{32, 8, 14, draw_actor_dragon},
	{32, 72, 26, draw_actor_dragon},
	{80, 72, 38, draw_actor_dragon}
};

unsigned int i, j;
actor *act;

void main(void) {
	
	SMS_useFirstHalfTilesforSprites(true);
	SMS_setSpriteMode(SPRITEMODE_TALL);

	SMS_loadSpritePalette(all_sprites_palette_bin);
	SMS_loadPSGaidencompressedTiles(all_sprites_tiles_psgcompr, 2);
	SMS_setClippingWindow(0, 0, 255, 192);
	SMS_displayOn();
	
	ply_frame_ctrl = 16;
	ply_frame = 0;
	
	flicker_ctrl = 0;

	while (true) {
		SMS_initSprites();

		for (i = MAX_ACTORS, j = 0; i; i--, j++) {
			act = actors + j;
			act->draw(act);
			//draw_ship(act->x, act->y, act->base_tile);
		}
		
		// Flicker test; far from clean
		/*
		for (i = 3, j = flicker_ctrl; i; i--, j++) {
			if (j > 2) {
				j = 0;
			}
			
			switch (j) {
			case 0:
				draw_dragon(32, 8, 14);
				break;
				
			case 1:
				draw_dragon(32, 72, 26);
				break;
				
			case 2:
				draw_dragon(80, 72, 38);
				break;
			}
		}
		*/
		
		flicker_ctrl++;
		if (flicker_ctrl > 2) {
			flicker_ctrl = 0;
		}
				
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
