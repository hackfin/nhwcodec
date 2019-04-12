/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: wavelet_filterbank.c  										       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -analysis and synthesis filterbanks							   *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "utils.h"

// Temporary: Hack for remote function overlay
#ifndef SWAPOUT_FUNCTION_netpp
#define SWAPOUT_FUNCTION_netpp(x) x
#endif

/* The wavelet transform works in-place
 *
 */

void transpose(short *dst, const short *src, int n, int step)
{
	int i, j, a;

	for (i=0; i<n; i++, dst+=step) {
		for (a = i, j = 0 ;j < n; j++, a += step)
			dst[j] = src[a];
	}
}

void SWAPOUT_FUNCTION_netpp(wla_luma)(short *dst, short *src, short *qs_storage, int norder, int n, int im_size, int highres)
{
	int i;
	const short *data;
	short *res, *res2;
	int offset;

	int halfno = norder >> 1;

	data = src;
	res  = dst;
	res2 = &res[halfno];

	for (i=0;i<norder;i++) {
		downfilter53IV(data,norder,0,res); downfilter53IV(data,norder,1,res2);
		data +=n;res +=n;res2 +=n;
	}

	// Transposing (Y direction) into im_jpeg buffer
	transpose(src, dst, norder, n);

	if (qs_storage) {
		for (i=0;i<(2*im_size);i++) qs_storage[i]=src[i];
	}

	data=src;
	res=dst;
	res2=&res[halfno];

	if (!highres)
	{
		for (i=0;i<halfno;i++)
		{
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=n;res +=n;res2 +=n;
		}
	} else {
		for (i=0;i<halfno;i++)
		{
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=n;res +=n;res2 +=n;
		}
	}

	offset = norder* n >> 1;

	data=&src[offset];
	res=&dst[offset]; res2=&res[halfno];

	for (i=halfno;i<norder;i++)
	{
		downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
		data +=n;res +=n;res2 +=n;
	}
}

void SWAPOUT_FUNCTION_netpp(wla_chroma)(short *dst, short *src, int norder, int n, int highres)
{
	// Chroma processing:
	int i;
	short *data,*res,*res2;
	int offset;
	int halfno = norder >> 1;

	data=src; // U data
	res=dst;
	res2=&res[halfno];

	for (i=0;i<norder;i++) {
		downfilter53IV(data,norder,0,res);
		downfilter53IV(data,norder,1,res2);
		data +=n;res +=n;res2 +=n;
	}

	// Transposing (Y direction) back to src:
	transpose(src, dst, norder, n);

	data=src;
	res=dst;
	res2=&res[halfno];

	if (!highres) {
		for (i=0;i<halfno;i++) {
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=n;res +=n;res2 +=n;
		}
	} else {
		for (i=0;i<halfno;i++) {
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=n;res +=n;res2 +=n;
		}
	}

	offset = norder * n >> 1;

	data=&src[offset];
	res=&dst[offset];
	res2=&res[halfno];

	for (i=halfno;i<norder;i++)
	{
		downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
		data +=n;res +=n;res2 +=n;
	}
}

void wavelet_analysis(image_buffer *im, int norder, int last_stage, int is_luma)
{
	int i,j,a;
	int n = im->fmt.tile_size; // Line size
	int halfno = norder >> 1;
	short *qs;

	for (i=0;i<(halfno*n);i+=n) {
		for (a=i,j=0;j<(halfno);j++,a++) im->im_process[a]=0; 
	}

	if (is_luma) {
		if (im->setup->quality_setting>HIGH1 && !last_stage) {
			// FIXME: move outside
			im->im_quality_setting=(short*)malloc(im->fmt.half*sizeof(short));
			qs = im->im_quality_setting;
		} else {
			qs = (short *) NULL;
		}
		// Always places result in im_process but uses im_jpeg as work buffer
		wla_luma(im->im_process, im->im_jpeg, qs, norder, n, im->fmt.end / 4, im->setup->RES_HIGH);
	} else {
		n /= 2;
		wla_chroma(im->im_process, im->im_jpeg, norder, n, im->setup->RES_HIGH);
		
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{
		transpose(im->im_jpeg, im->im_process, halfno, n);
	}
}

void wl_synth_upfilter_2d(image_buffer *im, int norder, int s)
{
	const short *data,*data2;
	short *res;
	int i;
	int halfno = norder / 2;

	data = im->im_jpeg;
	res = im->im_process;
	data2 = im->im_jpeg + halfno;

	if (im->setup->wavelet_type==WVLTS_97) {
		for (i=0;i<halfno;i++) {
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);	
			data +=s;res +=s;data2 +=s;
		}
	} else {
		for (i=0;i<halfno;i++) {
			upfilter53I(data,halfno,res);upfilter53III(data2,halfno,res);	
			data +=s;res +=s;data2 +=s;
		}
	}

	int offset = halfno * s;
	data = &im->im_jpeg[offset];
	res = &im->im_process[offset];
	data2 = &im->im_jpeg[offset + halfno];

	if (im->setup->wavelet_type==WVLTS_97) {
		for (i = halfno; i < norder; i++) {
			upfilter97(data, halfno, 1, res); upfilter97(data2, halfno, 0, res);
			data += s; res += s; data2 += s;
		}
	} else {
		for (i=halfno;i<norder;i++) {
			upfilter53I(data, halfno, res); upfilter53III(data2, halfno, res);
			data += s; res += s; data2 += s;
		}
	}
}

void wl_synth_upfilter_VI(image_buffer *im, int norder, int s)
{
	const short *data,*data2;
	short *res;
	int i;

	int halfno = norder / 2;
	data = im->im_jpeg;
	res = im->im_process;
	data2 = im->im_jpeg + halfno;

	if (im->setup->wavelet_type == WVLTS_97) {
		for (i = 0;i < norder; i++)
		{
			upfilter97(data, halfno, 1, res); upfilter97(data2, halfno, 0, res);
			data += s; res += s;data2 += s;
		}
	} else {
		for (i = 0; i < norder; i++) {
			upfilter53I(data, halfno, res); upfilter53VI(data2, halfno, res);
			data += s;res += s;data2 += s;
		}
	}
}

void wl_synth_luma(image_buffer *im, int norder, int last_stage)
{
	int s = im->fmt.tile_size;

	wl_synth_upfilter_2d(im, norder, s);

	//image transposition
	transpose(im->im_jpeg, im->im_process, norder, s);

	wl_synth_upfilter_VI(im, norder, s);

	if (last_stage!=im->setup->wvlts_order-1) {
		transpose(im->im_jpeg, im->im_process, norder, s);
	}
}

void wl_synth_chroma(image_buffer *im, int norder, int last_stage)
{
	int i;
	int s = im->fmt.tile_size / 2; // half line size for 420 chroma buffer

	wl_synth_upfilter_2d(im, norder, s);

	transpose(im->im_jpeg, im->im_process, norder, s);

	wl_synth_upfilter_VI(im, norder, s);

	if (last_stage!=im->setup->wvlts_order-1) {
		transpose(im->im_jpeg, im->im_process, norder, s);
	}
}

void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int is_luma)
{
	if (is_luma) {
		wl_synth_luma(im, norder, last_stage);
	} else {
		wl_synth_chroma(im, norder, last_stage);
	}
}

////////////////////////////////////////////////////////////////////////////
// DECODER specifics:


void dec_wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y)
{
	int n = im->fmt.tile_size;

	int halfn = n / 2;

	switch (Y) {
		case 1:
			wl_synth_upfilter_2d(im, norder, n);
			transpose(im->im_jpeg, im->im_process, norder, n);
		case 3:
			wl_synth_upfilter_VI(im, norder, n);
			break;
		case 0:
			n /= 2;
			wl_synth_upfilter_2d(im, norder, n);
			transpose(im->im_jpeg, im->im_process, norder, halfn);
			wl_synth_upfilter_VI(im, norder, n);
	}

}


void dec_wavelet_synthesis2(image_buffer *im,decode_state *os,int norder,int last_stage,int Y)
{
	short *data;
	int i, n, halfn;

	n = im->fmt.tile_size;
	halfn = n / 2;

	wl_synth_upfilter_2d(im, norder, n);

	data=(short*)im->im_process;

	if (im->setup->quality_setting>HIGH1)
	{
		for (i=0;i<os->nhw_res6_bit_len;i++) 
		{
			data[os->nhwresH3[i]]-=32;
		}
		free(os->nhwresH3);

		for (i=0;i<os->nhw_res6_len;i++) 
		{
			data[os->nhwresH4[i]]+=32;
		}
		free(os->nhwresH4);

		for (i=0;i<os->nhw_char_res1_len;i++)
		{
			if ((os->nhw_char_res1[i]&3)==0)
			{
				data[((os->nhw_char_res1[i]<<1)+halfn-2)]+=32;
			}
			else if ((os->nhw_char_res1[i]&3)==1)
			{
				data[(((os->nhw_char_res1[i]-1)<<1)+halfn-2)]-=32;
			}
			else if ((os->nhw_char_res1[i]&3)==2)
			{
				data[(((os->nhw_char_res1[i]-2)<<1)+halfn-1)]+=32;
			}
			else
			{
				data[(((os->nhw_char_res1[i]-3)<<1)+halfn-1)]-=32;
			}
		}

		free(os->nhw_char_res1);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		for (i=0;i<os->qsetting3_len;i++)
		{
			if (!(os->high_qsetting3[i]&1))
			{
				data[(os->high_qsetting3[i]>>1)]+=56;
			}
			else
			{
				data[(os->high_qsetting3[i]>>1)]-=56;
			}
		}

		free(os->high_qsetting3);
	}

	transpose(im->im_jpeg, im->im_process, norder, n);
}


