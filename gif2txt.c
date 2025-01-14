#include <stdio.h>

#include "ngiflib.h"

/* #define ANSI_COLOR_LUT */

enum ansi_color_mode {
	BASIC, /* 8 colors */
	VGACOLORS, /* 216 colors */
	TRUECOLORS /* 24 bits colors */
};

/**
 * Output an unicode character in utf-8 encoding
 */
int fputc_utf8(int c, FILE *stream) {
	if (c < 0x80) {
		return fputc(c, stream);
	}
	if (c < 0x800) {
		if (fputc(0xc0 | (c >> 6), stream) == EOF)
			return EOF;
		if (fputc(0x80 | (c & 0x7f), stream) == EOF)
			return EOF;
	} else if (c < 0x10000) {
		if (fputc(0xe0 | (c >> 12), stream) == EOF)
			return EOF;
		if (fputc(0x80 | ((c >> 6) & 0x3f), stream) == EOF)
			return EOF;
		if (fputc(0x80 | (c & 0x3f), stream) == EOF)
			return EOF;
	} else {
		if (fputc(0xf0 | (c >> 18), stream) == EOF)
			return EOF;
		if (fputc(0x80 | ((c >> 12) & 0x3f), stream) == EOF)
			return EOF;
		if (fputc(0x80 | ((c >> 6) & 0x3f), stream) == EOF)
			return EOF;
		if (fputc(0x80 | (c & 0x3f), stream) == EOF)
			return EOF;
	}
	return c;
}

/**
 * Convert from RGB 24 bits to 16 ANSI colors.
 * https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit
 */
unsigned int rgb24toansi(u32 c) {
#ifdef ANSI_COLOR_LUT
	unsigned int r = (((c >> 16) & 0xff) * 3 + 1) >> 8;
	unsigned int g = (((c >> 8) & 0xff) * 3 + 1) >> 8;
	unsigned int b = ((c & 0xff) * 3 + 1) >> 8;
	static const unsigned char ansicolors[] = {
		/*  0 - G = 0, B = 0 */
		0 /* Black */, 1 /* Red */, 61 /* Bright Red */,
		/*  3 - G = 1, B = 0 */
		2 /* Green */, 3 /* Yellow */, 61 /* Bright Red */,
		/*  6 - G = 2, B = 0 */
		62 /* Bright Green */, 62 /* Bright Green */, 63 /* Bright Yellow */,
		/*  9 - G = 0, B = 1 */
		4 /* Blue */, 5 /* Magenta */, 61 /* Bright Red */,
		/* 12 - G = 1, B = 1 */
		6 /* Cyan */, 7 /* White */, 61 /* Bright Red */,
		/* 15 - G = 2, B = 1 */
		62 /* Bright Green */, 62 /* Bright Green */, 63 /* Bright Yellow */,
		/* 18 - G = 0, B = 2 */
		64 /* Bright Blue */, 64 /* Bright Blue */, 65 /* Bright Magenta */,
		/* 21 - G = 1, B = 2 */
		64 /* Bright Blue */, 64 /* Bright Blue */, 65 /* Bright Magenta */,
		/* 24 - G = 2, B = 2 */
		66 /* Bright Cyan */, 66 /* Bright Cyan */, 67 /* Bright White */
	};
	return (unsigned int)ansicolors[r + g * 3 + b * 9];
#else
	static const u32 xterm_cols[] = {
		0x000000, /*  0 - Black */
		0xcd0000, /*  1 - Red */
		0x00cd00, /*  2 - Green */
		0xcdcd00, /*  3 - Yellow */
		0x0000ee, /*  4 - Blue */
		0xcd00cd, /*  5 - Magenta */
		0x00cdcd, /*  6 - Cyan */
		0xe5e5e5, /*  7 - White */
		0x7f7f7f, /* 60 - Gray */
		0xff0000, /* 61 - Bright Red */
		0x00ff00, /* 62 - Bright Green */
		0xffff00, /* 63 - Bright Yellow */
		0x5c5cff, /* 64 - Bright Blue */
		0xff00ff, /* 65 - Bright Magenta */
		0x00ffff, /* 66 - Bright Cyan */
		0xffffff /* 67 - Bright White */
	};
	int i, best = 0;
	u32 mindist = 0xffffffff;
	int r, g, b;
	b = c & 0xff;
	c >>= 8;
	g = c & 0xff;
	c >>= 8;
	r = c & 0xff;
	for (i = 0; i < (int)(sizeof(xterm_cols)/sizeof(u32)); i++) {
		int dr, dg, db;
		u32 dist;	/* squared */
		dr = (int)(xterm_cols[i] >> 16) - r;
		dg = (int)((xterm_cols[i] >> 8) & 0xff) - g;
		db = (int)(xterm_cols[i] & 0xff) - b;
		dist = dr * dr + dg * dg + db * db;
		if (dist < mindist) {
			mindist = dist;
			best = i;
		}
	}
	return (unsigned)((best < 8) ? best : best + 52);
#endif
}

/*
 * Convert RGB 24 bits to 6x6x6 VGA color cube (16-231)
 * and VGA grayscale (232-255)
 * https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
 */
unsigned int rgb24tovga216(u32 c) {
#if VGA_NOGRAYSCALE
	unsigned int r = (((c >> 16) & 0xff) * 6 + 3) >> 8;
	unsigned int g = (((c >> 8) & 0xff) * 6 + 3) >> 8;
	unsigned int b = ((c & 0xff) * 6 + 3) >> 8;
#else
	unsigned int r = (((c >> 16) & 0xff) * 24 + 12) >> 8;
	unsigned int g = (((c >> 8) & 0xff) * 24 + 12) >> 8;
	unsigned int b = ((c & 0xff) * 24 + 12) >> 8;
	if ((((r ^ g) | (g ^ b)) & ~3) == 0)
		return 232 + r; /* 24 grayscale colors */
	r >>= 2;
	g >>= 2;
	b >>= 2;
#endif
	return 16 + (r * 6 + g) * 6 + b;
}

/**
 * Set foreground color.
 * ANSI codes :
 * ESC[30m to ESC[37m    set foreground color (90 to 97 for bright color)
 * ESC[38;5;<n>m         set 8 bits foreground color
 * ESC[38;2;<r>,<g>,<b>m set 24 bits foreground color
 */
static u32 setfg(u32 fg, enum ansi_color_mode m) {
	switch (m) {
	case BASIC:
		printf("\033[%um", 30 + fg);
		break;
	case VGACOLORS:
		printf("\033[38;5;%um", fg);
		break;
	case TRUECOLORS:
		printf("\033[38;2;%u;%u;%um", (fg >> 16) & 0xff, (fg >> 8) & 0xff, fg & 0xff);
	}
	return fg;
}

/**
 * Set background color.
 * ANSI codes :
 * ESC[40m to ESC[47m    set background color (100 to 107 for bright color)
 * ESC[48;5;<n>m         set 8 bits background color
 * ESC[48;2;<r>,<g>,<b>m set 24 bits background color
 * see https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 */
static u32 setbg(u32 bg, enum ansi_color_mode m) {
	switch (m) {
	case BASIC:
		printf("\033[%um", 40 + bg);
		break;
	case VGACOLORS:
		printf("\033[48;5;%um", bg);
		break;
	case TRUECOLORS:
		printf("\033[48;2;%u;%u;%um", (bg >> 16) & 0xff, (bg >> 8) & 0xff, bg & 0xff);
	}
	return bg;
}

/**
 * Output text corresponding to two lines using ANSI escape sequences :
 * https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
 *
 * https://en.wikipedia.org/wiki/Block_Elements
 */
void twolines(const u32 *p, u16 width, int nosecondline, enum ansi_color_mode m) {
#define NOCOL 0xffffffff
	int x = (int)width;
	u32 fg = NOCOL;
	u32 bg = NOCOL;
	while (--x >= 0) {
		u32 upper, lower;
		switch (m) {
		case BASIC:
			upper = rgb24toansi(p[0]);
			lower = nosecondline ? NOCOL : rgb24toansi(p[width]);
			break;
		case VGACOLORS:
			upper = rgb24tovga216(p[0]);
			lower = nosecondline ? NOCOL : rgb24tovga216(p[width]);
			break;
		case TRUECOLORS:
			upper = p[0];
			lower = nosecondline ? NOCOL : p[width];
		}
		p++;
		if (upper == lower) {
			/* both pixels have same color */
			if (upper == fg) {
				fputc_utf8(0x2588, stdout); /* U+2588 Full block */
			} else if(upper == bg) {
				fputc(' ', stdout); /* U+0020 space */
			} else {
				/* Looking for the color that will be useful first, and change the other one */
				u32 tochange = fg;	/* default */
				int x2 = x;
				const u32* p2 = p;
				while(--x2 >= 0) {
					u32 upper2, lower2;
					switch (m) {
					case BASIC:
						upper2 = rgb24toansi(p2[0]);
						lower2 = nosecondline ? NOCOL : rgb24toansi(p2[width]);
						break;
					case VGACOLORS:
						upper2 = rgb24tovga216(p2[0]);
						lower2 = nosecondline ? NOCOL : rgb24tovga216(p2[width]);
						break;
					case TRUECOLORS:
						upper2 = p2[0];
						lower2 = nosecondline ? NOCOL : p2[width];
					}
					if (bg == upper2 || bg == lower2) {
						break;
					} else if (fg == upper2 || fg == lower2) {
						tochange = bg;
						break;
					}
					p2++;
				}
				if (tochange == fg) {
					fg = setfg(upper, m);
					fputc_utf8(0x2588, stdout); /* U+2588 Full block */
				} else {
					bg = setbg(upper, m);
					fputc(' ', stdout); /* U+0020 space */
				}
			}
		} else if (upper == bg || (lower == fg && lower != NOCOL)) {
			/* upper pixel is background, lower is foreground */
			if (lower != fg) {
				fg = setfg(lower, m);
			} else if (upper != bg) {
				bg = setbg(upper, m);
			}
			fputc_utf8(0x2584, stdout); /* U+2584 : lower half block */
		} else {
			/* upper pixel is foreground, lower is background */
			if (upper != fg) {
				fg = setfg(upper, m);
			}
			if (lower != bg) {
				bg = setbg(lower, m);
			}
			fputc_utf8(0x2580, stdout); /* U+2580 : upper half block */
		}
	}
	puts("\033[0m"); /* reset colors */
#undef NOCOL
}

static void usage(FILE * out, const char * argv0) {
	fprintf(out, "Usage: %s [--tc|--vga|--ansi] <file.gif>\n", argv0);
	fprintf(out, "       --tc     use 24 bits true colors\n");
	fprintf(out, "       --vga    use vga colors\n");
	fprintf(out, "       --ansi   use the standard 16 \"ANSI\" colors\n");
}

int main(int argc, char **argv) {
	struct ngiflib_gif *gif;
	FILE *fgif;
	const char *input_file = NULL;
	enum ansi_color_mode mode = BASIC;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			usage(stdout, argv[0]);
			return 0;
		} else if (i == argc - 1) {
			/* last argument => filename */
			input_file = argv[i];
		} else if(strcmp(argv[i], "--tc") == 0) {
			mode = TRUECOLORS;
		} else if(strcmp(argv[i], "--vga") == 0) {
			mode = VGACOLORS;
		} else if(strcmp(argv[i], "--ansi") == 0) {
			mode = BASIC;
		} else {
			fprintf(stderr, "Unrecognized argument : \"%s\"\n", argv[i]);
			usage(stderr, argv[0]);
			return 1;
		}
	}

	if (input_file == NULL) {
		fprintf(stderr, "Missing argument\n");
		return 1;
	}

	gif = (struct ngiflib_gif *)ngiflib_malloc(sizeof(struct ngiflib_gif));
#ifdef EXTRA_MALLOC_CHECK
	if(gif == NULL) {
		printf("Cannot allocate %ld bytes of memory.\n", sizeof(struct ngiflib_gif));
		return 4;
	}
#endif /* EXTRA_MALLOC_CHECK */
	ngiflib_memset(gif, 0, sizeof(struct ngiflib_gif));

	fgif = fopen(input_file, "rb");
	if (fgif == NULL) {
		fprintf(stderr, "Cannot open file \"%s\"\n", input_file);
		return 1;
	}
#ifdef NGIFLIB_NO_FILE
	/* TODO */
#else /* NGIFLIB_NO_FILE */
	gif->input.file = fgif;
	gif->mode = NGIFLIB_MODE_FROM_FILE;
#endif /* NGIFLIB_NO_FILE */
	gif->mode |= NGIFLIB_MODE_TRUE_COLOR;
#if defined(DEBUG)
	gif->log = stderr;
#endif

	while (LoadGif(gif) == 1) {
		int y;

		for (y = 0; y < gif->height; y += 2) {
			twolines(gif->frbuff.p32 + (y * gif->width), gif->width, y == gif->height - 1, mode);
		}
	}
	fclose(fgif);
	GifDestroy(gif);
	return 0;
}
