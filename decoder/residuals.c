#include <assert.h>
#include "codec.h"
#include "compression.h"

#define BUILD_INDEX(y, x, d) \
	((y) + ((x) << 8))


// FIXME: Check tiling support
#define NHW_INDEX(x) ((((&x)[0])&65280)<<1)+(((&x)[0])&255)

void inc_cond_bit(NhwIndex *res, unsigned short data)
{
	int k;
	for (k = 7; k >= 0; k--) {
		*res++ += (data >> k) & 1;
	}
}

void decode_res1(decode_state *os, NhwIndex *nhwres1, NhwIndex *nhwres2)
{
	int res;
	int e;
	int i, j, scan;
	int stage, count;
	NhwIndex *nhwres1I;
	
	nhwres1I = (NhwIndex *) calloc((os->res1.bit_len<<3), sizeof(NhwIndex));

	stage=0;

	if (os->res1.res[0]==127) {
		count=1;
	} else {
		count=0;
		nhwres1I[stage++]=BUILD_INDEX(os->res1.res[0]<<1, count, shift);
	}

	for (i=1;i<os->res1.len;i++)
	{
		if (os->res1.res[i]>=128)
		{
			e=(os->res1.res[i]-128);e>>=4;
			scan=os->res1.res[i]&15;
			if (os->res1.res[i-1]!=127)
				j=(nhwres1I[stage-1]&255)+(e<<1); // XXX
			else {os->res1.res[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->res1.res[i]=127;}
			else nhwres1I[stage++]=BUILD_INDEX(j, count, shift);

			j+=(scan<<1);
			if (j>=254) {count++;os->res1.res[i]=127;}
			else nhwres1I[stage++]=BUILD_INDEX(j, count, shift);
		}
		else
		{
			if (os->res1.res[i]==127) count++;
			else
			{
				if (((os->res1.res[i]<<1)<(nhwres1I[stage-1]&255)) && (os->res1.res[i-1]!=127)) count++;

				nhwres1I[stage++]=BUILD_INDEX(os->res1.res[i]<<1, count, shift);
			}
		}
	}

	for (i=0,count=0;i<os->res1.bit_len;i++)
	{
		inc_cond_bit(&nhwres1I[count], os->res1.res_bit[i]);
		count += 8;
	}

	free(os->res1.res);
	free(os->res1.res_bit);

	os->end_ch_res=0;os->d_size_tree1=0;

	int k; // mask

	for (i=0,count=0;i<os->res1.bit_len-1;i++)
	{
		unsigned char tmp = os->res1.res_word[i];

		for (k = 0x80; k != 0; k >>= 1) {
			if (!(tmp & k)) os->d_size_tree1++; else os->end_ch_res++;
		}
	}


	for (i=0,count=0,scan=0,res=0;i<os->res1.bit_len-1;i++)
	{
		unsigned char tmp = os->res1.res_word[i];

		for (k = 0x80; k != 0; k >>= 1) {
			if (!(tmp & k)) 
				nhwres2[scan++]=nhwres1I[count++];
			else
				nhwres1[res++]=nhwres1I[count++]; 
		}
	}

	free(nhwres1I);
	free(os->res1.res_word);

	os->res1.bit_len=os->end_ch_res;
	os->nhw_res2_bit_len=os->d_size_tree1;
	
}

void decode_res3(decode_state *os, NhwIndex *nhwres3, NhwIndex *nhwres4, NhwIndex *nhwres5,
	NhwIndex *nhwres6)
{
	int e;
	int nhw_selectII;
	int i, j, scan;
	int stage, count;
	NhwIndex *nhwres3I;

	nhwres3I=(NhwIndex *)calloc((os->res3.bit_len<<3),
		sizeof(NhwIndex));
	stage=0;
	os->d_size_tree1=0;os->end_ch_res=0;os->res_f1=0;os->res_f2=0;

	if (os->res3.res[0]==127) {
		count=1;
	} else {
		count=0;
		nhwres3I[stage++]=BUILD_INDEX(os->res3.res[0]<<1, count, shift);
	}

	for (i=1;i<os->res3.len;i++)
	{
		if (os->res3.res[i]>=128)
		{
			e=(os->res3.res[i]-128);e>>=4;
			scan=os->res3.res[i]&15;
			if (os->res3.res[i-1]!=127) j=(nhwres3I[stage-1]&255)+(e<<1);
			else {os->res3.res[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->res3.res[i]=127;}
			else nhwres3I[stage++]=BUILD_INDEX(j, count, shift);

			j+=(scan<<1);
			if (j>=254) {count++;os->res3.res[i]=127;}
			else nhwres3I[stage++]=BUILD_INDEX(j, count, shift);
		}
		else
		{
			if (os->res3.res[i]==127) count++;
			else
			{
				if (((os->res3.res[i]<<1)<(nhwres3I[stage-1]&255)) && (os->res3.res[i-1]!=127)) count++;

				nhwres3I[stage++]=(os->res3.res[i]<<1)+(count<<8);
			}
		}
	}

	for (i=0,count=0;i<os->res3.bit_len;i++)
	{
		inc_cond_bit(&nhwres3I[count], os->res3.res_bit[i]);
		count += 8;
	}

	free(os->res3.res);
	free(os->res3.res_bit);


	for (i=0,count=0,scan=0;i<((os->res3.bit_len<<1)-2);i++)
	{

		int k;
		for (k = 6; k >= 0; k -= 2) {

			nhw_selectII=((os->res3.res_word[i]>>k)&3);

			switch (nhw_selectII) {
				case 0:
					nhwres4[os->d_size_tree1++]=nhwres3I[count++]; break;
				case 1:
					nhwres3[os->end_ch_res++]=nhwres3I[count++]; break;
				case 2:
					nhwres5[os->res_f1++]=nhwres3I[count++]; break;
				default:
					nhwres6[os->res_f2++]=nhwres3I[count++];
			}
		}
	}

	free(nhwres3I);
	free(os->res3.res_word);

}


void decode_res5(decode_state *os, NhwIndex *nhwresH1, NhwIndex *nhwresH2)
{
	int res;
	int e;
	int i, j, scan;
	int stage, count;
	NhwIndex *nhwresH1I;
	
	// FIXME: Index list
	nhwresH1I=(NhwIndex *)calloc((os->res5.bit_len<<3),
		sizeof(NhwIndex));

	stage=0;

	if (os->res5.res[0]==127) {
		count=1;
	} else {
		count=0;
		nhwresH1I[stage++]= BUILD_INDEX(os->res5.res[0]<<1, count, shift);
	}

	for (i=1;i<os->res5.len;i++)
	{
		if (os->res5.res[i]>=128)
		{
			e=(os->res5.res[i]-128);e>>=4;
			scan=os->res5.res[i]&15;
			if (os->res5.res[i-1]!=127)
				j=(nhwresH1I[stage-1]&255)+(e<<1);
			else {os->res5.res[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->res5.res[i]=127;}
			else
				nhwresH1I[stage++] = BUILD_INDEX(j, count, shift);

			j+=(scan<<1);
			if (j>=254) {count++;os->res5.res[i]=127;}
			else
				nhwresH1I[stage++] = BUILD_INDEX(j, count, shift);
		}
		else
		{
			if (os->res5.res[i]==127) count++;
			else
			{
				if (((os->res5.res[i]<<1)<(nhwresH1I[stage-1]&255)) && (os->res5.res[i-1]!=127)) count++;

				nhwresH1I[stage++]=BUILD_INDEX(os->res5.res[i]<<1, count, shift);

			}
		}
	}

	for (i=0,count=0;i<os->res5.bit_len;i++) {
		inc_cond_bit(&nhwresH1I[count], os->res5.res_bit[i]);
		count += 8;
	}


	os->end_ch_res=0;os->d_size_tree1=0;
	for (i=0,count=0;i<os->res5.bit_len-1;i++)
	{
		unsigned char tmp = os->res5.res_word[i];
		int k;
		for (k = 0x80; k != 0; k >>= 1) {
			if (!(tmp & k)) os->d_size_tree1++; else os->end_ch_res++;
		}
	}

	for (i=0,count=0,scan=0,res=0;i<os->res5.bit_len-1;i++)
	{
		unsigned char tmp = os->res5.res_word[i];
		int k;
		for (k = 0x80; k != 0; k >>= 1) {
			if (!(tmp & k))
				nhwresH2[scan++]=nhwresH1I[count++];
			else
				nhwresH1[res++]=nhwresH1I[count++];
		}
	}

	free(nhwresH1I);

	os->res5.bit_len=os->end_ch_res;
	os->res5.len=os->d_size_tree1;
}

void decode_res4(image_buffer *im, decode_state *os)
{
	int i, e, count;
	int step = im->fmt.tile_size;

	for (i=0,e=0,count=0;i<os->nhw_res4_len;i++)
	{
		if (os->nhw_res4[i]==128) count += step;
		else if (os->nhw_res4[i]>128) 
		{
			e=count+os->nhw_res4[i]-129;
			if (!(im->im_jpeg[e]&1)) im->im_jpeg[e]++;
			if (!(im->im_jpeg[e+1]&1)) im->im_jpeg[e+1]++;
			if (!(im->im_jpeg[e+2]&1)) im->im_jpeg[e+2]++;
			if (!(im->im_jpeg[e+3]&1)) im->im_jpeg[e+3]++;
			//printf("\n %d",e);
			count +=step;
		}
		else 
		{
			e=count+os->nhw_res4[i]-1;
			if (!(im->im_jpeg[e]&1)) im->im_jpeg[e]++;
			if (!(im->im_jpeg[e+1]&1)) im->im_jpeg[e+1]++;
			if (!(im->im_jpeg[e+2]&1)) im->im_jpeg[e+2]++;
			if (!(im->im_jpeg[e+3]&1)) im->im_jpeg[e+3]++;
			//printf("\n %d",e);
		}
	}
}

void decode_res6(image_buffer *im, decode_state *os)
{
	int res;
	int e;
	int i, j, scan;
	int stage, count;
	int IM_DIM = im->fmt.tile_size / 2;

	NhwIndex *nhwresH3I;

	nhwresH3I=(NhwIndex *)calloc((os->nhw_res6_bit_len * 8),
			sizeof(NhwIndex)); // HACK: Pad extension, make consistent with encoder
		stage=0;

		if (os->nhw_res6[0]==127)
		{
			count=IM_DIM;
		}
		else 
		{
			nhwresH3I[stage++]=(os->nhw_res6[0]<<1);count=0;
		}

		for (i=1;i<os->nhw_res6_len;i++)
		{
			if (os->nhw_res6[i]>=128)
			{
				e=(os->nhw_res6[i]-128);e>>=4;
				scan=os->nhw_res6[i]&15;
				if (os->nhw_res6[i-1]!=127) j=(nhwresH3I[stage-1]&255)+(e<<1);
				else {os->nhw_res6[i]=127;count+=(2*IM_DIM);continue;}

				if (j>=254) {count+=IM_DIM;os->nhw_res6[i]=127;}
				else nhwresH3I[stage++]=(j)+count;

				j+=(scan<<1);
				if (j>=254) {count+=IM_DIM;os->nhw_res6[i]=127;}
				else nhwresH3I[stage++]=(j)+count;
			}
			else
			{
				if (os->nhw_res6[i]==127) count+=IM_DIM;
				else
				{
					if (((os->nhw_res6[i]<<1)<(nhwresH3I[stage-1]&255)) && (os->nhw_res6[i-1]!=127)) count+=IM_DIM;

					nhwresH3I[stage++]=(os->nhw_res6[i]<<1)+count;
				}
			}
		}

		for (i=0,count=0;i<os->nhw_res6_bit_len;i++)
		{
			inc_cond_bit(&nhwresH3I[count], os->nhw_res6_bit[i]);
			count += 8;
		}


		os->end_ch_res=0;os->d_size_tree1=0;
		for (i=0,count=0;i<os->nhw_res6_bit_len-1;i++)
		{
			unsigned char tmp = os->nhw_res6_word[i];
			int k;
			for (k = 0x80; k != 0; k >>= 1) {
				if (!(tmp & k)) os->d_size_tree1++; else os->end_ch_res++;
			}
		}

		os->nhwresH3=(unsigned int *)malloc(os->end_ch_res*sizeof(int));
		os->nhwresH4=(unsigned int *)malloc(os->d_size_tree1*sizeof(int));

		for (i=0,count=0,scan=0,res=0;i<os->nhw_res6_bit_len-1;i++)
		{
			unsigned char tmp = os->nhw_res6_word[i];
			int k;
			for (k = 0x80; k != 0; k >>= 1) {
				if (!(tmp & k))
					os->nhwresH4[scan++]=nhwresH3I[count++];
				else
					os->nhwresH3[res++]=nhwresH3I[count++];
			}
		}

		free(nhwresH3I);

		os->nhw_res6_bit_len=os->end_ch_res;
		os->nhw_res6_len=os->d_size_tree1;

}


int decode_exwY(image_buffer *im, decode_state *os)
{
	int i, count;
	int scan, exw1;

	for (i=0,exw1=0;i<os->exw_Y_end;i+=3,exw1+=3)
	{
		if (!os->exw_Y[i] && !os->exw_Y[i+1]) break;

		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		// Coordinate to offset:
		count=(os->exw_Y[i]<<im->fmt.tile_power)+os->exw_Y[i+1];
		assert(count < im->fmt.end);
		im->im_jpeg[count]=scan;

	}
	return exw1;
}

void residual_compensation(image_buffer *im, decode_state *os,
	NhwIndex *nhwresH1, NhwIndex *nhwresH2,
	NhwIndex *nhwres1, NhwIndex *nhwres2,
	NhwIndex *nhwres3, NhwIndex *nhwres4, NhwIndex *nhwres5, NhwIndex *nhwres6
)
{
	int i;
	short *pr = im->im_process;
	int e;
	int IM_DIM = im->fmt.tile_size / 2;

	if (os->res5.bit_len > 0)
	{
		for (i=0;i<os->res5.bit_len;i++) 
		{
			pr[NHW_INDEX(nhwresH1[i])]-=3;
		}
		free(nhwresH1);

		for (i=0;i<os->res5.len;i++) 
		{
			pr[NHW_INDEX(nhwresH2[i])]+=3;
		}
		free(nhwresH2);
	}
	
	if (os->res1.bit_len > 0)
	{
		if (im->setup->quality_setting>=LOW2) e=5;
		else if (im->setup->quality_setting>=LOW5) e=7;
		else e=9;

		for (i=0;i<os->res1.bit_len;i++) 
		{
			pr[NHW_INDEX(nhwres1[i])] -= e;

		}
		free(nhwres1);

		for (i=0;i<os->nhw_res2_bit_len;i++) 
		{
			pr[NHW_INDEX(nhwres2[i])] += e;
		}
		free(nhwres2);
	}


	if (os->res3.bit_len > 0)
	{
		for (i=0;i<os->end_ch_res;i++) 
		{
			pr[NHW_INDEX(nhwres3[i])] -= 4;
			pr[NHW_INDEX(nhwres3[i])+(2*IM_DIM)] -= 3;

		}
		free(nhwres3);

		for (i=0;i<os->d_size_tree1;i++) 
		{
			pr[NHW_INDEX(nhwres4[i])] += 4;
			pr[NHW_INDEX(nhwres4[i])+(2*IM_DIM)] += 3;

		}
		free(nhwres4);

		for (i=0;i<os->res_f1;i++) 
		{
			pr[NHW_INDEX(nhwres5[i])] += 2;
			pr[NHW_INDEX(nhwres5[i])+(2*IM_DIM)] += 2;
			pr[NHW_INDEX(nhwres5[i])+(4*IM_DIM)] += 2;
		}
		free(nhwres5);

		for (i=0;i<os->res_f2;i++) 
		{
			pr[NHW_INDEX(nhwres6[i])] -= 2;
			pr[NHW_INDEX(nhwres6[i])+(2*IM_DIM)] -= 2;
			pr[NHW_INDEX(nhwres6[i])+(4*IM_DIM)] -= 2;
		}
		free(nhwres6);
	}


}
