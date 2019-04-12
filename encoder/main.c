#include "imgio.h"
#include "codec.h"
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

int encode_tiles(image_buffer *im, const unsigned char *img, int width, int height,
	const char *output_filename, int rate);

int nhw_encode(image_buffer *im, const char *output_filename, int rate);

enum {
	OPT_HELP,
	OPT_OUTPUT,
};

static struct option long_options[] = {
	{ "help",          0,              0, OPT_HELP },
	{ "output",        0,              0, OPT_OUTPUT },
	{ NULL } // Terminator
};

struct config g_encconfig = {
	.tilepower = 9,
	.testmode = 0,
	.debug = 0,
	.debug_tile_x = 0,
	.debug_tile_y = 0,
	.loopback = 0,
};

void copy_to_tile(image_buffer *im, const unsigned char *img, int width)
{
	int i;
	int s = 3 * im->fmt.tile_size;
	int n = 3 * im->fmt.end;

	for (i = 0; i < n; i += s) {
		memcpy(&im->im_buffer4[i], img, s);
		img += width;
	}
}

void copy_from_tile(unsigned char *img, const image_buffer *im, int width)
{
	int i;
	int s = 3 * im->fmt.tile_size;
	int n = 3 * im->fmt.end;

	for (i = 0; i < n; i += s) {
		memcpy(img, &im->im_buffer4[i], s);
		img += width; // next line
	}
}

static
void init_decoder(decode_state *dec, encode_state *enc, int q)
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

	// 'fallthrough' switch case, missing break's by intention!
	switch (q) {
		case HIGH3:
			dec->qsetting3_len = enc->qsetting3_len;
		case HIGH2:
			dec->nhw_res6_len = enc->nhw_res6_len;
			dec->nhw_res6_bit_len = enc->nhw_res6_bit_len;
			dec->nhw_res6 = enc->nhw_res6;
			dec->nhw_res6_bit = enc->nhw_res6_bit;
			dec->nhw_res6_word = enc->nhw_res6_word;
		case HIGH1:
			*(&enc->res5) = *(&dec->res5);
		case NORM:
			dec->nhw_char_res1_len = enc->nhw_char_res1_len;
			dec->nhw_char_res1 = enc->nhw_char_res1;
		case LOW1:
			*(&enc->res3) = *(&dec->res3);
		case LOW2:
			dec->nhw_res4_len = enc->nhw_res4_len;
			dec->nhw_res4 = enc->nhw_res4;
		case LOW3:
		case LOW4:
			dec->highres_comp_len = enc->highres_comp_len;
		case LOW5:
		case LOW6:
		case LOW7:
			*(&enc->res1) = *(&dec->res1);
		default:
			break;
	}

	dec->nhw_select1 = enc->nhw_select1;
	dec->nhw_select2 = enc->nhw_select2;
	dec->end_ch_res = enc->end_ch_res;
	dec->exw_Y = enc->exw_Y;
	dec->high_qsetting3 = enc->high_qsetting3;
	dec->nhw_select_word1 = enc->nhw_select_word1;
	dec->nhw_select_word2 = enc->nhw_select_word2;
	dec->res_U_64 = enc->res_U_64;
	dec->res_V_64 = enc->res_V_64;
	dec->highres_comp = enc->highres_word;
	dec->res_ch = enc->res_ch;
	dec->packet1 = enc->encode;
	dec->packet2 = &enc->encode[enc->size_data1];

	// TODO Need to copy some more, in case we're bypassing decompression:
}

int record_stats(image_buffer *im, encode_state *enc)
{
	int n = 0;

	// int q = im->setup->quality_setting;

	n += enc->size_tree1;
	n += enc->size_tree2;
	// size_data2 is inclusive, skip this:
	// n += enc->size_data1 * 4;
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

	return n;
}

int encode_tiles(image_buffer *im, const unsigned char *img, int width, int height,
	const char *output_filename, int rate)
{
	int i, j;
	encode_state enc;
	decode_state dec;

	unsigned char *out = (unsigned char *) malloc(width * height * 3 * sizeof(char));

	int compressed_length = 0;

	int x, y;
	
	for (i = 0, y = 0; i < height; i += im->fmt.tile_size, y++) {
		for (j = 0, x = 0; j < width; j += im->fmt.tile_size, x++) {
		
			im->im_buffer4 = (unsigned char*) malloc(3*im->fmt.end*sizeof(char));

			memset(&dec, 0, sizeof(decode_state)); // Set all to 0
			memset(&enc, 0, sizeof(encode_state)); // Set all to 0


			if (g_encconfig.verbose) {
				printf("Process tile @(%d, %d)\n", x, y);
			}

			// Turn on debug mode for specific tile
			if (g_encconfig.debug_tile_x == x && g_encconfig.debug_tile_y == y) {
				enc.debug = g_encconfig.debug;
				if (enc.debug) printf("Dump tile\n");
			}

			int offset = 3 * (j + i * width);

			copy_to_tile(im, &img[offset], 3 * width);

			downsample_YUV420(im, &enc, rate);

			encode_image(im, &enc, rate);
			// Important:
			im->setup->RES_HIGH+=im->setup->wavelet_type;

			init_decoder(&dec, &enc, im->setup->quality_setting);
			dec.res_comp=(ResIndex *)calloc((48*im->fmt.tile_size+1),sizeof(ResIndex));
			im->im_process=(short*)calloc(im->fmt.end,sizeof(short));

			process_hrcomp(im, &dec);
#ifdef BYPASS
			decode_image(im, &dec, 1); // bypass compression
#else
			decode_image(im, &dec, 0);
#endif
			im->im_buffer4 = (unsigned char*) malloc(3*im->fmt.end*sizeof(char));
			yuv_to_rgb(im);

			copy_from_tile(&out[offset], im, 3 * width);

			free(im->im_bufferY); // malloced by decode_image()
			free(im->im_bufferU);
			free(im->im_bufferV);
			free(im->im_buffer4);

			compressed_length += record_stats(im, &enc);

		}
	}

	FILE *png_out;
	png_out = fopen(output_filename, "wb");
	if (png_out != NULL) {
		write_png(png_out, out, width, height, 1, 1);
		fclose(png_out);
	} else {
		perror("opening file for writing");
	}

	printf("Effective payload size: %d, compression rate: 1:%0.1f\n",
		compressed_length, (float) height * width * 3.0 / (float) compressed_length );

	return 0;
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
	if (strcmp("norm", optarg) == 0) {
		setup->quality_setting = NORM;
		return 0;
	}
	int n = atoi(&optarg[1]);
	n--;
	if (n < 0) {
		fprintf(stderr, "Bad quality mode given, using norm\n");
		return -1;
	}
	if (optarg[0] == 'h') {
		if (n < sizeof(hq_table)) {
			printf("HIGH quality mode %d\n", n);
			setup->quality_setting = hq_table[n]; return 0;
		}
	} else {
		if (n < sizeof(lq_table)) {
			printf("LOW quality mode %d\n", n);
			setup->quality_setting = lq_table[n];
			return 0;
		}
	}
	return -1;
}



int main(int argc, char **argv) 
{
	FILE *image;
	int ret;
	const char *str;
	image_buffer im;
	codec_setup setup;
	int rate = 8;
	char output_filename[256] = "";
	char input_filename[256];

	unsigned char *img;
	int width;
	int height;

	setup.quality_setting = NORM;
	setup.colorspace = YUV;
	setup.wavelet_type = WVLTS_53;
	setup.RES_HIGH = 0;
	setup.RES_LOW = 3;
	setup.wvlts_order = 2;

	im.setup = &setup;

	while (1) {
		int c;
		int option_index;
		c = getopt_long(argc, argv, "-q:s:LTvdno:t:", long_options, &option_index);

		if (c == EOF) break;
		switch (c) {
			case OPT_HELP:
				fprintf(stderr, "\nOptions:\n"
				"--help                    This text\n"
				);
				return 0;
			case 'o':
				strncpy(output_filename, optarg, sizeof(output_filename) - 1);
				break;
			case 'q':
				ret = set_quality(optarg, &setup, 1);
				break;
			case 't':
				ret = sscanf(optarg, "%d,%d",
					&g_encconfig.debug_tile_x, &g_encconfig.debug_tile_y);
				if (ret != 2) {
					g_encconfig.debug_tile_x =
					g_encconfig.debug_tile_y = 0;
				}
				break;
			case 'd':
				g_encconfig.debug = 1; break;
			case 'T':
				g_encconfig.testmode = 1; break;
			case 'L':
				g_encconfig.loopback = 1; break;
			case 'v':
				g_encconfig.verbose = 1; break;
			case 's':
				g_encconfig.tilepower = atoi(optarg);
				if (g_encconfig.tilepower < 5 || g_encconfig.tilepower > 9) {
					fprintf(stderr, "Bad tile power, must be in range [5 .. 9]\n");
					return -1;
				}
				break;
			default:
				if (optarg) strncpy(input_filename, optarg, sizeof(input_filename) - 1);
		}
	}

	if (output_filename[0] == 0 ) {
		sprintf(output_filename, "%s.nhw", input_filename);
		fprintf(stderr, "Output file not specified, using %s\n", output_filename);
	}


	imgbuf_init(&im, g_encconfig.tilepower);


	if ((image = fopen(input_filename, "rb")) == NULL ) {
		fprintf(stderr, "\nCould not open file %s\n", input_filename);
		exit(-1);
	}

	if (g_encconfig.loopback) {
		ret = read_png(image, &img, &width, &height);
	} else {
		im.im_buffer4 = (unsigned char*) malloc(3*im.fmt.end*sizeof(char));
		ret = read_png_tile(image, im.im_buffer4, im.fmt.tile_size);
	}
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
		if (g_encconfig.loopback) {
			if (height % im.fmt.tile_size || width % im.fmt.tile_size) {
				fprintf(stderr, "Not a integer multiple of tile size!\n");
			} else {
				encode_tiles(&im, img, width, height, output_filename, rate);
			}
			free(img);
		} else {
			nhw_encode(&im, output_filename, rate);
		}
	} else {
		fprintf(stderr, "Error: %s\n", str);
	}

	return ret;
}

