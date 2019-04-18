/* Image I/O */


#include <stdio.h>

// Error handling:
enum {
	ERR_PNG_MALLOC = -16,
	ERR_PNG_SIZE,
	ERR_PNG_READ_HEADER,
	ERR_NONE = 0,
	// Non-fatal:
	ERR_PNG_NORGB
};


int read_png_tile(FILE *fp, unsigned char *imagebuf, int square_size);
int read_png(FILE *fp, unsigned char **imagebuf, int *width, int *height);
int write_png(FILE *fp, unsigned char *imagebuf, int height, int width, int isrgb,
	int upside_down);


