#include <stdio.h>

#include "ngiflib.h"

/* decodeur GIF en C portable (pas de pb big/little endian
 * Thomas BERNARD. janvier 2004.
 */

/* Fonction de debug */
void fprintf_ngiflib_img(FILE * f, struct ngiflib_img * i) {
	fprintf(f, "  * ngiflib_img @ %p\n", i);
	fprintf(f, "    next = %p\n", i->next);
	fprintf(f, "    parent = %p\n", i->parent);
	fprintf(f, "    palette = %p\n", i->palette);
	fprintf(f, "    %3d couleurs", i->ncolors);
	if(i->interlaced) fprintf(f, " interlaced");
	fprintf(f, "\n    taille : %dx%d, pos (%d,%d)\n", i->width, i->height, i->posX, i->posY);
	fprintf(f, "    sort_flag=%x localpalbits=%d\n", i->sort_flag, i->localpalbits);
}

void GifImgDestroy(struct ngiflib_img * i) {
	if(i==NULL) return;
	if(i->next) GifImgDestroy(i->next);
	if(i->palette && (i->palette != i->parent->palette))
	  ngiflib_free(i->palette);
	ngiflib_free(i);
}

/* Fonction de debug */
void fprintf_ngiflib_gif(FILE * f, struct ngiflib_gif * g) {
	struct ngiflib_img * i;
	fprintf(f, "* ngiflib_gif @ %p %s\n", g, g->signature);
	fprintf(f, "  %dx%d, %d bits, %d couleurs\n", g->width, g->height, g->imgbits, g->ncolors);
	fprintf(f, "  palette = %p, backgroundcolorindex %d\n", g->palette, g->backgroundindex);
	fprintf(f, "  pixelaspectratio = %d\n", g->pixaspectratio);
	fprintf(f, "  frbuff = %p\n", g->frbuff);
	
	fprintf(f, "  cur_img = %p\n", g->cur_img);
	fprintf(f, "  %d images :\n", g->nimg);
	i = g->first_img;
	while(i) {
		fprintf_ngiflib_img(f, i);
		i = i->next;
	}
}

void GifDestroy(struct ngiflib_gif * g) {
	if(g==NULL) return;
	if(g->palette) ngiflib_free(g->palette);
	if(g->frbuff) ngiflib_free(g->frbuff);
	GifImgDestroy(g->first_img);
	ngiflib_free(g);
}

/* u8 GetByte(struct ngiflib_gif * g);
 * fonction qui renvoie un octet du fichier .gif
 * on pourait optimiser en faisant 2 fonctions.
 */
u8 GetByte(struct ngiflib_gif * g) {
	if(g->mode & NGIFLIB_MODE_FROM_MEM) {
		u8 b = *((u8 *)g->input);
		g->input = (u8 *)g->input + 1;
		return b;
	} else {
		return (u8)(getc((FILE *)g->input));
	}
}

/* u16 GetWord()
 * Renvoie un mot de 16bits
 * N'est pas influencee par l'endianess du CPU !
 */
u16 GetWord(struct ngiflib_gif * g) {
	u16 r = (u16)GetByte(g);
	r |= ((u16)GetByte(g) << 8);
	return r;
}

/* int GetByteStr(struct ngiflib_gif * g, u8 * p, int n);
 * prend en argument un pointeur sur la destination
 * et le nombre d'octet a lire.
 * Renvoie 0 si l'operation a reussi, -1 sinon.
 */
int GetByteStr(struct ngiflib_gif * g, u8 * p, int n) {
	if(!p) return -1;
	while(n-->0) {
		*p++ = GetByte(g);
	}
	return 0;
}

/* void WritePixel(struct ngiflib_img * i, u8 v);
 * ecrit le pixel de valeur v dans le frame buffer
 */
void WritePixel(struct ngiflib_img * i, u8 v) {
	struct ngiflib_gif * p = i->parent;
	if(v!=p->transparent_color || !p->transparent_flag) {
		if(p->mode & NGIFLIB_MODE_INDEXED)
			((u8 *)(p->frbuff))[i->curX + i->curY*p->width] = v;
		else
			p->frbuff[i->curX + i->curY*p->width] =
			   GifIndexToTrueColor(i->palette, v);
	}
	i->curX++;
	if(i->curX >= (i->posX+i->width)) {
		i->curX = i->posX;
		i->curY++;
	}
}

/*
 * u16 GetGifWord(struct ngiflib_img * i);
 * Renvoie un code LZW (taille variable)
 */
u16 GetGifWord(struct ngiflib_img * i) {
	u16 r;
	int bits_ok, bits_todo;
	
	r = (u16)i->lbyte;
	bits_ok = (int)i->restbits;
	bits_todo = (int)i->nbbit - bits_ok;
	if( bits_todo > 0 ) {
  	  u8 newbyte;
	  do {
		if(i->restbyte==0) i->restbyte = GetByte(i->parent);
		newbyte = GetByte(i->parent);
		i->restbyte--;
		r |= ((u16)newbyte << bits_ok);
		i->restbits = 8 - bits_todo;
		i->lbyte = newbyte >> bits_todo;
		bits_todo -= 8;
		bits_ok += 8;
	  } while(bits_todo > 0); 
	} else {
		i->restbits -= i->nbbit;
		i->lbyte >>= i->nbbit;
	}
	r &= ( (1 << i->nbbit) - 1 );	/* applique le bon masque
	                             	 * pour eliminer les bits en trop */
	return r; 
}

/* ------------------------------------------------ */
/* u32 * GifUninterlace(struct ngiflib_gif * g);
 * Retourne un pointeur sur l'image desentrelacee.
 * Ne pas oublier de liberer cette ram apres.
 */
u8 * GifUninterlace(struct ngiflib_gif * g) {
	int l, linesize;
	u8 * p, * src;
	src = (u8 *)g->frbuff;
	linesize = g->width;
	if((g->mode & NGIFLIB_MODE_INDEXED)==0) {
		linesize <<= 2;
	}
	p = (u8 *)ngiflib_malloc(linesize*g->height);
	/* 1st pass */
	for(l=0; l<g->height; l+=8) {
		ngiflib_memcpy(p + l*linesize, src, linesize);
		src += linesize;
	}
	/* 2nd pass */
	for(l=4; l<g->height; l+=8) {
		ngiflib_memcpy(p + l*linesize, src, linesize);
		src += linesize;
	}
	/* 3rd pass */
	for(l=2; l<g->height; l+=4) {
		ngiflib_memcpy(p + l*linesize, src, linesize);
		src += linesize;
	}
	/* 4th pass */
	for(l=1; l<g->height; l+=2) {
		ngiflib_memcpy(p + l*linesize, src, linesize);
		src += linesize;
	}
	return p;
}

/* ------------------------------------------------ */
void FillGifBackGround(struct ngiflib_gif * g) {
	long n = g->width*g->height;
	u32 bg_truecolor;
	if((g->frbuff==NULL)||(g->palette==NULL)) return;
	if(g->mode & NGIFLIB_MODE_INDEXED) {
		u8 * p = (u8 *)g->frbuff;
		while(n-->0) *p++ = g->backgroundindex;
	} else {
		u32 * p = g->frbuff;
		bg_truecolor = GifIndexToTrueColor(g->palette, g->backgroundindex);
		while(n-->0) *p++ = bg_truecolor;
	}
}

/* ------------------------------------------------ */
int CheckGif(u8 * b) {
	return (b[0]=='G')&&(b[1]=='I')&&(b[2]=='F')&&(b[3]=='8');
}

/* ------------------------------------------------ */
int DecodeGifImg(struct ngiflib_img * i) {
	long npix;
	int stackp;
	u16 clr;
	u16 eof;
	u16 free;
	u16 freesav;
	u16 max;
	u16 maxsav;
	u16 act_code = 0;
	u16 old_code = 0;
	u16 read_byt;
	u16 ab_prfx[4096];
	u8 ab_suffx[4096];
	u8 ab_stack[4096];
	u8 nbbitsav;
	u8 flags;
	u8 casspecial = 0;

	if(!i) return -1;

	i->posX = GetWord(i->parent);	// offsetX
	i->posY = GetWord(i->parent);	// offsetY
	i->width = GetWord(i->parent);	// SizeX
	i->height = GetWord(i->parent);	// SizeY

	i->curX = i->posX;
	i->curY = i->posY;

	npix = i->width * i->height;
	flags = GetByte(i->parent);
	i->interlaced = (flags & 64) >> 6;
	i->sort_flag = (flags & 32) >> 5;	// is local palette sorted by color frequency ?
	i->localpalbits = (flags & 7) + 1;
	if(flags&128) { // palette locale
		int k;
		int localpalsize = 1 << i->localpalbits;
		printf("Local palette\n");
		i->palette = (struct ngiflib_rgb *)ngiflib_malloc(sizeof(struct ngiflib_rgb)*localpalsize);
		for(k=0; k<localpalsize; k++) {
			i->palette[k].r = GetByte(i->parent);
			i->palette[k].g = GetByte(i->parent);
			i->palette[k].b = GetByte(i->parent);
		}
	} else {
		i->palette = i->parent->palette;
		i->localpalbits = i->parent->imgbits;
	}
	i->ncolors = 1 << i->localpalbits;
	
	i->imgbits = GetByte(i->parent);	// LZW Minimum Code Size

	if(i->interlaced) printf("interlaced ");
	printf("img pos(%d,%d) size %dx%d palbits=%d imgbits=%d ncolors=%d\n",
	       i->posX, i->posY, i->width, i->height, i->localpalbits, i->imgbits, i->ncolors);

	if(i->imgbits==1) {	/* fix for 1bit images ? */
		i->imgbits = 2;
	}
	clr = 1 << i->imgbits;
	eof = clr + 1;
	free = clr + 2;
	freesav = free;
	nbbitsav = i->imgbits + 1;
	i->nbbit = nbbitsav;
	max = (1 << nbbitsav) - 1;
	maxsav = max;
	stackp = 0;
	
	i->restbits = 0;	// initialise le "buffer" de lecture
	i->restbyte = 0;	// des codes LZW
	i->lbyte = 0;
	for(;;) {
		act_code = GetGifWord(i);
		if(act_code==eof) {
			printf("End of image code\n");
			return 0;
		}
		if(npix==0) {
			printf("assez de pixels, On se casse !\n");
			return 1;
		}	
		if(act_code==clr) {
			printf("Code clear (free=%d)\n", free);
			// clear
			free = freesav;
			i->nbbit = nbbitsav;
			max = maxsav;
			act_code = GetGifWord(i);
			casspecial = (u8)act_code;
			old_code = act_code;
			WritePixel(i, (u8)act_code); npix--;
		} else {
			read_byt = act_code;
			if(act_code>=free) {	// code pas encore dans alphabet
//				printf("Code pas dans alphabet : %d>=%d push %d\n", act_code, free, casspecial);
				ab_stack[stackp++] = (u8)casspecial; // dernier debut de chaine !
				act_code = old_code;
			}
//			printf("actcode=%d\n", act_code);
			while(act_code>clr) { // code non concret
				// fillstackloop empile les suffixes !
//				printf("fillstackloop : word=%4d (stackp=%d) suffix=%d\n",
//				       act_code, stackp, ab_suffx[act_code]);
				ab_stack[stackp++] = ab_suffx[act_code];
				act_code = ab_prfx[act_code];	// prefixe
			}
			// act_code est concret
			//ab_stack[stackp++] = (u8)act_code; printf("push %d   (stackp=%d)\n", act_code, stackp-1);
			WritePixel(i, (u8)act_code); npix--;
			casspecial = (u8)act_code;	// dernier debut de chaine !
			while(stackp>0) {
				WritePixel(i, ab_stack[--stackp]); npix--;
//				printf("%02X ", ab_stack[stackp]);
			}
//			putchar('\n');
			if(free<4096) { // la taille du dico est 4096 max !
				ab_prfx[free] = old_code;
				ab_suffx[free] = (u8)act_code;
				free++;
			}
			old_code = read_byt;
			if((free>max)&&(i->nbbit<12)) {
				i->nbbit++;	// 1 bit de plus pour les codes LZW
				max = (1 << i->nbbit) - 1;
			}
		}
			
	}
	return 0;
}

/* ------------------------------------------------ 
 * int LoadGif(struct ngiflib_gif *);
 * s'assurer que nimg=0 au depart !
 * retourne : 
 *    0 si GIF terminé
 *    un nombre negatif si ERREUR
 *    1 si image Decodée
 * rappeler pour decoder les images suivantes
 * ------------------------------------------------ */
int LoadGif(struct ngiflib_gif * g) {
	u8 sign;
	u8 tmp;
	int i;

	if(!g) return -1;	
	
	if(g->nimg==0) {
	GetByteStr(g, g->signature, 6);
	g->signature[6] = '\0';
	if(   g->signature[0] != 'G'
	   || g->signature[1] != 'I'
	   || g->signature[2] != 'F'
	   || g->signature[3] != '8') {
		return -1;
	}
	puts((char *)g->signature);
	
	g->width = GetWord(g);
	g->height = GetWord(g);
	/* allocate frame buffer */
	g->frbuff = ngiflib_malloc(4*(long)g->height*(long)g->width);
	
	tmp = GetByte(g);/* <Packed Fields> = Global Color Table Flag       1 Bit
	                                      Color Resolution              3 Bits
	                                      Sort Flag                     1 Bit
	                                      Size of Global Color Table    3 Bits */
	g->colorresolution = ((tmp & 0x70) >> 4) + 1;
	g->sort_flag = (tmp & 8) >> 3;
	g->imgbits = (tmp & 7) + 1;	// Global Palette color resolution
	g->ncolors = 1 << g->imgbits;	//
	
	g->backgroundindex = GetByte(g);
	g->transparent_flag = 0;
	
	printf("%dx%d %dbits %d couleurs  bg=%d\n",
	       g->width, g->height, g->imgbits, g->ncolors, g->backgroundindex);

	g->pixaspectratio = GetByte(g);	// pixel aspect ratio (0 : unspecified)

	if(tmp&0x80) {
		// la palette globale suit.
		g->palette = (struct ngiflib_rgb *)ngiflib_malloc(sizeof(struct ngiflib_rgb)*g->ncolors);
		for(i=0; i<g->ncolors; i++) {
			g->palette[i].r = GetByte(g);
			g->palette[i].g = GetByte(g);
			g->palette[i].b = GetByte(g);
	//		printf("%3d %02X %02X %02X\n", i, g->palette[i].r,g->palette[i].g,g->palette[i].b);
		}
	} else {
		g->palette = NULL;
	}
	
	FillGifBackGround(g);
	}
	sign = GetByte(g);	// signature du prochain bloc
	printf("0x%02X\n", sign);
	while(sign!=0x3B) { // END OF GIF
	if(sign=='!') {
		u8 id,size;//, term;
		u8 ext[256];
		int blockindex=0;
		id = GetByte(g);
		while( (size = GetByte(g)) ) {
		GetByteStr(g, ext, size);
		
		printf("extension (id=0x%02x) index %d, size = %dbytes\n",id,blockindex,size);

		switch(id) {
		case 0xF9:	//Graphic Control Extension
			g->disp_method = (ext[0] >> 2) & 7;
			g->transparent_flag = ext[0] & 1;
			g->userinputflag = (ext[0] >> 1) & 1;
			g->delay_time = ext[1] | (ext[2]<<8);
			g->transparent_color = ext[3];
			printf("disp_method=%d delay_time=%d (transp=%d)transparent_color=0x%02X\n",
			       g->disp_method, g->delay_time, g->transparent_flag, g->transparent_color);
			break;
		case 0xFE:	//Comment Extension.
			printf("-------------------- Comment extension --------------------\n");
			ext[size] = '\0';
			fputs((char *)ext, stdout);
			printf("-----------------------------------------------------------\n");
			break;
		case 0xFF:	// app extension      faire qqch avec ?
			if(blockindex==0) {
				char appid[9];
				ngiflib_memcpy(appid, ext, 8);
				appid[8] = 0;
				printf("---------------- Application extension ---------------\n");
				printf("Application identifier : '%s', auth code : %02X %02X %02X (",
				       appid, ext[8], ext[9], ext[10]);
				putchar((ext[8]<32)?' ':ext[8]);
				putchar((ext[9]<32)?' ':ext[9]);
				putchar((ext[10]<32)?' ':ext[10]);
				printf(")\n");
			} else {
				printf("Datas (as hex) : ");
				for(i=0; i<size; i++) {
					printf("%02x ", ext[i]);
				}
				printf("\nDatas (as text) : '");
				for(i=0; i<size; i++) {
					putchar((ext[i]<32)?' ':ext[i]);
				}
				printf("'\n");
			}
			break;
		case 0x01:	// plain text extension
			printf("Plain text extension\n");
			for(i=0; i<size; i++) {
				putchar((ext[i]<32)?' ':ext[i]);
			}
			putchar('\n');
			break;
		}
		blockindex++;
		}		
	} else if(sign==0x2C) {
		if(g->nimg==0) {
			g->cur_img = ngiflib_malloc(sizeof(struct ngiflib_img));
			g->first_img = g->cur_img;
		} else {
			g->cur_img->next = ngiflib_malloc(sizeof(struct ngiflib_img));
			g->cur_img = g->cur_img->next;
		}
		g->cur_img->next = NULL;
		g->cur_img->parent = g;
		DecodeGifImg(g->cur_img);
		g->nimg++;
		
		printf("0x%02X\n", GetByte(g));//0 final
		return 1;	// image decodée
	}
	sign = GetByte(g);
	printf("0x%02X\n", sign);
	}
	return 0;
}

u32 GifIndexToTrueColor(struct ngiflib_rgb * palette, u8 v) {
	return palette[v].b | (palette[v].g << 8) | (palette[v].r << 16);
}
