#ifndef __NGIFLIB_H_
#define __NGIFLIB_H_

/* Types */
#if __VBCC__
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#else
#include <inttypes.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

/* LIBC functions :
 */
#include <stdlib.h>
#define ngiflib_malloc malloc
#define ngiflib_free free
#include <string.h>
#define ngiflib_memcpy memcpy
#define ngiflib_memset memset

#ifndef NGIFLIB_NO_FILE
#include <stdio.h>
#endif /* NGIFLIB_NO_FILE */

/* Modes pour struct ngiflib_gif.mode
 */
#define NGIFLIB_MODE_TRUE_COLOR	(0x00)
#define NGIFLIB_MODE_INDEXED	(0x01)
#define NGIFLIB_MODE_FROM_FILE	(0x00)
#define NGIFLIB_MODE_FROM_MEM	(0x02)

/* palette element */
struct ngiflib_rgb {
	u8 r, g ,b;
};
u32 GifIndexToTrueColor(struct ngiflib_rgb * palette, u8 v);

union ngiflib_pixpointer {
	u8 * p8;
#ifndef NGIFLIB_INDEXED_ONLY
	u32 * p32;
#endif /* NGIFLIB_INDEXED_ONLY */
};

#ifdef NGIFLIB_ENABLE_CALLBACKS
struct ngiflib_gif;

typedef void (*ngiflib_palette_cb)(struct ngiflib_gif *, struct ngiflib_rgb *, int);
typedef void (*ngiflib_line_cb)(struct ngiflib_gif *, union ngiflib_pixpointer, int);
#endif /* NGIFLIB_ENABLE_CALLBACKS */

/* struct stoquant les parametres relatifs a une image. */
struct ngiflib_img {
	struct ngiflib_img * next;
	struct ngiflib_gif * parent;
	struct ngiflib_rgb * palette;
	u16 ncolors;
	u16 width;
	u16 height;
	u16 posX;
	u16 posY;
	u8 interlaced;
	u8 sort_flag;		/* is local palette sorted by color frequency ? */
	u8 localpalbits;	/* bits/pixel ! (de 1 a 8) */
	u8 imgbits;	/* bits mini du code LZW (de 2 a 9 ?) */
};

/* utilises juste pour la decompression */
struct ngiflib_decode_context {
	union ngiflib_pixpointer frbuff_p;	/* current offset in frame buffer */
#ifdef NGIFLIB_ENABLE_CALLBACKS
	union ngiflib_pixpointer line_p;	/* start of line pointer */
#endif /* NGIFLIB_ENABLE_CALLBACKS */
	const u8 * srcbyte;
	u16 Xtogo;
	u16 curY;
	u16 lbyte;
	u16 max;	/* maximum value = (1 << nbbit) - 1 */
	u8 pass;	/* current pass of interlaced image decoding */
	u8 restbyte;
	u8 nbbit;	/* bits courants du code LZW */
	u8 restbits;
	u8 byte_buffer[256];	/* for get word */
};

void GifImgDestroy(struct ngiflib_img * i);
/* Fonction de debug */
#ifdef DEBUG
void fprintf_ngiflib_img(FILE * f, struct ngiflib_img * i);
#endif /* DEBUG */

/* disp_method :
            iv) Disposal Method - Indicates the way in which the graphic is to
            be treated after being displayed.

            Values :    0 -   No disposal specified. The decoder is
                              not required to take any action.
                        1 -   Do not dispose. The graphic is to be left
                              in place.
                        2 -   Restore to background color. The area used by the
                              graphic must be restored to the background color.
                        3 -   Restore to previous. The decoder is required to
                              restore the area overwritten by the graphic with
                              what was there prior to rendering the graphic.
                     4-7 -    To be defined.
*/
/* struct stoquant les parametres relatifs a un fichier gif
 * fichier qui peut contenir plusieurs images !
 * NE PAS oublier d'initialiser tous ses champs a ZERO
 *       (avec memset par exemple)
 */
struct ngiflib_gif {
	struct ngiflib_img * first_img;
	struct ngiflib_img * cur_img;
	struct ngiflib_rgb * palette;
	union {
#ifndef NGIFLIB_NO_FILE
		FILE * file;
#endif /* NGIFLIB_NO_FILE */
		const u8 * bytes;
	} input;	/* used by GetByte */
	union ngiflib_pixpointer frbuff;	/* frame buffer    */
#ifndef NGIFLIB_NO_FILE
	FILE * log;		/* to output log   */
#endif /* NGIFLIB_NO_FILE */
#ifdef NGIFLIB_ENABLE_CALLBACKS
	ngiflib_palette_cb palette_cb;
	ngiflib_line_cb line_cb;
	/* void * priv; */
#endif /* NGIFLIB_ENABLE_CALLBACKS */
	int nimg;
	u16 ncolors;
	u16 width;
	u16 height;
	u16 delay_time;
	u8 backgroundindex;
	u8 pixaspectratio;	/* width/height = (pixaspectratio + 15) / 64 */
	u8 disp_method;
	u8 userinputflag;
	u8 transparent_color;
	u8 transparent_flag;
	u8 imgbits;
	u8 colorresolution;
	u8 sort_flag;
	u8 mode; /* voir avant */
	u8 signature[7];	/* 0 terminated  !=) */
};

void GifDestroy(struct ngiflib_gif * g);
/* Fonction de debug */
#ifdef DEBUG
void fprintf_ngiflib_gif(FILE * f, struct ngiflib_gif * g);
#endif /* DEBUG */

int CheckGif(u8 * b);

/* ------------------------------------------------ 
 * int LoadGif(struct ngiflib_gif *);
 * s'assurer que nimg=0 au depart !
 * retourne : 
 *    0 si GIF terminé
 *    un nombre negatif si ERREUR
 *    1 si image Decodée
 * rappeler pour decoder les images suivantes
 * ------------------------------------------------ */
int LoadGif(struct ngiflib_gif * g);

#endif
