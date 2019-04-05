/* PNG support
 *
 * added to NHW project <hackfin@section5.ch>
 *
 */

#include <stdlib.h>
#include "imgio.h"
#include <png.h>

int write_png(FILE *fp, unsigned char *imagebuf, int height, int width, int isrgb,
	int upside_down)
{
	png_structp pngp;
	png_infop infop;
	png_bytep *row_pointers;
	int i;
	int step;
	unsigned char *p;
	int retcode = -1;

	row_pointers = (png_bytep *) malloc(height * sizeof(png_bytep));
	if (row_pointers == 0) {
		printf("Could not malloc\n");
		goto fail;
	}

	if (isrgb) {
		step = 3 * width;
	} else {
		step = width;
	}

	p = imagebuf;

	if (upside_down) {
		i = height;
		while (i--) {
			row_pointers[i] = p;
			p += step;
		}
	} else {
		for (i = 0; i < height; i++) {
			row_pointers[i] = p;
			p += step;
		}
	}

	pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngp) {
		printf("Could not create PNG write struct\n");
		goto fail;
	}
	infop = png_create_info_struct(pngp);
	if (!infop) {
		printf("Could not create PNG info struct\n");
		goto fail_info;
	}

	if (setjmp(png_jmpbuf(pngp))) {
		printf("Error in init_io\n");
		goto fail_png;
	}

	png_init_io(pngp, fp);

	if (setjmp(png_jmpbuf(pngp))) {
		printf("Error writing PNG file\n");
		goto fail_png;
	}

	png_set_IHDR(pngp, infop, width, height, 8,
			isrgb ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

	png_write_info(pngp, infop);

	png_write_image(pngp, row_pointers);

	png_write_end(pngp, NULL);

	free(row_pointers);
	retcode = 0;
fail_png:
	png_destroy_info_struct(pngp, &infop);
fail_info:
	png_destroy_write_struct(&pngp, &infop);
fail:
	return retcode;
}
