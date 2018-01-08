#ifndef __NGIFLIBSDL_H_
#define __NGIFLIBSDL_H_

#include <SDL/SDL.h>

SDL_Surface *SDL_LoadGIF(const char *file);

struct ngiflibSDL_image {
		int delay_time;
		SDL_Surface * surface;
};

struct ngiflibSDL_animation {
	int image_count;
	struct ngiflibSDL_image images[0];
};

struct ngiflibSDL_animation * SDL_LoadAnimatedGif(const char * filename);

#endif
