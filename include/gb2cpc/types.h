/* GB2CPC -- GBDK-compatible integer type aliases.
 *
 * Decompiled Game Boy / GBDK code uses these typedefs pervasively. We provide
 * them here so source files compile unchanged against SDCC for the Z80.
 */
#ifndef GB2CPC_TYPES_H
#define GB2CPC_TYPES_H

typedef unsigned char  UINT8;
typedef signed char    INT8;
typedef unsigned int   UINT16;
typedef signed int     INT16;

typedef unsigned char  UBYTE;
typedef signed char    BYTE;
typedef unsigned int   UWORD;
typedef signed int     WORD;

typedef UINT8          BOOLEAN;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* GB2CPC_TYPES_H */
