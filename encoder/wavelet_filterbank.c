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

// Temporary: Hack for remote function overlay
#ifndef SWAPOUT_FUNCTION
#define SWAPOUT_FUNCTION(x) x
#endif

/* The wavelet transform works in-place
 *
 */

static
void transpose(short *dst, const short *src, int n, int step)
{
	int i, j, a;

	for (i=0; i<n; i++, dst+=step) {
		for (a = i, j = 0 ;j < n; j++, a += step)
			dst[j] = src[a];
	}
}

void SWAPOUT_FUNCTION(wla_luma)(short *dst, short *src, short *qs_storage, int norder, int highres)
{
	int i;
	int n = 2 * IM_DIM; // Line length
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
		for (i=0;i<(2*IM_SIZE);i++) qs_storage[i]=src[i];
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

void SWAPOUT_FUNCTION(wla_chroma)(short *dst, short *src, int norder, int highres)
{
	// Chroma processing:
	int i;
	short *data,*res,*res2;
	int n = IM_DIM; // Half the pixel line size for chroma
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
	short *data,*res,*res2;
	int n = IM_DIM; // Half the pixel line size
	int halfno = norder >> 1;
	short *qs;

	for (i=0;i<(norder*n);i+=(2*n)) {
		for (a=i,j=0;j<(halfno);j++,a++) im->im_process[a]=0; 
	}

	if (is_luma) {
		if (im->setup->quality_setting>HIGH1 && !last_stage) {
			// FIXME: move outside
			im->im_quality_setting=(short*)malloc(2*IM_SIZE*sizeof(short));
			qs = im->im_quality_setting;
		} else {
			qs = (short *) NULL;
		}
		// Always places result in im_process but uses im_jpeg as work buffer
		wla_luma(im->im_process, im->im_jpeg, qs, norder, im->setup->RES_HIGH);
		n *= 2; // For transpose() below
	} else {
		wla_chroma(im->im_process, im->im_jpeg, norder, im->setup->RES_HIGH);
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{
		transpose(im->im_jpeg, im->im_process, halfno, n);
	}
}

static
void wls_luma(image_buffer *im, int norder, int last_stage)
{
	const short *data,*data2;
	short *res;
	int i, IM_SYNTH=IM_DIM;
	int s = 2 * IM_SYNTH;
	int halfno = norder >> 1;

	data = im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + halfno;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<halfno;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);	
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=0;i<halfno;i++)
		{
			upfilter53I(data,halfno,res);upfilter53III(data2,halfno,res);	
			data +=s;res +=s;data2 +=s;
		}
	}

	int offset = halfno * s;
	data=&im->im_jpeg[offset];
	res=&im->im_process[offset];
	data2=&im->im_jpeg[offset + halfno];

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=halfno;i<norder;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=halfno;i<norder;i++)
		{
			upfilter53I(data,halfno,res);upfilter53III(data2,halfno,res);
			data +=s;res +=s;data2 +=s;
		}
	}

	//image transposition
	transpose(im->im_jpeg, im->im_process, norder, s);

	data=im->im_jpeg;
	res=im->im_process;
	data2=&im->im_jpeg[halfno];

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,halfno,res);upfilter53VI(data2,halfno,res);
			data +=s;res +=s;data2 +=s;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1) {
		transpose(im->im_jpeg, im->im_process, norder, s);
	}
}

static
void wls_chroma(image_buffer *im, int norder, int last_stage)
{
	const short *data,*data2;
	short *res;
	int i, IM_SYNTH=IM_DIM;
	int s = IM_SYNTH; // half line size for 420 chroma buffer

	int halfno = norder >> 1;

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + halfno;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<halfno;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);	
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=0;i<halfno;i++)
		{
			upfilter53I(data,halfno,res);upfilter53III(data2,halfno,res);	
			data +=s;res +=s;data2 +=s;
		}
	}

	int offset = halfno * s;
	data  = &im->im_jpeg[offset];
	res   = &im->im_process[offset];
	data2 = &im->im_jpeg[offset + halfno];

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=halfno;i<norder;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=halfno;i<norder;i++)
		{
			upfilter53I(data,halfno,res);upfilter53III(data2,halfno,res);
			data +=s;res +=s;data2 +=s;
		}
	}

	transpose(im->im_jpeg, im->im_process, norder, s);

	data=im->im_jpeg;
	res=im->im_process;
	data2=&im->im_jpeg[halfno];

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,halfno,1,res);upfilter97(data2,halfno,0,res);
			data +=s;res +=s;data2 +=s;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,halfno,res);upfilter53VI(data2,halfno,res);
			data +=s;res +=s;data2 +=s;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1) {
		transpose(im->im_jpeg, im->im_process, norder, s);
	}
}

void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int is_luma)
{
	if (is_luma) {
		wls_luma(im, norder, last_stage);
	} else {
		wls_chroma(im, norder, last_stage);
	}
}

void wavelet_synthesis_high_quality_settings(image_buffer *im,encode_state *enc)
{
	int i,j,e,res,Y,scan,count,wavelet_half_synth_res;
	short *wavelet_half_synthesis,*data,*res_w,*data2;
	unsigned char *nhw_res6I_word,*ch_comp,*highres,*scan_run;

	wavelet_half_synthesis=(short*)malloc(2*IM_SIZE*sizeof(short));

	res_w=(short*)wavelet_half_synthesis;
	data=(short*)im->im_wavelet_first_order;
	data2=(short*)im->im_wavelet_band;
	
	for (i=0;i<IM_DIM;i++)
	{
		upfilter53I(data,IM_DIM,res_w);upfilter53III(data2,IM_DIM,res_w);	
		data +=(IM_DIM);res_w +=(2*IM_DIM);data2 +=(IM_DIM);
	}

	if (im->setup->quality_setting>HIGH2) wavelet_half_synth_res=30;
	else wavelet_half_synth_res=34;

	for (i=0,count=0,scan=0,e=0;i<(2*IM_SIZE);i++) 
	{
		if (abs(im->im_quality_setting[i]-wavelet_half_synthesis[i])>wavelet_half_synth_res)
		{
			if (im->setup->quality_setting>HIGH2 && abs(im->im_quality_setting[i]-wavelet_half_synthesis[i])>56)
			{
				if ((im->im_quality_setting[i]-wavelet_half_synthesis[i])>0) 
				{
					wavelet_half_synthesis[i]=32000;e++;
				}
				else 
				{
					wavelet_half_synthesis[i]=32500;e++;
				}
			}
			else if ((im->im_quality_setting[i]-wavelet_half_synthesis[i])>0) 
			{
				wavelet_half_synthesis[i]=30000;count++;
			}
			else 
			{
				wavelet_half_synthesis[i]=31000;count++;
			}
		}
	}


	if (im->setup->quality_setting>HIGH2)
	{
		enc->high_qsetting3=(unsigned int*)malloc(e*sizeof(int));

		for (i=0,e=0;i<(2*IM_SIZE);i++) 
		{
			if (wavelet_half_synthesis[i]==32000)
			{
				enc->high_qsetting3[e++]=(i<<1);
			}
			else if (wavelet_half_synthesis[i]==32500)
			{
				enc->high_qsetting3[e++]=(i<<1)+1;
			}
		}

		enc->qsetting3_len=e;
	}

	highres=(unsigned char*) malloc((count+(2*IM_DIM))*sizeof(char));
	nhw_res6I_word = (unsigned char*) calloc(count , sizeof(char));

	enc->nhw_char_res1=(unsigned short*)malloc(256*sizeof(short));

	for (i=0,count=0,e=0,res=0;i<(2*IM_SIZE);i+=(2*IM_DIM)) 
	{
		for (scan=i,j=0;j<(2*IM_DIM);j++,scan++)
		{
			if (j==(IM_DIM-2) || j==((2*IM_DIM)-2))
			{
				highres[count++]=IM_DIM-2;

				if (j==(IM_DIM-2))
				{
					if (wavelet_half_synthesis[scan]==30000)
					{
						enc->nhw_char_res1[res++]=(i>>1);
					}
					else if (wavelet_half_synthesis[scan]==31000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+1;
					}
					
					if (wavelet_half_synthesis[scan+1]==30000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+2;
					}
					else if (wavelet_half_synthesis[scan+1]==31000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+3;
					}
				}

				j++;scan++;
			}
			else if (wavelet_half_synthesis[scan]==30000)
			{
				highres[count++]=(j&255); nhw_res6I_word[e++]=0;
			}
			else if (wavelet_half_synthesis[scan]==31000)
			{
				highres[count++]=(j&255); nhw_res6I_word[e++]=1;
			}
		}
	}


	free(wavelet_half_synthesis);

	enc->nhw_char_res1_len=res;

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res6_len=res;
	enc->nhw_res6_word_len=e;

	enc->nhw_res6=(unsigned char*)malloc((enc->nhw_res6_len)*sizeof(char));

	for (i=0;i<enc->nhw_res6_len;i++) enc->nhw_res6[i]=highres[i];

	e = enc->nhw_res6_len;

	e = (e + 7) & ~7; // Round up to next multiple of 8 for padding

	scan_run = (unsigned char*) malloc(e * sizeof(char));

	for (i=0;i<enc->nhw_res6_len;i++) scan_run[i]=enc->nhw_res6[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res6_len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]=128+((scan_run[i]-scan_run[i-1])<<4)+scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,scan=0;i<enc->nhw_res6_len;i++) 
	{
		if (enc->nhw_res6[i]!=(IM_DIM-2)) scan_run[scan++]=enc->nhw_res6[i];
	}

	// CHECK:
	// Do we need to pad the remaining? If we don't, there is remaining data from
	// the second loop above
	for (i = scan; i < e; i++) scan_run[i] = 0;

	Y = ((scan + 7) >> 3);

	enc->nhw_res6_bit_len = Y;
	enc->nhw_res6_bit = (unsigned char*) malloc (enc->nhw_res6_bit_len*sizeof(char));

	copy_bitplane0(scan_run, Y, enc->nhw_res6_bit);
	free(scan_run); // no longer needed here

	enc->nhw_res6_len=count;


	Y = enc->nhw_res6_word_len + 7; Y >>= 3;
	enc->nhw_res6_word_len = Y;

	enc->nhw_res6_word= (unsigned char*) malloc((enc->nhw_res6_bit_len<<1) * sizeof(char));
	copy_bitplane0(nhw_res6I_word, Y, enc->nhw_res6_word);

	for (i=0;i<count;i++) enc->nhw_res6[i]=highres[i];

	free(highres);
	free(nhw_res6I_word);

}
