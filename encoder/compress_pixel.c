/***************************************************************************
****************************************************************************
*  NHW Image Codec														   *
*  file: compress_pixel.c											       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  rmk: -mixed entropy coding                        					   * 
***************************************************************************/

/* Copyright (C) 2007-2015 NHW Project
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
#include <assert.h>

#include "codec.h"
#include "utils.h"
#include "compression.h"
#include "tree.h"

#define PRINTF(...)

#define NUM_CODE_WORDS   2 * DEPTH

struct compression_context {
	int enc_ptr;
	int s1_pos;
	int s2_pos;
	int zone_entrance;
	unsigned char *nhw_comp;
	unsigned char *nhw_s1;
	unsigned char *nhw_s2;
	int rle_buf[256];
	int rle_128[256];
	unsigned short rle_tree[NUM_CODE_WORDS];
};


static unsigned int huffman_tree[DEPTH]={
	0x0000,0x0002,0x0004,0x000a,
	0x000b,0x0006,0x0007,0x0018,
	0x0019,0x001a,0x0036,0x0037,
	0x0070,0x0071,0x00e8,0x00e9,
	0x00ea,0x00eb,0x00ec,0x00ed,
	0x00ee,0x00ef,0x00f0,0x00f1,
	0x00f2,0x00f3,0x01c8,0x01c9,
	0x01ca,0x01cb,0x01cc,0x01cd,
	0x01ce,0x01cf,0x01e8,0x01e9,
	0x01ea,0x01eb,0x01ec,0x01ed,
	0x01ee,0x01ef,0x03e8,0x03e9,
	0x03ea,0x03eb,0x03ec,0x03ed,
	0x03ee,0x03ef,0x03e4,0x03e5,
	0x03e6,0x03e7,0x07c0,0x07c1,
	0x07e0,0x07e1,0x07f0,0x07f1,
	0x07f2,0x07f3,0x07f4,0x07f5,
	0x07f6,0x07f7,0x07f8,0x07f9,
	0x07fa,0x07fb,0x07fc,0x07fd,
	0x07fe,0x07ff,0x07e8,0x07e9,
	0x07ea,0x07eb,0x07ec,0x07ed,
	0x07ee,0x07ef,0x0f88,0x0f89,
	0x0f8a,0x0f8b,0x0f8c,0x0f8d,
	0x0f8e,0x0f8f,0x0fc8,0x0fc9,
	0x0fca,0x0fcb,0x0fcc,0x0fcd,
	0x0fce,0x0fcf,0x1f08,0x1f09,
	0x1f0a,0x1f0b,0x3f10,0x3f11,
	0x3f12,0x3f13,0x3f14,0x3f15,
	0x3f16,0x3f17,0x1f0c0,0x1f0c1,
	0x1f0c2,0x1f0c3,0x1f0c4,0x1f0c5,
	0x1f0c6,0x1f0c7,0x1f0c8,0x1f0c9,
	0x1f0ca,0x1f0cb,0x1f0cc,0x1f0cd,
	0x1f0ce,0x1f0cf,0x1f0d0,0x1f0d1,
	0x1f0d2,0x1f0d3,0x1f0d4,0x1f0d5,
	0x1f0d6,0x1f0d7,0x1f0d8,0x1f0d9,
	0x1f0da,0x1f0db,0x1f0dc,0x1f0dd,
	0x1f0de,0x1f0df,0x1f0e0,0x1f0e1,
	0x1f0e2,0x1f0e3,0x1f0e4,0x1f0e5,
	0x1f0e6,0x1f0e7,0x1f0e8,0x1f0e9,
	0x1f0ea,0x1f0eb,0x1f0ec,0x1f0ed,
	0x1f0ee,0x1f0ef,0x1f0f0,0x1f0f1,
	0x1f0f2,0x1f0f3,0x1f0f4,0x1f0f5,
	0x1f0f6,0x1f0f7,0x1f0f8,0x1f0f9,
	0x1f0fa,0x1f0fb,0x1f0fc,0x1f0fd,
	0x1f0fe,0x1f0ff,0x1f8c0,0x1f8c1,
	0x1f8c2,0x1f8c3,0x1f8c4,0x1f8c5,
	0x1f8c6,0x1f8c7,0x1f8c8,0x1f8c9,
	0x1f8ca,0x1f8cb,0x1f8cc,0x1f8cd,
	0x1f8ce,0x1f8cf,0x1f8d0,0x1f8d1,
	0x1f8d2,0x1f8d3,0x1f8d4,0x1f8d5,
	0x1f8d6,0x1f8d7,0x1f8d8,0x1f8d9,
	0x1f8da,0x1f8db,0x1f8dc,0x1f8dd,
	0x1f8de,0x1f8df,0x1f8e0,0x1f8e1,
	0x1f8e2,0x1f8e3,0x1f8e4,0x1f8e5,
	0x1f8e6,0x1f8e7,0x1f8e8,0x1f8e9,
	0x1f8ea,0x1f8eb,0x1f8ec,0x1f8ed,
	0x3f1dc,0x3f1dd,0x3f1de,0x3f1df,
	0x3f1e0,0x3f1e1,0x3f1e2,0x3f1e3,
	0x3f1e4,0x3f1e5,0x3f1e6,0x3f1e7,
	0x7e3d0,0x7e3d1,0x7e3d2,0x7e3d3,
	0x7e3d4,0x7e3d5,0x7e3d6,0x7e3d7,
	0x7e3d8,0x7e3d9,0x7e3da,0x7e3db,
	0x7e3dc,0x7e3dd,0x7e3de,0x7e3df,
	0x7e3e0,0x7e3e1,0x7e3e2,0x7e3e3,
	0x7e3e4,0x7e3e5,0x7e3e6,0x7e3e7,
	0x7e3e8,0x7e3e9,0x7e3ea,0x7e3eb,
	0x7e3ec,0x7e3ed,0x7e3ee,0x7e3ef,
	0x7e3f0,0x7e3f1,0x7e3f2,0x7e3f3,
	0x7e3f4,0x7e3f5,0xfc7ec,0xfc7ed,0xfc7ee,0xfc7ef,
	0xfc7f0,0xfc7f1,0xfc7f2,0xfc7f3,0xfc7f4,0xfc7f5,0xfc7f6,0xfc7f7,
	0xfc7f8,0xfc7f9,0xfc7fa,0xfc7fb,0xfc7fc,0xfc7fd,0xfc7fe,0xfc7ff};

static unsigned char len[DEPTH]={
	2,3,3,4,4,4,4,5,5,5,6,6,7,7,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	13,13,13,13,14,14,14,14,14,14,14,14,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,18,18,18,18,19,19,19,19,19,19,
	19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
	19,19,19,19,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20};

void dump_values_u16(unsigned short *u16, int n, const char *filename, const char *comment);


static
void rle_histo(struct compression_context *ctx, int p1, int p2)
{
	int i, e,c,d;
	const unsigned char *nhw_comp = ctx->nhw_comp;
	int *rle_buf = ctx->rle_buf;
	int *rle_128 = ctx->rle_128;

	i = p1;
	int n = 0; // XXX DEBUG
	e=0;c=0;d=0;
	PRINTF("-------\n 0: ");
	
	/*for (i=p1;i<p2-1;i++)   
	{
L_RUN1:	if (nhw_comp[i]==128)   
		{
			while (i<(p2-1) && nhw_comp[i+1]==128)
			{
				e++;c=1;
				if (e==255)
				{
					if (nhw_comp[i+2]!=128) 
					{
						rle_buf[128]+=2;
						e=254;rle_128[254]++;i--;e=0;c=0;
						goto L_RUN1;
					}
					else if (nhw_comp[i+2]==128 && nhw_comp[i+3]!=128) 
					{
						rle_buf[128]+=3;
						e=254;rle_128[254]++;i--;e=0;c=0;
						goto L_RUN1;
					}
					i++;
				}
				else if (e>255) 
				{
					
					e=254;rle_128[254]++;i--;e=0;c=0;
					goto L_RUN1;
				}
				else i++;
			}
		}

		if (c) 
		{
			rle_128[e+1]++;
		}
		else 
		{
			rle_buf[nhw_comp[i]]++;
		}
		e=0;c=0;
	}*/
	
	e=1;
	
	for (i=p1;i<p2-1;i++)   
	{
L_RUN1:	if (nhw_comp[i]==128)   
		{
			while (i<(p2-1) && nhw_comp[i+1]==128)
			{
				e++;c=1;
				if (e>255) 
				{
					
					e=254;rle_128[254]++;e=1;c=0;
					goto L_RUN1;
				}
				else i++;
			}
		}

		if (c) 
		{
			rle_128[e]++;
		}
		else 
		{
			rle_buf[nhw_comp[i]]++;
		}
		e=1;c=0;
	}
			
	/*while (i < p2-1) {
		if (nhw_comp[i]==128) { // (1)
			e=0;
			while (i<(p2-1) && nhw_comp[i+1]==128) {
				e++;
				// nasty: If more than 255 occurences found,
				// decrement i and resume. This is not effective,
				// as the condition (1) is THEN always true.
				// FIXME: State machine / unwrap loop
				if (e > 255) { i--; e = 254; break; }
				else i++;
			}

			if (e)  { rle_128[e+1]++; PRINTF("[%d]", e);}
				
			else    { rle_buf[nhw_comp[i]]++; PRINTF("%d,", nhw_comp[i]);}

			n++; if (n == 4) { PRINTF("\n %d: ", i); n = 0; } // XXX DEBUG
		} else {
			rle_buf[nhw_comp[i]]++; PRINTF("%d,", nhw_comp[i]);
			n++; if (n == 4) { PRINTF("\n %d: ", i); n = 0; } // XXX DEBUG
		}
		i++;
	}*/
	PRINTF("\n---------------------------------------------------\n");
	

}

static
int rle_weights(struct compression_context *ctx,
	unsigned int *weight, unsigned int *weight2, int sel)
{
	int i, j, k;
	int *rle_buf = ctx->rle_buf;
	int *rle_128 = ctx->rle_128;
	unsigned short *rle_tree = ctx->rle_tree;

	for (i=0;i<109;i+=2) {
		if (rle_buf[i]>0) {
			weight2[i] = rle_buf[i]; 
		}
	}

	if (rle_buf[112]>0) {
		weight2[112] = rle_buf[112]; 
	}

	for (i=120;i<141;i++) {
		if (rle_buf[i]>0) {
			weight2[i] = rle_buf[i]; 
		}
	}

	for (i=144;i<256;i+=4) {
		if (rle_buf[i]>0) {
			weight2[i] = rle_buf[i]; 
		}
	}

	for (j=2;j<256;j++) {
			if (rle_128[j]>0) {
				weight2[128] += j*rle_128[j]; 
			}
		}

	for (j=2;j<sel;j++) {rle_128[j]=0;}

	for(j=sel;j<256;j++) {
		if (rle_128[j]>0) weight2[128]-=j*rle_128[j];
	}

	rle_buf[128]=weight2[128];

	k=0;
	for (j=sel;j<256;j++) {
		if (rle_128[j]>0) {
			rle_tree[k]=(unsigned short)((j<<8)|128);
			weight[k]=rle_128[j];
			k++;
		}
	}

	for (i=0;i<109;i+=2) {
		if (rle_buf[i]>0) {
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

	if (rle_buf[112]>0) {
		rle_tree[k]=(unsigned short)((1<<8)|112);
		weight[k]=rle_buf[112];
		k++;
	}

	for (i=120;i<141;i++) {
		if (rle_buf[i]>0) {
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

	for (i=144;i<256;i+=4) {
		if (rle_buf[i]>0) {
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

	return k;
}

static
int code_occurence(const unsigned char *codebook, int e, int cmp, unsigned char *tree)
{
	int i;
	int c = 0;
	int n = 0;

	i = 0;
	while (i < e) {
		if (codebook[i] == cmp) {
			c++;
		} else {
			if (c > 0) {
				*tree++ = cmp; *tree++ = c;
				*tree++ = codebook[i];
				c = 0; n += 3;
			} else {
				*tree++ = codebook[i]; n++; }
		}
		i++;
	}
	// If all last codebook values are == cmp, flush last
	// RLE word
	if (c > 0) { *tree++ = cmp; *tree++ = c; n += 2; }

	return n;
}

static
int rle_tree_bitplane_code(struct compression_context *ctx,
	int k, unsigned char *codebook, encode_state *enc)
{
	int b, e;
	int i;

	int c = ctx->s1_pos;
	int j = ctx->s2_pos;

	c = (c + 7) & ~7; // Round up to next multiple of 8 for padding

	b = (c>>3); e=0;
	enc->nhw_select_word1=(unsigned char*)calloc((b<<3),sizeof(char));

	copy_bitplane0(ctx->nhw_s1, b, enc->nhw_select_word1);

	enc->nhw_select1=b;
	
	j= (j + 7) & ~7; // Round up to next multiple of 8 for padding

	b=(j>>3);e=0;
	enc->nhw_select_word2=(unsigned char*)calloc((b<<3),sizeof(char));

	copy_bitplane0(ctx->nhw_s2, b, enc->nhw_select_word2);

	enc->nhw_select2=b;

	e=0;b=0;c=0;
	for (i=0;i<k;i++) {
		unsigned short tmp = ctx->rle_tree[i];
		if (tmp >> 8 == 0x1)
			enc->tree1[e++]= (unsigned char)((tmp & 0xff));
		else {
			enc->tree1[e++] = CODE_TREE1_RLE;
			enc->tree1[e++] = (tmp >> 8);
		}
	}

	unsigned char *cur = codebook;

	for (i=0;i<e;i+=2) *cur++ = enc->tree1[i];
	for (i=1;i<e;i+=2) *cur++ = enc->tree1[i];

//
//	for (i=0;i<e;i++) printf("%d %d\n",i,codebook[i]);
	b = code_occurence(codebook, e, 3, enc->tree1);
	// for (i=0;i<b;i++) printf("%d %d\n",i,enc->tree1[i]);
	return b;
}

static
void rle_tree2_code(struct compression_context *ctx,
	int k, unsigned char *codebook, encode_state *enc)
{
	int i, e, b;

	e=0;b=0;

	for (i=0;i<k;i++) {
		unsigned short tmp = ctx->rle_tree[i];
		if ((tmp >> 8)==0x1)
			enc->tree2[e++] = (unsigned char)((tmp & 0xff) | 1);
		else {
			enc->tree2[e++] = (tmp&0xff);
			enc->tree2[e++] = (tmp>>8);}
	}

	enc->tree_end=e;

	unsigned char *cur = codebook;

	for (i=0;i<e;i+=2) *cur++ = enc->tree2[i];
	for (i=1;i<e;i+=2) *cur++ = enc->tree2[i];

//	for (i=0;i<e;i++) printf("%d %d\n",i,codebook[i]);

	b = code_occurence(codebook, e, 128, enc->tree2);

	enc->size_tree2=b;
}

static
void rle_pixelcount(struct compression_context *ctx,
	int p1, int p2, int sel, encode_state *enc)
{
	int i, j;
	int c, e;
	int tag, pos, match, pack;
	unsigned char pixel;
	// unsigned char *nhw_s1 = ctx->nhw_s1;
	// unsigned char *nhw_s2 = ctx->nhw_s1;

	int a = ctx->enc_ptr;
	
	e=1; match=0; pack=0; tag=0; c=0, j=0;
	for (i=p1;i<p2-1;i++)   
	{
		pixel=ctx->nhw_comp[i];

		if (pixel>=153)
		{
			if (pixel==153) 
			{
				ctx->nhw_s1[c++]=0;continue;
			}
			else if (pixel==155) 
			{
				ctx->nhw_s1[c++]=1;continue;
			}
			else if (pixel==157) 
			{
				ctx->nhw_s2[j++]=0;continue;
			}
			else if (pixel==159) 
			{
				ctx->nhw_s2[j++]=1;continue;
			}
		}

		if (pixel!=128 && IS_CODE_WORD_1(pixel))
		{
			pos=(unsigned short) ctx->rle_buf[pixel];
			if (pixel>131) i+=4;
			goto L_ZE;
		}

		if (pixel==128)   
		{
			while (i<(p2-1) && ctx->nhw_comp[i+1]==128)
			{
				e++;
				if (e>255) {e=254;i--;goto L_JUMP;}
				else i++;
			}

			if (e>1) 
			{ 
				if (e<sel) {i-=(e-1);tag=e;e=1;}
			}
		}

L_JUMP:	if (e==1) pos=(unsigned short) ctx->rle_buf[pixel];
		else pos=(unsigned short) ctx->rle_128[e];

L_ZE:		if (pos>=110 && pos<174 && ctx->zone_entrance)
		{
			pack += 15;
			if (pack<=32) enc->encode[a] |= ((1<<6)|(pos-110))<<(32-pack); 
			else
			{
				match=pack-32;
				enc->encode[a] |= ((1<<6)|(pos-110))>>match;
				a++;
				enc->encode[a] |= (((1<<6)|(pos-110))&((1<<match)-1))<<(32-match);
				pack=match;
			}
		}
		else
		{
			if (pos>=174) if (ctx->zone_entrance) pos -=UNZONE1;
			pack += len[pos];

			if (pack<=32) enc->encode[a] |= huffman_tree[pos]<<(32-pack); 
			else
			{
				match=pack-32;
				enc->encode[a] |= huffman_tree[pos]>>match;
				a++;
				enc->encode[a] |= (huffman_tree[pos]&((1<<match)-1))<<(32-match);
				pack=match;
			}
		}

/* L_TAG: */	e=1;
		// check the tag, maybe can be elsewhere faster...
		if (tag>0) {tag--;if (tag>0){i++;goto L_JUMP;}} 
	}
	// write back encode data counters:
	ctx->s2_pos = j; ctx->s1_pos = c;
	ctx->enc_ptr = a;
}

	
int wavlts2packet(image_buffer *im,encode_state *enc)
{
	int i,j,k,b,sel,part,p1,p2;

 	struct compression_context C;
 
	unsigned int weight[354],weight2[354],l1,l2;
	unsigned char codebook[NUM_CODE_WORDS],color;

	C.nhw_comp=(unsigned char*)im->im_nhw;
	C.zone_entrance = 0;

	enc->encode=(unsigned int*)calloc(80000,sizeof(int));
	enc->tree1=(unsigned char*)calloc((DEPTH+UNZONE1+1)*2,sizeof(char));
	enc->tree2=(unsigned char*)calloc((DEPTH+1)*2,sizeof(char));

	C.enc_ptr = 0; // Encoded output pointer index

	int n = im->fmt.end;
	part=0;p1=0;p2=n;
	color=im->im_nhw[n];im->im_nhw[n]=3;

	struct local_parameters {
		int sel;
		int thresh;
	} parms[] = {
		{ 4, DEPTH+UNZONE1 },
		{ 3, DEPTH },
	};
	
	enc->nhw_select1 = (enc->nhw_select1 + 7) & ~7; // Round up to next multiple of 8 for padding
	enc->nhw_select2 = (enc->nhw_select2 + 7) & ~7; // Round up to next multiple of 8 for padding

	C.nhw_s1=(unsigned char*)calloc(enc->nhw_select1,sizeof(char));
	C.nhw_s2=(unsigned char*)calloc(enc->nhw_select2,sizeof(char));

	for (part = 0; part < 2; part++) {
		sel = parms[part].sel;


		memset(C.rle_buf,0,256*sizeof(int));
		memset(C.rle_128,0,256*sizeof(int));
		
		/////// FIRST STAGE ////////

		// Statistics on occurences of values in nhw_comp[]:
		// rle_128 contains the histogram for run lengths of following
		// values of 128
		// rle_buf contains statistics on single value occurence.
		rle_histo(&C, p1, p2);

		do {
			memset(weight,0,354*sizeof(int));
			memset(weight2,0,354*sizeof(int));


			// Set weights
			k = rle_weights(&C, weight, weight2, sel);

			if (k > parms[part].thresh)   
			{
				sel++;
			} else break;
		} while (sel < 100);

		if (sel>=100) exit(-1);

		for (i = 0; i < k; i++) {
			for (j = 1; j < (k-i); j++) {
				if (weight[j] > weight[j-1]) {
					l1 = C.rle_tree[j-1];
					l2 = weight[j-1];
					C.rle_tree[j-1] = C.rle_tree[j];
					weight[j-1] = weight[j];
					C.rle_tree[j] = l1;
					weight[j] = l2;
				}
			}
		}

#ifdef NHW_BOOKS
		printf("\nsize= %d\n",k);printf("\nCodeBooks\n");
			for(i=0;i< NUM_CODE_WORDS/2 ;i++) printf("%d %d %d %d\n",i,(unsigned char)C.rle_tree[i],C.rle_tree[i]>>8,weight[i]);
#endif


		//////// LAST STAGE ///////

		for (i=0;i<k;i++)
		{
			//l1=(unsigned char)(C.rle_tree[i]);
			//l2=(unsigned char)(C.rle_tree[i]>>8);
			if ((C.rle_tree[i]>>8)==1) C.rle_buf[(C.rle_tree[i]&0xFF)]=i;
			else C.rle_128[(C.rle_tree[i]>>8)]=i;
		}

		if ((C.rle_tree[0]==((1<<8)|128))) {
			b=1;
			if (sel==4)    C.zone_entrance = 1;
			else           C.zone_entrance = 0;
		} else {
			b=0;
			if ((part==0) && (k> NUM_CODE_WORDS/2)) assert(0);
		}

		if (enc->debug) {
			printf("part: %d  sel: %d  b: %d\n", part, sel, b);
		}

		if ((part==1) && (sel!=4) && (k> NUM_CODE_WORDS/2)) exit(-1);

		if (part==1) C.zone_entrance=0;
		
		// Second pass: Run through nhw_comp[] and grab huffman codes from
		// huffman_tree[] according to occurence based RLE coding
		//
		// Again, use state machine

		rle_pixelcount(&C, p1, p2, sel, enc);

		// Output parameters used next: c, j
		
		if (part==0) {

			enc->size_data1= C.enc_ptr+1;
			if (sel>4 || b==0) im->setup->wavelet_type=4;
			else im->setup->wavelet_type=0;
			b = rle_tree_bitplane_code(&C, k, codebook, enc);
			if (enc->debug) {
				char comment[64];
				sprintf(comment, "Encoder RLE tree, effective size %d", k);
				dump_values_u16((unsigned short *) C.rle_tree,
					NUM_CODE_WORDS, "/tmp/rle_tree.dat", comment);
			}

			enc->size_tree1=b;
			C.enc_ptr++; 
			p1=im->fmt.end;
			p2=im->fmt.end * 3 / 2;
			im->im_nhw[p1]=color;
			im->im_nhw[p2-1]=im->im_nhw[p2-2];

		} else {
			enc->size_data2= C.enc_ptr+1;
			rle_tree2_code(&C, k, codebook, enc);

		}
	} // repeat for part == 1



	free(C.nhw_s1);
	free(C.nhw_s2);
	return 0;
}

//FIXME:
#define LOOP_VAR_CONDITION(i, n) (i < (n)-2)

// These three functions are all similar and could possibly be
// squashed into one.


static
int compress_res0(image_buffer *im,
	const unsigned char *highres, unsigned char *ch_comp, encode_state *enc)
{
	int i, j, a, e, mem;

	int im_size = im->fmt.end / 4;
	int n = im_size / 4;
	int quality = im->setup->quality_setting;
	mem = 0;

	for (i=1,j=1;i<(im_size>>2);i++)
	{
		int dh0 = highres[i]-highres[i-1];
		int dh1 = highres[i+1]-highres[i];

		if (dh0==0 && dh1==0) {
			// Count subsequent equal values in a:
			a = 0;
			const unsigned char *p = &highres[i];
			// FIXME: Turn into state machine.
			while (a < 1 && p[2] == p[1]) { a++; p++; }

			i+=(a+2);
			ch_comp[j]=(a<<3);

			int d0 = highres[i] - highres[i-1];
			int d1 = highres[i+1] - highres[i];

			int comp = ch_comp[j];

			switch (d0) {
				case 2:
					switch (d1) {
						case -2: comp +=2; i++; break;
						case 0:  comp +=3; i++; break;
						default: comp +=1;
					}
					break;
				case -2:
					switch (d1) {
						case 2:  comp +=4; i++; break;
						case 0:  comp +=5; i++; break;
						default: comp +=6;
					}
					break;
				case 4:   comp += 7; break;
				default:  i--;
			}
			ch_comp[j++] = comp;
		} else
		if (abs(dh0)<=6 && abs(dh1)<=8) {
			dh0+=6;dh1+=8;

			if (dh0==12 || dh1==16) {
				int d2 = highres[i+2]-highres[i+1];

				if (abs(d2)<=32 && LOOP_VAR_CONDITION(i, n)) {
					e = d2 + 32; dh0 += 26; dh1 += 8; goto COMP3;
				} else {
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
					if (quality>LOW5)
					{
						ch_comp[j++]=128+(highres[i+1]>>1);
						enc->highres_word[mem++]=enc->res_ch[i];
						enc->highres_mem[enc->highres_mem_len++]=i;
						i++;
					}
				}
			} else {
				if (dh0<8)
					ch_comp[j++]=32+ (dh0<<2) + (dh1>>1);
				else {
					if (dh0==8) {
						ch_comp[j++]= 16 + (dh1>>1);
					} else {
						ch_comp[j++]= 24 + (dh1>>1);
					}
				}
				i++;
			}
		} else
		if (abs(dh0)<=32 && abs(dh1)<=16
		  && abs(highres[i+2]-highres[i+1])<=32 && LOOP_VAR_CONDITION(i, n))
		{
			dh0+=32;dh1+=16;e=highres[i+2]-highres[i+1]+32;
COMP3:
			if (dh0==64 || dh1==32 || e==64) 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);

				if (quality>LOW5)
				{
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->res_ch[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
			}
			else 
			{
				dh1>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(dh0) + (dh1>>3);
				ch_comp[j++]=((dh1&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			ch_comp[j++]=128;
			ch_comp[j++]=128+(highres[i]>>1);

			if (quality>LOW5) {
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->res_ch[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
		}
	}

	enc->highres_comp_len=mem;

	return j; // # of values in ch_comp[]
}

static
int compress_res1(image_buffer *im,
	const unsigned char *highres, unsigned char *ch_comp, encode_state *enc)
{
	int i, j, a, e, mem;
	int quality = im->setup->quality_setting;
	int im_size = im->fmt.end / 4;
	int n = im_size / 4;

	mem = 0;

	for (i=1,j=1;i<(im_size>>2);i++)
	{
		int dh0 = highres[i]-highres[i-1];
		int dh1 = highres[i+1]-highres[i];

		if (dh0==0 && dh1==0) {
			// Count subsequent equal values in a:
			a = 0;
			const unsigned char *p = &highres[i];
			// FIXME: Turn into state machine.
			while (a < 7 && p[2] == p[1]) { a++; p++; }

			i+=(a+2);
			ch_comp[j]=(a<<2);

			int d0 = highres[i] - highres[i-1];

			int comp = ch_comp[j];

			switch (d0) {
				case 2:
					comp+=1; break;
				case -2:
					comp+=2; break;
				case 0:
					comp+=3; break;
				default:
					i--; 
			}
			ch_comp[j++] = comp;
		}
		else if (abs(dh0)<=4 && abs(dh1)<=8)
		{
			dh0+=4;dh1+=8;

			if (dh0==8 || dh1==16) 
			{
				int d2 = highres[i+2]-highres[i+1];

				if (abs(d2)<=32 && LOOP_VAR_CONDITION(i, n)) 
				{
					e = d2 + 32; dh0 += 28; dh1 += 8; goto COMP4;
				} else {
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
					if (quality>LOW5)
					{
						ch_comp[j++]=128+(highres[i+1]>>1);
						enc->highres_word[mem++]=enc->res_ch[i];
						enc->highres_mem[enc->highres_mem_len++]=i;
						i++;
					}
				}
			} else {
				ch_comp[j++]=32+ (dh0<<2) + (dh1>>1);
				//if (highres[i+2]==highres[i+1] && i<12286) { ch_comp[j-1]+=1;i+=2;}
				//else i++;
				i++;
			}
		}
		else if (abs(dh0)<=32 && abs(dh1)<=16
		  && abs(highres[i+2]-highres[i+1])<=32 && LOOP_VAR_CONDITION(i, n))
		{
			dh0+=32;dh1+=16;e=highres[i+2]-highres[i+1]+32;
COMP4:
			if (dh0==64 || dh1==32 || e==64) 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);

				if (quality>LOW5)
				{
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->res_ch[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
			}
			else 
			{
				dh1>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(dh0) + (dh1>>3);
				ch_comp[j++]=((dh1&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			ch_comp[j++]=128;
			ch_comp[j++]=128+(highres[i]>>1);

			if (quality>LOW5) {
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->res_ch[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
		}
	}

	enc->highres_comp_len=mem;

	return j; // # of values in ch_comp[]
}


static
int compress_res2(image_buffer *im,
	const unsigned char *highres, unsigned char *ch_comp, encode_state *enc)
{
	int i, j, a, e, mem;
	int quality = im->setup->quality_setting;
	mem = 0;

	int im_size = im->fmt.end / 4;
	int n = im_size / 4;

	for (i=1,j=1;i<(im_size>>2);i++)
	{
		int dh0 = highres[i]-highres[i-1];
		int dh1 = highres[i+1]-highres[i];

		if (dh0==0 && dh1==0) {
			// Count subsequent equal values in a:
			a = 0;
			const unsigned char *p = &highres[i];
			// FIXME: Turn into state machine.
			while (a < 63 && p[2] == p[1]) { a++; p++; }
			i+=(a+2);
			ch_comp[j++]=a;
		}
		else if (abs(dh0)<=32 && abs(dh1)<=16
		  && abs(highres[i+2]-highres[i+1])<=32 && LOOP_VAR_CONDITION(i, n))
		{
			dh0+=32;dh1+=16;e=highres[i+2]-highres[i+1]+32;
			if (dh0==64 || dh1==32 || e==64) 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);

				if (quality>LOW5)
				{
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->res_ch[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
			}
			else 
			{
				dh1>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(dh0) + (dh1>>3);
				ch_comp[j++]=((dh1&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			ch_comp[j++]=128;
			ch_comp[j++]=128+(highres[i]>>1);

			if (quality>LOW5) {
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->res_ch[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
		}
	}

	enc->highres_comp_len=mem;

	return j; // # of values in ch_comp[]
}

static
void compress_pass2(int quality, int j,
	const unsigned char *highres, unsigned char *ch_comp, encode_state *enc)
{
	int i, e;

	unsigned char *comp_tmp;

	comp_tmp=(unsigned char*)calloc(j,sizeof(char));

	memcpy(comp_tmp,ch_comp,j*sizeof(char));

	for (i=1,e=1;i<j-1;i++)
	{
		if (comp_tmp[i]==64)
		{
			ch_comp[e++]=comp_tmp[i+1];
			ch_comp[e++]=comp_tmp[i+2];
			i+=2;
		}
		else if (comp_tmp[i]==128)
		{
			if (quality>LOW5)
			{
				i++;
				ch_comp[e++]=comp_tmp[i+1];
				//if (comp_tmp[i+1]<128) ch_comp[e]+=128;
				//e++;

				i++;
			}
			else
			{
				i++;
				ch_comp[e++]=comp_tmp[i];
			}
		}
		else ch_comp[e++]=comp_tmp[i];
	}

	if (i<j) ch_comp[e++]=comp_tmp[j-1];

	free(comp_tmp);

	enc->Y_res_comp=e;
}

void Y_highres_compression(image_buffer *im,encode_state *enc)
{
	int i, e, Y, a,count;
	unsigned char *ch_comp;

	int im_size = im->fmt.end / 4;
	const unsigned char *highres;

	highres=(unsigned char*)enc->tree1;

	enc->highres_word=(unsigned char*)calloc((im_size>>2),sizeof(char));

	//for (i=0;i<300;i++) printf("%d %d\n",i,highres[i]);
	e = 0; Y = 0; a = 0;

	// Count occurences of equal values in highres array:

	for (i = 1; i<(im_size>>2); i++) {
		if (highres[i] == highres[i-1]) {
			e++;
			if (e<16) {
				if (e==8) a++;
			}
			else {Y++; e = 0;}
		} else {
			e = 0;
		}
	}

	a+=Y;

	enc->highres_mem_len=0;
	enc->highres_comp=(unsigned char*)calloc((im_size>>1),sizeof(char));
	enc->highres_mem=(unsigned short*)calloc((im_size>>2),sizeof(short));
	ch_comp=(unsigned char*)enc->highres_comp;

	ch_comp[0]=highres[0];

	if (Y>299)       im->setup->RES_LOW=2;
	else if (a>179)  im->setup->RES_LOW=1;
	else             im->setup->RES_LOW=0;

	int quality = im->setup->quality_setting;

	switch (im->setup->RES_LOW) {
		case 2:
			count = compress_res2(im, highres, ch_comp, enc);
			compress_pass2(quality, count, highres, ch_comp, enc);
			break;
		case 1:
			count = compress_res1(im, highres, ch_comp, enc);
			compress_pass2(quality, count, highres, ch_comp, enc);
			break;
		default:
			count = compress_res0(im, highres, ch_comp, enc);
			compress_pass2(quality, count, highres, ch_comp, enc);

	}
}

void highres_compression(image_buffer *im, encode_state *enc)
{
	int i,j,a,res,d0,d1;
	unsigned char *highres,*ch_comp;

	highres=(unsigned char*)enc->tree1;
	ch_comp=(unsigned char*)enc->highres_comp;

	int im_size = im->fmt.end / 4;

	// Null 2 LSBs:
	for (i=(im_size>>2);i<(im_size>>2)+(im_size>>3);i++)
		highres[i] &= ~3;


	im->setup->RES_HIGH=im->setup->RES_LOW;

	j=enc->Y_res_comp;

	ch_comp[j++]=highres[(im_size>>2)];

	for (i=(im_size>>2)+1,a=0,res=0;i<(im_size>>2)+(im_size>>3);i++)
	{
		d0=highres[i]-highres[i-1];
		d1=highres[i+1]-highres[i];

		if (d0==0 && d1==0)
		{
RES_COMPR3:	if ((i+a+2)<((im_size>>2)+(im_size>>3)) && highres[i+a+2]==highres[i+a+1])
			{
				a++;
				if (a<7) goto RES_COMPR3;
				//if (a==7) goto END_RES3;
				if (a==7 || res==1) 
				{
					res=1;
					if (a<14) goto RES_COMPR3; 
					else goto END_RES3;
				}
			}
END_RES3:	
			i+=(a+1);

			if (res==1)
			{
				ch_comp[j]=64+ (7<<3) + a-7;
			}
			else
			{
				i++;
				ch_comp[j]=64+ (a<<3);

				int d0 = highres[i]-highres[i-1];
				int d1 = highres[i+1]-highres[i];
				int d2 = highres[i+2]-highres[i+1];
			
				if (d0==4) 
				{
					if (d1==-4) 
					{
						if (d2==0) {ch_comp[j]+=3;i+=2;}
						else  {ch_comp[j]+=2;i++;}
					}
					else ch_comp[j]+=1;
				}
				else if (d0==-4)
				{
					if (d1==4) 
					{
						if (d2==0) {ch_comp[j]+=4;i+=2;}
						else  {ch_comp[j]+=5;i++;}
					}
					else ch_comp[j]+=6;
				
				}
				else if (d0==8) ch_comp[j]+=7;
				else i--;
			}

			a=0;res=0;
			j++;
		}
		else if (abs(d0)<=4 && abs(d1)<=4)
		{
			if (!d0 && d1==4) res=0;
			else if (!d0 && d1==-4) res=1;
			else if (d0==4 && !d1) res=2;
			else if (d0==-4 && !d1) res=3;
			else if (d0==4 && d1==4) res=4;
			else if (d0==4 && d1==-4) res=5;
			else if (d0==-4 && d1==4) res=6;
			else if (d0==-4 && d1==-4) res=7;

			if (!(highres[i+2]-highres[i+1])) 
			{
				ch_comp[j++]=128 + 64 + (res<<2);i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==4) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 1;i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==-4) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 2;i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==8) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 3;i+=2;
			}
			else 
			{
				d0+=16;d1+=16;
				ch_comp[j++]= (d0<<1) + (d1>>2);
				i++;
			}

			res=0;
		}
		else if (abs(d0)<=16 && abs(d1)<=16)
		{
			d0+=16;d1+=16;//e=(highres[i+1]>>2)&1;
			if (d0==32 || d1==32) 
			{
				ch_comp[j++]=(1<<7)+(highres[i]>>2);
				//if (highres[i+1]==128) {ch_comp[j-1]+=(1<<6);i++;}
				//else {ch_comp[j++]=(1<<7)+(highres[i+1]>>2);i++;}
			}
			/*else if (((highres[i+2]-highres[i+1])==0 && !e)|| ((highres[i+2]-highres[i+1])==4 && e==1))
			{
				ch_comp[j++]=128 + 64 + (d0<<1) + (d1>>2);i+=2;
			}*/
			else 
			{
				ch_comp[j++]= (d0<<1) + (d1>>2);
				//if (highres[i+2]==highres[i+1] && i<12286) { ch_comp[j-1]+=1;i+=2;}
				//else i++;
				i++;
			}
		}
		else
		{
			ch_comp[j++]=(1<<7)+(highres[i]>>2);
			//if (highres[i+1]==128) {ch_comp[j-1]+=(1<<6);i++;}
			//else {ch_comp[j++]=(1<<7)+(highres[i+1]>>2);i++;}

		}
	}

	enc->res_ch = (unsigned char*) calloc(j, sizeof(char));
	enc->end_ch_res = j;
	memcpy(enc->res_ch, ch_comp, enc->end_ch_res * sizeof(char));
}


