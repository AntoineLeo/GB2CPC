/* GB2CPC -- GBDK-compatible high-level API.
 *
 * A subset of GBDK's <gb/gb.h> sufficient for the Early-Alpha proof of concept
 * (sprites, background tiles, joypad, vsync). Each function is implemented on
 * top of CPCtelera in src/gb2cpc_core.c, translating Game Boy tile/sprite
 * semantics into CPC Mode 1 software-sprite operations.
 *
 * Coordinate system: like the real Game Boy, sprite coordinates are offset.
 * move_sprite(n, x, y) places the sprite so that (x-8, y-16) is its top-left
 * on screen -- matching hardware OAM, so GB source needs no coordinate edits.
 */
#ifndef GB2CPC_GB_GB_H
#define GB2CPC_GB_GB_H

#include "../types.h"
#include "hardware.h"

/* --- Limits (mirror Game Boy hardware) ------------------------------------ */
#define GB2CPC_MAX_SPRITES 40 /* OAM holds 40 sprite slots */
#define GB2CPC_TILE_BYTES  16 /* one 8x8 Mode 1 tile        */

/* --- Joypad bit masks (returned by joypad()) ------------------------------ */
#define J_RIGHT  0x01
#define J_LEFT   0x02
#define J_UP     0x04
#define J_DOWN   0x08
#define J_A      0x10
#define J_B      0x20
#define J_SELECT 0x40
#define J_START  0x80

/* --- Sprite attribute flags (set_sprite_prop) ----------------------------- */
#define S_PALETTE   0x10
#define S_FLIPX     0x20
#define S_FLIPY     0x40
#define S_PRIORITY  0x80

/* --- Display control macros (GBDK names) ---------------------------------- */
#define DISPLAY_ON     (rLCDC |= LCDCF_ON)
#define DISPLAY_OFF    (rLCDC &= (UINT8)~LCDCF_ON)
#define SHOW_SPRITES   (rLCDC |= LCDCF_OBJON)
#define HIDE_SPRITES   (rLCDC &= (UINT8)~LCDCF_OBJON)
#define SHOW_BKG       (rLCDC |= LCDCF_BGON)
#define HIDE_BKG       (rLCDC &= (UINT8)~LCDCF_BGON)

/* --- System / lifecycle --------------------------------------------------- */

/* Initialise the GB2CPC runtime: CPC Mode 1, double buffer, clear OAM.
 * Not part of GBDK; call once at the top of main(). */
void gb2cpc_init(void);

/* Block until the next vertical blank, committing all pending sprite/background
 * changes to the screen (erase-and-redraw of the software sprites). This is the
 * GB2CPC render tick; call it once per frame. */
void wait_vbl_done(void);
#define vsync() wait_vbl_done()

/* Busy-wait roughly ``n`` milliseconds (frame-based approximation). */
void delay(UINT16 n);

/* --- Tile data ------------------------------------------------------------ */

/* Load ``nb_tiles`` background tiles starting at index ``first_tile`` from
 * CPC Mode 1 tile data produced by the asset pipeline. */
void set_bkg_data(UINT8 first_tile, UINT8 nb_tiles, const UINT8 *data);

/* Load ``nb_tiles`` sprite tiles starting at ``first_tile``. */
void set_sprite_data(UINT8 first_tile, UINT8 nb_tiles, const UINT8 *data);

/* Set the active hardware palette (4 CPC ink numbers, e.g. from the pipeline's
 * *_palette array). Not in GBDK; GB2CPC needs an explicit palette on the CPC. */
void gb2cpc_set_palette(const UINT8 inks[4]);

/* --- Background map ------------------------------------------------------- */

/* Place a ``w`` x ``h`` block of tile indices into the background map at
 * (x, y) in tile coordinates. */
void set_bkg_tiles(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const UINT8 *tiles);

/* --- Sprites (virtual OAM) ------------------------------------------------ */

/* Assign tile ``tile`` to sprite slot ``nb``. */
void set_sprite_tile(UINT8 nb, UINT8 tile);

/* Set attribute flags (S_FLIPX / S_FLIPY / S_PALETTE / ...) for slot ``nb``. */
void set_sprite_prop(UINT8 nb, UINT8 prop);

/* Move sprite slot ``nb`` to Game Boy OAM coordinates (x, y). */
void move_sprite(UINT8 nb, UINT8 x, UINT8 y);

/* Hide sprite slot ``nb`` (Game Boy idiom: move off-screen). */
#define hide_sprite(nb) move_sprite((nb), 0, 0)

/* --- Input ---------------------------------------------------------------- */

/* Read the joypad: returns a bitfield of the J_* masks for keys held this
 * frame. Maps the CPC keyboard (arrows + Z/X/Enter/Space) onto GB buttons. */
UINT8 joypad(void);

#endif /* GB2CPC_GB_GB_H */
