/* ******************************************
 * thomas BERNARD.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include "ngiflibSDL.h"

static int appactive = 1;

void manage_event() {
	SDL_Event event;
	SDL_WaitEvent(&event);
#if 0
	while(SDL_PollEvent(&event)) {
		printf("\\o/\n");
#endif
		switch(event.type) {
		case SDL_ACTIVEEVENT:
			if(event.active.state & SDL_APPACTIVE)
				appactive = event.active.gain;
			printf("Active Event : gain=%d state=%d   appactive=%d\n",
			       event.active.gain,
				   event.active.state,
				   appactive);
			break;
		case SDL_QUIT:
			printf("Event Quit !\n");
			exit(0);
		case SDL_KEYDOWN:
			printf("Touche %s _\n", SDL_GetKeyName(event.key.keysym.sym));
			if(event.key.keysym.sym == SDLK_ESCAPE) exit(0);
			break;
		case SDL_KEYUP:
			printf("Touche %s ^\n", SDL_GetKeyName(event.key.keysym.sym));
			break;
		case SDL_MOUSEBUTTONDOWN:
			printf("Bouton %d _\n", event.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			printf("Bouton %d ^\n", event.button.button);
			if(event.button.button == 1) exit(0);
			break;
		case SDL_MOUSEMOTION:
			break;
		default:
			printf("event.type = %d\n", event.type);
		}
#if 0
	}
#endif
}

void ShowGIF(char *file, SDL_Surface *screen, int x, int y)
{
    SDL_Surface *image;
    SDL_Rect dest;

    /* Charger une image GIF dans une surface*/
    image = SDL_LoadGIF(file);
    if ( image == NULL ) {
        fprintf(stderr, "Impossible de charger %s: %s\n", file, SDL_GetError());
        return;
    }

    /* Copie à l'écran.
	La surface ne doit pas être bloquée maintenant
     */
    dest.x = x;
    dest.y = y;
    dest.w = image->w;
    dest.h = image->h;
	printf("%d %d\n", image->w, image->h);
    SDL_BlitSurface(image, NULL, screen, &dest);

    /*Mise à jour de la portion qui a changé */
    SDL_UpdateRects(screen, 1, &dest);
	SDL_FreeSurface(image);
}

/* ======================================================================= */
int main(int argc, char* * argv) {
	const SDL_VideoInfo * vidinf;
	SDL_Rect ** modes;
	int bpp;
	SDL_Surface * screen;
	char * gifname;
	struct ngiflibSDL_animation * animation;
	int i;
	
	if(argc>1)
		gifname = argv[1];
	else
		gifname = "amigagry.gif";
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Erreur d'init de la SDL !\n");
		return 1;
	}
	atexit(SDL_Quit);
	SDL_WM_SetCaption("SDL gif viewer", "gif viewer"); /* window caption, icon caption */
	vidinf = SDL_GetVideoInfo();
	printf("bpp of the \"best\" video mode = %d \n", vidinf->vfmt->BitsPerPixel);
	modes = SDL_ListModes(NULL, SDL_HWSURFACE);
	if(modes == (SDL_Rect **)-1){
		printf("All resolutions available.\n");
	} else {
		/* Print valid modes */
		printf("Available Modes\n");
		for(i=0;modes[i];++i) {
			printf("  %d x %d\n", modes[i]->w, modes[i]->h);
		}
	}

	bpp = SDL_VideoModeOK(640, 480, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);

	if(!bpp){
		printf("Mode not available.\n");
		exit(-1);
	}
	printf("SDL Recommends 640x480@%dbpp.\n", bpp);
	screen = SDL_SetVideoMode(640, 480, bpp, SDL_HWSURFACE | SDL_DOUBLEBUF);
#if 0
	/* code to display single image : */
	ShowGIF(gifname, screen, 0,0);
	SDL_Flip(screen);

	for(;;) {
		manage_event();
	}
#endif

	animation = SDL_LoadAnimatedGif(gifname);
	printf("SDL_LoadAnimatedGif(%s) => %p\n", gifname, animation);
	if(animation != NULL) {
		printf("%d images\n", animation->image_count);
		for(;;)
		for(i = 0; i < animation->image_count; i++) {
			SDL_Event event;
			SDL_Rect dest;
			/* Copie à l'écran.
			   La surface ne doit pas être bloquée maintenant
			 */
			dest.x = 0;
			dest.y = 0;
			dest.w = animation->images[i].surface->w;
			dest.h = animation->images[i].surface->h;
			SDL_BlitSurface(animation->images[i].surface, NULL, screen, &dest);

			/*Mise à jour de la portion qui a changé */
			SDL_UpdateRects(screen, 1, &dest);
			SDL_Flip(screen);
			if(animation->image_count > 1) {
				if(animation->images[i].delay_time == 0) {
					SDL_Delay(100);	/* default delay of 1/10th of seconds */
				} else {
					SDL_Delay(10*animation->images[i].delay_time);
				}
				while(SDL_PollEvent(&event)) {
					if(event.type == SDL_QUIT) {
						printf("SDL_QUIT\n");
						exit(0);
					} else if(event.type == SDL_KEYDOWN) {
						printf("Touche %s _\n", SDL_GetKeyName(event.key.keysym.sym));
						if(event.key.keysym.sym == SDLK_ESCAPE) exit(0);
					}
				}
			} else {
				for(;;) {
					manage_event();
				}
			}
		}
	}
	return 0;
}

