#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngiflib.h"

FILE * fgif;
FILE * ftga;

/* ==================================== MAIN =============================== */
int main(int argc, char * * argv) {
	struct ngiflib_gif * gif;
	struct ngiflib_img * img;
	int err, i;
	char tganame[256];
	const char * input_file = NULL;
	int indexed = 0;
	const char * outbase = NULL;
	FILE * log = NULL;
#ifdef NGIFLIB_NO_FILE
	long filesize;
	u8 * buffer;
#endif /* NGIFLIB_NO_FILE */

	if(argc<2) {
		printf("Usage: machin [--log logfile] [--indexed|-i] [--outbase path/file] truc.gif\n");
		return 1;
	}
	for(i=1; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "--indexed") == 0
			   || strcmp(argv[i], "-i") == 0) {
				indexed = 1;
			} else if(strcmp(argv[i], "--outbase") == 0) {
				if(++i >= argc) {
					fprintf(stderr, "option %s need one argument.\n", argv[i - 1]);
					return 2;
				}
				outbase = argv[i];
			} else if(strcmp(argv[i], "--log") == 0) {
				if(++i >= argc) {
					fprintf(stderr, "option %s need one argument.\n", argv[i - 1]);
					return 2;
				}
				log = fopen(argv[i], "w");
				if(log == NULL) fprintf(stderr, "failed to open log file %s\n", argv[i]);
				/*setvbuf(log, NULL, _IONBF, 0);*/
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
#ifndef NGIFLIB_NO_FILE
	gif->log = log;
#endif /* NGIFLIB_NO_FILE */

	fgif = fopen(input_file, "rb");
	if(fgif==NULL) {
		printf("Cannot open %s\n",argv[1]);
		return 3;
	}
#ifdef NGIFLIB_NO_FILE
	fseek(fgif, 0, SEEK_END);
	filesize = ftell(fgif);
	fseek(fgif, 0, SEEK_SET);
	buffer = malloc(filesize);
	if(buffer == NULL) {
		printf("Cannot allocate %ld bytes of memory.\n", filesize);
		fclose(fgif);
		return 4;
	}
	fread(buffer, 1, filesize, fgif);
	gif->input.bytes = buffer;
	gif->mode = NGIFLIB_MODE_FROM_MEM;
#else /* NGIFLIB_NO_FILE */
	gif->input.file = fgif;
	gif->mode = NGIFLIB_MODE_FROM_FILE;
#endif /* NGIFLIB_NO_FILE */
	if(indexed) {
		gif->mode |= NGIFLIB_MODE_INDEXED;
		printf("INDEXED MODE\n");
	} else gif->mode |= NGIFLIB_MODE_TRUE_COLOR;
	
	/* MAIN LOOP */
	do {
	  err = LoadGif(gif); /* en retour 0=fini, 1=une image decodee, -1=ERREUR */
	  printf("LoadGif() returned %d\n", err);
	
	  if(err==1) {
	  	int localpalsize;
		char * p;
		img = gif->cur_img;
		localpalsize = 1 << img->localpalbits;
		if(outbase) {
			p = tganame + snprintf(tganame, sizeof(tganame), "%s", outbase);
		} else {
			snprintf(tganame, sizeof(tganame), "%s", input_file);
			p = strrchr(tganame, '.');
			if(p == NULL) {
				p = tganame + strlen(tganame);
			}
		}
		sprintf(p, "_out%02d.tga", gif->nimg);
		ftga = fopen(tganame, "wb");
		if(ftga == NULL) {
			fprintf(stderr, "cannot open file %s for writing.\n", tganame);
			return 5;
		}
		putc(0, ftga);	/* 0 */
		if(gif->mode & NGIFLIB_MODE_INDEXED) {
			putc(1, ftga);/* With palette */
			putc(1, ftga);/* With palette */
			putc(0, ftga);/* Color Map Origin. */
			putc(0, ftga);/*  "                */
			putc(localpalsize & 255, ftga);/* Color Map Length. */
			putc(localpalsize >> 8, ftga);/*  "                 */
			putc(24, ftga);/* Color Map Entry Size. */
			for(i=0; i<4; i++) putc(0, ftga);
		} else {
			putc(0, ftga);
			putc(2, ftga);/* Truecolor */
			for(i=0; i<9; i++) putc(0, ftga);
		}
		putc(gif->width & 255, ftga);
		putc(gif->width >> 8, ftga);
		putc(gif->height & 255, ftga);
		putc(gif->height >>8, ftga);
		if(gif->mode & NGIFLIB_MODE_INDEXED) {
			putc(8, ftga);	/* bits per pixel */
			putc(32, ftga);	/* top down       */
			for(i=0; i<localpalsize; i++) {
#ifdef NGIFLIB_PALETTE_USE_BYTES
				putc(img->palette[i*3+2], ftga);
				putc(img->palette[i*3+1], ftga);
				putc(img->palette[i*3+0], ftga);
#else
				putc(img->palette[i].b, ftga);
				putc(img->palette[i].g, ftga);
				putc(img->palette[i].r, ftga);
#endif /* NGIFLIB_PALETTE_USE_BYTES */
			}
			if(img->interlaced) {
				unsigned int y, line;
				for(y = 0; y < gif->height; y++) {
					switch(y & 7) {
					case 0:	/* 1st pass */
						line = y >> 3;
						break;
					case 4:	/* 2nd pass */
						line = (gif->height >> 3) + (y >> 3);
						break;
					case 2:	/* 3rd pass */
					case 6:
						line = (gif->height >> 2) + (y >> 2);
						break;
					default:	/* 4th pass */
						line = (gif->height >> 1) + (y >> 1);
					}
					fwrite(gif->frbuff.p8 + line * gif->width, 1,
					       (size_t)gif->width, ftga);
				}
			} else {
				fwrite(gif->frbuff.p8, 1,
				      (size_t)gif->width * (size_t)gif->height, ftga);
			}
		} else {
			long l;
			u32 pixel;
			putc(32, ftga);	/* 16	/ bits per pixel */
			putc(32+8, ftga); /* top down */
			for(l = 0; l < (long)gif->width * (long)gif->height; l++) {
				pixel = gif->frbuff.p32[l];
				putc(pixel & 0xff, ftga);	/* B */
				putc((pixel >> 8) & 0xff, ftga);	/* G */
				putc((pixel >> 16) & 0xff, ftga);	/* R */
				putc(0xff, ftga);	/* A */
			}
		}
		fclose(ftga);
		printf("%s written\n",tganame);
	  }

	} while(err==1);

	fclose(fgif);
#ifdef NGIFLIB_NO_FILE
	free(buffer);
#endif /* NGIFLIB_NO_FILE */
	
#if DEBUG
	fprintf_ngiflib_gif(stdout, gif);
#endif /* DEBUG */
	GifDestroy(gif);	/* libere la ram */
	
	if(log) fclose(log);
	return 0;
}
