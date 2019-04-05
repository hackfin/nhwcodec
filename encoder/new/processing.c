#include "codec.h"
#include "utils.h"

#if 0
void offsetY_recons256_part0(image_buffer *im, encode_state *enc, int m1)
{
	int i,j,a,t;
	short *nhw1,*highres_tmp;
	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;
	short thresh0, thresh1;

	nhw1=(short*)im->im_process;

	if (im->setup->quality_setting>LOW3) {
		offsetY_recons256_q3_part0(im, nhw1);
		thresh0 = 10000;
		thresh1 = 1;
	} else {
		thresh0 = -32768;
		thresh1 = 32767;
	}

	for (i=0,a=0,t=0;i<im_size;i+=step)
	{
		short *p = &im->im_process[i];
		short *q = &im->im_jpeg[i];

		for (a=i,j=0;j<(step/4);j++,a++, p++, q++) 
		{

			if (p[0]>10000) 
			{
				q[0]=p[0];
				continue;
			}
			/*else if (!part && im->setup->quality_setting<=LOW3 && j>0 && j<((step/4)-1) && abs(p[-1]-p[1])<1 && abs(p[-1]-p[0])<=3)
		 	{
				p[0]=p[-1];//p[1]=p[-1];
			}*/
			else if ((p[0]&1)==1 && a>i && (p[1]&1)==1 /*&& !(p[-1]&1)*/)
			{
				if (j<((step/4)-2) && (p[2]&1)==1 /*&& !(im->im_process[a+3]&1)*/)
				{
					if (abs(p[0]-p[2])> thresh1) p[1]++;
				}
				/*else if (j<((step/4)-4) && (p[2]&1)==1 && (p[3]&1)==1
						&& !(p[4]&1))
				{
					p[2]++;
				}*/
				else if (i<(im_size-step-2) && (p[step]&1)==1 
							&& (p[(step+1)]&1)==1 && !(p[(step+2)]&1))
				{
					if (p[step]<thresh0) p[step]++;
				}
			}
			else if ((p[0]&1)==1 && i>=step && i<(im_size-(3*step)))
			{
				if ((p[step]&1)==1 && (p[(step+1)]&1)==1)
				{
					if ((p[(2*step)]&1)==1 && !(p[(3*step)]&1)) 
					{
						if (p[step]<thresh0) p[step]++;
					}
				}
			}

		}
	}

	highres_tmp=(short*)malloc((im_size>>2)*sizeof(short));

	for (i=0,t=0;i<im_size;i+=step)
	{
		short *p = &nhw1[i];
		short *q = &im->im_jpeg[i];
		for (j=0;j<(step / 4);j++,p++, q++) 
		{
			if (p[0]<10000)
			{
				highres_tmp[t++]=p[0];
				if (p[0]>=0 && p[0]<256) q[0]=ROUND_2(p[0]);
				else q[0]=p[0]; 
			}
			else
			{
				p[0]-=16000;
				highres_tmp[t++]=p[0];
				q[0]=p[0];
			}
		}
	}

	if (im->setup->quality_setting>LOW5)
	{
		for (i=0;i<enc->highres_mem_len;i++)
		{
			j=(enc->highres_mem[i]>>7);
			a=enc->highres_mem[i]&127;

			im->im_jpeg[(j<<9)+a]=highres_tmp[enc->highres_mem[i]];
		}

		// XXX Don't free this here.
		// free(enc->highres_mem);
	}
	free(highres_tmp);



	
	if (im->setup->quality_setting>LOW4)
	{
		offsetY_recons256_q4_part0(im->im_process, im->im_jpeg, im->fmt.end / 4, step);
	}

	for (i=0;i< im_size;i+=(step))
	{
		for (j=(step/4);j<step/2;j++) 
		{
			int skip;
			short *p = &im->im_process[i+j]; // FIXME: optimize
			short *q = &im->im_jpeg[i+j];
			skip = offsetY_subbands_H4_part0(p, q, m1, j < step/2-1);
			j += skip;
		}
	}

	for (i=im_size;i<(2*im_size);i+=(step))
	{
		for (j=0;j<step/2;j++) 
		{
			int skip;
			short *p = &im->im_process[i+j];
			short *q = &im->im_jpeg[i+j];

			skip = offsetY_subbands_H4_part0(p, q, m1, j < step/2-1);
			j += skip;
		}
	}


	// Run through all subbands except LL:
	for (i=(step);i<((2*im_size)-(step));i+=(step))
	{
		short *q = &im->im_jpeg[i + 1];

		for (j=1;j<step/2-1;j++,q++) 
		{
			if (abs(q[0])>=8)
			{
				if (abs(q[-(step+1)])>=8) continue;
				if (abs(q[-(step)])>=8) continue;
				if (abs(q[-(step-1)])>=8) continue;
				if (abs(q[-1])>=8) continue;
				if (abs(q[1])>=8) continue;
				if (abs(q[(step-1)])>=8) continue;
				if (abs(q[(step)])>=8) continue;
				if (abs(q[(step+1)])>=8) continue;

				if (i>=im_size || j>=(step/4))
				{
					if (q[0]>0) q[0]--;
					else q[0]++;
				}
			}
		}
	}
}

#endif

extern short s_comp_lut_w0[12];

#ifndef PROTOTYPE
void process_residuals_simple(image_buffer *im, ResIndex *highres,
	const short *res256, encode_state *enc)
{
	int step = im->fmt.tile_size / 2;
	int quad_size = im->fmt.end / 4;
	int i, j;
	int count = 0;

	ResIndex *ch_comp[3];
	unsigned char *nhw_res_word[3];

	int n = 48 * im->fmt.tile_size + 1;
	ch_comp[0] = (ResIndex*) calloc(n, sizeof(ResIndex));
	ch_comp[1] = (ResIndex*) calloc(n, sizeof(ResIndex));
	ch_comp[2] = (ResIndex*) calloc(n, sizeof(ResIndex));

	nhw_res_word[0] = (unsigned char*) malloc(enc->res1.word_len * sizeof(char));
	nhw_res_word[1] = (unsigned char*) malloc(enc->res3.word_len * sizeof(char));
	nhw_res_word[2] = (unsigned char*) malloc(enc->res5.word_len * sizeof(char));

	const short *p = res256;

	unsigned char *res_p[3];
	res_p[0] = nhw_res_word[0];
	res_p[1] = nhw_res_word[1];
	res_p[2] = nhw_res_word[2];

	for (i = 0; i < quad_size; i += step) {

		for (j = 0; j < step-2; j++, p++) {
			short r = p[0];
			if (IS_OPCODE(r)) {
				if (r & RES1) {
					highres[count++]=j;
					*(res_p[0])++ = ((r & RI) >> RI_SHFT);
				}
				if (r & RES3) {
					highres[count++]=j;
					*(res_p[1])++ = ((r & RI) >> RI_SHFT);
				}
				if (r & RES5) {
					highres[count++]=j;
					*(res_p[2])++ = ((r & RI) >> RI_SHFT);
				}
			}
			
		}
		highres[count++]=step-2;
		p += 2; // skip last two columns
	}

	for (i = 0; i < 3; i++) free(ch_comp[i]);
}

#endif

void process_res_hq(image_buffer *im, const short *res256)
{
	int i, j;
	int k = 0;
	short *w;

	short *wl_first_order = im->im_wavelet_first_order;
	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	for (i=0;i<quad_size;i+=step, res256 += 2)
	{
		// Walk the transposed way:
		w = &wl_first_order[k++];
		for (j=0; j<step-2; j++, w += step)
		{
			short r = *res256++;

// Sign extend value in bit field:
#define _EXTENDBF(val, mask, msb)   \
	( ((val & mask) ^ (1 << (msb)) ) - (1 << (msb))) >> mask##_SHFT

			if (IS_OPCODE(r)) {
				short tmp = s_comp_lut_w0[(r & ADD_W0) >> ADD_W0_SHFT];
				// Probably a LUT is more efficient
				short tmp2 = _EXTENDBF(r, ADD_W1, ADD_W1_SHFT+2);
				w[0] += tmp;
				w[1] += tmp2;
				switch (r & ADD_W2) {
					case ADD_W2_PLUS2:  w[2] += 2; break;
					case ADD_W2_MINUS2: w[2] -= 2; break;
				}
			}
		}
	}
}

void translate_bitplanes(struct nhw_res *r, ResIndex *nhw_res_word,
	int e, ResIndex *highres, int count, int step, int mode)
{
	ResIndex *scan_run;
	ResIndex *ch_comp;
	ResIndex *nhw_res;

	int res;
	int i;
	int Y, stage;

	scan_run = &nhw_res_word[e];

	// Pad remaining with zeros:
	for (i = e % 8; i < 8; i++) *scan_run++ = 0;

	ch_comp=(ResIndex *)malloc(count*sizeof(ResIndex));
	memcpy(ch_comp,highres,count*sizeof(ResIndex));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(step-2))
		{
			if (ch_comp[i-1]!=(step-2) && ch_comp[i+1]!=(step-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	r->len = res;
	r->word_len = e;
	r->res =(ResIndex *) malloc(res * sizeof(ResIndex));

	int len = r->len;
	ResIndex *cur = r->res;

	int cur_word_len = e;

	for (i = 0; i < len; i++) cur[i] = highres[i];

	scan_run = (ResIndex *) calloc((len+8), sizeof(ResIndex));

	for (i = 0; i < len; i++) scan_run[i] = cur[i] >> 1;

	highres[0] = scan_run[0];

	for (i = 1, count=1; i < len-1; i++) {
		int d = scan_run[i]-scan_run[i-1];
		if (d>=0 && d<8) {
			int d1 = scan_run[i+1]-scan_run[i];
			if (d1 >= 0 && d1 < 16) {
				highres[count++]= 128 + (d << 4) + d1;
				i++;
			}
			else highres[count++] = scan_run[i];
		}
		else highres[count++] = scan_run[i];
	}

	for (i = 0, stage = 0;i < len; i++) {
		if (cur[i]!=254) scan_run[stage++] = cur[i];
	}

	res = (stage + 7) & ~7; // Round up to next mul 8 and zero-pad:
	for (i = stage; i < res; i++) scan_run[i] = 0;

	Y = res >> 3;
	int cur_bit_len = Y;
	ResIndex *res_bit = (ResIndex *) malloc(Y * sizeof(char));
	copy_bitplane0(scan_run, Y, res_bit);
	free(scan_run); // no longer needed

	if (mode) {
		nhw_res = (ResIndex *) malloc(((Y+1) * 2) * sizeof(ResIndex));

		const ResIndex *sp;
		sp = nhw_res_word;

		Y = cur_word_len;
		Y = (Y + 7) & ~7; // Round up to next multiple of 8 for padding
		Y >>= 2;

		for (i=0; i < Y;i++) {
#define _SLICE_BITS_POS(val, mask, shift) (((val) & mask) << shift)
			nhw_res[i] = _SLICE_BITS_POS(sp[0], 0x3, 6) |
									_SLICE_BITS_POS(sp[1], 0x3, 4) |
									_SLICE_BITS_POS(sp[2], 0x3, 2) |
									_SLICE_BITS_POS(sp[3], 0x3, 0);
			sp += 4;
		}
	} else {
		Y = cur_word_len + 7;
		Y = Y >> 3;
		nhw_res = (ResIndex *) malloc((Y+1)*sizeof(ResIndex));
		copy_bitplane0(nhw_res_word, Y, nhw_res);
	}
	cur_word_len = Y;

	for (i = 0; i < count; i++) cur[i] = highres[i];

	r->res_word = nhw_res;
	r->len = count;
	r->res_bit = res_bit;
	r->bit_len = cur_bit_len;
	r->word_len = cur_word_len;

}

void process_hires_q8(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j;
	unsigned char *nhw_res1I_word;
	int count, e;

	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	count = enc->res1.word_len;
	count = (count + 7) & ~7; // Round up to next multiple of 8 for padding

	nhw_res1I_word = (unsigned char*) malloc(count * sizeof(char));
	
	for (i=0,count=0,e=0;i<quad_size;i+=step)
	{
		short *p = &res256[i];

		for (j=0;j<step-2;j++, p++)
		{
			short r = p[0];
			if (IS_OPCODE(r)) {
				if (r & RES1) {
					highres[count++]=j;
					nhw_res1I_word[e++] = ((r & RI) >> RI_SHFT);
				}
			}
		}
		highres[count++]=(step-2);

	}
	translate_bitplanes(&enc->res1, nhw_res1I_word, e, highres, count, step, 0);
	free(nhw_res1I_word);
}

void process_res3_q1(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j, e, count;
	unsigned char *nhw_res3I_word;

	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	e = enc->res3.word_len;
	e = (e + 7) & ~7; // Round up to next multiple of 8 for padding

	nhw_res3I_word = (unsigned char*) calloc(e, sizeof(char));
	enc->res3.word_len = e;

	e = 0;

	for (i=0,count=0;i<quad_size;i+=step)
	{
		short *p = &res256[i];
		for (j=0;j<step-2;j++,p++) {
			short r = p[0];
			if (IS_OPCODE(r)) {
				if (r & RES3) {
					highres[count++]=j;
					nhw_res3I_word[e++] = ((r & RI) >> RI_SHFT);
				}
			}
		}
		highres[count++]=(step-2);
	}
	translate_bitplanes(&enc->res3, nhw_res3I_word, e, highres, count, step, 1);
	free(nhw_res3I_word);
}

void process_res5_q1(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j, e, count;
	unsigned char *nhw_res5I_word;

	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	nhw_res5I_word=(unsigned char*)calloc(enc->res5.word_len,sizeof(char));

	for (i=0,count=0,e=0;i<quad_size;i+=step)
	{
		short *p = &res256[i];
		for (j=0;j<step-2;j++,p++)
		{
			int r = p[0];
			if (IS_OPCODE(r)) {
				if (r & RES5) {
					highres[count++]=j;
					nhw_res5I_word[e++] = ((r & RI) >> RI_SHFT);
				}
			}
		}

		p[0]=0;
		p[1]=0;
		highres[count++]=j++;
	}

	translate_bitplanes(&enc->res5, nhw_res5I_word, e, highres, count, step, 0);
	free(nhw_res5I_word);
}


void process_residuals(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int quality = im->setup->quality_setting;

	if (quality > HIGH1) {
		// Evaluates the tags and compensates im_wavelet_first_order
		// Tags are not modified
		process_res_hq(im, res256);
	}

	process_hires_q8(im, highres, res256, enc);
	process_res3_q1(im, highres, res256, enc);
	process_res5_q1(im, highres, res256, enc);
}
