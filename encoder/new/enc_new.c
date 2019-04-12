#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include "utils.h"
#include "imgio.h"
#include "codec.h"
#include "remote.h"

#define FIRST_STAGE  0
#define SECOND_STAGE 1

extern unsigned char extra_words1[19];
extern unsigned char extra_words2[19];

#define CRUCIAL
#define NOT_CRUCIAL
// #define NOT_CRUCIAL_QUALITY_BETTER_THAN_LOW8


// PROTOS
void offsetY_non_LL(image_buffer *im);
void offsetY_LL_q4(image_buffer *im);
void offsetY_process(image_buffer *im, int m1);

// Original functions:
void SWAPOUT_FUNCTION(offsetY)(image_buffer *im,encode_state *enc, int m1);
void SWAPOUT_FUNCTION(quant_ac_final)(image_buffer *im, int ratio, const short *y_wl);

#ifdef __linux__
#define TMPDIR "/tmp/"
#else
#define TMPDIR ""
#endif

#define DEBUG

#ifdef DEBUG
#define WRITE_IMAGE16(file, buf, ts, m) \
	if (enc->debug) write_image16(TMPDIR file, buf, ts, m)
#else
#define WRITE_IMAGE16(file, buf, ts, m)
#endif

void wl_synth_luma(image_buffer *im, int norder, int last_stage);

#ifdef CRUCIAL

void revert_compensate_offsets(image_buffer *im, short *res256);
void tag_thresh_ranges(image_buffer *im, short *result);

void preprocess_res_q8(image_buffer *im, short *res256, encode_state *enc);
void process_hires_q8(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc);


void process_residuals(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc);

void reduce_lowres_generic(image_buffer *im, char *wvlt, int ratio);

void reduce_generic_simplified(image_buffer *im,
	const short *resIII, char *wvlt, encode_state *enc, int ratio);

#endif

void init_lut(uint8_t *lut, float gamma);

void Y_highres_compression(image_buffer *im,encode_state *enc);

const short *lookup_ywlthreshold(int quality);

void write_image(const char *filename, uint8_t *buf)
{
	FILE *out = fopen(filename, "wb");
	int s = 1 << g_encconfig.tilepower;
	if (out) {
		write_png(out, buf, s, s, 0, 0);
		fclose(out);
	} else {
		perror("Opening file");
	}
}

uint8_t g_lut[0x10000 * 3];

// Image conversion modes:

enum {
	CONV_LL = 0,
	CONV_LUT,
	CONV_GRAY
};


void write_image16(const char *filename, const int16_t *buf, int tilesize, int mode)
{
	uint8_t *buf8;
	uint8_t *dst;
	uint8_t *rgb;
	uint16_t tmp;
	int i, j;
	buf8 = malloc(3 * tilesize * tilesize * sizeof(uint8_t));
	dst = buf8;
	switch (mode) {
		case CONV_LL:
			for (i = 0; i < tilesize / 2; i++) {
			// Don't use LUT for LL quadrant
				for (j = 0; j < tilesize / 2; j++) {
					tmp = *buf++;
					if (tmp <= 255) {
						*dst++ = tmp; *dst++ = tmp; *dst++ = tmp;
					} else {
						rgb = &g_lut[tmp * 3];
						*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
					}
				}
				for (j = 0; j < tilesize / 2; j++) {
					tmp = *buf++;
					rgb = &g_lut[tmp * 3];
					*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
				}

			}
			for (i = 0; i < tilesize / 2; i++) {
				for (j = 0; j < tilesize; j++) {
					tmp = *buf++;
					rgb = &g_lut[tmp * 3];
					*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
				}
			}
			break;
		case CONV_LUT:
			for (i = 0; i < tilesize * tilesize; i++) {
				tmp = *buf++;
				rgb = &g_lut[tmp * 3];
				*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
			}
			break;
		case CONV_GRAY:
			for (i = 0; i < tilesize * tilesize; i++) {
				tmp = *buf++;
				if (tmp <= 255) {
					*dst++ = tmp; *dst++ = tmp; *dst++ = tmp;
				} else {
					rgb = &g_lut[tmp * 3];
					*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
				}
			}
			break;
		default:
			for (i = 0; i < tilesize; i++) {
				for (j = 0; j < tilesize; j++) {
					tmp = *buf++;
					if (tmp < 0) {
						*dst++ = 0xff; *dst++ = 0; *dst++ = 0;
					} else {
						*dst++ = tmp >> 8; *dst++ = tmp >> 8; *dst++ = tmp >> 8;
					}
				}
			}

	}

	FILE *out = fopen(filename, "wb");
	if (out) {
		printf("Write image %s\n", filename);
		write_png(out, buf8, tilesize, tilesize, 1, 0); // never upside down
		fclose(out);
	} else {
		perror("Opening file");
	}

	free(buf8);
}

void set_range(uint8_t *lut, int from, int to, uint8_t *rgb)
{
	int i;
	uint8_t *p;

	for (i = from; i < to; i++) {
		p = &lut[3*i];

		p[0] = rgb[0];
		p[1] = rgb[1];
		p[2] = rgb[2];
	}
}

unsigned char col_table[][3] = {
	{ 64,   0,   0 },
	{  0,  64,   0 },

	{ 127,   0,   0 },
	{  0,  127,   0 },

	{ 255,   0, 255 },
	{ 0,     0, 255 },

	{ 255,   0, 0 },
	{ 255,   0, 127 },
	{ 255, 127, 0 },
	{ 255, 255, 0 },
	{ 255, 255, 255 },
	{ 0, 255, 255 },
	{ 127, 127, 127 },
};

#define DARK_RED       0
#define DARK_GREEN     1
#define MID_RED        2
#define MID_GREEN      3
#define FULL_PINK      4
#define FULL_BLUE      5
#define FULL_RED       6
#define MEDIUM_PINK    7
#define ORANGE         8
#define YELLOW         9
#define WHITE          10
#define CYAN           11
#define GRAY           12

void custom_init_lut(uint8_t *lut, int thresh)
{
	memset(lut, 0, sizeof(g_lut));

	set_range(lut, 0x10000 - thresh, 0xffff, col_table[DARK_RED]);
	set_range(lut, 0, thresh, col_table[DARK_GREEN]);
	set_range(lut, 0x10000 - 2 * thresh, 0xffff - thresh, col_table[MID_RED]);
	set_range(lut, thresh, 2 * thresh, col_table[MID_GREEN]);

	set_range(lut, 0x8000, 0xffff - 2 * thresh, col_table[GRAY]);
	set_range(lut, 2 * thresh, 255, col_table[FULL_BLUE]);
	set_range(lut, 256, 10000, col_table[FULL_PINK]);

	set_range(lut, 10000, 12000, col_table[YELLOW]);
	set_range(lut, 12000, 16000, col_table[CYAN]);
	set_range(lut, 16000, 25000, col_table[ORANGE]);
}

/*  Simplified encoder function for fixed quality
 */

void offsetY(image_buffer *im,encode_state *enc, int m1)
{
	offsetY_non_LL(im);
	
	if (im->setup->quality_setting>LOW4) {
		offsetY_LL_q4(im);
	}

	offsetY_process(im, m1);
}

void encode_y_simplified(image_buffer *im, encode_state *enc, int ratio)
{
	int quality = im->setup->quality_setting;
	short *res256, *resIII;
	short *pr;
	char wvlt[7];
	pr = im->im_process;
	int quad_size = im->fmt.end / 4;

	int n = im->fmt.tile_size; // line size Y

	// Buffer for first stage WL transform (LL)
	res256 = (short*) calloc((quad_size + n), sizeof(short));

	// Buffer for second stage WL transform, all quadrants
	resIII = (short*) malloc(quad_size*sizeof(short));
	enc->tree1 = (unsigned char*)  calloc(((48*n)+4),sizeof(char));
	enc->exw_Y = (unsigned char*)  malloc(32*n*sizeof(char));
	enc->res_ch= (unsigned char *) calloc((quad_size>>2),sizeof(char));

	custom_init_lut(g_lut, 8);
#ifdef HAVE_NETPP
	virtfb_init(n, n, g_lut);
#endif

	// This always places the result in pr:
	wavelet_analysis(im, n, FIRST_STAGE, 1);                  // CAN_HW
	WRITE_IMAGE16("wl1.png", im->im_process, n, CONV_LL);
	// Copy DC subband:
	copy_from_quadrant(res256, im->im_jpeg, n, n);  // CAN_HW

	WRITE_IMAGE16("res256.png", res256, n/2, CONV_GRAY);

	im->setup->RES_HIGH=0;
	wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);             // CAN_HW

	WRITE_IMAGE16("wl2.png", im->im_process, n, CONV_LL);

	configure_wvlt(quality, wvlt);                  // CAN_HW

#ifdef NOT_CRUCIAL_QUALITY_GREATER_THAN_LOW14
	if (quality > LOW14) // Better quality than LOW14?
	{
		// Tag all values in a specific threshold range with an offset.
		// The tags are applied to the res256 array, process is untouched.
		//
		// range:    ...  | -7 .. -5  | -4 .. -1 | 2 .. 4 |  5 .. 7  | ...
		// offset:  (T16) |    T12    |     -    | (T12)  |   T16    | (T12)
		// ( ) : dependent on special condition:
		// When outside this range, tag them according to its modulo 8:
		//  MOD8(abs(val)):      7 (val<0)      1 (val > 0)
		//  offset:              T16               T12
		//
		// FIXME: We should probably use a separate tag buffer
		// to save cycles
		tag_thresh_ranges(im, res256);

		WRITE_IMAGE16("thresh.png", res256, n / 2, CONV_GRAY);

		offsetY_recons256(im,enc,ratio,1);
		// Drops result in im_process:
		wl_synth_luma(im, n>>1, FIRST_STAGE);
		// Modifies all 3 buffers:
		revert_compensate_offsets(im, res256);
		WRITE_IMAGE16("compens.png", res256, n / 2, CONV_GRAY);
		wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);
		WRITE_IMAGE16("wl_feedback.png", im->im_process, n, CONV_LL);

	}

	reduce_lowres_generic(im, wvlt, ratio);
#endif

	// Copy process array second stage LL LH HL HH quads into resIII:
	copy_from_quadrant(resIII, pr, n, n);           // CAN_HW
	WRITE_IMAGE16("resIII.png", resIII, n / 2, CONV_LL);

	WRITE_IMAGE16("wl_yres.png", im->im_process, n, CONV_LL);
	tree_compress(im, enc);                // CAN_HW (< LOW3)
	Y_highres_compression(im, enc);  // Very complex. TODO: Simplify

	copy_to_quadrant(pr, resIII, n, n);             // CAN_HW

#ifdef NOT_CRUCIAL_QUALITY_BETTER_THAN_LOW8
	if (quality > LOW8) { // Better than LOW8?
		// Operates on DC quad: (LL):
		offsetY_recons256(im, enc, ratio, 0); // FIXME: Eliminate 'part'
		//offsetY_recons256_part0(im, enc, ratio);

		// Drops result in im_process:
		wl_synth_luma(im, n>>1, FIRST_STAGE);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(quad_size*sizeof(short));
			copy_from_quadrant(im->im_wavelet_first_order, im->im_jpeg, n, n);
		}
	}
#endif

	WRITE_IMAGE16("reduced0.png", im->im_process, n, 0);
	
	// If LOW3 or better, im_process pixels are tagged by this one:
	reduce_generic(im, resIII, wvlt, enc, ratio); // CAN_HW
	WRITE_IMAGE16("reduced1.png", im->im_process, n, 0);

#ifdef HAVE_NETPP
	virtfb_set((unsigned short *) im->im_process);
#endif

#ifdef NOT_CRUCIAL_QUALITY_BETTER_THAN_LOW8
	// Residual coding stage:
	if (quality > LOW8) {
		ResIndex *highres;
		// This is a big complex function, operating on the LL first stage (DC)
		// quadrant.
		// In first pass, res256 are tagged with CODE_* values
		// Second pass reverts these back to the corresponding TAGs
		// Basically, it looks at the differences from the quantization/reduction and
		// tags all values with the 'fixup' CODE that are outside a specific tolerance.
		preprocess_res_q8(im, res256, enc);

		WRITE_IMAGE16("rtags.png", res256, n / 2, CONV_GRAY);

		highres=(ResIndex*)calloc(((48*im->fmt.tile_size)+1),sizeof(ResIndex));

		process_residuals(im, highres, res256, enc); // CAN_HW

		free(highres);
	}
#endif

	// Copy resIII quadrants to process array:
	// - LL is copied only, when resIII greater than threshold (8000),
	//   else it's nulled
	// - All other subbands are copied without condition
	//

	WRITE_IMAGE16("r3_thresh.png", resIII, n / 2, CONV_LL);

	copy_thresholds(pr, resIII, im->fmt.end / 4, n);
	// again, quantize AC sub bands if smaller than above threshold:

	quant_ac_final(im, ratio, lookup_ywlthreshold(quality));   // CAN_HW
	WRITE_IMAGE16("ac.png", im->im_process, n, CONV_LL);
	// Add offset of 128 and do some more processing:
	// This also fixes up the tagged pixels from reduce_generic()
	offsetY(im,enc,ratio);                          // CAN_HW, complex

	WRITE_IMAGE16("offset.png", im->im_process, n, CONV_GRAY);

#ifdef HAVE_NETPP
	virtfb_close();
#endif
	free(enc->res_ch);
}


/*
 * Some gathered details:
 *
 * An image block is filtered during a wavelet_analysis() stage as follows:
 *
 *
 *  +-------+-------+               +-------+-------+
 *  +       |       +               +LL2|   |   |   +
 *  +  LL1  |       +               +---+---+---+---+
 *  +       |       +               +   |   |   |   +
 *  +-------+-------+               +-------+-------+
 *  +       |       +               +   |   |   |   +
 *  +       |       +               +---+---+---+---+
 *  +       |       +               +   |   |   |   +
 *  +-------+-------+               +-------+-------+
 *      STAGE1                           STAGE2
 *
 */


void encode_y(image_buffer *im, encode_state *enc, int ratio)
{

	int quality = im->setup->quality_setting;
	short *res256, *resIII;
	ResIndex *highres;
	short *pr;
	char wvlt[7];
	pr=(short*)im->im_process;

	// First order quadrant size:
	int quad_size = im->fmt.end / 4;
	int n = im->fmt.tile_size;

	custom_init_lut(g_lut, 20);

	// This always places the result in pr:
	wavelet_analysis(im, n, FIRST_STAGE, 1);

	// WRITE_IMAGE16("wl1.png", im->im_process, n, 0);

	// Add some head room for padding (PAD is initialized to 0!)
	res256 = (short*) calloc((quad_size + n), sizeof(short));
	resIII = (short*) malloc(quad_size*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);

	// WRITE_IMAGE16("wl2.png", im->im_process, n, 0);

	if (quality > LOW14) // Better quality than LOW14?
	{
		tag_thresh_ranges(im, res256);

		// WRITE_IMAGE16("res_p14.png", res256, n / 2, 0);

		offsetY_recons256(im,enc,ratio,1);
		// Drops result in im_process:
		wl_synth_luma(im, n>>1, FIRST_STAGE);
		// Modifies all 3 buffers:
		revert_compensate_offsets(im, res256);
		// WRITE_IMAGE16("res_p14_pp.png", res256, n / 2, 0);
		wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);
	}
	
	reduce_lowres_generic(im, wvlt, ratio);

	copy_from_quadrant(resIII, pr, n, n);
	
	enc->tree1=(unsigned char*) calloc(((48*im->fmt.tile_size)+4),sizeof(char));
	enc->exw_Y=(unsigned char*) malloc(32*im->fmt.tile_size*sizeof(char));
	

	enc->res_ch=(unsigned char *)calloc((quad_size>>2),sizeof(char));

	// This fills the tree structures and coordinate index
	// arrays for tagged values in the im_process array. Tagged values are
	// reverted.
	tree_compress(im, enc);

	Y_highres_compression(im, enc);

	free(enc->res_ch);

	copy_to_quadrant(pr, resIII, n, n);

	if (quality > LOW8) { // Better than LOW8?
		// Operates on DC quad: (LL):
		offsetY_recons256(im, enc, ratio, 0); // FIXME: Eliminate 'part'
		//offsetY_recons256_part0(im, enc, ratio);

		// Drops result in im_process:
		wl_synth_luma(im, n>>1, FIRST_STAGE);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(quad_size*sizeof(short));
			copy_from_quadrant(im->im_wavelet_first_order, im->im_jpeg, n, n);
		}
	}

	// After last call of offsetY_recons256() we can free:
	free(enc->highres_mem);
	
	// Release original data buffer
	//
	free(im->im_jpeg); // XXX

////////////////////////////////////////////////////////////////////////////

	reduce_generic(im, resIII, wvlt, enc, ratio);

	// WRITE_IMAGE16("reduced.png", im->im_process, n, 0);

	if (quality > LOW8)
	{
		// This is a big complex function.
		// In first pass, res256 are tagged with CODE_* values
		// Second pass reverts these back to the corresponding TAGs
		preprocess_res_q8(im, res256, enc);

		highres=(ResIndex*)calloc(((48*im->fmt.tile_size)+1),sizeof(ResIndex));

		process_residuals(im, highres, res256, enc);

		free(highres);
	}
	
	free(res256);

	copy_thresholds(pr, resIII, im->fmt.end / 4, n);

	free(resIII);
	SWAPOUT_FUNCTION(quant_ac_final)(im, ratio, lookup_ywlthreshold(quality));
	// Quantize DC quadrant:
	SWAPOUT_FUNCTION(offsetY)(im,enc,ratio);

	if (quality>HIGH1) {
		im->im_wavelet_band=(short*)calloc(quad_size,sizeof(short));
		// These two functions use .im_wavelet_band:
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
		free(im->im_wavelet_first_order);
		free(im->im_wavelet_band);
		free(im->im_quality_setting);
	}
}


void encode_image(image_buffer *im,encode_state *enc, int ratio)
{
	int res_uv;

	if (im->setup->quality_setting > LOW3) res_uv=4;
	else                                   res_uv=5;


	im->im_process=(short*) calloc(im->fmt.end,sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

#ifdef NOT_CRUCIAL
	if (im->setup->quality_setting<HIGH2) 
	{
		pre_processing(im);
	}
#endif

	if (g_encconfig.testmode) {
		// printf("Running in test mode (simplified pipeline)\n");
		encode_y_simplified(im, enc, ratio);
	} else {
		encode_y(im, enc, ratio);
	}

	im->im_nhw=(unsigned char*)calloc(im->fmt.end * 3 / 2,sizeof(char));

	// This fills the im_nhw array with the shuffled value packs:
	code_y_chunks(im, im->im_nhw, enc);

	free(im->im_process);
	
////////////////////////////////////////////////////////////////////////////	

	int uv_size = im->fmt.end / 4;

	im->im_process=(short*)calloc(uv_size,sizeof(short)); // {
	im->im_jpeg=(short*)calloc(uv_size,sizeof(short)); // Work buffer {

	encode_uv(im, enc, ratio, res_uv, 0);
	copy_uv_chunks(&im->im_nhw[im->fmt.end], im->im_process, im->fmt.tile_size / 2);
	free(im->im_bufferU); // Previously reserved buffer XXX

	encode_uv(im, enc, ratio, res_uv, 1);
	copy_uv_chunks(&im->im_nhw[im->fmt.end+1], im->im_process, im->fmt.tile_size / 2);
	free(im->im_bufferV); // Previously reserved buffer XXX

	free(im->im_jpeg); // Free Work buffer }

	free(im->im_process); // }

	// This frees previously allocated enc->tree[1,2]:
	// ..and creates a new im->res_ch buffer (FIXME):
	highres_compression(im, enc);
	free(enc->tree1);
	free(enc->highres_comp);

	// This creates new enc->tree structures
#ifndef BYPASS
	wavlts2packet(im,enc);

	free(im->im_nhw);
#endif
}


