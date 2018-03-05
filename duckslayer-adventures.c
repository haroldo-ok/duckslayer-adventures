#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "SMSlib/src/SMSlib.h"
#include "PSGlib/src/PSGlib.h"
#include "gfx.h"

#define MAX_ACTORS 4

typedef struct _actor_class {
	unsigned char base_tile;
	void (*draw)(void *);
} actor_class;

typedef struct _actor {
	int x, y;
	actor_class *class;
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
	draw_ship(act->x, act->y, act->class->base_tile);
}

void draw_actor_dragon(void *p) {
	actor *act = p;
	draw_dragon(act->x, act->y, act->class->base_tile);
}

const actor_class cube_class = {2, draw_actor_player};
const actor_class green_dragon_class = {14, draw_actor_dragon};
const actor_class red_dragon_class = {26, draw_actor_dragon};
const actor_class yellow_dragon_class = {38, draw_actor_dragon};

actor actors[MAX_ACTORS];
actor *ply_actor = actors;

void init_actor(int id, int x, int y, actor_class *class) {
	actor *act = actors + id;
	act->x = x;
	act->y = y;
	act->class = class;
}

unsigned int i, j;
actor *act;
int joy;

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
	
	init_actor(0, 8, 8, &cube_class);
	init_actor(1, 32, 8, &green_dragon_class);
	init_actor(2, 32, 72, &red_dragon_class);
	init_actor(3, 80, 72, &yellow_dragon_class);

	while (true) {
		joy = SMS_getKeysStatus();

		if (joy & PORT_A_KEY_UP) {
			ply_actor->y--;
		} else if (joy & PORT_A_KEY_DOWN) {
			ply_actor->y++;
		}
		
		if (joy & PORT_A_KEY_LEFT) {
			ply_actor->x--;
		} else if (joy & PORT_A_KEY_RIGHT) {
			ply_actor->x++;
		}
	
		actors[1].x--;
		actors[2].y++;
		actors[3].y--;
		
		SMS_initSprites();

		for (i = MAX_ACTORS, j = flicker_ctrl; i; i--, j++) {
			if (j >= MAX_ACTORS) {
				j = 0;
			}
			
			act = actors + j;
			act->class->draw(act);
		}
		
		flicker_ctrl++;
		if (flicker_ctrl >= MAX_ACTORS) {
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
