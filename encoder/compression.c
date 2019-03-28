#include "codec.h"

static
void shuffle_quadpacks(unsigned char *s, const short *pr, int step)
{
	int i, j;

	// Re-order coefficients in four-blocks:


	for (j=0;j<(IM_DIM<<1);)
	{
		for (i=0;i<IM_DIM;i++)
		{
			*s++ = pr[0]; *s++ = pr[1]; *s++ = pr[2]; *s++ = pr[3];
	
			j += step;
			pr += step;

			// Next line in reverse order:
			*s++ = pr[3]; *s++ = pr[2]; *s++ = pr[1]; *s++ = pr[0];

			j += step;
			pr += step;
		}

		j-=((4*IM_SIZE)-4);
		pr-=((4*IM_SIZE)-4);
	}
}

void scan_run_code(unsigned char *s, const short *pr, encode_state *enc)
{
	int i, j, count;

	int step = 2 * IM_DIM;

	shuffle_quadpacks(s, pr, step);

	for (i=0;i<4*IM_SIZE-4;i++)
	{
		if (s[i]!=128 && s[i+1]==128)
		{
			if (s[i+2]==128)
			{
				if  (s[i+3]==128)
				{
					if (s[i]==136 && s[i+4]==136)
						{s[i] = 132; s[i+4]=201;i+=4;}
					else if (s[i]==136 && s[i+4]==120)
						{s[i] = 133; s[i+4]=201;i+=4;}
					else if (s[i]==120 && s[i+4]==136)
						{s[i] = 134; s[i+4]=201;i+=4;}
					else if (s[i]==120 && s[i+4]==120)
						{s[i] = 135; s[i+4]=201;i+=4;}

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

	s[0]=128;s[1]=128;s[2]=128;s[3]=128;
	s[(4*IM_SIZE)-4]=128;
	s[(4*IM_SIZE)-3]=128;
	s[(4*IM_SIZE)-2]=128;
	s[(4*IM_SIZE)-1]=128;

	for (i=4,enc->nhw_select1=0,enc->nhw_select2=0,count=0;i<((4*IM_SIZE)-4);i++)
	{
		if (s[i]==136)
		{
			if (s[i+2]==128 && (s[i+1]==120 || s[i+1]==136)
			 && s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && (s[i+1]==120 || s[i+1]==136)
			      && s[i+2]==128 && s[i+3]==128 && s[i+4]==128 && s[i+5]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && s[i-2]==128 && s[i-3]==128
			      && s[i-4]==128 && s[i+1]==128)
			{
				s[i]=153;enc->nhw_select1++;
			}
			else if (s[i-1]==128 && s[i+1]==128 && s[i+2]==128
			      && s[i+3]==128 && s[i+4]==128)
			{
				s[i]=153;enc->nhw_select1++;
			}
		}
		else if (s[i]==120)
		{
			if (s[i+2]==128 && (s[i+1]==120 || s[i+1]==136)
			 && s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && (s[i+1]==120 || s[i+1]==136)
			      && s[i+2]==128 && s[i+3]==128 && s[i+4]==128 && s[i+5]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && s[i-2]==128 && s[i-3]==128
			      && s[i-4]==128 && s[i+1]==128)
			{
				s[i]=155;enc->nhw_select1++;
			}
			else if (s[i-1]==128 && s[i+1]==128 && s[i+2]==128
			      && s[i+3]==128 && s[i+4]==128)
			{
				s[i]=155;enc->nhw_select1++;
			}
		}
	}


	for (i=0,count=0;i<(4*IM_SIZE);i++)
	{
		while (s[i]==128 && s[i+1]==128)   
		{
			count++;

			if (count>255)
			{
				if (s[i]==153) s[i]=124;
				else if (s[i]==155) s[i]=123;

				if (s[i+1]==153) s[i+1]=124;
				else if (s[i+1]==155) s[i+1]=123;

				if (s[i+2]==153) s[i+2]=124;
				else if (s[i+2]==155) s[i+2]=123;

				if (s[i+3]==153) s[i+3]=124;
				else if (s[i+3]==155) s[i+3]=123;

				i--;count=0;
			}
			else i++;
		}
		 
		if (count>=252) {
			if (s[i+1]==153) s[i+1]=124;
			else if (s[i+1]==155) s[i+1]=123;
		}

		count=0;
	}
}


