/* GB2CPC -- "Hello Sprite" proof of concept.
 *
 * A single sprite the player moves with the arrow keys. This source is written
 * in plain GBDK style: were it compiled with GBDK for a real Game Boy it would
 * behave identically. GB2CPC re-targets it to the Amstrad CPC unchanged.
 *
 * Controls (CPC keyboard, mapped to the GB d-pad):
 *   Cursor keys -> move   |   Z = A   X = B   Enter = Start   Space = Select
 */
#include <gb2cpc/gb/gb.h>

#include "assets.h"

#define PLAYER_SPRITE 0
#define START_X 84   /* GB OAM coords (screen = x-8, y-16) -> ~centre */
#define START_Y 80
#define SPEED   2

void main(void) {
   UINT8 x = START_X;
   UINT8 y = START_Y;
   UINT8 keys;

   gb2cpc_init();

   /* Load the converted sprite tile + its palette. */
   gb2cpc_set_palette(player_palette);
   set_sprite_data(0, PLAYER_NUM_TILES, player);
   set_sprite_tile(PLAYER_SPRITE, 0);
   move_sprite(PLAYER_SPRITE, x, y);

   SHOW_SPRITES;
   DISPLAY_ON;

   for (;;) {
      keys = joypad();

      if ((keys & J_LEFT)  && x > 8 + SPEED)        x -= SPEED;
      if ((keys & J_RIGHT) && x < 8 + 160 - SPEED)  x += SPEED;
      if ((keys & J_UP)    && y > 16 + SPEED)       y -= SPEED;
      if ((keys & J_DOWN)  && y < 16 + 144 - SPEED) y += SPEED;

      move_sprite(PLAYER_SPRITE, x, y);
      wait_vbl_done();   /* render this frame + flip */
   }
}
