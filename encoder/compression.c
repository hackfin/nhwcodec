#include "codec.h"
#include "compression.h"

#define MARK_TMP_153  153
#define MARK_TMP_155  155

void copy_uv_chunks(unsigned char *dst, const short *src, int n)
{
	int i, j;
	int halfn = n / 2;

	const short *p;

	// Copies UV chunks the 'interleaved' way into the im_nhw array:
	//
	//  pr:
	//
	//  P0 P1 P2 P3 P4 P5 P6 P7 ...
	//  Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7 ...

	// -->
	//
	//  P0 .. P1 .. P2 .. P3 .. P4 .. P5 .. P6 .. P7 ...
	//  Q7 .. Q6 .. Q5 .. Q4 .. Q3 .. Q2 .. Q1 .. Q0 ...

	for (j = 0; j < n; j += 8) {
		p = &src[j];
		for (i=0;i < halfn ;i++) {
			int k;
			for (k = 0; k < 8; k++) {
				*dst = *p++; dst += 2;
			}
			p += n;

			// copy reverse:
			for (k = 0; k < 8; k++) {
				p--;
				*dst = *p; dst += 2;
			}
			p += n;
		}
	}
}


void shuffle_quadpacks(unsigned char *s, const short *pr, int n, int step)
{
	int i, j;

	// Re-order coefficients in four-blocks:
	//
	//  pr:
	//
	//  P0 P1 P2 P3 ...
	//  Q0 Q1 Q2 Q3 ...

	// -->
	//
	//  P0 P1 P2 P3
	//  Q3 Q2 Q1 Q0

	int im_dim = step / 2;

	for (j=0;j<step;)
	{
		for (i=0;i<im_dim;i++)
		{
			*s++ = pr[0]; *s++ = pr[1]; *s++ = pr[2]; *s++ = pr[3];
	
			j += step;
			pr += step;

			// Next line in reverse order:
			*s++ = pr[3]; *s++ = pr[2]; *s++ = pr[1]; *s++ = pr[0];

			j += step;
			pr += step;
		}

		j-=(n-4);
		pr-=(n-4);
	}
}



void code_y_chunks(image_buffer *im, unsigned char *nhwbuf, encode_state *enc)
{
	int i, count;

	unsigned char *s = nhwbuf;
	const short *pr = im->im_process;

	int n = im->fmt.end;
	int step = im->fmt.tile_size;

	shuffle_quadpacks(s, pr, n, step);

	// FIXME: Turn into state machine
	for (i=0;i<n-4;i++)
	{
		if (s[i]!=CODE_WORD_128 && s[i+1]==CODE_WORD_128)
		{
			if (s[i+2]==CODE_WORD_128)
			{
				if  (s[i+3]==CODE_WORD_128)
				{
					unsigned char *p = &s[i];
					unsigned char *q = &s[i+4];

					if      (*p == CODE_WORD_136 && *q == CODE_WORD_136)
						{*p = CODE_WORD_132; *q = 201; i+=4;}
					else if (*p == CODE_WORD_136 && *q == CODE_WORD_120)
						{*p = CODE_WORD_133; *q = 201; i+=4;}
					else if (*p == CODE_WORD_120 && *q == CODE_WORD_136)
						{*p = CODE_WORD_134; *q = 201; i+=4;}
					else if (*p == CODE_WORD_120 && *q == CODE_WORD_120)
						{*p = CODE_WORD_135; *q = 201; i+=4;}


					

					//else if (s[i]==136 && s[i+4]==112) {s[i]=127;i+=4;}
					//else if (s[i]==112 && s[i+4]==136) {s[i]=126;i+=4;}
					//else if (s[i]==136 && s[i+4]==144) {s[i]=125;i+=4;}
					//else if (s[i]==144 && s[i+4]==136) {s[i]=123;i+=4;}
					//else if (s[i]==120 && s[i+4]==112) {s[i]=121;i+=4;}
					//else if (s[i]==112 && s[i+4]==120) {s[i]=122;i+=4;}
					else i+=3;
				}
				else i+=2;
			}
			else i++;
		}
	}

	s[0]=CODE_WORD_128;s[1]=CODE_WORD_128;s[2]=CODE_WORD_128;s[3]=CODE_WORD_128;
	s[(n)-4]=CODE_WORD_128;
	s[(n)-3]=CODE_WORD_128;
	s[(n)-2]=CODE_WORD_128;
	s[(n)-1]=CODE_WORD_128;

	// Second pass:

#define _M(x) ((x) == CODE_WORD_128)

#define FOUR_LEADING(p)  \
	  _M((p)[-2]) && _M((p)[-3]) && _M((p)[-4])

#define FOUR_TRAILING(p)  \
	_M((p)[1]) && _M((p)[2]) && _M((p)[3]) && _M((p)[4])

	for (i=4,enc->nhw_select1=0,enc->nhw_select2=0,count=0;i<(n-4);i++)
	{
		char z = (s[i+1]==CODE_WORD_120) ? 157 : ((s[i+1]==CODE_WORD_136) ? 159 : 0);

		if (_M(s[i-1])) {
			if (s[i]==CODE_WORD_136) {
				if (FOUR_LEADING(&s[i])) {
					if (_M(s[i+1])) {
						s[i]=MARK_TMP_153;enc->nhw_select1++;
					} else
					if (z && _M(s[i+2])) {
						s[i+1]=z;
						enc->nhw_select2++;
					}
				} else {
					if (z && FOUR_TRAILING(&s[i+1])) {
						s[i+1]=z;
						enc->nhw_select2++;
					}
					else if (FOUR_TRAILING(&s[i])) {
						s[i]=MARK_TMP_153;enc->nhw_select1++;
					}

				}
			} else
			if (s[i]==CODE_WORD_120) {
				if (FOUR_LEADING(&s[i])) {
					if (z && _M(s[i+2])) {
						s[i+1]=z;
						enc->nhw_select2++;
					} else if (_M(s[i+1])) {
						s[i]=MARK_TMP_155;enc->nhw_select1++;
					}
				} else {
					if (z && FOUR_TRAILING(&s[i+1])) {
						s[i+1]=z;
						enc->nhw_select2++;
					}
					else if (FOUR_TRAILING(&s[i])) {
						s[i]=MARK_TMP_155;enc->nhw_select1++;
					}
				}
			}
		}

	}


	// Another pass:

	for (i=0,count=0;i<n;i++)
	{
		while (_M(s[i]) && _M(s[i+1]))   
		{
			count++;

			if (count>255)
			{
				if (s[i]==MARK_TMP_153) s[i]=CODE_WORD_124;
				else if (s[i]==MARK_TMP_155) s[i]=CODE_WORD_123;

				if (s[i+1]==MARK_TMP_153) s[i+1]=CODE_WORD_124;
				else if (s[i+1]==MARK_TMP_155) s[i+1]=CODE_WORD_123;

				if (s[i+2]==MARK_TMP_153) s[i+2]=CODE_WORD_124;
				else if (s[i+2]==MARK_TMP_155) s[i+2]=CODE_WORD_123;

				if (s[i+3]==MARK_TMP_153) s[i+3]=CODE_WORD_124;
				else if (s[i+3]==MARK_TMP_155) s[i+3]=CODE_WORD_123;

				i--;count=0;
			}
			else i++;
		}
		 
		if (count>=252) {
			if (s[i+1]==MARK_TMP_153) s[i+1]=CODE_WORD_124;
			else if (s[i+1]==MARK_TMP_155) s[i+1]=CODE_WORD_123;
		}

		count=0;
	}
}

