/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: compress_pixel.c  										           *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

 Copyright (C) 2007-2015 NHW Project
   Written by Raphael Canut - nhwcodec_at_gmail.com

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
#include "compression.h"
#include "tables.h"

int g_debug = 0;

void dump_values_u16(unsigned short *u16, int n, const char *filename, const char *comment);

static unsigned short s_nhw_table1[512] = {
1024, 0, 1537, 0, 1538, 0, 2053, 2054,
0, 0, 2051, 2052, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 2567, 
2568, 2569, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 3082, 3083, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 3596, 
3597, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4110, 
4111, 4112, 4113, 4114, 4115, 4116, 4117, 4118, 
4119, 4120, 4121, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4634, 
4635, 4636, 4637, 4638, 4639, 4640, 4641, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4642, 
4643, 4644, 4645, 4646, 4647, 4648, 4649, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0};


static unsigned short s_nhw_table2[512] = {
5686, 0, 0, 0, 0, 0, 0, 0, 
5687, 0, 0, 0, 0, 0, 0, 0, 
6754, 0, 6755, 0, 6756, 0, 6757, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
6226, 0, 0, 0, 6227, 0, 0, 0, 
6228, 0, 0, 0, 6229, 0, 0, 0, 
6230, 0, 0, 0, 6231, 0, 0, 0, 
6232, 0, 0, 0, 6233, 0, 0, 0, 
5170, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5171, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5172, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5173, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5162, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5163, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5164, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5165, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5166, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5167, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5168, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5169, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5688, 0, 0, 0, 0, 0, 0, 0, 
5689, 0, 0, 0, 0, 0, 0, 0, 
7270, 7271, 7272, 7273, 7274, 7275, 7276, 7277, 
0, 0, 0, 0, 0, 0, 0, 0, 
6234, 0, 0, 0, 6235, 0, 0, 0, 
6236, 0, 0, 0, 6237, 0, 0, 0, 
6238, 0, 0, 0, 6239, 0, 0, 0, 
6240, 0, 0, 0, 6241, 0, 0, 0, 
5706, 0, 0, 0, 0, 0, 0, 0, 
5707, 0, 0, 0, 0, 0, 0, 0, 
5708, 0, 0, 0, 0, 0, 0, 0, 
5709, 0, 0, 0, 0, 0, 0, 0, 
5710, 0, 0, 0, 0, 0, 0, 0, 
5711, 0, 0, 0, 0, 0, 0, 0, 
5712, 0, 0, 0, 0, 0, 0, 0, 
5713, 0, 0, 0, 0, 0, 0, 0, 
5690, 0, 0, 0, 0, 0, 0, 0, 
5691, 0, 0, 0, 0, 0, 0, 0, 
5692, 0, 0, 0, 0, 0, 0, 0, 
5693, 0, 0, 0, 0, 0, 0, 0, 
5694, 0, 0, 0, 0, 0, 0, 0, 
5695, 0, 0, 0, 0, 0, 0, 0, 
5696, 0, 0, 0, 0, 0, 0, 0, 
5697, 0, 0, 0, 0, 0, 0, 0, 
5698, 0, 0, 0, 0, 0, 0, 0, 
5699, 0, 0, 0, 0, 0, 0, 0, 
5700, 0, 0, 0, 0, 0, 0, 0, 
5701, 0, 0, 0, 0, 0, 0, 0, 
5702, 0, 0, 0, 0, 0, 0, 0, 
5703, 0, 0, 0, 0, 0, 0, 0, 
5704, 0, 0, 0, 0, 0, 0, 0, 
5705, 0, 0, 0, 0, 0, 0, 0};

static char extra_table[256] = {
0,0,0,0,0,0,0,0,0,0,1,0,2,0,3,0,0,0,4,0,5,0,6,0,0,0,7,0,8,0,9,0,0,0,10,0, 
11,0,12,0,0,0,13,0,14,0,15,0,0,0,16,0,17,0,18,0,0,0,19,0, 
-1,0,-2,0,0,0,-3,0,-4,0,-5,0,0,0,-6,0,-7,0,-8,0,0,0,-9,0,-10,0, 
-11,0,0,0,-12,0,-13,0,-14,0,0,0,-15,0,-16,0,-17,0,0,0,-18,0,-19,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0
};

#define INV_QUANT1 125
#define INV_QUANT2 131

short reverse_offset_correction_coding(unsigned char v)
{
	short s;

	switch (v) {

	case CODE_WORD_127: return CODE_NHW_NB_1;
	case CODE_WORD_129: return CODE_NHW_NB_2;
	case CODE_WORD_125: return CODE_NHW_NB_5;
	case CODE_WORD_126: return CODE_NHW_NB_6;
	case CODE_WORD_121: return CODE_NHW_NB_3;
	case CODE_WORD_122: return CODE_NHW_NB_4;
	default:
		if (extra_table[v] > 0) {
			return WVLT_ENERGY_NHW + (extra_table[v] << 3);
		} else if (extra_table[v] < 0) {
			return (extra_table[v] << 3) - WVLT_ENERGY_NHW;
		} else {
			if      (v > 128) { s = v - INV_QUANT1; }
			else if (v < 128) { s = v - INV_QUANT2; }
			else s = 0;
			return s;
		}
	}
}

short reverse_offset_correction_coding_UV(unsigned char v)
{
	short s;
											
	switch (v) {
		case CODE_WORD_124: s = 5005; break;
		case CODE_WORD_126: s = 5006; break;
		case CODE_WORD_122: s = 5003; break;
		case CODE_WORD_130: s = 5004; break;
		default:
			if (extra_table[v] > 0) {
				return WVLT_ENERGY_NHW + (extra_table[v] << 3);
			} else if (extra_table[v] < 0) {
				return (extra_table[v] << 3) - WVLT_ENERGY_NHW;
			} else {
				if      (v > 128) { s = v - INV_QUANT1; }
				else if (v < 128) { s = v - INV_QUANT2; }
				else s = 0;
			}
		}
	return s;
}

int decode_codebook(unsigned short *book, unsigned char *tree, int len, int debug)
{
	int i, j, e;

	unsigned char *decode1, *tree_out;

	decode1=(unsigned char*)calloc(DEPTH1,sizeof(short));
	for (i=0,e=0;i<len;i++) 
	{
		if (tree[i]== CODE_TREE1_RLE)
		{
			for (j=0;j<tree[i+1];j++) decode1[e++]=tree[i];
			i++;
		}
		else decode1[e++]=tree[i];
	}

	tree_out =(unsigned char*)calloc(DEPTH1<<1,sizeof(char));

	// Re-shuffle RLE decoded tree (even/odd):
	for (i=0,j=0;i<e;i+=2) tree_out[i]=decode1[j++];
	for (i=1;i<e;i+=2)     tree_out[i]=decode1[j++];

	for (i=0,j=0;i<e;i++)
	{
		// When RLE code occuring, repeat by # of repetitions:
		if (tree_out[i]== CODE_TREE1_RLE) {book[j++]=((tree_out[i+1]<<8)|128);i++;}
		else {book[j++]=(256|(tree_out[i]&0xff));}
	}

	free(decode1);
	free(tree_out);

	return j;
}


void retrieve_pixel_Y_comp(image_buffer *im,decode_state *os,int p1,unsigned int *d1,short *im3)
{
	int i,j,tr,size,path,temp1,e,word,zone,zone_number,mem,mem2,nhw_ac1,run_over,t,t2;
	unsigned short *ntree,*nhw_book,dec;
	unsigned char *dec_select_word1,*dec_select_word2,*nhw_rle;

	// Lazy padding:
	dec_select_word1=(unsigned char*)calloc(((os->nhw_select1<<3) + 8),sizeof(char));

	for (i=0,e=0;i<os->nhw_select1;i++)
	{
		dec_select_word1[e++]=(os->nhw_select_word1[i]>>7);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>6)&1);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>5)&1);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>4)&1);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>3)&1);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>2)&1);
		dec_select_word1[e++]=((os->nhw_select_word1[i]>>1)&1);
		dec_select_word1[e++]=(os->nhw_select_word1[i]&1);
	}

	free(os->nhw_select_word1);

	// Lazy padding:
	dec_select_word2=(unsigned char*)calloc(((os->nhw_select2<<3) + 8), sizeof(char));

	for (i=0,e=0;i<os->nhw_select2;i++)
	{
		dec_select_word2[e++]=(os->nhw_select_word2[i]>>7);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>6)&1);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>5)&1);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>4)&1);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>3)&1);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>2)&1);
		dec_select_word2[e++]=((os->nhw_select_word2[i]>>1)&1);
		dec_select_word2[e++]=(os->nhw_select_word2[i]&1);
	}

	free(os->nhw_select_word2);


	if (im->setup->RES_HIGH>=4) zone_number=2;else zone_number=1;

	//RETRIEVE BOOKS
	//

	unsigned short *codebook=(unsigned short*)calloc(os->d_size_tree1,sizeof(short));

	j = decode_codebook(codebook, os->d_tree1, os->d_size_tree1, os->debug);

	if (os->debug) {
		char comment[64];
		sprintf(comment, "Decoder, effective book size %d", j);
		dump_values_u16(codebook, os->d_size_tree1, "/tmp/codebook.dat", comment);
	}

	free(os->d_tree1); // no longer needed

	nhw_rle=(unsigned char*)malloc(j*sizeof(char));

	for (i=0;i<j;i++) nhw_rle[i]=(codebook[i]>>8);


	nhw_book=(unsigned short*) codebook;

	// Decode packets over bitstream
	zone=zone_number;path=0;e=0;tr=0;size=0;ntree=s_nhw_table1;mem=0;mem2=0;nhw_ac1=0;run_over=-257;t=0;t2=0;
	for (i=0;;i++)
	{
		for (j=31;j>=0;j--)
		{
			if (zone&0x1)
			{
				// read 9 bit - detect special word (zone 1)
				if (j>=8) word=(d1[i]>>(j-8))&511;
				else { word=(d1[i]&((1<<(j+1))-1))<<(8-j);
					   word |= (d1[i+1]>>(24+j));}

				if (word==0x1)
				{
					if (j>=9) j-=9;
					else {i++;j+=23;}
					// read 6 bit
					if (j>=5) word=(d1[i]>>(j-5))&63;
					else { word=(d1[i]&((1<<(j+1))-1))<<(5-j);
						   word |= (d1[i+1]>>(27+j));}

					dec=word+ZONE1;
					if (j>=5) j-=5;
					else {i++;j+=27;}
					g_debug = __LINE__;
					goto SKIP_ZONE;
				}
				else 
				{
					zone=0;

					if (!(word>>7)) 
					{
						dec=0;
						if (j>=1) j-=1;
						else {i++;j+=31;}
						g_debug = __LINE__;
						goto SKIP_ZONE;
					}
					else if ((word>>6)==2) 
					{
						dec=1;
						if (j>=2) j-=2;
						else {i++;j+=30;}
						g_debug = __LINE__;
						goto SKIP_ZONE;
					}
					else if ((word>>6)==4) 
					{
						dec=2;
						if (j>=2) j-=2;
						else {i++;j+=30;}
						g_debug = __LINE__;
						goto SKIP_ZONE;
					}
					else 
					{
						tr = (word>>6);size+=3;
						if (j>=2) j-=2;
						else {i++;j+=30;}
						goto L_TREE;
					}
				}
			}

			tr |= (d1[i]>>j)&0x1;size++; 

			if (tr==0x1F) 
			{
				tr=0;ntree=s_nhw_table2;path=1;
				// read 5 next bit
				if (j>0)j--;else {j=31;i++;}
				if (j>=4) tr=(d1[i]>>(j-4))&31;
				else { tr=(d1[i]&((1<<(j+1))-1))<<(4-j);
					   tr |= (d1[i+1]>>(28+j));}
				if (j>=4) j-=4;
				else {i++;j+=28;}

				temp1=tr<<4;size+=5;
				dec=ntree[temp1];
				if (dec!=0 && size==dec>>9) goto L_STREAM;
				goto L_TREE;
			}

			if (path) 
			{
				if (size==0xb)
				{
					temp1=tr<<3;
					dec=ntree[temp1];
					if (dec!=0 && size==dec>>9) goto L_STREAM;

					if (tr==0x3) 
					{
						// read next 6 bit
						if (j>0)j--;else {j=31;i++;}
						if (j>=5) tr=(d1[i]>>(j-5))&63;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(5-j);
							   tr |= (d1[i+1]>>(27+j));}
						if (j>=5) j-=5;
						else {i++;j+=27;}
						dec=tr+110;
						goto L_STREAM;
					}
					else if (tr==0x23) 
					{
						// read next 6 bit
						if (j>0)j--;else {j=31;i++;}
						if (j>=5) tr=(d1[i]>>(j-5))&63;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(5-j);
							   tr |= (d1[i+1]>>(27+j));}

						if (tr<46)
						{
							dec=tr+174;if (j>=5) j-=5;else {i++;j+=27;}
							goto L_STREAM;
						}
						else if (tr<52)
						{
							// read 7 bit
							if (j>=6) tr=(d1[i]>>(j-6))&127;
							else { tr=(d1[i]&((1<<(j+1))-1))<<(6-j);
								    tr |= (d1[i+1]>>(26+j));}
							dec= (tr>>1) + ((tr>>1)-46) + (tr&1) + 174;
							if (j>=6) j-=6;
							else {i++;j+=26;}
							goto L_STREAM;
						}

						// read 8 bit
						if (j>=7) tr=(d1[i]>>(j-7))&255;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(7-j);
							   tr |= (d1[i+1]>>(25+j));}

						if (tr<246)
						{
							dec= 6 + (((tr>>2)-52)*3)+ (tr>>2) + (tr&3) + 174;
							if (j>=7) j-=7;
							else {i++;j+=25;}
							goto L_STREAM;
						}
						else
						{
							// read 9 bit
							if (j>=8) tr=(d1[i]>>(j-8))&511;
							else { tr=(d1[i]&((1<<(j+1))-1))<<(8-j);
							   tr |= (d1[i+1]>>(24+j));}
							dec=  tr - 492 + 270;
							if (j>=8) j-=8;
							else {i++;j+=24;}
							goto L_STREAM;
						}
					}

					goto L_TREE;
				}

				temp1=tr<<(14-size);
				dec=ntree[temp1];
				if (dec!=0 && size==dec>>9) goto L_STREAM;
				goto L_TREE;
			}

			dec=ntree[tr];

			if (dec!=0 && size==dec>>9) 
			{
L_STREAM:			dec&=MSW;
				if (dec>=ZONE1) if (zone_number==1) dec+=UNZONE1; 
SKIP_ZONE: 		
				if (dec >= os->d_size_tree1) {
					printf("@nhw_table1: tr = %d, val= %d  (0x%x)\n"
					       "word: 0x%04x\n", 
							tr, ntree[tr], ntree[tr], word);
					fprintf(stderr,
						"%s:%d bt: %d  >> Out of bounds in nhw_book[%d]: index %d\n",
						__FILE__, __LINE__, g_debug, os->d_size_tree1, dec);

					word = 0; // XXX
				} else {
					word=(unsigned char)nhw_book[dec];
				}

				if (word==0x80) 
				{
					mem++;

					if (mem2==1)
					{
						if (e>=5 && !im3[e-2] && !im3[e-3] && !im3[e-4] && !im3[e-5])
						{
							if (t2 >= os->nhw_select2<<3) {
								fprintf(stderr,
									"%s:%d Out of bounds in dec_select_word2[]\n",
									__FILE__, __LINE__);
							} else {
								if (!dec_select_word2[t2++]) im3[e++]=-11;else im3[e++]=11;
							}
						}
						else if (nhw_rle[dec]>=4 && !im3[e-2])
						{
							if (!dec_select_word2[t2++]) im3[e++]=-11;else im3[e++]=11;
						}

						mem2=0;
					}
					else if (mem==2 && !nhw_ac1) 
					{
						if (e>=4 && !im3[e-1] && !im3[e-2] && !im3[e-3] && !im3[e-4] && ((e+nhw_rle[dec]-257)>=run_over))
						{
							if (!dec_select_word1[t++]) im3[e++]=11;
							else im3[e++]=-11;
							
							mem=1;
						}
						else if (nhw_rle[dec]>=4 && e>0 && !im3[e-1] && !nhw_ac1 && ((e+nhw_rle[dec]-257)>=run_over))
						{
							if (!dec_select_word1[t++]) im3[e++]=11;
							else im3[e++]=-11;
							
							mem=1;
						}
					}
					else if (nhw_rle[dec]>=4 && e>0 && !im3[e-1] && !nhw_ac1 && ((e+nhw_rle[dec]-257)>=run_over) )
					{
						if (!dec_select_word1[t++]) im3[e++]=11;
						else im3[e++]=-11;
							
						mem=1;
					}

					if (nhw_rle[dec]==254) {nhw_ac1=1;mem=0;run_over=e;}
					else nhw_ac1=0;

					e += nhw_rle[dec];
				}
				else
				{
					mem=0;mem2=0;nhw_ac1=0;

					if (IS_CODE_WORD(word))
					{
						switch (word) {
						case CODE_WORD_136:
							im3[e++]=11;mem2=1;goto L_SCAN_END;				
						case CODE_WORD_120: 
							im3[e++]=-11;mem2=1;goto L_SCAN_END;				
						case CODE_WORD_132: 
							im3[e]=11;e+=4;im3[e++]=11;goto L_SCAN_END;				
						case CODE_WORD_133: 
							im3[e]=11;e+=4;im3[e++]=-11;goto L_SCAN_END;	
						case CODE_WORD_134: 
							im3[e]=-11;e+=4;im3[e++]=11;goto L_SCAN_END;				
						case CODE_WORD_135: 
							im3[e]=-11;e+=4;im3[e++]=-11;goto L_SCAN_END;	
						case CODE_WORD_127: 
							im3[e++]=CODE_NHW_NB_1;goto L_SCAN_END;
							//im3[e]=12;e+=4;im3[e++]=-20;goto L_SCAN_END;	
						case CODE_WORD_129: 
							im3[e++]=CODE_NHW_NB_2;goto L_SCAN_END;
							//im3[e]=12;e+=4;im3[e++]=-20;goto L_SCAN_END;	
						case CODE_WORD_125: 
							im3[e++]=CODE_NHW_NB_5;goto L_SCAN_END;
							//im3[e]=12;e+=4;im3[e++]=-20;goto L_SCAN_END;	
						case CODE_WORD_126: 
							im3[e++]=CODE_NHW_NB_6;goto L_SCAN_END;
							//im3[e]=-12;e+=4;im3[e++]=12;goto L_SCAN_END;	
						case CODE_WORD_121: 
							im3[e++]=CODE_NHW_NB_3;goto L_SCAN_END;
						case CODE_WORD_122: 
							im3[e++]=CODE_NHW_NB_4;goto L_SCAN_END;	
						case CODE_WORD_124: 
							im3[e++]=11;goto L_SCAN_END;	
						case CODE_WORD_123: 
							im3[e++]=-11;goto L_SCAN_END;	
						}
					}

					if (word<ZONE1)
					{
						if (!extra_table[word]) goto L_INVQ;
						else if (extra_table[word]>0) 
						{
							im3[e++]=WVLT_ENERGY_NHW+(extra_table[word]<<3);
						}
						else 
						{
							im3[e++]=(extra_table[word]<<3)-WVLT_ENERGY_NHW;
						}
					}
					else
					{
L_INVQ:					if (word>0x80) im3[e++]=(word-INV_QUANT1);
						else im3[e++]=(word-INV_QUANT2);
					}
				}

L_SCAN_END:	  		if (e>=(p1-3)) goto L4;
				tr=0;size=0;if (path) {path=0;ntree=s_nhw_table1;} zone=zone_number;
			}

L_TREE:		tr <<=1;
		}

	}


L4:	free(codebook);
	free(dec_select_word1);
	free(dec_select_word2);
	free(nhw_rle);
}

void retrieve_pixel_UV_comp(image_buffer *im,decode_state *os,int p1,unsigned int *d1,short *im3)
{
	int i,j,tr,size,path,temp1,e,word;
	unsigned short *ntree,dec;
	unsigned char *decode1;

	decode1=(unsigned char*)calloc(DEPTH1,sizeof(short));

	//RETRIEVE BOOKS
	for (i=0,e=0;i<os->d_size_tree2;i++) 
	{
		if (os->d_tree2[i]==128)
		{
			for (j=0;j<os->d_tree2[i+1];j++) decode1[e++]=os->d_tree2[i];
			i++;
		}
		else decode1[e++]=os->d_tree2[i];
	}

	e=os->tree_end;
	free(os->d_tree2);
	os->d_tree2=(unsigned char*)calloc(DEPTH1<<1,sizeof(char));

	for (i=0,j=0;i<e;i+=2) os->d_tree2[i]=decode1[j++];
	for (i=1;i<e;i+=2) os->d_tree2[i]=decode1[j++];

	unsigned short *codebook =(unsigned short*)calloc(e,sizeof(short));

	for (i=0,j=0;i<e;i++)
	{
		if (!(os->d_tree2[i]&0x1)) {codebook[j++]=((os->d_tree2[i+1]<<8)|(os->d_tree2[i]));i++;}
		else {codebook[j++]=(256|(os->d_tree2[i]&0xfe));}
	}

	free(os->d_tree2);
	free(decode1);

	// Decode packets over bitstream
	path=0;e=0;tr=0;size=0;ntree=s_nhw_table1;
	for (i=0;;i++)
	{
		for (j=31;j>=0;j--)
		{
			tr |= (d1[i]>>j)&0x1;size++; 

			if (tr==0x1F) 
			{
				tr=0;ntree=s_nhw_table2;path=1;
				// read 5 next bit
				if (j>0)j--;else {j=31;i++;}
				if (j>=4) tr=(d1[i]>>(j-4))&31;
				else { tr=(d1[i]&((1<<(j+1))-1))<<(4-j);
					   tr |= (d1[i+1]>>(28+j));}
				if (j>=4) j-=4;
				else {i++;j+=28;}

				temp1=tr<<4;size+=5;
				dec=ntree[temp1];
				if (dec!=0 && size==dec>>9) goto L_STREAMUV;
				goto L_TREEUV;
			}

			if (path) 
			{
				if (size==0xb)
				{
					temp1=tr<<3;
					dec=ntree[temp1];
					if (dec!=0 && size==dec>>9) goto L_STREAMUV;

					if (tr==0x3) 
					{
						// read next 6 bit
						if (j>0)j--;else {j=31;i++;}
						if (j>=5) tr=(d1[i]>>(j-5))&63;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(5-j);
							   tr |= (d1[i+1]>>(27+j));}
						if (j>=5) j-=5;
						else {i++;j+=27;}
						dec=tr+110;
						goto L_STREAMUV;
					}
					else if (tr==0x23) 
					{
						// read next 6 bit
						if (j>0)j--;else {j=31;i++;}
						if (j>=5) tr=(d1[i]>>(j-5))&63;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(5-j);
							   tr |= (d1[i+1]>>(27+j));}

						if (tr<46)
						{
							dec=tr+174;if (j>=5) j-=5;else {i++;j+=27;}
							goto L_STREAMUV;
						}
						else if (tr<52)
						{
							// read 7 bit
							if (j>=6) tr=(d1[i]>>(j-6))&127;
							else { tr=(d1[i]&((1<<(j+1))-1))<<(6-j);
								    tr |= (d1[i+1]>>(26+j));}
							dec= (tr>>1) + ((tr>>1)-46) + (tr&1) + 174;
							if (j>=6) j-=6;
							else {i++;j+=26;}
							goto L_STREAMUV;
						}

						// read 8 bit
						if (j>=7) tr=(d1[i]>>(j-7))&255;
						else { tr=(d1[i]&((1<<(j+1))-1))<<(7-j);
							   tr |= (d1[i+1]>>(25+j));}

						if (tr<246)
						{
							dec= 6 + (((tr>>2)-52)*3)+ (tr>>2) + (tr&3) + 174;
							if (j>=7) j-=7;
							else {i++;j+=25;}
							goto L_STREAMUV;
						}
						else
						{
							// read 9 bit
							if (j>=8) tr=(d1[i]>>(j-8))&511;
							else { tr=(d1[i]&((1<<(j+1))-1))<<(8-j);
							   tr |= (d1[i+1]>>(24+j));}
							dec=  tr - 492 + 270;
							if (j>=8) j-=8;
							else {i++;j+=24;}
							goto L_STREAMUV;
						}
					}

					goto L_TREEUV;
				}

				temp1=tr<<(14-size);
				dec=ntree[temp1];
				if (dec!=0 && size==dec>>9) goto L_STREAMUV;
				goto L_TREEUV;
			}

			dec=ntree[tr];

			if (dec!=0 && size==dec>>9) 
			{
L_STREAMUV: 	word=(unsigned char)codebook[(dec&MSW)];

				if (word==0x80 ) 
				{
					e += codebook[(dec&MSW)]>>8;
				}
				else
				{
					if (word<ZONE1)
					{
						if (!extra_table[word]) goto L_INVQUV;
						else if (extra_table[word]>0) 
						{
							im3[e++]=WVLT_ENERGY_NHW+(extra_table[word]<<3);
						}
						else 
						{
							im3[e++]=(extra_table[word]<<3)-WVLT_ENERGY_NHW;
						}
					}
					else
					{
						if (word==124) {im3[e++]=5005;goto L_END_UV;}
						else if (word==126) {im3[e++]=5006;goto L_END_UV;}
						else if (word==122) {im3[e++]=5003;goto L_END_UV;}
						else if (word==130) {im3[e++]=5004;goto L_END_UV;}

L_INVQUV:				if (word>0x80) 
						{
							im3[e++]=(word-INV_QUANT1);
						}
						else 
						{
							im3[e++]=(word-INV_QUANT2);
						}
					}
				}
L_END_UV:		if (e>=(p1-3)) goto L4UV;
				tr=0;size=0;path=0;ntree=s_nhw_table1;
			}

L_TREEUV:		tr <<=1;
		}
	}

L4UV:	free(codebook);

}
