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
	unsigned char frame_count;
	unsigned char frame_delay;
	unsigned int *frames;
	void (*draw)(void *);
} actor_class;

typedef struct _actor {
	int x, y;
	char life;
	unsigned char curr_frame;
	unsigned char frame_delay_ctr;
	unsigned char frame_offset;
	actor_class *class;
} actor;

typedef struct _room {
	unsigned char *map;
	unsigned int base_fg_tile;
	unsigned int base_bg_tile;
	struct _room *top_exit;
	struct _room *bottom_exit;
	struct _room *left_exit;
	struct _room *right_exit;
} room;

int ply_frame_ctrl, ply_frame;
int flicker_ctrl;

const unsigned int ply_frames[] = { 0, 4, 8, 4 };
const unsigned int no_frames[] = { 0 };

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
	
	if (!act->life) {
		return;
	} 
	
	draw_ship(act->x, act->y, act->class->base_tile + act->class->frames[act->curr_frame] + act->frame_offset);

	if (!act->frame_delay_ctr) {
		act->curr_frame++;
		if (act->curr_frame >= act->class->frame_count) {
			act->curr_frame = 0;
		}		
		
		act->frame_delay_ctr = act->class->frame_delay;
	} else {
		act->frame_delay_ctr--;
	}
}

void draw_actor_dragon(void *p) {
	actor *act = p;

	if (act->life <= 0) {
		return;
	}

	draw_dragon(act->x, act->y, act->class->base_tile);
}

const actor_class cube_class = {2, 4, 6, ply_frames, draw_actor_player};
const actor_class green_dragon_class = {14, 0, 0, no_frames, draw_actor_dragon};
const actor_class red_dragon_class = {26, 0, 0, no_frames, draw_actor_dragon};
const actor_class yellow_dragon_class = {38, 0, 0, no_frames, draw_actor_dragon};

const room yellow_castle_front = {
	yellow_castle_front_txt, 260, 272, 
	0, &garden_center, 0, 0
};
	
const room garden_center = {
	garden_center_txt, 264, 272, 
	&yellow_castle_front, 0, &garden_left, &garden_right
};

const room garden_left = {
	garden_left_txt, 264, 272, 
	&labyrinth_entrance, 0, 0, &garden_center
};

const room garden_right = {
	garden_right_txt, 264, 272, 
	0, 0, &garden_center, 0	
};

const room labyrinth_entrance = {
	labyrinth_entrance_txt, 256, 280, 
	&labyrinth_left, &garden_left, &labyrinth_middle, &labyrinth_middle
};

const room labyrinth_middle = {
	labyrinth_middle_txt, 256, 280, 
	&labyrinth_top, &labyrinth_bottom, &labyrinth_entrance, &labyrinth_entrance	
};

const room labyrinth_top = {
	labyrinth_top_txt, 256, 280, 
	&black_castle_front, &labyrinth_middle, &labyrinth_bottom, &labyrinth_left
};

const room labyrinth_bottom = {
	labyrinth_bottom_txt, 256, 280, 
	&labyrinth_middle, 0, &labyrinth_left, &labyrinth_top
};

const room labyrinth_left = {
	labyrinth_left_txt, 256, 280, 
	0, &labyrinth_entrance, &labyrinth_top, &labyrinth_bottom
};

const room black_castle_front = {
	black_castle_front_txt, 268, 280, 
	&black_castle_interior, &labyrinth_top, 0, 0
};

const room black_castle_interior = {
	black_castle_interior_txt, 268, 276, 
	0, &black_castle_front, 0, 0
};

actor actors[MAX_ACTORS];
actor *ply_actor = actors;

room *curr_room;

void init_actor(int id, int x, int y, char life, actor_class *class) {
	actor *act = actors + id;
	act->x = x;
	act->y = y;
	act->life = life;
	act->curr_frame = 0;
	act->frame_delay_ctr = class->frame_delay;
	act->frame_offset = 0;
	act->class = class;
}

// Right now, I'm too lazy to use a proper map editor.. :P
void draw_room(unsigned char *map, unsigned int base_fg_tile, unsigned int base_bg_tile) {
	unsigned char i, j, ch, *o = map, *line;
	unsigned int tile;

	SMS_setNextTileatXY(0, 0);
	for (i = 0; i != 12; i++) {
		// Skip blanks
		ch = *o;
		while (ch == ' ' || ch == '\n' || ch == '\r') {
			o++;
			ch = *o;
		}
		
		line = o;
		
		for (j = 0; j != 16; j++) {
			ch = *o;
			
			tile = ch == '#' ? base_fg_tile : base_bg_tile;
			SMS_setTile(tile);
			SMS_setTile(tile + 2);
			
			o++;
		}
		
		o = line;
		
		for (j = 0; j != 16; j++) {
			ch = *o;
			
			tile = ch == '#' ? base_fg_tile : base_bg_tile;
			SMS_setTile(tile + 1);
			SMS_setTile(tile + 3);
			
			o++;
		}		
	}
}

void draw_current_room() {
	draw_room(curr_room->map, curr_room->base_fg_tile, curr_room->base_bg_tile);
}

unsigned int i, j;
actor *act;
int joy;

void main(void) {
	
	SMS_useFirstHalfTilesforSprites(true);
	SMS_setSpriteMode(SPRITEMODE_TALL);

	SMS_loadBGPalette(background_tiles_palette_bin);
	SMS_loadSpritePalette(all_sprites_palette_bin);
	SMS_loadPSGaidencompressedTiles(background_tiles_psgcompr, 256);
	SMS_loadPSGaidencompressedTiles(all_sprites_tiles_psgcompr, 2);
	SMS_setClippingWindow(0, 0, 255, 192);
	SMS_displayOn();
	
	ply_frame_ctrl = 16;
	ply_frame = 0;
	
	flicker_ctrl = 0;

	curr_room = &yellow_castle_front;
	draw_current_room();
	
	init_actor(0, 8, 8, 1, &cube_class);
	init_actor(1, 32, 8, 1, &green_dragon_class);
	init_actor(2, 32, 72, 0, &red_dragon_class);
	init_actor(3, 80, 72, 1, &yellow_dragon_class);

	while (true) {
		joy = SMS_getKeysStatus();

		if (joy & PORT_A_KEY_UP) {
			ply_actor->y--;
			ply_actor->frame_offset = 144;
		} else if (joy & PORT_A_KEY_DOWN) {
			ply_actor->y++;
			ply_actor->frame_offset = 0;
		}
		
		if (joy & PORT_A_KEY_LEFT) {
			ply_actor->x--;
			ply_actor->frame_offset = 48;
		} else if (joy & PORT_A_KEY_RIGHT) {
			ply_actor->x++;
			ply_actor->frame_offset = 96;
		}
		
		if (ply_actor->y < -8) {
			ply_actor->y = 182;
			if (curr_room->top_exit) {
				curr_room = curr_room->top_exit;
				draw_current_room();
			}
		}
	
		if (ply_actor->y > 184) {
			ply_actor->y = -7;
			if (curr_room->bottom_exit) {
				curr_room = curr_room->bottom_exit;
				draw_current_room();
			}
		}
		
		if (ply_actor->x < -8) {
			ply_actor->x = 247;
			if (curr_room->left_exit) {
				curr_room = curr_room->left_exit;
				draw_current_room();
			}
		}
		
		if (ply_actor->x > 248) {
			ply_actor->x = -7;
			if (curr_room->right_exit) {
				curr_room = curr_room->right_exit;
				draw_current_room();
			}
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
