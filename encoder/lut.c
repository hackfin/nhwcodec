#include <stdint.h>
#include <math.h>


#define SET_RGB(rgb, u, v, w) \
	rgb[0] = u; rgb[1] = v; rgb[2] = w;



struct {
	float gamma;
} g_config = {
	.gamma = 0.7
};

void hwb2rgb(uint16_t hwb[3], uint8_t rgb[3])
{
	short h, w, b;
	short u, v;
	short i, f;

	h = hwb[0]; w = hwb[1]; b = hwb[2];

	v = 255 - b;
	i = (h & 0x0f00) >> 8;
	f = h & 0xff;

	if (i & 1)
		f = 255 - f;
	u = w + ((f * (v - w)) >> 8);

	switch (i) {
		case 0:  SET_RGB(rgb, w, u, v); break; // blue to cyan
		case 1:  SET_RGB(rgb, w, v, u); break; // green to cyan
		case 2:  SET_RGB(rgb, u, v, w); break; // green to yellow
		case 3:  SET_RGB(rgb, v, u, w); break; // red to yellow
		case 4:  SET_RGB(rgb, v, w, u); break; // red to magenta
		case 5:  SET_RGB(rgb, u, w, v); break; // blue to magenta
		default:  SET_RGB(rgb, 127, 127, 127);
	}
}

uint16_t gamma15(uint16_t norm, uint16_t v)
{
	float f = (float) v / 32767.0;
	f = (float) norm * powf(f, g_config.gamma);
	return (uint16_t) f;
}

void convert_pseudo15(uint16_t v, uint8_t rgb[3])
{
	uint16_t hwb[3];

	if (v < 256) {
		rgb[0] = v;
		rgb[1] = v;
		rgb[2] = v;
	} else if (v > 0x8000) { // negative values
		hwb[0] = 0x800 - (v >> 5);
		hwb[2] = (0x10000 - v) >> 8; 
		hwb[1] = 127 - hwb[2] / 2;
		hwb2rgb(hwb, rgb);
	} else {
		v = gamma15(0x500, v);
		hwb[0] = v;
		hwb[2] = 0;
		hwb[1] = 0;
		hwb2rgb(hwb, rgb);
	}
}

void init_lut(uint8_t *lut, float gamma)
{
	int i;
	int j = 0;
	// Gray values for first 255:
	g_config.gamma = gamma;

	for (i = 0; i < 0x10000; i++) {
		convert_pseudo15(i, &lut[j]);
		j += 3;
	}

}

