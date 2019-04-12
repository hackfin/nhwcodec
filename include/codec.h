/***************************************************************************
****************************************************************************
*  NHW Image Codec														   *
*  file: codec.h											               *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06202012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -image parameters, wavelet orders, ... (beta version)		   *
***************************************************************************/

/* Copyright (C) 2007-2013 NHW Project
   Written by Raphael Canut - nhwcodec_at_gmail.com */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of NHW Codec, or NHW Project, nor the names of 
   specific contributors, may be used to endorse or promote products 
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CODEC_H
#define _CODEC_H

// API tricks, temporary
#include "debug_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Main encoder options/config:

struct config {
	char *outfilename;
	int tilepower;
	char loopback;
	char verbose;
	char testmode;
	int debug;
	int debug_tile_x;
	int debug_tile_y;
};

extern struct config g_encconfig;

/*==========================================*/

#define LOSSY  

//QUALITY SETTINGS
#define HIGH3 23
#define HIGH2 22
#define HIGH1 21
#define NORM 20
#define LOW1 19
#define LOW2 18
#define LOW3 17
#define LOW4 16
#define LOW5 15
#define LOW6 14
#define LOW7 13
#define LOW8 12
#define LOW9 11
#define LOW10 10
#define LOW11 9
#define LOW12 8
#define LOW13 7
#define LOW14 6
#define LOW15 5
#define LOW16 4
#define LOW17 3
#define LOW18 2
#define LOW19 1
#define LOW20 0

/*==========================================*/

// COLORSPACE DEFINITION
#define RGB 0
#define YUV 1
#define R_COMP (-56992-128) 
#define G_COMP  (34784-128)
#define B_COMP (-70688-128) 

// WAVELET TYPE DEFINITION
#define WVLTS_53 0
#define WVLTS_97 1

#define WVLT_ENERGY_NHW 123

//#define NHW_BOOKS

// All the ResIndex arrays are coordinate index lists
typedef unsigned char ResIndex;

typedef struct{
	unsigned char colorspace;
	unsigned char wavelet_type;
	unsigned char RES_HIGH;
	unsigned char RES_LOW;
	unsigned char wvlts_order;
	unsigned char quality_setting;
}codec_setup;

typedef struct
{
	unsigned int tile_power; // Tile power [5..9]
#ifdef TILE_SUPPORT // would allow to use arbitrary image dimensions. NOT YET.
	unsigned int width;      // Full image width
	unsigned int height;     // Full image height, arbitrary
#endif
	// cached/precalced values:
	unsigned int tile_size;  // Size of tile: 2**tile_power
	unsigned int half;       // Lower half offset of tile
	unsigned int end;        // End of buffer address, non-inclusive
} img_format;

typedef struct{
	short *im_process;
	short *im_jpeg;
	unsigned char *im_bufferY;
	unsigned char *im_bufferU;
	unsigned char *im_bufferV;
	unsigned char *im_buffer4;
	unsigned char *im_nhw;
	short *im_wavelet_first_order;
	short *im_quality_setting;
	short *im_wavelet_band;
	short *im_nhw3;
	unsigned char *scale;
	codec_setup *setup;
	img_format  fmt;
} image_buffer;

struct nhw_res {
	unsigned int   len;
	unsigned short word_len;
	unsigned short bit_len;
	ResIndex *res;
	unsigned char *res_bit;
	unsigned char *res_word;
};

typedef struct{
	unsigned int *encode;
	unsigned char *tree1;
	unsigned char *tree2;
	struct nhw_res res1;
	struct nhw_res res3;
	struct nhw_res res5;

	unsigned short nhw_res4_len;
	unsigned short nhw_res5_len;
	unsigned int nhw_res6_len;
	unsigned short nhw_res6_word_len;
	unsigned short nhw_res6_bit_len;
	ResIndex *nhw_res4;
	ResIndex *nhw_res6;
	unsigned char *nhw_res6_bit;
	unsigned char *nhw_res6_word;

	unsigned short *nhw_char_res1;
	unsigned short nhw_char_res1_len;

	unsigned short nhw_select1;
	unsigned short nhw_select2;
	unsigned char *nhw_select_word1;
	unsigned char *nhw_select_word2;
	int size_data1;
	int size_data2;
	unsigned short size_tree1;
	unsigned short size_tree2;
	unsigned short tree_end;
	unsigned short Y_res_comp;
	unsigned short exw_Y_end;
	unsigned short end_ch_res;
	unsigned short qsetting3_len;
	unsigned int *high_qsetting3;
	unsigned short highres_mem_len;
	unsigned short highres_comp_len;
	unsigned short *highres_mem;
	unsigned char *highres_comp;
	unsigned char *highres_word;
	unsigned char *res_U_64;
	unsigned char *res_V_64;
	unsigned char *exw_Y;
	unsigned char *res_ch;
	unsigned int *high_res;
	int debug;
}encode_state;

typedef struct{
	unsigned int *packet1;
	unsigned int *packet2;
	int d_size_data1;
	int d_size_data2;

	struct nhw_res res1;
	struct nhw_res res3;
	struct nhw_res res5;

	unsigned short nhw_res4_len;
	unsigned int nhw_res6_len;
	unsigned short nhw_res2_bit_len;
	unsigned short nhw_res6_bit_len;
	unsigned short nhw_select1;
	unsigned short nhw_select2;
	ResIndex *nhw_res4;
	ResIndex *nhw_res6;
	unsigned char *nhw_res6_bit;
	unsigned char *nhw_res6_word;
	unsigned int *nhwresH3;
	unsigned int *nhwresH4;
	unsigned char *nhw_select_word1;
	unsigned char *nhw_select_word2;
	unsigned short *nhw_char_res1;
	unsigned short nhw_char_res1_len;
	unsigned short res_f1;
	unsigned short res_f2;
	unsigned short d_size_tree1;
	unsigned short d_size_tree2;
	unsigned char *d_tree1;
	unsigned char *d_tree2;
	unsigned short exw_Y_end;
	unsigned short end_ch_res;
	unsigned short qsetting3_len;
	unsigned int *high_qsetting3;
	unsigned short highres_comp_len;
	unsigned char *highres_comp;
	unsigned short tree_end;
	unsigned short *book;
	unsigned int *resolution;
	unsigned char *exw_Y;
	unsigned char *res_U_64;
	unsigned char *res_V_64;
	unsigned char *res_ch;
	ResIndex *res_comp;
}decode_state;


typedef unsigned long NhwIndex;

/* ENCODER */

extern void encode_image(image_buffer *im,encode_state *enc, int ratio);

extern int menu(char **argv,image_buffer *im,encode_state *os,int rate);
extern int write_compressed_file(image_buffer *im,encode_state *enc, const char *outfile);

extern void downsample_YUV420(image_buffer *im,encode_state *enc,int rate);

extern void wavelet_analysis(image_buffer *im,int norder,int last_stage,int Y);
extern void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y);


void dec_wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y);
void dec_wavelet_synthesis2(image_buffer *im,decode_state *os,int norder,int last_stage,int Y);

extern void downfilter53(const short *x,int N,int decalage,short *res);
extern void downfilter53II(const short *x,int N,int decalage,short *res);
extern void downfilter53IV(const short *x,int N,int decalage,short *res);
extern void downfilter53VI(const short *x,int N,int decalage,short *res);
extern void downfilter97(short *x,int N,int decalage,short *res);
extern void upfilter53(short *x,int M,short *res);
extern void upfilter53I(const short *x,int M,short *res);
extern void upfilter53III(const short *x,int M,short *res);
extern void upfilter53VI(const short *x,int M,short *res);
extern void upfilter53II(short *_X,int M,short *_RES);
extern void upfilter53IV(short *_X,int M,short *_RES);
extern void upfilter97(const short *_X,int M,int E,short *_RES);

extern void pre_processing(image_buffer *im);
extern void pre_processing_UV(image_buffer *im);
extern void block_variance_avg(image_buffer *im);
extern void offsetY(image_buffer *im,encode_state *enc,int m1);
extern void offsetY_recons256(image_buffer *im, encode_state *enc, int m1, int part);
extern void im_recons_wavelet_band(image_buffer *im);
extern void wavelet_synthesis_high_quality_settings(image_buffer *im,encode_state *enc);
extern void offsetUV_recons256(image_buffer *im, int m1, int comp);
extern void offsetUV(image_buffer *im,encode_state *enc,int m2);
extern void quantizationY(image_buffer *im);
extern void quantizationUV(image_buffer *im);

extern void Y_highres_compression(image_buffer *im,encode_state *enc);
extern void highres_compression(image_buffer *im,encode_state *enc);
extern int wavlts2packet(image_buffer *im,encode_state *enc);

/* DECODER */

void decode_image(image_buffer *im,decode_state *os, int bypass_compression);

extern int parse_file(image_buffer *imd,decode_state *os,char **argv);
extern int write_image_decompressed(char **argv,image_buffer *im);

void yuv_to_rgb(image_buffer *im);
int process_hrcomp(image_buffer *imd, decode_state *os);

// extern void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y);
extern void wavelet_synthesis2(image_buffer *im,decode_state *os,int norder,int last_stage,int Y);

void wl_synth_luma(image_buffer *im, int norder, int last_stage);
void wl_synth_chroma(image_buffer *im, int norder, int last_stage);
void wl_synth_upfilter_2d(image_buffer *im, int norder, int s);
void wl_synth_upfilter_VI(image_buffer *im, int norder, int s);

extern void upfilter53(short *x,int M,short *res);
// extern void upfilter53I(short *x,int M,short *res);
// extern void upfilter53III(short *x,int M,short *res);
// extern void upfilter53VI(short *x,int M,short *res);
extern void upfilter53VI_III(short *x,int M,short *res);
extern void upfilter53VI_IV(short *x,int M,short *res);
extern void upfilter53VI_V(short *x,int M,short *res);
extern void upfilter53II(short *_X,int M,short *_RES);
extern void upfilter53IV(short *_X,int M,short *_RES);
extern void upfilter53V(short *_X,int M,short *_RES);
// extern void upfilter97(short *x,int M,int E,short *res);

extern void retrieve_pixel_Y_comp(image_buffer *imd,decode_state *os,int p1,unsigned int *d1,short *im3);
extern void retrieve_pixel_UV_comp(image_buffer *imd,decode_state *os,int p1,unsigned int *d1,short *im3);


void imgbuf_init(image_buffer *im, int tile_power);
void quant_ac_final(image_buffer *im, int ratio, const short *y_wl);
void code_y_chunks(image_buffer *im, unsigned char *nhwbuf, encode_state *enc);
void encode_uv(image_buffer *im, encode_state *enc, int ratio, int res_uv, int uv);
void copy_thresholds(short *process, const short *resIII, int size, int step);
int mark_res_q3(image_buffer *im);
void tree_compress(image_buffer *im, encode_state *enc);
void tree_compress_q(image_buffer *im, encode_state *enc);
void tree_compress_q3(image_buffer *im,  encode_state *enc);

int configure_wvlt(int quality, char *wvlt);
void reduce_generic(image_buffer *im, short *resIII, char *wvlt,
	encode_state *enc, int ratio);

void reduce_HL(image_buffer *im, const short *resIII, int ratio, char *wvlt);
void reduce_LH_HH_q56(image_buffer *im, int ratio, char *wvlt);

void reduce_uv_q4(image_buffer *im, int ratio);
void reduce_uv_q9(image_buffer *im);

void lowres_uv_compensate(short *dst, const short *pr, const short *lowres, int end, int step, int t0, int t1);
#endif


