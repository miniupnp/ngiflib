#ifndef __NGIFLIBSDL_H_
#define __NGIFLIBSDL_H_

#include "SDL.h"
#include "ngiflib.h"

/* LIBC functions :
 */
#include <string.h>
#define ngiflib_memcpy memcpy
#define ngiflib_memset memset

SDL_Surface *SDL_LoadGIF(const char *file);

#endif

