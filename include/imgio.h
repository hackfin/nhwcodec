/* Image I/O */


#include <stdio.h>

int read_png(FILE *fp, unsigned char *imagebuf);
int write_png(FILE *fp, unsigned char *imagebuf, int isrgb);
