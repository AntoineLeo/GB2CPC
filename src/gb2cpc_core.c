/* GB2CPC -- runtime compatibility core.
 *
 * Implements the GBDK API declared in <gb2cpc/gb/gb.h> on top of CPCtelera.
 *
 * Rendering model (Early Alpha)
 * -----------------------------
 * The Game Boy is a tile/sprite machine; the CPC is a bitmap. GB2CPC keeps a
 * virtual OAM (40 sprite slots) and a virtual background map. Each frame,
 * wait_vbl_done() redraws the visible scene into the *inactive* video page and
 * flips it on VSYNC, giving flicker-free double-buffered output.
 *
 * For clarity and correctness this alpha performs a full background+sprite
 * redraw every frame. The dirty-rectangle "erase-and-redraw only what moved"
 * optimisation called out in the README is the next milestone (see TODO marks).
 *
 * NOTE: This file targets SDCC + CPCtelera. It is written to the documented
 * CPCtelera API but has not been compiled in this environment (no toolchain
 * present); treat the first on-hardware build as the integration checkpoint.
 */
#include <cpctelera.h>
#include <string.h>

#include <gb2cpc/gb/gb.h>

/* ------------------------------------------------------------------------- */
/* Hardware register shadow (see hardware.h).                                */
/* ------------------------------------------------------------------------- */
volatile UINT8 _gb2cpc_io[128];

/* ------------------------------------------------------------------------- */
/* Configuration                                                             */
/* ------------------------------------------------------------------------- */
#define SCR_BYTES_W      80          /* CPC Mode 1: 320 px / 4 px-per-byte    */
#define SCR_H            200
#define TILE_W_BYTES     2           /* 8 px wide @ 4 px/byte                  */
#define TILE_H           8
#define MAX_TILES        96          /* tile bank size (sprite+bkg shared)    */

/* GB 160x144 viewport, centred in the 320x200 CPC screen. */
#define VIEW_OFF_X_BYTES ((SCR_BYTES_W - (160 / 4)) / 2)  /* (80-40)/2 = 20   */
#define VIEW_OFF_Y       ((SCR_H - 144) / 2)              /* (200-144)/2 = 28 */

/* Double buffer pages. Back buffer at 0x8000 requires code/data to stay below
 * it (enforced by the project link config). Front buffer is the CPC default. */
#define VMEM_FRONT       (u8 *)0xC000
#define VMEM_BACK        (u8 *)0x8000

/* ------------------------------------------------------------------------- */
/* State                                                                     */
/* ------------------------------------------------------------------------- */
static u8 *g_backbuffer;   /* page currently being drawn (off-screen)         */
static u8  g_palette[4] = { 11, 6, 26, 12 };  /* default DMG->CPC mapping     */

/* Tile bank: CPC Mode 1 bytes, 16 per tile. */
static u8 g_tiles[MAX_TILES * GB2CPC_TILE_BYTES];

/* Virtual OAM. */
static u8 g_spr_x[GB2CPC_MAX_SPRITES];
static u8 g_spr_y[GB2CPC_MAX_SPRITES];
static u8 g_spr_tile[GB2CPC_MAX_SPRITES];
static u8 g_spr_prop[GB2CPC_MAX_SPRITES];

/* Virtual background map (20x18 visible tiles, like the GB viewport). */
#define BKG_W 20
#define BKG_H 18
static u8 g_bkg[BKG_W * BKG_H];

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */
static u8 *tile_ptr(u8 idx) {
   return &g_tiles[(u16)idx * GB2CPC_TILE_BYTES];
}

/* Draw one 8x8 tile to the back buffer at pixel (px, py). Coordinates are in
 * CPC screen space (px is a *pixel* x; converted to a byte column here). */
static void draw_tile_at(u8 idx, u8 px, u8 py) {
   u8 *mem = cpct_getScreenPtr(g_backbuffer, (u8)(px >> 2), py);
   cpct_drawSprite(tile_ptr(idx), mem, TILE_W_BYTES, TILE_H);
}

/* ------------------------------------------------------------------------- */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------- */
void gb2cpc_init(void) {
   cpct_disableFirmware();
   cpct_setVideoMode(1);

   memset(g_tiles, 0, sizeof(g_tiles));
   memset(g_bkg, 0, sizeof(g_bkg));
   memset(g_spr_x, 0, sizeof(g_spr_x));
   memset(g_spr_y, 0, sizeof(g_spr_y));
   memset(g_spr_tile, 0, sizeof(g_spr_tile));
   memset(g_spr_prop, 0, sizeof(g_spr_prop));

   gb2cpc_set_palette(g_palette);

   /* Clear both video pages so the first flip shows a clean screen. */
   cpct_memset(VMEM_FRONT, 0, 0x4000);
   cpct_memset(VMEM_BACK, 0, 0x4000);
   g_backbuffer = VMEM_BACK;

   rLCDC = LCDCF_ON;  /* mark display enabled in the GB shadow */
}

void gb2cpc_set_palette(const UINT8 inks[4]) {
   u8 i;
   for (i = 0; i < 4; i++) g_palette[i] = inks[i];
   /* CPC Mode 1 uses 4 pens; set pens 0..3 from the GB shade mapping. */
   cpct_setPalette((u8 *)g_palette, 4);
   cpct_setBorder(g_palette[3]);
}

void delay(UINT16 n) {
   /* ~1 frame per 20 ms at 50 Hz; good enough for a busy-wait approximation. */
   UINT16 frames = n / 20u;
   if (frames == 0) frames = 1;
   while (frames--) wait_vbl_done();
}

/* ------------------------------------------------------------------------- */
/* Tile / palette loading                                                    */
/* ------------------------------------------------------------------------- */
void set_bkg_data(UINT8 first_tile, UINT8 nb_tiles, const UINT8 *data) {
   u16 n = (u16)nb_tiles * GB2CPC_TILE_BYTES;
   if ((u16)first_tile >= MAX_TILES) return;
   if ((u16)first_tile * GB2CPC_TILE_BYTES + n > sizeof(g_tiles))
      n = sizeof(g_tiles) - (u16)first_tile * GB2CPC_TILE_BYTES;
   memcpy(tile_ptr(first_tile), data, n);
}

/* Sprite and background tiles share one bank in this alpha. */
void set_sprite_data(UINT8 first_tile, UINT8 nb_tiles, const UINT8 *data) {
   set_bkg_data(first_tile, nb_tiles, data);
}

/* ------------------------------------------------------------------------- */
/* Background map                                                            */
/* ------------------------------------------------------------------------- */
void set_bkg_tiles(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const UINT8 *tiles) {
   u8 row, col;
   for (row = 0; row < h; row++) {
      u8 ty = y + row;
      if (ty >= BKG_H) break;
      for (col = 0; col < w; col++) {
         u8 tx = x + col;
         if (tx >= BKG_W) continue;
         g_bkg[(u16)ty * BKG_W + tx] = tiles[(u16)row * w + col];
      }
   }
}

/* ------------------------------------------------------------------------- */
/* Sprites (virtual OAM)                                                     */
/* ------------------------------------------------------------------------- */
void set_sprite_tile(UINT8 nb, UINT8 tile) {
   if (nb < GB2CPC_MAX_SPRITES) g_spr_tile[nb] = tile;
}

void set_sprite_prop(UINT8 nb, UINT8 prop) {
   if (nb < GB2CPC_MAX_SPRITES) g_spr_prop[nb] = prop;
}

void move_sprite(UINT8 nb, UINT8 x, UINT8 y) {
   if (nb < GB2CPC_MAX_SPRITES) {
      g_spr_x[nb] = x;
      g_spr_y[nb] = y;
   }
}

/* ------------------------------------------------------------------------- */
/* Input -- CPC keyboard -> Game Boy joypad bits                             */
/* ------------------------------------------------------------------------- */
UINT8 joypad(void) {
   UINT8 keys = 0;
   cpct_scanKeyboard();
   if (cpct_isKeyPressed(Key_CursorUp))    keys |= J_UP;
   if (cpct_isKeyPressed(Key_CursorDown))  keys |= J_DOWN;
   if (cpct_isKeyPressed(Key_CursorLeft))  keys |= J_LEFT;
   if (cpct_isKeyPressed(Key_CursorRight)) keys |= J_RIGHT;
   if (cpct_isKeyPressed(Key_Z))           keys |= J_A;
   if (cpct_isKeyPressed(Key_X))           keys |= J_B;
   if (cpct_isKeyPressed(Key_Return))      keys |= J_START;
   if (cpct_isKeyPressed(Key_Space))       keys |= J_SELECT;
   rP1 = keys;  /* reflect into the GB register shadow */
   return keys;
}

/* ------------------------------------------------------------------------- */
/* Frame render + page flip                                                  */
/* ------------------------------------------------------------------------- */
static void render_background(void) {
   u8 row, col;
   for (row = 0; row < BKG_H; row++) {
      u8 py = VIEW_OFF_Y + row * TILE_H;
      for (col = 0; col < BKG_W; col++) {
         u8 t = g_bkg[(u16)row * BKG_W + col];
         /* Tile 0 == transparent/empty background; skip to save time. */
         if (t == 0) continue;
         draw_tile_at(t, (u8)((VIEW_OFF_X_BYTES + col * TILE_W_BYTES) << 2), py);
      }
   }
}

static void render_sprites(void) {
   u8 i;
   for (i = 0; i < GB2CPC_MAX_SPRITES; i++) {
      u8 gx = g_spr_x[i];
      u8 gy = g_spr_y[i];
      u8 px, py;
      if (gx == 0 && gy == 0) continue;            /* hidden (GB idiom)      */
      if (gx < 8 || gy < 16) continue;             /* fully off-screen left  */
      /* GB OAM coords: on-screen top-left = (x-8, y-16). */
      px = (u8)(VIEW_OFF_X_BYTES * 4 + (gx - 8));
      py = (u8)(VIEW_OFF_Y + (gy - 16));
      if (py >= VIEW_OFF_Y + 144) continue;
      draw_tile_at(g_spr_tile[i], px, py);
   }
}

void wait_vbl_done(void) {
   /* Display off: just sync, draw nothing. */
   if (!(rLCDC & LCDCF_ON)) {
      cpct_waitVSYNC();
      return;
   }

   /* 1. Clear the back buffer to pen 0 (background colour). */
   cpct_memset(g_backbuffer, 0, 0x4000);

   /* 2. Draw background tilemap, then sprites on top. */
   if (rLCDC & LCDCF_BGON)  render_background();
   if (rLCDC & LCDCF_OBJON) render_sprites();

   /* 3. Flip: wait for VSYNC then point the CRTC at the freshly drawn page. */
   cpct_waitVSYNC();
   cpct_setVideoMemory(g_backbuffer);

   /* 4. Swap buffers for the next frame. */
   g_backbuffer = (g_backbuffer == VMEM_BACK) ? VMEM_FRONT : VMEM_BACK;
}
