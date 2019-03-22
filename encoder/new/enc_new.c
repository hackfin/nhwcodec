#include <getopt.h>
#include <stdint.h>
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


void init_lut(uint8_t *lut, float gamma);

void write_image(const char *filename, const uint8_t *buf)
{
	FILE *out = fopen(filename, "wb");
	if (out) {
		write_png(out, buf, 512, 512, 0);
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
	unsigned char *highres;
	short *pr;
	char wvlt[7];
	pr=(short*)im->im_process;

	int n = 2 * IM_DIM; // line size Y

	init_lut(g_lut, 0.7);
	virtfb_init(512, 512);

	end_transform=0;
	// Unused:
	// wavelet_order=im->setup->wvlts_order;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,(2*IM_DIM)>>stage,end_transform++,1); 

	// This always places the result in pr:
	wavelet_analysis(im, n, end_transform++, 1);

	write_image16("/tmp/wl1.png", im->im_process, 512, 0);

	virtfb_set(im->im_process);

	res256 = (short*) calloc((IM_SIZE + n), sizeof(short));
	resIII = (short*) malloc(IM_SIZE*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, end_transform,1);

	write_image16("/tmp/wl2.png", im->im_process, 512, 0);

	if (quality > LOW14) // Better quality than LOW14?
	{
		preprocess14(res256, pr);

		write_image16("/tmp/res_p14.png", res256, 256, 0);

		offsetY_recons256(im,enc,ratio,1);
		wavelet_synthesis(im, n>>1, end_transform-1,1);
		// Modifies all 3 buffers:
		postprocess14(im->im_jpeg, pr, res256);
		wavelet_analysis(im, n >> 1, end_transform,1);
	}
	
	if (quality <= LOW9) // Worse than LOW9?
	{
		if (quality > LOW14) wvlt[0] = 10; else wvlt[0] = 11;
		reduce_q9_LH(pr, wvlt, ratio, n);
	}
	
	configure_wvlt(quality, wvlt);

	if (quality < LOW7) {
			
		reduce_q7_LL(quality, pr, wvlt);
		
		if (quality <= LOW9)
		{
			reduce_q9_LL(pr, wvlt, n);
		}
	}
	
	copy_from_quadrant(resIII, pr, n, n);
	
	enc->tree1=(unsigned char*) calloc(((96*IM_DIM)+4),sizeof(char));
	enc->exw_Y=(unsigned char*) malloc(32*IM_DIM*sizeof(short));
	
	if (quality > LOW3)
	{
		res = process_res_q3(pr);
		enc->nhw_res4_len=res;
		enc->nhw_res4=(unsigned char*)calloc(enc->nhw_res4_len,sizeof(char));
	}

	enc->ch_res=(unsigned char*)calloc((IM_SIZE>>2),sizeof(char));

	compress1(quality, pr, enc);

	Y_highres_compression(im, enc);

	free(enc->ch_res);

	copy_to_quadrant(pr, resIII, n, n);

	
	if (quality > LOW8) { // Better than LOW8?

		offsetY_recons256(im, enc, ratio, 0);

		wavelet_synthesis(im,n>>1,end_transform-1,1);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(IM_SIZE*sizeof(short));
			copy_from_quadrant(im->im_wavelet_first_order, im->im_jpeg, n, n);
		}
	}

	// After last call of offsetY_recons256() we can free:
	free(enc->highres_mem);
	
	// Release original data buffer
	//
	free(im->im_jpeg); // XXX

////////////////////////////////////////////////////////////////////////////

	reduce_generic(quality, resIII, pr, wvlt, enc, ratio);
	
	if (quality > LOW8) {
		process_res_q8(quality, pr, res256, enc);
	}

	highres=(unsigned char*)calloc(((96*IM_DIM)+1),sizeof(char));

	if (quality > HIGH1) {
		process_res_hq(quality, im->im_wavelet_first_order, res256);
	}
	
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
	free(res256);

	copy_thresholds(pr, resIII, n);

	free(resIII);
	ywl(quality, pr, ratio);
	offsetY(im,enc,ratio);

	if (quality>HIGH1) {
		im->im_wavelet_band=(short*)calloc(IM_SIZE,sizeof(short));
		// These two functions use .im_wavelet_band:
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
		free(im->im_wavelet_first_order);
		free(im->im_wavelet_band);
		free(im->im_quality_setting);
	}

	virtfb_close();

}

struct config {
	char *outfilename;
} g_encconfig;

const char lq_table[] = {
	LOW1, LOW2, LOW3, LOW4, LOW5, LOW6, 
	LOW7, LOW8, LOW9, LOW10, LOW11, LOW12,
	LOW13, LOW14, LOW15, LOW16, LOW17, LOW18,
	LOW19
};

const char hq_table[] = {
	HIGH1, HIGH2, HIGH3 
};

int set_quality(const char *optarg, char high)
{
	int n = atoi(optarg);
	n--;
	if (n < 0) return -1;
	if (high) {
		if (n < sizeof(hq_table)) return hq_table[n];
	} else {
		if (n < sizeof(lq_table)) return lq_table[n];
	}
	return -1;
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

	while (1) {
		int c;
		int option_index;
		c = getopt_long(argc, argv, "-l:h:no:", long_options, &option_index);

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
				ret = set_quality(optarg, 1);
				break;
			case 'n':
				break;
			case 'l':
				ret = set_quality(optarg, 0);
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

	im.im_buffer4 = (unsigned char*) malloc(4*3*IM_SIZE*sizeof(char));

	if ((image = fopen(input_filename, "rb")) == NULL ) {
		fprintf(stderr, "\n Could not open file \n");
		exit(-1);
	}

	ret = read_png(image, im.im_buffer4, 512); 
	fclose(image);

	switch (ret) {
		case -1:
			str = "Bad size"; break;
		case -2:
			str = "Bad header"; break;
		default:
			str = "Unknown error";
	}

	if (ret >= 0) {
		downsample_YUV420(&im, &enc, rate);
	} else {
		fprintf(stderr, "Error: %s\n", str);
	}

	encode_image(&im, &enc, rate);

	write_compressed_file(&im, &enc, output_filename);

	free(enc.tree1);
	free(enc.tree2);
}

