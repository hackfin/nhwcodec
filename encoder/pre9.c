
#include "codec.h"

void pre9(short *pr, const char *wvlt)
{
	int i, j, count, scan;
	int e;
	int s = 2 * IM_DIM;

	for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			if (abs(pr[scan+2]-pr[scan+1])<wvlt[6] && abs(pr[scan+2]-pr[scan])<wvlt[6]  && abs(pr[scan+1]-pr[scan])<wvlt[6])
			{
				count=scan+1;
					
				if (abs(pr[(count<<1)+IM_DIM])<wvlt[5]) pr[(count<<1)+IM_DIM]=0;
				if (abs(pr[(count<<1)+IM_DIM+1])<wvlt[5]) pr[(count<<1)+IM_DIM+1]=0;
				if (abs(pr[(count<<1)+(3*IM_DIM)])<wvlt[5]) pr[(count<<1)+(3*IM_DIM)]=0;
				if (abs(pr[(count<<1)+(3*IM_DIM)+1])<wvlt[5]) pr[(count<<1)+(3*IM_DIM)+1]=0;
					
				if (abs(pr[(count<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) pr[(count<<1)+(2*IM_SIZE)]=0;
				if (abs(pr[(count<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) pr[(count<<1)+(2*IM_SIZE)+1]=0;
				if (abs(pr[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) pr[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
				if (abs(pr[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) pr[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
					
				e=(2*IM_SIZE)+IM_DIM;
				if (abs(pr[(count<<1)+e])<34) pr[(count<<1)+e]=0;
				if (abs(pr[(count<<1)+e+1])<34) pr[(count<<1)+e+1]=0;
				if (abs(pr[(count<<1)+e+(2*IM_DIM)])<34) pr[(count<<1)+e+(2*IM_DIM)]=0;
				if (abs(pr[(count<<1)+e+(2*IM_DIM)+1])<34) pr[(count<<1)+e+(2*IM_DIM)+1]=0;
				
				//for (e=0;e<3;e++)
				//{
						if (abs(pr[count+(IM_DIM>>1)])<11) pr[count+(IM_DIM>>1)]=0;
						if (abs(pr[count+IM_SIZE])<12) pr[count+IM_SIZE]=0;
						if (abs(pr[count+IM_SIZE+(IM_DIM>>1)])<13) pr[count+IM_SIZE+(IM_DIM>>1)]=0;
				//}
			}
		}
	}
}
