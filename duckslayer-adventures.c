#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "SMSlib/src/SMSlib.h"
#include "PSGlib/src/PSGlib.h"
#include "gfx.h"

#define MAX_ACTORS 8

typedef struct _actor_class {
	unsigned char base_tile;
	unsigned char frame_count;
	unsigned char frame_delay;
	unsigned char can_be_carried;
	unsigned int *frames;
	void (*draw)(void *);
} actor_class;

typedef struct _actor {
	int x, y;	
	int spd_x, spd_y, spd_delay;
	char life;
	unsigned char curr_frame;
	unsigned char frame_delay_ctr;
	unsigned char frame_offset;
	struct _room *room;
	
	struct _actor *carried_by;
	struct _actor *carrying;
	int carry_dx, carry_dy;
	
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
room *curr_room;
char castle_locked;

const unsigned int ply_frames[] = { 0, 4, 8, 4 };
const unsigned int chalice_frames[] = { 0, 48, 96, 48 };
const unsigned int no_frames[] = { 0 };

void draw_ship(unsigned char x, unsigned char y, unsigned char base_tile) {
	SMS_addSprite(x, y, base_tile);
	SMS_addSprite(x + 8, y, base_tile + 2);
}

// Made into globals for Z80 performance
unsigned char drg_x1, drg_y1, drg_tile, drg_i, drg_j;

void draw_dragon(unsigned char x, unsigned char y, unsigned char base_tile) {
	drg_tile = base_tile;

	for (drg_i = 4, drg_y1 = y; drg_i; drg_i--, drg_y1 += 16) {
		for (drg_j = 6, drg_x1 = x; drg_j; drg_j--, drg_x1 += 8) {
			SMS_addSprite(drg_x1, drg_y1, drg_tile);
			drg_tile += 2;
		}
		drg_tile += 36;
	}
}

void draw_actor_player(void *p) {
	actor *act = p;
	
	if (!act->life) {
		return;
	} 
	
	if (act->room != curr_room) {
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

	if (act->room != curr_room) {
		return;
	}

	draw_dragon(act->x - 24, act->y - 24, act->class->base_tile);
}

const actor_class cube_class = {2, 4, 6, 0, ply_frames, draw_actor_player};
const actor_class green_dragon_class = {14, 0, 0, 0, no_frames, draw_actor_dragon};
const actor_class red_dragon_class = {26, 0, 0, 0, no_frames, draw_actor_dragon};
const actor_class yellow_dragon_class = {38, 0, 0, 0, no_frames, draw_actor_dragon};
const actor_class sword_class = {26, 0, 0, 1, no_frames, draw_actor_player};
const actor_class chalice_class = {74, 4, 4, 1, chalice_frames, draw_actor_player};
const actor_class yellow_key_class = {78, 0, 0, 1, no_frames, draw_actor_player};
const actor_class black_key_class = {174, 0, 0, 1, no_frames, draw_actor_player};

const room yellow_castle_front = {
	yellow_castle_front_txt, 260, 272, 
	&yellow_castle_interior, &garden_center, 0, 0
};
	
const room yellow_castle_interior = {
	yellow_castle_interior_txt, 260, 276, 
	0, &yellow_castle_front, 0, 0
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
	0, &green_dragon_lair, &garden_center, 0	
};

const room green_dragon_lair = {
	green_dragon_lair_txt, 264, 276, 
	&garden_right, 0, 0, 0	
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
actor *green_dragon_actor = actors + 1;
actor *yellow_dragon_actor = actors + 3;
actor *sword_actor = actors + 4;
actor *chalice_actor = actors + 5;
actor *yellow_key_actor = actors + 6;
actor *black_key_actor = actors + 7;

unsigned char *row_pointers[12];

void init_actor(int id, int x, int y, char life, actor_class *class) {
	actor *act = actors + id;
	act->x = x;
	act->y = y;
	act->spd_x = 0;
	act->spd_y = 0;
	act->spd_delay = 0;
	act->life = life;
	act->curr_frame = 0;
	act->frame_delay_ctr = class->frame_delay;
	act->frame_offset = 0;
	act->carried_by = 0;
	act->carrying = 0;
	act->class = class;
}

unsigned int base_tile_for_char(unsigned char ch) {
	switch (ch) {
	case '#':
		return curr_room->base_fg_tile;
		
	case '^':
		return castle_locked ? 284 : curr_room->base_bg_tile;
		
	default:
		return curr_room->base_bg_tile;
	}
}

void check_castle_locks() {
	castle_locked = 1;
	
	if (curr_room == &yellow_castle_front) {
		castle_locked = ply_actor->carrying != yellow_key_actor && yellow_key_actor->room != &yellow_castle_interior;
	}
	
	if (curr_room == &black_castle_front) {
		castle_locked = ply_actor->carrying != black_key_actor && black_key_actor->room != &black_castle_interior;
	}
}

// Right now, I'm too lazy to use a proper map editor.. :P
void draw_room(unsigned char *map) {
	unsigned char i, j, ch, *o = map, *line;
	unsigned int tile;
	
	check_castle_locks();

	SMS_setNextTileatXY(0, 0);
	for (i = 0; i != 12; i++) {
		// Skip blanks
		ch = *o;
		while (ch == ' ' || ch == '\n' || ch == '\r') {
			o++;
			ch = *o;
		}
		
		row_pointers[i] = o;
		
		line = o;
		
		for (j = 0; j != 16; j++) {
			ch = *o;
			
			tile = base_tile_for_char(ch);
			SMS_setTile(tile);
			SMS_setTile(tile + 2);
			
			o++;
		}
		
		o = line;
		
		for (j = 0; j != 16; j++) {
			ch = *o;
			
			tile = base_tile_for_char(ch);
			SMS_setTile(tile + 1);
			SMS_setTile(tile + 3);
			
			o++;
		}		
	}
}

void draw_current_room() {
	draw_room(curr_room->map);
}

unsigned char block_at(int x, int y) {
	if (x < 0 || x > 255 || y < 0 || y > 255) {
		return 0;
	}
	
	return row_pointers[y >> 4][x >> 4];
}

char can_move_delta(int dx, int dy) {
	char ch = block_at(ply_actor->x + dx, ply_actor->y + dy);
	if ('^' == ch) {
		return !castle_locked;
	}
	return '#' != ch;
}

char room_has_char(unsigned char expected) {
	unsigned char i, j, *o;
	
	for (i = 0; i != 12; i++) {
		o = row_pointers[i];
		
		for (j = 0; j != 16; j++) {
			if (*o == expected) {
				return 1;
			}
			
			o++;
		}
	}	
	
	return 0;
}

void try_pickup(actor *picker, actor *target) {
	int dx = picker->x - target->x;
	int dy = picker->y - target->y;
	
	if (!target->class->can_be_carried ||
		picker->room != target->room ||
		picker->carrying == target) {
		return;
	}
	
	if (dx > -12 && dx < 12 && dy > -12 && dy < 12) {
		picker->carry_dx = dx;
		picker->carry_dy = dy;

		target->carried_by = picker;
		picker->carrying = target;
		
		PSGPlayNoRepeat(duckslayer_pickup_psg);	
	
		if (target == yellow_key_actor || target == black_key_actor) {
			draw_current_room();
		}
	}
}

void move_towards(actor *act, actor *target) {
	act->spd_x = 0;	
	if (act->x > target->x) {
		act->spd_x = -1;
	} else if (act->x < target->x) {
		act->spd_x = 1;
	}

	act->spd_y = 0;	
	if (act->y > target->y) {
		act->spd_y = -1;
	} else if (act->y < target->y) {
		act->spd_y = 1;
	}
}

void apply_speed(actor *act) {
	if (act->spd_delay) {
		act->spd_delay--;
	} else {
		act->x += act->spd_x;
		act->y += act->spd_y;		
		act->spd_delay = 1;
	}	
}

unsigned char try_moving_towards(actor *act, actor *target) {
	if (act->room != target->room) {
		return 0;
	}
	move_towards(act, target);
	return 1;
}

unsigned char try_moving_away(actor *act, actor *target) {
	if (act->room != target->room) {
		return 0;
	}
	move_towards(act, target);
	act->spd_x = -act->spd_x;
	act->spd_y = -act->spd_y;
	return 1;
}

unsigned char try_moving_randomly(actor *act) {
	if (rand() & 0x1FF) {
		return 0;
	}
	
	act->spd_x = 0;
	act->spd_y = 0;

	switch (rand() & 0x07) {
	case 0:
		act->spd_y = -1;
		break;
	case 1:
		act->spd_x = 1;
		act->spd_y = -1;
		break;
	case 2:
		act->spd_x = 1;
		break;
	case 3:
		act->spd_x = 1;
		act->spd_y = 1;
		break;
	case 4:
		act->spd_y = 1;
		break;
	case 5:
		act->spd_x = -1;
		act->spd_y = 1;
		break;
	case 6:
		act->spd_x = -1;
		break;
	case 7:
		act->spd_x = -1;
		act->spd_y = -1;
		break;
	}
	
	return 1;
}

unsigned char check_exits(actor *act) {
	// Exiting from the top
	if (act->y < -8) {
		act->y = 182;
		if (act->room->top_exit) {
			act->room = act->room->top_exit;
		}
	}

	// Exiting from the bottom
	if (act->y > 184) {
		act->y = -7;
		if (act->room->bottom_exit) {
			act->room = act->room->bottom_exit;
		}
	}
	
	// Exiting from the left
	if (act->x < -8) {
		act->x = 247;
		if (act->room->left_exit) {
			act->room = act->room->left_exit;
		}
	}	

	// Exiting from the right
	if (act->x > 248) {
		act->x = -7;
		if (act->room->right_exit) {
			act->room = act->room->right_exit;
		}
	}
}

void try_killing(actor *killer, actor *victim) {
	int dx = killer->x - victim->x;
	int dy = killer->y - victim->y;

	if (killer->room != victim->room) {
		return;
	}
	
	if (dx > -12 && dx < 12 && dy > -12 && dy < 12) {
		victim->life = 0;
	}
}

void green_dragon_ai() {
	try_moving_away(green_dragon_actor, sword_actor) ||
	try_moving_towards(green_dragon_actor, ply_actor) ||
	try_moving_towards(green_dragon_actor, chalice_actor) ||
	try_moving_towards(green_dragon_actor, black_key_actor) ||
	try_moving_randomly(green_dragon_actor);
	
	apply_speed(green_dragon_actor);
	try_killing(green_dragon_actor, ply_actor);
	check_exits(green_dragon_actor);
}

void yellow_dragon_ai() {
	try_moving_away(yellow_dragon_actor, sword_actor) ||
	try_moving_away(yellow_dragon_actor, yellow_key_actor) ||
	try_moving_towards(yellow_dragon_actor, ply_actor) ||
	try_moving_towards(yellow_dragon_actor, chalice_actor) ||
	try_moving_randomly(yellow_dragon_actor);
	
	apply_speed(yellow_dragon_actor);
	try_killing(yellow_dragon_actor, ply_actor);
	check_exits(yellow_dragon_actor);
}

void load_normal_palette() {
	SMS_loadBGPalette(background_tiles_palette_bin);
	SMS_loadSpritePalette(all_sprites_palette_bin);	
	SMS_setSpritePaletteColor (0, 0);
}

void drop_carried_object() {
	actor *target = ply_actor->carrying;
	
	if (target) {
		ply_actor->carrying->carried_by = 0;
		ply_actor->carrying = 0;
		
		if (target == yellow_key_actor || target == black_key_actor) {
			draw_current_room();
		}
	}	
}

void check_player_death() {
	if (ply_actor->life) {
		// Player is still alive
		return;
	}
	
	// Player died; send him back to the yellow castle.

	drop_carried_object();	
	init_actor(0, 120, 160, 1, &cube_class);
	curr_room = &yellow_castle_front;
	draw_current_room();
	ply_actor->life = 1;	
	
	PSGPlayNoRepeat(dspdiehi_psg);	
}

void check_ending() {
	unsigned char i;
	unsigned char anim_time;
	unsigned char pal1[16], pal2[16];
	
	if (ply_actor->carrying != chalice_actor || curr_room != &yellow_castle_interior) {
		// Didn't reach the endgame condition, yet.
		return;
	}
	
	// Endgame
	
	// Plays the victory SFX
	PSGPlayNoRepeat(win_psg);					
				
	// Cycles through the palette for a while
	for (anim_time = 32; anim_time; anim_time--) {
		SMS_waitForVBlank();		

		for (i = 0; i < 16; i++) {
			pal1[i] = background_tiles_palette_bin[(i + anim_time) & 0xF];
			pal2[i] = all_sprites_palette_bin[(i + anim_time) & 0xF];
		}


		SMS_loadBGPalette(pal1);
		SMS_loadSpritePalette(pal2);			

		SMS_waitForVBlank();		
		SMS_waitForVBlank();		
		SMS_waitForVBlank();		
		SMS_waitForVBlank();
	}

	load_normal_palette();
	
	for (;;) {
		SMS_waitForVBlank();		
	}
}

void interrupt_handler() {
	PSGFrame();
}

unsigned int i, j;
actor *act;
int joy;

void main(void) {
	SMS_useFirstHalfTilesforSprites(true);
	SMS_setSpriteMode(SPRITEMODE_TALL);

	load_normal_palette();
	SMS_loadPSGaidencompressedTiles(background_tiles_psgcompr, 256);
	SMS_loadPSGaidencompressedTiles(all_sprites_tiles_psgcompr, 2);
	SMS_setClippingWindow(0, 0, 255, 192);
	SMS_displayOn();
	
	ply_frame_ctrl = 16;
	ply_frame = 0;
	
	flicker_ctrl = 0;

	curr_room = &yellow_castle_front;
	draw_current_room();
	
	init_actor(0, 120, 160, 1, &cube_class);
	init_actor(1, 32, 8, 1, &green_dragon_class);
	init_actor(2, 32, 72, 0, &red_dragon_class);
	init_actor(3, 80, 72, 1, &yellow_dragon_class);
	init_actor(4, 120, 88, 1, &sword_class);
	init_actor(5, 120, 88, 1, &chalice_class);
	init_actor(6, 32, 64, 1, &yellow_key_class);
	init_actor(7, 120, 88, 1, &black_key_class);
	
	actors[0].room = &yellow_castle_front;
	actors[1].room = &green_dragon_lair;
	actors[2].room = &green_dragon_lair;
	actors[3].room = &garden_left;
	actors[4].room = &yellow_castle_interior;
	actors[5].room = &black_castle_interior;
	actors[6].room = &yellow_castle_front;
	actors[7].room = &green_dragon_lair;
	
	SMS_setLineInterruptHandler(&interrupt_handler);
	SMS_setLineCounter (180);
	SMS_enableLineInterrupt(); 

	while (true) {
		//PSGFrame();
		
		check_ending();
		check_player_death();
		
		joy = SMS_getKeysStatus();		

		if (joy & PORT_A_KEY_UP) {
			// Move up
			ply_actor->frame_offset = 144;
			if (can_move_delta(3, 2) && can_move_delta(13, 2)) {
				ply_actor->y--;
				
				// Entering a gate
				if ('^' == block_at(ply_actor->x + 8, ply_actor->y + 8)) {
					ply_actor->y = 182;
					curr_room = curr_room->top_exit;
					draw_current_room();					
				}
			}
		} else if (joy & PORT_A_KEY_DOWN) {
			// Move down
			ply_actor->frame_offset = 0;
			if (can_move_delta(3, 14) && can_move_delta(13, 14)) {
				ply_actor->y++;
			}
		}
		
		if (joy & PORT_A_KEY_LEFT) {
			// Move left
			ply_actor->frame_offset = 48;
			if (can_move_delta(2, 3) && can_move_delta(2, 13)) {
				ply_actor->x--;
			}
		} else if (joy & PORT_A_KEY_RIGHT) {
			// Move right
			ply_actor->frame_offset = 96;
			if (can_move_delta(14, 3) && can_move_delta(14, 13)) {
				ply_actor->x++;
			}
		}

		// Player exiting from the top
		if (ply_actor->y < -8) {
			ply_actor->y = 182;
			if (curr_room->top_exit) {
				curr_room = curr_room->top_exit;
				draw_current_room();
			}
		}
	
		// Player exiting from the bottom
		if (ply_actor->y > 184) {
			ply_actor->y = -7;
			if (curr_room->bottom_exit) {
				curr_room = curr_room->bottom_exit;
				draw_current_room();				

				// Special case: coming back from a gate
				if (room_has_char('^')) {
					ply_actor->x = 120;					
					ply_actor->y = 144;	
				}
			}
		}
		
		// Player exiting from the left
		if (ply_actor->x < -8) {
			ply_actor->x = 247;
			if (curr_room->left_exit) {
				curr_room = curr_room->left_exit;
				draw_current_room();
			}
		}
		
		// Player exiting from the right
		if (ply_actor->x > 248) {
			ply_actor->x = -7;
			if (curr_room->right_exit) {
				curr_room = curr_room->right_exit;
				draw_current_room();
			}
		}
		
		ply_actor->room = curr_room;

		if (joy & (PORT_A_KEY_1 | PORT_A_KEY_2)) {
			// Holding a button: drop carried object
			if (ply_actor->carrying) {
				PSGPlayNoRepeat(duckslayer_drop_psg);					
			}
			drop_carried_object();
		}
		// Should have been an else, but SDCC is behaving in a weird way when doing so
		if (!(joy & (PORT_A_KEY_1 | PORT_A_KEY_2))) {
			// Not holding a button; see if the player can pickup something
			for (i = MAX_ACTORS - 1, act = actors + 1; i; i--, act++) {
				try_pickup(ply_actor, act);
			}			
		}

		// If the player is moving while carryng something, adjust the coordinates of the held object
		if (ply_actor->carrying) {
			act = ply_actor->carrying;
			
			act->x = ply_actor->x;
			act->y = ply_actor->y;
			act->x -= ply_actor->carry_dx;
			act->y -= ply_actor->carry_dy;
			act->room = ply_actor->room;
		}
		
		green_dragon_ai();
		yellow_dragon_ai();
	
		// Draw sprites, doing flickering if necessary	(rotates the positions so the sprites are drawn in a different order every frame)
		
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
SMS_EMBED_SDSC_HEADER(0,6, 2018,4,1, "Haroldo-OK\\2018", "Duckslayer Adventures",
  "A homage to a classic.\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
