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

	if(sprites == NULL)
		return;

	for(sp = barr_begin(sprites); sp < (sprite_t *) barr_end(sprites);
	    ++sp) {

		printf("\n\"%s\" (%d x %d)\n", bget(sp->sp_name), sp->sp_width,
		    sp->sp_height);

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
sprite2bytes(barr_t *sprites, int horiz)
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

	if(barr_cnt(sprites) == 0) {
		return;
	}

	bitcnt = 0;
	for(sp = (sprite_t *)barr_begin(sprites);
	    sp < (sprite_t *)barr_end(sprites); ++sp) {
		if(sp == (sprite_t *)barr_begin(sprites)) {
			bitcnt = bstrlen(sp->sp_pixels);
		} else {
			if(bitcnt != bstrlen(sp->sp_pixels)) {
				fprintf(stderr, "Sprites in file have"
				    " differing pixel counts\n");
				return;
			}
		}
		if(sp->sp_width % 8 || sp->sp_height % 8) {
			fprintf(stderr, "Sprite with /height is not divisible"
			    " by 8\n");
			return;
	}

	}

	if(bitcnt <= 0) {
		fprintf(stderr, "Invalid pixel count\n");
		return;
	}

	bytecnt = bitcnt / 8;

	if(barr_cnt(sprites) == 1) {
		printf("uint8_t	<buf_name>[%d] = \n", bytecnt);
	} else {
		printf("uint8_t	<buf_array_name>[%d][%d] = {\n",
		    barr_cnt(sprites), bytecnt);
	}

	for(spriteidx = 0; spriteidx < barr_cnt(sprites); ++spriteidx) {

		sp = (sprite_t *)barr_elem(sprites, spriteidx);
		byte = 0;
		printedcnt = 0;
		printf("\t{ /* \"%s\" (%dx%d): %s mapping */\n",
		    bget(sp->sp_name), sp->sp_width, sp->sp_height,
		    horiz?"horizontal":"vertical");
		printf("\t  ");
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
							printf("\n\t  ");
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
							printf("\n\t  ");
						else
							printf(" ");
					}

					byte = 0;
				}
			}
		}
		if(barr_cnt(sprites) == 1) 
			printf(" };\n");
		else
			printf(" }%s\n",
			    spriteidx < barr_cnt(sprites) - 1?",\n":"");
	}

	if(barr_cnt(sprites) > 1)
		printf("};\n\n");
	else
		printf("\n");
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
		if(argc != 3) {
			usage(execn);
			err = -1;
			goto end_label;
		}

		sprite2bytes(sprites, !xstrcmp(argv[2], "tobytesh")?1:0);
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
	printf("\n");
	printf("  Playback sprite animation:\n");
	printf("\n");
	printf("    %s <spritefile.json> animate <milliseconds> <loopcnt>\n",
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
	printf("    %s <spritefile.json> tobytesh\n",
	    progn);
	printf("\n");
	printf("  Convert to C vertical-mapped byte array:\n");
	printf("\n");
	printf("    %s <spritefile.json> tobytesv\n",
	    progn);
	printf("\n");
}
			
