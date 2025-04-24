#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include "bstr.h"
#include "blog.h"
#include "cJSON.h"
#include "cJSON_helper.h"

void usage(const char *);

typedef struct sprite {
	bstr_t	*sp_name;
	int	sp_width;
	int	sp_height;
	bstr_t	*sp_pixels; /* length must be (width * height), values
			     * must be '.' or 'X' */
} sprite_t;


int
readsprites(const char *filen, barr_t *sprites)
{
	bstr_t		*filecont;
	int		err;
	int		ret;
	cJSON		*json;
	cJSON		*chld;
	sprite_t	newsp;
	sprite_t	*sp;
	cJSON		*pdata;
	cJSON		*pdatalin;
	char		*ch;
	
	err = 0;	
	filecont = NULL;
	json = NULL;

	if(xstrempty(filen))
		return EINVAL;

	filecont = binit();
	if(filecont == NULL) {
		fprintf(stderr, "Could not allocate filecont\n");
		err = ENOMEM;
		goto end_label;
	}

	ret = bfromfile(filecont, filen);
	if(ret != 0) {
		fprintf(stderr, "Could not read file\n");
		err = ret;
		goto end_label;
	}

	json = cJSON_Parse(bget(filecont));
	if(json == NULL) {
		fprintf(stderr, "Could not parse json\n");
		err = ENOEXEC;
		goto end_label;
	}

	if(!cJSON_IsArray(json)) {
		fprintf(stderr, "File is not a JSON array\n");
		err = ENOEXEC;
		goto end_label;
	}

	for(chld = json->child; chld; chld = chld->next) {
		memset(&newsp, 0, sizeof(sprite_t));

		newsp.sp_name = binit();
		if(!newsp.sp_name) {
			fprintf(stderr, "Could not allocate sprite name\n");
			err = ENOMEM;
			goto end_label;
		}

		ret = cjson_get_childstr(chld, "name", newsp.sp_name);
		if(ret != 0 || bstrempty(newsp.sp_name)) {
			fprintf(stderr, "Sprite did not contain valid name\n");
			err = ENOMEM;
			goto end_label;
		}

		for(sp = barr_begin(sprites);
		    sp < (sprite_t *) barr_end(sprites);
		    ++sp) {
			if(!bstrcmp(newsp.sp_name, bget(sp->sp_name))) {
				fprintf(stderr, "Repeated sprite name \"%s\"\n",
				    bget(newsp.sp_name));
				err = ENOEXEC;
				goto end_label;
			}
		}

		ret = cjson_get_childint(chld, "width", &newsp.sp_width);
		if(ret != 0 || newsp.sp_width <= 0) {
			fprintf(stderr, "Sprite \"%s\" did not contain"
			    " valid width\n", bget(newsp.sp_name));
			err = ENOEXEC;
			goto end_label;
		}

		ret = cjson_get_childint(chld, "height", &newsp.sp_height);
		if(ret != 0 || newsp.sp_height <= 0) {
			fprintf(stderr, "Sprite \"%s\" did not contain"
			    " valid height\n", bget(newsp.sp_name));
			err = ENOEXEC;
			goto end_label;
		}

		newsp.sp_pixels = binit();
		if(!newsp.sp_pixels) {
			fprintf(stderr, "Could not allocate sprite pixels\n");
			err = ENOMEM;
			goto end_label;
		}

		pdata = cJSON_GetObjectItemCaseSensitive(chld, "pixels");
		if(pdata == NULL) {
			fprintf(stderr, "Sprite \"%s\" did not contain"
			    " \"pixels\"\n", bget(newsp.sp_name));
			err = ENOEXEC;
			goto end_label;
		}

		if(!cJSON_IsArray(pdata)) {
			fprintf(stderr, "Sprite \"%s\": \"pixels\" is not"
			    " an array\n", bget(newsp.sp_name));
			err = ENOEXEC;
			goto end_label;
		}

		for(pdatalin = pdata->child; pdatalin;
		    pdatalin = pdatalin->next) {

			if(!cJSON_IsString(pdatalin)) {
				fprintf(stderr, "Sprite \"%s\": pixel data"
				    " array member is not string\n",
				    bget(newsp.sp_name));
				err = ENOEXEC;
				goto end_label;
			}
			bstrcat(newsp.sp_pixels, pdatalin->valuestring);
		}

		if(bstrlen(newsp.sp_pixels) !=
		    (newsp.sp_width * newsp.sp_height)) {
			fprintf(stderr, "Sprite \"%s\": pixel data length"
			    " mismatch, expecing: %d, got %d\n",
			    bget(newsp.sp_name),
			    newsp.sp_width * newsp.sp_height,
			    bstrlen(newsp.sp_pixels));
			err = ENOEXEC;
			goto end_label;
		}

		for(ch = bget(newsp.sp_pixels); *ch; ++ch) {
			if(*ch != '.' && *ch != 'X') {
				fprintf(stderr, "Sprite \"%s\": unexpected"
				    " pixel character '%c'\n",
				    bget(newsp.sp_name), *ch);
				err = ENOEXEC;
				goto end_label;
			}
		}

		barr_add(sprites, &newsp);
	}

end_label:

	buninit(&filecont);

	if(json != NULL)
		cJSON_Delete(json);

	return err;
}


void
clearsprites(barr_t *sprites)
{
	sprite_t	*sp;

	for(sp = barr_begin(sprites); sp < (sprite_t *) barr_end(sprites);
	    ++sp) {
		buninit(&sp->sp_name);
		buninit(&sp->sp_pixels);
	}
}


int
ispixelon(sprite_t *sp, int x, int y)
{
	int	idx;

	if(sp == NULL || x < 0 ||  x >= sp->sp_width || y < 0 ||
	    y >= sp->sp_height)
		return 0;

	idx = y * sp->sp_width + x;

	if(idx >= bstrlen(sp->sp_pixels))
		return 0;

	return bget(sp->sp_pixels)[idx] == 'X';
}

void
dispsp(sprite_t *sp, int invert)
{
	int	x;
	int	y;

	if(sp == NULL)
		return;


	for(y = 0; y < sp->sp_height; y +=2) {
		for(x = 0; x < sp->sp_width; ++x) {
			if(ispixelon(sp, x, y)) {
				if(ispixelon(sp, x, y + 1)) {
					if(!invert)
						printf("█");
					else
						printf(" ");
				} else {
					if(!invert)
						printf("▀");
					else
						printf("▄");
				}
			} else {
				if(ispixelon(sp, x, y + 1)) {
					if(!invert)
						printf("▄");
					else
						printf("▀");
				} else {
					if(!invert)
						printf(" ");
					else
						printf("█");
				}
			}
		}
		printf("\n");
	}

	printf("\n");
}


void
displaysprites(barr_t *sprites, int invert)
{
	sprite_t	*sp;
	int		idx;

	if(sprites == NULL)
		return;

	for(sp = barr_begin(sprites), idx = 0;
	    sp < (sprite_t *) barr_end(sprites);
	    ++sp, ++idx) {

		printf("\n(%d) \"%s\" (%d x %d)\n", idx, bget(sp->sp_name),
		    sp->sp_width, sp->sp_height);

		dispsp(sp, invert);
	}

}


#define BIL_INTERPOL_THRESH	45

void
rotatesprite(sprite_t *sp, int degree, int goaround)
{
	int	siz;
	int	x;
	int	y;
	float	curx;
	float	cury;
	float	oldx;
	float	oldy;
	float	xcenteroffs;
	float	ycenteroffs;
	float	rad;
	int	oldxint;
	int	oldxdec;
	int	oldyint;
	int	oldydec;
	int	interpol_horiz;
	int	interpol_vert;
	int	curdeg;


	if(sp == NULL || sp->sp_width != sp->sp_height)
		return;

	siz = sp->sp_width;

	if(goaround) {
		/* Emit unmodified sprite at the beginning of the sprite
		 * sheet */
		printf("[\n");
		printf("  {\n");
		printf("    \"name\": \"%s_0_deg\",\n", bget(sp->sp_name));
		printf("    \"width\": %d,\n", siz);
		printf("    \"height\": %d,\n", siz);
		printf("    \"pixels\": [\n");
		for(y = 0; y < siz; ++y) {
			printf("      \"");
			for(x = 0; x < siz; ++x) {
				if(ispixelon(sp, x, y))
					printf("X");
				else
					printf(".");
			}
			printf("\"%s\n", y < siz - 1?",":"");
		}

		printf("    ]\n");
		printf("  },\n");
	}


	for(curdeg = degree; curdeg < 360; curdeg += degree) {

		if(!goaround)
			printf("[\n");

		printf("  {\n");
		printf("    \"name\": \"%s_%d_deg\",\n", bget(sp->sp_name),
		    curdeg);
		printf("    \"width\": %d,\n", siz);
		printf("    \"height\": %d,\n", siz);
		printf("    \"pixels\": [\n");

		xcenteroffs = (float)(siz - 1) / 2;
		ycenteroffs = (float)(siz - 1) / 2;

		/* Math functiones expect the angle in radians */
		rad = curdeg * M_PI / 180;

		for(y = 0; y < siz; ++y) {
			printf("      \"");
			for(x = 0; x < siz; ++x) {

				/* If the rotation is a multiple of 90,
				 * don't need to do any math at all, we can
				 * simply map pixels 1:1 */
				if(curdeg == 90) {
					if(ispixelon(sp, siz - 1 - y, x))
						printf("X");
					else
						printf(".");
					continue;
				} else
				if(curdeg == 180) {
					if(ispixelon(sp, siz - 1 - x,
					    siz - 1 - y))
						printf("X");
					else
						printf(".");
					continue;
				} else
				if(curdeg == 270) {
					if(ispixelon(sp, y,
					    siz - 1 - x))
						printf("X");
					else
						printf(".");
					continue;
				}
	
				/* Rotate sprite around its center
				 * and not (0,0) */
				curx = (float)x - xcenteroffs;
				cury = (float)y - ycenteroffs;

				/* When rotating bitmaps, it's best to do
				 * reverse mapping: instead of calculating
				 * the new positions of all "before" pixels,
				 * we calculate the old positions of all
				 * "after" pixel. This way we make sure that
				 * each "after" pixel gets a value. */

				oldx = cos(rad) * curx - sin(rad) * cury;
				oldy = sin(rad) * curx + cos(rad) * cury;

				/* Switch back to 0 indexed coordinates. */
				oldx += xcenteroffs;
				oldy += ycenteroffs;

				/* Calculate "after" pixel value using bilinear
				 * interpolation. */

				/* Switch to int, keep two decimals */
		                oldxint = floor(oldx);
				oldxdec = (int)(oldx * 100) - (oldxint * 100);
				oldyint = floor(oldy);
				oldydec = (int)(oldy * 100) - (oldyint * 100);

				interpol_horiz = 
         ( ispixelon(sp, oldxint, oldyint) * (100-oldxdec) +
           ispixelon(sp, oldxint+1 , oldyint) * oldxdec +
           ispixelon(sp, oldxint , oldyint + 1) * (100-oldxdec) +
           ispixelon(sp, oldxint + 1, oldyint + 1) * oldxdec ) / 2;

				interpol_vert = 
          ( ispixelon(sp, oldxint, oldyint) * (100 - oldydec) +
            ispixelon(sp, oldxint, oldyint + 1) * oldydec +
            ispixelon(sp, oldxint + 1, oldyint) * (100 - oldydec) +
            ispixelon(sp, oldxint + 1, oldyint + 1) * oldydec) / 2;

				if((interpol_horiz + interpol_vert) / 2 >=
				    BIL_INTERPOL_THRESH)
					printf("X");
				else
					printf(".");
			}
			printf("\"%s\n", y < siz - 1?",":"");
		}

		printf("    ]\n");
	
		if(goaround && curdeg + degree < 360)
			printf("  },\n");
		else
			printf("  }\n");

		if(!goaround)
			break;

	}
	printf("]\n");
}


void
animatesprites(barr_t *sprites, int invert, int msec, int loopcnt)
{
	sprite_t	*sp;
	int		i;

	if(sprites == NULL)
		return;
	
	for(i = 0; i < loopcnt; ++i) {

		for(sp = barr_begin(sprites);
		    sp < (sprite_t *) barr_end(sprites);
		    ++sp) {
			dispsp(sp, invert);

			usleep(msec * 1000);
			
			/* Move cursor up */
			printf("\e[%dA", sp->sp_height / 2 + 1 +
			    sp->sp_height % 2); /* Account for odd heights */
							
		}
	}

}


void
flipsprite(sprite_t *sp, int horiz)
{
	int	x;
	int	y;

	if(sp == NULL)
		return;

	printf("[\n");

	printf("  {\n");
	printf("    \"name\": \"%s_fliph\",\n", bget(sp->sp_name));
	printf("    \"width\": %d,\n", sp->sp_width);
	printf("    \"height\": %d,\n", sp->sp_height);
	printf("    \"pixels\": [\n");

	for(y = 0; y < sp->sp_height; ++y) {
		printf("      \"");
		for(x = 0; x < sp->sp_width; ++x) {

			if(horiz) {
				if(ispixelon(sp, sp->sp_width - 1 - x, y))
					printf("X");
				else
					printf(".");
			} else {
				if(ispixelon(sp, x, sp->sp_height - 1 - y))
					printf("X");
				else
					printf(".");
			}
		}

		printf("\"%s\n", y < sp->sp_height - 1?",":"");
	}

	printf("    ]\n");
	printf("  }\n");
	printf("]\n");
}


int
sprite_issquare(sprite_t *sp)
{
	if(sp == NULL)
		return 0;

	return sp->sp_width == sp->sp_height;
}


void
sprite2bytes(barr_t *sprites, int horiz, const char *fontn)
{
	int		bitcnt;
	int		bytecnt;
	sprite_t	*sp;
	int		x;
	int		y;
	uint8_t		byte;
	int		printedcnt;
	int		i;
	int		spriteidx;	
	bstr_t		*char_table;
	int		offset;

	char_table = NULL;

	if(barr_cnt(sprites) == 0) {
		goto end_label;
	}

	for(sp = (sprite_t *)barr_begin(sprites);
	    sp < (sprite_t *)barr_end(sprites); ++sp) {
		if(horiz && sp->sp_width % 8) {
			fprintf(stderr, "Sprite \"%s\" width is not divisible"
			    " by 8\n", bget(sp->sp_name));
			goto end_label;
		} else
		if(!horiz && sp->sp_height % 8) {
			fprintf(stderr, "Sprite \"%s\" height is not divisible"
			    " by 8\n", bget(sp->sp_name));
			goto end_label;
		}
	}

	char_table = binit();
	if(char_table == NULL) {
		fprintf(stderr, "Could not allocate char_table\n");
		goto end_label;
	}
	bprintf(char_table, "font_%s_chars[] = {\n", fontn);

	printf("uint8_t	font_%s_bitmaps[] = {\n\n", fontn);
	offset = 0;

	for(spriteidx = 0; spriteidx < barr_cnt(sprites); ++spriteidx) {
		sp = (sprite_t *)barr_elem(sprites, spriteidx);

		bitcnt = bstrlen(sp->sp_pixels);
		bytecnt = bitcnt / 8;

		byte = 0;
		printedcnt = 0;
		printf("\t/* \"%s\" (%dx%d): %s mapping, %d pixels,"
		    " %d bytes */\n",
		    bget(sp->sp_name), sp->sp_width, sp->sp_height,
		    horiz?"horizontal":"vertical", bitcnt, bytecnt);
		printf("\t   ");
		if(horiz) {
			for(y = 0; y < sp->sp_height; ++y) {
				for(x = 0; x < sp->sp_width; x += 8) {
					for(i = 0; i < 8; ++i) {
						byte <<= 1;
						if(ispixelon(sp, x + i, y))
							byte |= 0x01;

					}

					printf("0x%02x", byte);
					++printedcnt;
					if(printedcnt < bytecnt) {
						printf(",");

						if(printedcnt % 8 == 0)
							printf("\n\t   ");
						else
							printf(" ");
					}

					byte = 0;
				}
			}
		} else { /* Vertical mapping */

			for(y = 0; y < sp->sp_height; y += 8) {
				for(x = 0; x < sp->sp_width; ++x) {
					for(i = 7; i >= 0; --i) {
						byte <<= 1;
						if(ispixelon(sp, x, y + i))
							byte |= 0x01;
					}

					printf("0x%02x", byte);
					++printedcnt;
					if(printedcnt < bytecnt) {
						printf(",");

						if(printedcnt % 8 == 0)
							printf("\n\t   ");
						else
							printf(" ");
					}

					byte = 0;
				}
			}
		}
		printf("%s\n", spriteidx < barr_cnt(sprites) - 1?",\n":"");

		
		bprintf(char_table, "\n\t/* %s */\n", bget(sp->sp_name));
		bprintf(char_table, "\t{ %d,\t%d,\t%d};\n", sp->sp_width,
		    offset, bytecnt);

		offset += bytecnt;
	}

	printf("};\n\n");

	bprintf(char_table, "};\n");

	printf("\n%s\n\n", bget(char_table));

	printf("font_t font_%s = {\n", fontn);
	printf("	<fill in>,\n");
	printf("	<fill in>,\n");
	printf("	<fill in>,\n");
	printf("	font_%s_bitmaps\n", fontn);
	printf("};");
	

end_label:

	buninit(&char_table);
}



void outpsp(barr_t *sprites, int idx, const char *nam, int last)
{
	int		x;
	int 		y;
	sprite_t	*sp;

	sp = barr_elem(sprites, idx);
	if(sp == NULL)
		return;	

	printf("  {\n");
	printf("    \"name\": \"%s\",\n", nam);
	printf("    \"width\": %d,\n", sp->sp_width);
	printf("    \"height\": %d,\n", sp->sp_height);
	printf("    \"pixels\": [\n");
	for(y = 0; y < sp->sp_height; ++y) {
		printf("      \"");
		for(x = 0; x < sp->sp_width; ++x) {
			if(ispixelon(sp, x, y))
				printf("X");
			else
				printf(".");
			
		}
		printf("\"%s\n", y < sp->sp_height - 1?",":"");
	}
	
	printf("    ]\n");
	printf("  }%s\n", last?"":",");
}

void
conv_c64_rom_order_to_ascii(barr_t *sprites)
{

	if(sprites == NULL || barr_cnt(sprites) < 283)
		return;
	
	printf("[\n");

	outpsp(sprites, 32, "ASCII 0x20 ' '", 0);
	outpsp(sprites, 33, "ASCII 0x21 '!'", 0);
	outpsp(sprites, 34, "ASCII 0x22 '\\\"'", 0);
	outpsp(sprites, 35, "ASCII 0x23 '#'", 0);
	outpsp(sprites, 36, "ASCII 0x24 '$'", 0);
	outpsp(sprites, 37, "ASCII 0x25 '%'", 0);
	outpsp(sprites, 38, "ASCII 0x26 '&'", 0);
	outpsp(sprites, 39, "ASCII 0x27 '\'", 0);
	outpsp(sprites, 40, "ASCII 0x28 '('", 0);
	outpsp(sprites, 41, "ASCII 0x29 ')'", 0);
	outpsp(sprites, 42, "ASCII 0x2a '*'", 0);
	outpsp(sprites, 43, "ASCII 0x2b '+'", 0);
	outpsp(sprites, 44, "ASCII 0x2c ','", 0);
	outpsp(sprites, 45, "ASCII 0x2d '-'", 0);
	outpsp(sprites, 46, "ASCII 0x2e '.'", 0);
	outpsp(sprites, 47, "ASCII 0x2f '/'", 0);
	outpsp(sprites, 48, "ASCII 0x30 '0'", 0);
	outpsp(sprites, 49, "ASCII 0x31 '1'", 0);
	outpsp(sprites, 50, "ASCII 0x32 '2'", 0);
	outpsp(sprites, 51, "ASCII 0x33 '3'", 0);
	outpsp(sprites, 52, "ASCII 0x34 '4'", 0);
	outpsp(sprites, 53, "ASCII 0x35 '5'", 0);
	outpsp(sprites, 54, "ASCII 0x36 '6'", 0);
	outpsp(sprites, 55, "ASCII 0x37 '7'", 0);
	outpsp(sprites, 56, "ASCII 0x38 '8'", 0);
	outpsp(sprites, 57, "ASCII 0x39 '9'", 0);
	outpsp(sprites, 58, "ASCII 0x3a ':'", 0);
	outpsp(sprites, 59, "ASCII 0x3b ';'", 0);
	outpsp(sprites, 60, "ASCII 0x3c '<'", 0);
	outpsp(sprites, 61, "ASCII 0x3d '='", 0);
	outpsp(sprites, 62, "ASCII 0x3e '>'", 0);
	outpsp(sprites, 63, "ASCII 0x3f '?'", 0);
	outpsp(sprites, 0, "ASCII 0x40 '@'", 0);
	outpsp(sprites, 1, "ASCII 0x41 'A'", 0);
	outpsp(sprites, 2, "ASCII 0x42 'B'", 0);
	outpsp(sprites, 3, "ASCII 0x43 'C'", 0);
	outpsp(sprites, 4, "ASCII 0x44 'D'", 0);
	outpsp(sprites, 5, "ASCII 0x45 'E'", 0);
	outpsp(sprites, 6, "ASCII 0x46 'F'", 0);
	outpsp(sprites, 7, "ASCII 0x47 'G'", 0);
	outpsp(sprites, 8, "ASCII 0x48 'H'", 0);
	outpsp(sprites, 9, "ASCII 0x49 'I'", 0);
	outpsp(sprites, 10, "ASCII 0x4a 'J'", 0);
	outpsp(sprites, 11, "ASCII 0x4b 'K'", 0);
	outpsp(sprites, 12, "ASCII 0x4c 'L'", 0);
	outpsp(sprites, 13, "ASCII 0x4d 'M'", 0);
	outpsp(sprites, 14, "ASCII 0x4e 'N'", 0);
	outpsp(sprites, 15, "ASCII 0x4f 'O'", 0);
	outpsp(sprites, 16, "ASCII 0x50 'P'", 0);
	outpsp(sprites, 17, "ASCII 0x51 'Q'", 0);
	outpsp(sprites, 18, "ASCII 0x52 'R'", 0);
	outpsp(sprites, 19, "ASCII 0x53 'S'", 0);
	outpsp(sprites, 20, "ASCII 0x54 'T'", 0);
	outpsp(sprites, 21, "ASCII 0x55 'U'", 0);
	outpsp(sprites, 22, "ASCII 0x56 'V'", 0);
	outpsp(sprites, 23, "ASCII 0x57 'W'", 0);
	outpsp(sprites, 24, "ASCII 0x58 'X'", 0);
	outpsp(sprites, 25, "ASCII 0x59 'Y'", 0);
	outpsp(sprites, 26, "ASCII 0x5a 'Z'", 0);
	outpsp(sprites, 27, "ASCII 0x5b '['", 0);
	outpsp(sprites, 32, "ASCII 0x5c '\\'", 0);
	outpsp(sprites, 29, "ASCII 0x5d ']'", 0);
	outpsp(sprites, 32, "ASCII 0x5e '^'", 0);
	outpsp(sprites, 32, "ASCII 0x5f '_'", 0);
	outpsp(sprites, 32, "ASCII 0x60 '`'", 0);
	outpsp(sprites, 257, "ASCII 0x61 'a'", 0);
	outpsp(sprites, 258, "ASCII 0x62 'b'", 0);
	outpsp(sprites, 259, "ASCII 0x63 'c'", 0);
	outpsp(sprites, 260, "ASCII 0x64 'd'", 0);
	outpsp(sprites, 261, "ASCII 0x65 'e'", 0);
	outpsp(sprites, 262, "ASCII 0x66 'f'", 0);
	outpsp(sprites, 263, "ASCII 0x67 'g'", 0);
	outpsp(sprites, 264, "ASCII 0x68 'h'", 0);
	outpsp(sprites, 265, "ASCII 0x69 'i'", 0);
	outpsp(sprites, 266, "ASCII 0x6a 'j'", 0);
	outpsp(sprites, 267, "ASCII 0x6b 'k'", 0);
	outpsp(sprites, 268, "ASCII 0x6c 'l'", 0);
	outpsp(sprites, 269, "ASCII 0x6d 'm'", 0);
	outpsp(sprites, 270, "ASCII 0x6e 'n'", 0);
	outpsp(sprites, 271, "ASCII 0x6f 'o'", 0);
	outpsp(sprites, 272, "ASCII 0x70 'p'", 0);
	outpsp(sprites, 273, "ASCII 0x71 'q'", 0);
	outpsp(sprites, 274, "ASCII 0x72 'r'", 0);
	outpsp(sprites, 275, "ASCII 0x73 's'", 0);
	outpsp(sprites, 276, "ASCII 0x74 't'", 0);
	outpsp(sprites, 277, "ASCII 0x75 'u'", 0);
	outpsp(sprites, 278, "ASCII 0x76 'v'", 0);
	outpsp(sprites, 279, "ASCII 0x77 'w'", 0);
	outpsp(sprites, 280, "ASCII 0x78 'x'", 0);
	outpsp(sprites, 281, "ASCII 0x79 'y'", 0);
	outpsp(sprites, 282, "ASCII 0x7a 'z'", 0);
	outpsp(sprites, 32, "ASCII 0x7b '{'", 0);
	outpsp(sprites, 32, "ASCII 0x7c '|'", 0);
	outpsp(sprites, 32, "ASCII 0x7d '}", 0);
	outpsp(sprites, 32, "ASCII 0x7e '~'", 1);

	
	printf("]\n");


}


int
main(int argc, char **argv)
{
	int	err;
	int	ret;
	char	*execn;
	barr_t	*sprites;

	err = 0;
	sprites = NULL;

        execn = basename(argv[0]);
        if(xstrempty(execn)) {
                fprintf(stderr, "Can't get executable name\n");
                err = -1;
                goto end_label;
        }

	if(argc < 3) {
		usage(execn);
		err = -1;
		goto end_label;
	}

	if(xstrempty(argv[1])) {
		fprintf(stderr, "Invalide filename\n");
		err = -1;
		goto end_label;
	}

	sprites = barr_init(sizeof(sprite_t));
	if(sprites == NULL) {
		fprintf(stderr, "Could not allocate sprites array\n");
		err = -1;
		goto end_label;
	}

	/* Read sprites into memory (first argument is always the sprite file)*/
	ret  = readsprites(argv[1], sprites);
	if(ret != 0) {
		fprintf(stderr, "Could read sprites\n");
		err = -1;
		goto end_label;
	}

	if(!xstrcmp(argv[2], "display")) {
		displaysprites(sprites, 0);
	} else
	if(!xstrcmp(argv[2], "invdisplay")) {
		displaysprites(sprites, 1);
	} else
	if(!xstrcmp(argv[2], "animate") || !xstrcmp(argv[2], "invanimate")) {
		if(argc != 5) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		if(xatoi(argv[3]) <= 0) {
			fprintf(stderr, "Invalid animation argument\n",
			    stderr);
			err = -1;
			goto end_label;
		}
		animatesprites(sprites, xstrcmp(argv[2], "animate")?1:0,
		    xatoi(argv[3]), xatoi(argv[4]));
	} else
	if(!xstrcmp(argv[2], "rotate")) {
		if(argc != 4) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		if(xatoi(argv[3]) <= 0 || xatoi(argv[3]) >= 360) {
			fprintf(stderr, "Invalid rotation angle\n",
			    stderr);
			err = -1;
			goto end_label;
		}
		if(barr_cnt(sprites) != 1) {
			fprintf(stderr, "Input must contain exactly one"
			    " sprite\n", stderr);
			err = -1;
			goto end_label;
		}
		if(!sprite_issquare(barr_elem(sprites, 0))) {
			fprintf(stderr, "Sprite must be square\n", stderr);
			err = -1;
			goto end_label;
		}
		rotatesprite(barr_elem(sprites, 0), xatoi(argv[3]), 0);
	} else
	if(!xstrcmp(argv[2], "rotate360")) {
		if(argc != 4) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		if(xatoi(argv[3]) <= 0 || xatoi(argv[3]) >= 360) {
			fprintf(stderr, "Invalid rotation increment\n",
			    stderr);
			err = -1;
			goto end_label;
		}
		if((360 % xatoi(argv[3])) != 0) {
			fprintf(stderr, "%s not a divisor of 360\n", argv[3],
			    stderr);
			err = -1;
			goto end_label;
		}
		if(barr_cnt(sprites) != 1) {
			fprintf(stderr, "Input must contain exactly one"
			    " sprite\n", stderr);
			err = -1;
			goto end_label;
		}
		if(!sprite_issquare(barr_elem(sprites, 0))) {
			fprintf(stderr, "Sprite must be square\n", stderr);
			err = -1;
			goto end_label;
		}
		rotatesprite(barr_elem(sprites, 0), xatoi(argv[3]), 1);
	} else
	if(!xstrcmp(argv[2], "fliph") || !xstrcmp(argv[2], "flipv")) {
		if(argc != 3) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		if(barr_cnt(sprites) != 1) {
			fprintf(stderr, "Input must contain exactly one"
			    " sprite\n", stderr);
			err = -1;
			goto end_label;
		}
		flipsprite(barr_elem(sprites, 0),
		    !xstrcmp(argv[2], "fliph")?1:0);
	} else
	if(!xstrcmp(argv[2], "tobytesh") || !xstrcmp(argv[2], "tobytesv")) {
		if(argc != 4) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		if(xstrempty(argv[3])) {
			fprintf(stderr, "Invalid font name\n", stderr);
			err = -1;
			goto end_label;
		}

		sprite2bytes(sprites, !xstrcmp(argv[2], "tobytesh")?1:0,
		    argv[3]);
	} else
	if(!xstrcmp(argv[2], "c64toascii")) {
		if(argc != 3) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		conv_c64_rom_order_to_ascii(sprites);
	} else {
		fprintf(stderr, "Unknown command: %s\n", argv[2]);
		err = -1;
		goto end_label;
	}

end_label:

	if(sprites) {
		clearsprites(sprites);
		barr_uninit(&sprites);
	}

	return err;
}



void
usage(const char *progn)
{
	printf("usage:\n");
	printf("\n");
	printf("  Display sprite(s):\n");
	printf("\n");
	printf("    %s <spritefile.json> display\n", progn);
	printf("    %s <spritefile.json> invdisplay\n", progn);
	printf("\n");
	printf("  Playback sprite animation:\n");
	printf("\n");
	printf("    %s <spritefile.json> animate <milliseconds> <loopcnt>\n",
	    progn);
	printf("    %s <spritefile.json> invanimate <milliseconds> <loopcnt>\n",
	    progn);
	printf("\n");
	printf("  Rotate sprite to specified degrees:\n");
	printf("\n");
	printf("    %s <spritefile.json> rotate360 <degree>\n",
	    progn);
	printf("\n");
	printf("  Create 360 degree rotation sprite sheet:\n");
	printf("\n");
	printf("    %s <spritefile.json> rotate360 <degree increments>\n",
	    progn);
	printf("\n");
	printf("  Flip (mirror) horizontally:\n");
	printf("\n");
	printf("    %s <spritefile.json> fliph\n",
	    progn);
	printf("\n");
	printf("  Flip (mirror) vertically:\n");
	printf("\n");
	printf("    %s <spritefile.json> flipv\n",
	    progn);
	printf("\n");
	printf("  Convert to C horizontal-mapped byte array:\n");
	printf("\n");
	printf("    %s <spritefile.json> tobytesh <fontname>\n",
	    progn);
	printf("\n");
	printf("  Convert to C vertical-mapped byte array:\n");
	printf("\n");
	printf("    %s <spritefile.json> tobytesv\n",
	    progn);
	printf("\n");
	printf("  Convert font from PETSCII (Commodore 8-bit"
	    " order to ASCII order:\n");
	printf("\n");
	printf("    %s <spritefile.json> c64toascii\n",
	    progn);
	printf("\n");
}
			
