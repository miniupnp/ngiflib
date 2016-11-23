#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngiflib.h"

FILE * fgif;
FILE * ftga;

// ====================================== MAIN ===============================
int main(int argc, char * * argv) {
	struct ngiflib_gif * gif;
	struct ngiflib_img * img;
	int err, i;
	char tganame[256];
	const char * input_file = NULL;
	int indexed = 0;
	const char * output_dir = ".";
	
	if(argc<2) {
		printf("Usage: machin [--indexed|-i] [--outdir dir] truc.gif\n");
		return 1;
	}
	for(i=1; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "--indexed") == 0
			   || strcmp(argv[i], "-i") == 0) {
				indexed = 1;
			} else if(strcmp(argv[i], "--outdir") == 0) {
				if(++i >= argc) {
					fprintf(stderr, "option %s need one argument.\n", argv[i - 1]);
					return 2;
				}
				output_dir = argv[i];
			} else {
				fprintf(stderr, "%s: unknown option.\n", argv[i]);
				return 2;
			}
		} else if(input_file == NULL) {
			input_file = argv[i];
		} else {
			fprintf(stderr, "%s: invalid argument.\n", argv[i]);
			return 2;
		}
	}

	gif = (struct ngiflib_gif *)malloc(sizeof(struct ngiflib_gif));
	memset(gif, 0, sizeof(struct ngiflib_gif));

	fgif = fopen(input_file, "rb");
	if(fgif==NULL) {
		printf("Cannot open %s\n",argv[1]);
		return 3;
	}
	gif->input = (void *)fgif;
	gif->mode = NGIFLIB_MODE_FROM_FILE;
	if(indexed) {
		gif->mode |= NGIFLIB_MODE_INDEXED;
		printf("INDEXED MODE\n");
	} else gif->mode |= NGIFLIB_MODE_TRUE_COLOR;
	
	/* MAIN LOOP */
	do {
	  err = LoadGif(gif);	// en retour 0=fini, 1=une image decodee, -1=ERREUR
	
	  if(err==1) {
	  	int localpalsize;
		char * p;
		img = gif->cur_img;
		localpalsize = 1 << img->localpalbits;
		snprintf(tganame, sizeof(tganame), "%s/%s", output_dir, input_file);
		p = strrchr(tganame, '.');
		if(p == NULL) {
			p = tganame + strlen(tganame);
		}
		sprintf(p, "_out%02d.tga", gif->nimg);
		ftga = fopen(tganame, "wb");
		if(ftga == NULL) {
			fprintf(stderr, "cannot open file %s for writing.\n", tganame);
			return 5;
		}
		putc(0, ftga);	//0
		if(gif->mode & NGIFLIB_MODE_INDEXED) {
			putc(1, ftga);// With palette
			putc(1, ftga);// With palette
			putc(0, ftga);// Color Map Origin.
			putc(0, ftga);//  "
			putc(localpalsize & 255, ftga);// Color Map Length.
			putc(localpalsize >> 8, ftga);//  "
			putc(24, ftga);// Color Map Entry Size.
			for(i=0; i<4; i++) putc(0, ftga);
		} else {
			putc(0, ftga);
			putc(2, ftga);// Truecolor
			for(i=0; i<9; i++) putc(0, ftga);
		}
		putc(gif->width & 255, ftga);
		putc(gif->width >> 8, ftga);
		putc(gif->height & 255, ftga);
		putc(gif->height >>8, ftga);
		if(gif->mode & NGIFLIB_MODE_INDEXED) {
			putc(8, ftga);	// bits per pixel
			putc(32, ftga);	// top down
			for(i=0; i<localpalsize; i++) {
				putc(img->palette[i].b, ftga);
				putc(img->palette[i].g, ftga);
				putc(img->palette[i].r, ftga);
			}
		} else {
			putc(32, ftga);	//16	// bits per pixel
			putc(32+8, ftga); // top down
		}
		if(img->interlaced) {
			u8 * p = GifUninterlace(gif);
			fwrite(p, (gif->mode & NGIFLIB_MODE_INDEXED)?1:4,
			       (size_t)gif->width * (size_t)gif->height, ftga);
			free(p);
		} else {
			fwrite(gif->frbuff, (gif->mode & NGIFLIB_MODE_INDEXED)?1:4,
			      (size_t)gif->width * (size_t)gif->height, ftga);
		}
		fclose(ftga);
		printf("%s written\n",tganame);
	  }

	} while(err==1);

	fclose(fgif);
	
	fprintf_ngiflib_gif(stdout, gif);
	GifDestroy(gif);	// libere la ram
	
	return 0;
}
