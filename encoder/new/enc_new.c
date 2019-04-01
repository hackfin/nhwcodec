#include <getopt.h>
#include <stdint.h>
#include "utils.h"
#include "imgio.h"
#include "codec.h"
#include "remote.h"


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
} g_encconfig = {
	.tilepower = 9
};

void init_lut(uint8_t *lut, float gamma);

void Y_highres_compression(image_buffer *im,encode_state *enc);

const short *lookup_ywlthreshold(int quality);

void write_image(const char *filename, uint8_t *buf)
{
	FILE *out = fopen(filename, "wb");
	int s = 1 << g_encconfig.tilepower;
	if (out) {
		write_png(out, buf, s, s, 0);
	} else {
		perror("Opening file");
	}
}

uint8_t g_lut[0x10000 * 3];

void write_image16(const char *filename, const int16_t *buf, int tilesize, int norm)
{
	uint8_t *buf8;
	uint8_t *dst;
	uint8_t *rgb;
	uint16_t tmp;
	int n = tilesize * tilesize;
	buf8 = malloc(3 * n * sizeof(uint8_t));
	dst = buf8;
	if (norm) {
		while (n--) {
			tmp = *buf++;
			if (tmp < 0) {
				*dst++ = 0xff; *dst++ = 0; *dst++ = 0;
			} else {
				*dst++ = tmp >> 8;
				*dst++ = tmp >> 8;
				*dst++ = tmp >> 8;
			}
		}

	} else {
		while (n--) {
			tmp = *buf++;
			rgb = &g_lut[tmp * 3];
			*dst++ = rgb[0];
			*dst++ = rgb[1];
			*dst++ = rgb[2];
		}
	}

	FILE *out = fopen(filename, "wb");
	if (out) {
		write_png(out, buf8, tilesize, tilesize, 1);
	} else {
		perror("Opening file");
	}

	free(buf8);
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
	int IM_SIZE = im->fmt.end / 4;

	int n = im->fmt.tile_size; // line size Y
	res256 = (short*) calloc((IM_SIZE + n), sizeof(short));
	resIII = (short*) malloc(IM_SIZE*sizeof(short));
	enc->tree1 = (unsigned char*) calloc(((48*n)+4),sizeof(char));
	enc->exw_Y = (unsigned char*) malloc(16*n*sizeof(short)); // CHECK: short??
	enc->ch_res=(unsigned char*)calloc((IM_SIZE>>2),sizeof(char));

	init_lut(g_lut, 0.7);
	virtfb_init(512, 512);

	// This always places the result in pr:
	wavelet_analysis(im, n, 0, 1);                  // CAN_HW
	// copy LL1
	copy_from_quadrant(res256, im->im_jpeg, n, n);  // CAN_HW

	im->setup->RES_HIGH=0;
	wavelet_analysis(im, n >> 1, 1, 1);             // CAN_HW

	configure_wvlt(quality, wvlt);                  // CAN_HW
	copy_from_quadrant(resIII, pr, n, n);           // CAN_HW

	compress_q(im, enc);                // CAN_HW (< LOW3)
	Y_highres_compression(im, enc);  // Very complex. TODO: Simplify

	copy_to_quadrant(pr, resIII, n, n);             // CAN_HW
	reduce_generic(im, resIII, wvlt, enc, ratio);

	copy_thresholds(pr, resIII, im->fmt.end / 4, n);
	ywl(im, ratio, lookup_ywlthreshold(quality));
	offsetY(im,enc,ratio);                          // CAN_HW, complex

	virtfb_close();
	free(enc->ch_res);
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
	int res;
	int end_transform;
	short *res256, *resIII;
	// unsigned char *highres;
	short *pr;
	char wvlt[7];
	pr=(short*)im->im_process;

	int n = im->fmt.tile_size; // line size Y

	init_lut(g_lut, 0.7);
	virtfb_init(512, 512);

	end_transform=0;

	// This always places the result in pr:
	wavelet_analysis(im, n, end_transform++, 1);

	write_image16("/tmp/wl1.png", im->im_process, 512, 0);

	virtfb_set((unsigned short *) im->im_process);

	int quad_size = im->fmt.end / 4;

	res256 = (short*) calloc((quad_size + n), sizeof(short));
	resIII = (short*) malloc(quad_size*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, end_transform,1);

	write_image16("/tmp/wl2.png", im->im_process, 512, 0);

#ifdef CRUCIAL
	if (quality > LOW14) // Better quality than LOW14?
	{
		preprocess14(res256, pr);

		write_image16("/tmp/res_p14.png", res256, 256, 0);

		offsetY_recons256(im,enc,ratio,1);
		wavelet_synthesis(im, n>>1, end_transform-1,1);
		write_image16("/tmp/syn_stage1.png", im->im_process, 512, 0);
		// Modifies all 3 buffers:
		postprocess14(im->im_jpeg, pr, res256);
		wavelet_analysis(im, n >> 1, end_transform,1);
	}
	
	if (quality <= LOW9) // Worse than LOW9?
	{
		if (quality > LOW14) wvlt[0] = 10; else wvlt[0] = 11;
		reduce_LH_q9(pr, wvlt, ratio, n);
	}
#endif
		
	configure_wvlt(quality, wvlt);

#ifdef CRUCIAL
	if (quality < LOW7) {
			
		reduce_LL_q7(quality, pr, wvlt);
		
		if (quality <= LOW9)
		{
			reduce_LL_q9(pr, wvlt, n);
		}
	}
#endif
	
	copy_from_quadrant(resIII, pr, n, n);
	
	enc->tree1=(unsigned char*) calloc(((48*im->fmt.tile_size)+4),sizeof(char));
	enc->exw_Y=(unsigned char*) malloc(16*im->fmt.tile_size*sizeof(short)); // CHECK: short??
	
	if (quality > LOW3)
	{
		res = process_res_q3(im);
		enc->nhw_res4_len=res;
		enc->nhw_res4=(unsigned char*)calloc(enc->nhw_res4_len,sizeof(char));
	}

	enc->ch_res=(unsigned char*)calloc((quad_size>>2),sizeof(char));

	compress1(im, enc);

	Y_highres_compression(im, enc);

	free(enc->ch_res);

	copy_to_quadrant(pr, resIII, n, n);

	
	if (quality > LOW8) { // Better than LOW8?

		offsetY_recons256(im, enc, ratio, 0);

		wavelet_synthesis(im,n>>1,end_transform-1,1);

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
	
#ifdef CRUCIAL
	if (quality > LOW8) {
		process_res_q8(quality, pr, res256, enc);
	}
#endif

#ifdef CRUCIAL
	highres=(unsigned char*)calloc(((96*IM_DIM)+1),sizeof(char));

	if (quality > HIGH1) {
		process_res_hq(quality, im->im_wavelet_first_order, res256);
	}
#endif
	
#ifdef CRUCIAL
	if (quality > LOW8)
	{
		process_hires_q8(highres, res256, enc);
	}

	// Further residual processing:
	if (quality>=LOW1) {
		process_res3_q1(highres, res256, enc);
		if (quality>=HIGH1)
		{
			process_res5_q1(highres, res256, enc);
		}
	}
	
	free(highres);
#endif
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
		if (n < sizeof(hq_table)) { setup->quality_setting = hq_table[n]; return 0; }
	} else {
		if (n < sizeof(lq_table)) { setup->quality_setting = lq_table[n]; return 0; }
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
	encode_y_simplified(im, enc, ratio);
	// encode_y(im, enc, ratio);

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
	highres_compression(im, enc);
	free(enc->tree1);
	free(enc->highres_comp);

	// This creates new enc->tree structures
	wavlts2packet(im,enc);

	free(im->im_nhw);
}

int main(int argc, char **argv) 
{
	FILE *image;
	int ret;
	const char *str;
	image_buffer im;
	encode_state enc;
	codec_setup setup;
	int rate = 8;
	const char *output_filename = NULL;
	const char *input_filename;
	char outfile[256];

	im.setup = &setup;
	setup.quality_setting=NORM;

	imgbuf_init(&im, 9);


	while (1) {
		int c;
		int option_index;
		c = getopt_long(argc, argv, "-l:h:s:no:", long_options, &option_index);

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

	setup.colorspace = YUV;
	setup.wavelet_type=WVLTS_53;
	setup.RES_HIGH = 0;
	setup.RES_LOW = 3;
	setup.wvlts_order = 2;

	im.im_buffer4 = (unsigned char*) malloc(3*im.fmt.end*sizeof(char));

	if ((image = fopen(input_filename, "rb")) == NULL ) {
		fprintf(stderr, "\n Could not open file \n");
		exit(-1);
	}

	ret = read_png(image, im.im_buffer4, 1 << g_encconfig.tilepower); 
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
#ifndef NOT_CRUCIAL
		setup.quality_setting = LOW8; // force
#endif
		downsample_YUV420(&im, &enc, rate);

		encode_image(&im, &enc, rate);

		write_compressed_file(&im, &enc, output_filename);

		free(enc.tree1);
		free(enc.tree2);

	} else {
		fprintf(stderr, "Error: %s\n", str);
	}
	return ret;
}

