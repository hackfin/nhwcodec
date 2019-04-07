#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include "utils.h"
#include "imgio.h"
#include "codec.h"
#include "remote.h"

#define FIRST_STAGE  0
#define SECOND_STAGE 1


// #define SIMPLIFIED
#define CRUCIAL


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

enum {
	OPT_HELP,
	OPT_OUTPUT,
};

static struct option long_options[] = {
	{ "help",          0,              0, OPT_HELP },
	{ "output",        0,              0, OPT_OUTPUT },
	{ NULL } // Terminator
};

struct config {
	char *outfilename;
	int tilepower;
	char loopback;
	char verbose;
} g_encconfig = {
	.tilepower = 9,
	.loopback = 0,
};

void init_lut(uint8_t *lut, float gamma);

void Y_highres_compression(image_buffer *im,encode_state *enc);

const short *lookup_ywlthreshold(int quality);

void write_image(const char *filename, uint8_t *buf)
{
	FILE *out = fopen(filename, "wb");
	int s = 1 << g_encconfig.tilepower;
	if (out) {
		write_png(out, buf, s, s, 0, 0);
	} else {
		perror("Opening file");
	}
}

uint8_t g_lut[0x10000 * 3];

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
		case 0:
			for (i = 0; i < tilesize / 2; i++) {
			// Don't use LUT for LL quadrant
				for (j = 0; j < tilesize / 2; j++) {
					tmp = *buf++;
					*dst++ = tmp; *dst++ = tmp; *dst++ = tmp;
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
		case 1:
			for (i = 0; i < tilesize * tilesize; i++) {
				tmp = *buf++;
				rgb = &g_lut[tmp * 3];
				*dst++ = rgb[0]; *dst++ = rgb[1]; *dst++ = rgb[2];
			}
			break;
		case 2:
			for (i = 0; i < tilesize * tilesize; i++) {
				tmp = *buf++;
				*dst++ = tmp; *dst++ = tmp; *dst++ = tmp;
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
		write_png(out, buf8, tilesize, tilesize, 1, 1);
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

void custom_init_lut(uint8_t *lut, int thresh)
{
	int i;
	uint8_t *p;
	memset(lut, 0, sizeof(g_lut));

	int f = 255 / thresh;

	for (i = 0; i < thresh; i++) {
		p = &lut[3*i];
		p[0] = 0;
		p[1] = i * f;
		p[2] = 0;
	}

	for (i = thresh; i < 256; i++) {
		p = &lut[3*i];
		p[0] = 0;
		p[1] = 0;
		p[2] = 255;
	}

	for (i = 0xff00; i < 0xfff0; i++) {
		p[0] = 255;
		p[1] = 0;
		p[2] = 255;
	}

	for (i = 0xfff0; i < 0xffff; i++) {
		p = &lut[3*i];
		p[0] = (0xffff - i) * 8 + 32;
		p[1] = 0;
		p[2] = 0;
	}

	uint8_t rgb[3];


	rgb[0] = 220; rgb[1] = 220; rgb[2] = 0;
	set_range(lut, 10000, 12000, rgb);
	rgb[0] = 0; rgb[1] = 220; rgb[2] = 220;
	set_range(lut, 12000, 16000, rgb);
	rgb[0] = 220; rgb[1] = 220; rgb[2] = 220;
	set_range(lut, 16000, 18000, rgb);

}

/* Substantially 'broken' simplified encoder function for fixed quality
 */

void encode_y_simplified(image_buffer *im, encode_state *enc, int ratio)
{
	int quality = im->setup->quality_setting;
	short *res256, *resIII;
	short *pr;
	char wvlt[7];
	pr = im->im_process;
	int quad_size = im->fmt.end / 4;
	ResIndex *highres;

	int n = im->fmt.tile_size; // line size Y

	// Buffer for first stage WL transform (LL)
	res256 = (short*) calloc((quad_size + n), sizeof(short));

	// Buffer for second stage WL transform, all quadrants
	resIII = (short*) malloc(quad_size*sizeof(short));
	enc->tree1 = (unsigned char*) calloc(((48*n)+4),sizeof(char));
	enc->exw_Y = (unsigned char*) malloc(16*n*sizeof(short)); // CHECK: short??
	enc->res_ch=(unsigned char *)calloc((quad_size>>2),sizeof(char));

	custom_init_lut(g_lut, 20);
	virtfb_init(n, n, g_lut);

	// This always places the result in pr:
	wavelet_analysis(im, n, FIRST_STAGE, 1);                  // CAN_HW
	// copy LL1
	// Unused in this compression mode:
	copy_from_quadrant(res256, im->im_jpeg, n, n);  // CAN_HW

	write_image16("/tmp/res256.png", res256, n / 2, 2);

	im->setup->RES_HIGH=0;
	wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);             // CAN_HW

	configure_wvlt(quality, wvlt);                  // CAN_HW
	copy_from_quadrant(resIII, pr, n, n);           // CAN_HW
	write_image16("/tmp/resIII.png", resIII, n / 2, 0);

	tree_compress(im, enc);                // CAN_HW (< LOW3)
	Y_highres_compression(im, enc);  // Very complex. TODO: Simplify

	copy_to_quadrant(pr, resIII, n, n);             // CAN_HW
	reduce_generic_simplified(im, resIII, wvlt, enc, ratio); // CAN_HW

	virtfb_set((unsigned short *) im->im_process);

	if (quality > LOW8) {
		// This is a big ugly function.
		// In first pass, res256 are tagged with CODE_* values
		// Second pass reverts these back to the corresponding TAGs
		preprocess_res_q8(im, res256, enc);

		highres=(ResIndex*)calloc(((48*im->fmt.tile_size)+1),sizeof(ResIndex));

		process_residuals(im, highres, res256, enc); // CAN_HW

		free(highres);
	}

	copy_thresholds(pr, resIII, im->fmt.end / 4, n);
	ywl(im, ratio, lookup_ywlthreshold(quality));   // CAN_HW
	offsetY(im,enc,ratio);                          // CAN_HW, complex

	virtfb_close();
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

	// write_image16("/tmp/wl1.png", im->im_process, n, 0);


	// Add some head room for padding (PAD is initialized to 0!)
	res256 = (short*) calloc((quad_size + n), sizeof(short));
	resIII = (short*) malloc(quad_size*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);

	// write_image16("/tmp/wl2.png", im->im_process, n, 0);

#ifdef CRUCIAL
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
		tag_thresh_ranges(im, res256);

		// write_image16("/tmp/res_p14.png", res256, n / 2, 0);

		offsetY_recons256(im,enc,ratio,1);
		// Drops result in im_process:
		wl_synth_luma(im, n>>1, FIRST_STAGE);
		// Modifies all 3 buffers:
		revert_compensate_offsets(im, res256);
		// write_image16("/tmp/res_p14_pp.png", res256, n / 2, 0);
		wavelet_analysis(im, n >> 1, SECOND_STAGE, 1);
	}
#endif
	
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
		// 
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

	// write_image16("/tmp/reduced.png", im->im_process, n, 0);

	if (quality > LOW8)
	{
		// This is a big ugly function.
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
	ywl(im, ratio, lookup_ywlthreshold(quality));
	offsetY(im,enc,ratio);

	if (quality>HIGH1) {
		im->im_wavelet_band=(short*)calloc(quad_size,sizeof(short));
		// These two functions use .im_wavelet_band:
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
		free(im->im_wavelet_first_order);
		free(im->im_wavelet_band);
		free(im->im_quality_setting);
	}

	virtfb_close();

}

const char lq_table[] = {
	LOW1, LOW2, LOW3, LOW4, LOW5, LOW6, 
	LOW7, LOW8, LOW9, LOW10, LOW11, LOW12,
	LOW13, LOW14, LOW15, LOW16, LOW17, LOW18,
	LOW19
};

const char hq_table[] = {
	HIGH1, HIGH2, HIGH3 
};

int set_quality(const char *optarg, codec_setup *setup, char high)
{
	int n = atoi(optarg);
	n--;
	if (n < 0) return -1;
	if (high) {
		if (n < sizeof(hq_table))
			{ setup->quality_setting = hq_table[n]; return 0; }
	} else {
		if (n < sizeof(lq_table))
			{ setup->quality_setting = lq_table[n]; return 0; }
	}
	return -1;
}

void encode_image(image_buffer *im,encode_state *enc, int ratio)
{
	int res_uv;

	if (im->setup->quality_setting > LOW3) res_uv=4;
	else                                   res_uv=5;


	im->im_process=(short*) calloc(im->fmt.end,sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

#ifdef CRUCIAL
	if (im->setup->quality_setting<HIGH2) 
	{
		pre_processing(im);
	}
#endif

#ifdef SIMPLIFIED
	encode_y_simplified(im, enc, ratio);
#else
	encode_y(im, enc, ratio);
#endif

	im->im_nhw=(unsigned char*)calloc(im->fmt.end * 3 / 2,sizeof(char));

	scan_run_code(im, enc);

	free(im->im_process);
	
////////////////////////////////////////////////////////////////////////////	

	int uv_size = im->fmt.end / 4;

	im->im_process=(short*)calloc(uv_size,sizeof(short)); // {
	im->im_jpeg=(short*)calloc(uv_size,sizeof(short)); // Work buffer {

	encode_uv(im, enc, ratio, res_uv, 0);
	free(im->im_bufferU); // Previously reserved buffer XXX

	encode_uv(im, enc, ratio, res_uv, 1);
	free(im->im_bufferV); // Previously reserved buffer XXX

	free(im->im_jpeg); // Free Work buffer }

	free(im->im_process); // }

	// This frees previously allocated enc->tree[1,2]:
	// ..and creates a new im->res_ch buffer (FIXME):
	highres_compression(im, enc);
	free(enc->tree1);
	free(enc->highres_comp);

	// This creates new enc->tree structures
	wavlts2packet(im,enc);

	free(im->im_nhw);
}

void init_decoder(decode_state *dec, encode_state *enc)
{
	memset(dec, 0, sizeof(decode_state));
	dec->d_size_tree1 = enc->size_tree1;
	dec->d_size_tree2 = enc->size_tree2;
	dec->d_size_data1 = enc->size_data1;
	dec->d_size_data2 = enc->size_data2;
	dec->d_tree1 = enc->tree1;
	dec->d_tree2 = enc->tree2;

	dec->tree_end = enc->tree_end;
	dec->exw_Y_end = enc->exw_Y_end;
	dec->nhw_res1_len = enc->res1.len;
	dec->nhw_res3_len = enc->res3.len;
	dec->nhw_res3_bit_len = enc->res3.bit_len;
	dec->nhw_res4_len = enc->nhw_res4_len;
	dec->nhw_res1_bit_len = enc->res1.bit_len;
	dec->nhw_res5_len = enc->res5.len;
	dec->nhw_res5_bit_len = enc->res5.bit_len;
	dec->nhw_res6_len = enc->nhw_res6_len;
	dec->nhw_res6_bit_len = enc->nhw_res6_bit_len;
	dec->nhw_char_res1_len = enc->nhw_char_res1_len;
	dec->qsetting3_len = enc->qsetting3_len;
	dec->nhw_select1 = enc->nhw_select1;
	dec->nhw_select2 = enc->nhw_select2;
	dec->highres_comp_len = enc->highres_comp_len;
	dec->end_ch_res = enc->end_ch_res;
	dec->exw_Y = enc->exw_Y;
	dec->nhw_res1 = enc->res1.res;
	dec->nhw_res1_bit = enc->res1.res_bit;
	dec->nhw_res1_word = enc->res1.res_word;
	dec->nhw_res4 = enc->nhw_res4;
	dec->nhw_res3 = enc->res3.res;
	dec->nhw_res3_bit = enc->res3.res_bit;
	dec->nhw_res3_word = enc->res3.res_word;
	dec->nhw_res5 = enc->res5.res;
	dec->nhw_res5_bit = enc->res5.res_bit;
	dec->nhw_res5_word = enc->res5.res_word;
	dec->nhw_res6 = enc->nhw_res6;
	dec->nhw_res6_bit = enc->nhw_res6_bit;
	dec->nhw_res6_word = enc->nhw_res6_word;
	dec->nhw_char_res1 = enc->nhw_char_res1;
	dec->high_qsetting3 = enc->high_qsetting3;
	dec->nhw_select_word1 = enc->nhw_select_word1;
	dec->nhw_select_word2 = enc->nhw_select_word2;
	dec->res_U_64 = enc->res_U_64;
	dec->res_V_64 = enc->res_V_64;
	dec->highres_comp = enc->highres_word;
	dec->res_ch = enc->res_ch;
	dec->packet1 = enc->encode;
	dec->packet2 = &enc->encode[enc->size_data1];
}

void dump_stats(image_buffer *im, encode_state *enc)
{
	int n = 0;

	// int q = im->setup->quality_setting;

	n += enc->size_tree1;
	n += enc->size_tree2;
	// n += enc->size_data1; // unused
	n += enc->size_data2 * 4;
	n += enc->tree_end;
	n += enc->exw_Y_end;
	n += enc->res3.len;
	n += enc->res3.bit_len;
	n += enc->nhw_res4_len;
	n += enc->res1.bit_len;
	n += enc->res5.len;
	n += enc->res5.bit_len;
	n += enc->nhw_res6_len;
	n += enc->nhw_res6_bit_len;
	n += enc->nhw_char_res1_len * 2;
	n += enc->qsetting3_len * 4;
	n += enc->nhw_select1;
	n += enc->nhw_select2;
	n += enc->highres_comp_len;
	n += enc->end_ch_res;

	printf("Effective payload size: %d, compression rate: %0.4f\n",
		n, (float) n / (float) im->fmt.end);
	
}

int main(int argc, char **argv) 
{
	FILE *image;
	int ret;
	const char *str;
	image_buffer im;
	encode_state enc;
	decode_state dec;
	codec_setup setup;
	int rate = 8;
	const char *output_filename = NULL;
	const char *input_filename;
	char outfile[256];

	setup.quality_setting = NORM;
	setup.colorspace = YUV;
	setup.wavelet_type = WVLTS_53;
	setup.RES_HIGH = 0;
	setup.RES_LOW = 3;
	setup.wvlts_order = 2;

	memset(&enc, 0, sizeof(encode_state)); // Set all to 0
	im.setup = &setup;

	while (1) {
		int c;
		int option_index;
		c = getopt_long(argc, argv, "-l:h:s:Lvno:", long_options, &option_index);

		if (c == EOF) break;
		switch (c) {
			case OPT_HELP:
				fprintf(stderr, "\nOptions:\n"
				"--help                    This text\n"
				);
				return 0;
			case 'o':
				output_filename = optarg;
				break;
			case 'h':
				ret = set_quality(optarg, &setup, 1);
				break;
			case 'n':
				break;
			case 'l':
				ret = set_quality(optarg, &setup, 0);
				break;
			case 'L':
				g_encconfig.loopback = 1;
				break;
			case 'v':
				g_encconfig.verbose = 1;
				break;
			case 's':
				g_encconfig.tilepower = atoi(optarg);
				break;
			default:
				input_filename = optarg;
		}
	}

	if (output_filename == NULL ) {
		sprintf(outfile, "%s.nhw", input_filename);
		fprintf(stderr, "Output file not specified, using %s\n", outfile);
		output_filename = outfile;
	}


	imgbuf_init(&im, g_encconfig.tilepower);

	im.im_buffer4 = (unsigned char*) malloc(3*im.fmt.end*sizeof(char));

	if ((image = fopen(input_filename, "rb")) == NULL ) {
		fprintf(stderr, "\n Could not open file \n");
		exit(-1);
	}

	ret = read_png(image, im.im_buffer4, im.fmt.tile_size); 
	fclose(image);

	switch (ret) {
		case 0: break; // No error
		case -1:
			str = "Bad size"; break;
		case -2:
			str = "Bad header"; break;
		default:
			str = "Unknown error";
	}

	if (ret == 0) {

#ifdef SIMPLIFIED
		setup.quality_setting = LOW8; // force
#endif
		downsample_YUV420(&im, &enc, rate);

		encode_image(&im, &enc, rate);

		if (g_encconfig.loopback) {
			FILE *png_out;
			// Initialize decoder with encoder values:
			init_decoder(&dec, &enc);
			dec.res_comp=(ResIndex *)calloc((48*im.fmt.tile_size+1),sizeof(ResIndex));
			im.im_process=(short*)calloc(im.fmt.end,sizeof(short));

			process_hrcomp(&im, &dec);
			decode_image(&im, &dec);
			im.im_buffer4 = (unsigned char*) malloc(3*im.fmt.end*sizeof(char));
			yuv_to_rgb(&im);

			png_out = fopen(output_filename, "wb");
			int s = im.fmt.tile_size;
			if (png_out != NULL) {
				write_png(png_out, im.im_buffer4, s, s, 1, 1);
				fclose(png_out);
			} else {
				perror("opening file for writing");
			}

			if (g_encconfig.verbose) {
				dump_stats(&im, &enc);
			}
			free(im.im_bufferY); // malloced by decode_image()
			free(im.im_bufferU);
			free(im.im_bufferV);
			free(im.im_buffer4);
			free(dec.packet1); // HACK
		} else {
			write_compressed_file(&im, &enc, output_filename);
			free(enc.tree1);
			free(enc.tree2);
		}

	} else {
		fprintf(stderr, "Error: %s\n", str);
	}
	return ret;
}

