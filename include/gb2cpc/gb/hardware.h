/* GB2CPC -- Game Boy hardware register simulation.
 *
 * On real hardware these are memory-mapped I/O registers at 0xFF00..0xFF7F.
 * On the Amstrad CPC there is no such hardware, so GB2CPC backs them with a
 * plain 128-byte RAM shadow (_gb2cpc_io[]). Decompiled code that pokes
 * rLCDC/rSTAT/rSCX/etc. therefore still compiles and runs; the GB2CPC runtime
 * reads this shadow each frame and reflects meaningful changes (display on/off,
 * scroll, palette) onto the CPC screen.
 *
 * Register names and bit flags follow GBDK's <gb/hardware.h> so that existing
 * sources need no edits.
 */
#ifndef GB2CPC_GB_HARDWARE_H
#define GB2CPC_GB_HARDWARE_H

#include "../types.h"

/* 0xFF00..0xFF7F I/O shadow. Indexed by (address & 0x7F). */
extern volatile UINT8 _gb2cpc_io[128];

#define _GBIO(addr) (_gb2cpc_io[(addr) & 0x7F])

/* --- Core registers -------------------------------------------------------- */
#define rP1   _GBIO(0x00) /* Joypad             */
#define rDIV  _GBIO(0x04) /* Divider            */
#define rTIMA _GBIO(0x05) /* Timer counter      */
#define rTMA  _GBIO(0x06) /* Timer modulo       */
#define rTAC  _GBIO(0x07) /* Timer control      */
#define rIF   _GBIO(0x0F) /* Interrupt flag     */

/* --- LCD / PPU ------------------------------------------------------------- */
#define rLCDC _GBIO(0x40) /* LCD control        */
#define rSTAT _GBIO(0x41) /* LCD status         */
#define rSCY  _GBIO(0x42) /* Scroll Y           */
#define rSCX  _GBIO(0x43) /* Scroll X           */
#define rLY   _GBIO(0x44) /* LCDC Y-coord (RO)  */
#define rLYC  _GBIO(0x45) /* LY compare         */
#define rDMA  _GBIO(0x46) /* OAM DMA            */
#define rBGP  _GBIO(0x47) /* BG palette         */
#define rOBP0 _GBIO(0x48) /* Object palette 0   */
#define rOBP1 _GBIO(0x49) /* Object palette 1   */
#define rWY   _GBIO(0x4A) /* Window Y           */
#define rWX   _GBIO(0x4B) /* Window X           */
#define rIE   _GBIO(0xFF) /* Interrupt enable   */

/* --- LCDC bit flags (rLCDC) ----------------------------------------------- */
#define LCDCF_OFF     0x00 /* LCD off            */
#define LCDCF_ON      0x80 /* LCD on             */
#define LCDCF_WINOFF  0x00
#define LCDCF_WINON   0x20
#define LCDCF_BG8800  0x00
#define LCDCF_BG8000  0x10
#define LCDCF_OBJ8    0x00 /* 8x8 sprites        */
#define LCDCF_OBJ16   0x04 /* 8x16 sprites       */
#define LCDCF_OBJOFF  0x00
#define LCDCF_OBJON   0x02
#define LCDCF_BGOFF   0x00
#define LCDCF_BGON    0x01

/* --- STAT bit flags (rSTAT) ----------------------------------------------- */
#define STATF_LYC     0x40
#define STATF_MODE10  0x20
#define STATF_MODE01  0x10
#define STATF_MODE00  0x08
#define STATF_LYCF    0x04
#define STATF_VBL     0x01 /* mode 1 - vertical blank */
#define STATF_OAM     0x02 /* mode 2 - OAM scan       */

#endif /* GB2CPC_GB_HARDWARE_H */
