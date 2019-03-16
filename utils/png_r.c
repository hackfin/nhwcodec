/* PNG support
 *
 * added to NHW project <hackfin@section5.ch>
 *
 */

#include "imgio.h"
#include <stdarg.h>
#include <stdlib.h>
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}


int read_png(FILE *fp, unsigned char *imagebuf, int square_size)
{
	png_structp pngp;
	png_infop infop;
	png_bytep *row_pointers;
	unsigned char *p;
	int ret = 0;
	int n;
	int y;
	int width, height;
	unsigned char header[8];	// 8 is the maximum size that can be checked

	ret = fread(header, 1, 8, fp);
	if (ret < 0 || png_sig_cmp(header, 0, 8))
		return -2;


	/* initialize stuff */
	pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!pngp)
		abort_("[read_png_file] png_create_read_struct failed");

	infop = png_create_info_struct(pngp);
	if (!infop)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(pngp)))
		abort_("[read_png_file] Error during init_io");

	png_init_io(pngp, fp);
	png_set_sig_bytes(pngp, 8);

	png_read_info(pngp, infop);

	width = png_get_image_height(pngp, infop);
	height = png_get_image_height(pngp, infop);

	if (width != height || width != square_size) {
		ret = -1;
		fprintf(stderr, "Img size %ux%u\n", width, height);
	} else {
		n = png_set_interlace_handling(pngp);
		printf("Number of passes: %d\n", n);
		png_read_update_info(pngp, infop);

		/* read file */
		if (setjmp(png_jmpbuf(pngp)))
			abort_("[read_png_file] Error during read_image");

		row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
		p = imagebuf;
		y = height;
		int rowbytes = png_get_rowbytes(pngp, infop);
		while (y--) {
			row_pointers[y] = p;
			p += rowbytes;
		}

		png_read_image(pngp, row_pointers);

		free(row_pointers);
	}
	return ret;
}
