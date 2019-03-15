#include "codec.h"

void pre7(int quality, short *pr, const char *wvlt)
{
	int i, j, count, scan;
	int a, e;
	char w;

	int s = 2 * IM_DIM;

	for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-4;j++,scan++)
		{
			short *p = &pr[scan];
			w = wvlt[0];

			if (abs(p[4]-p[0])<w &&
				abs(p[4]-p[3])<w && abs(p[1]-p[0])<w && 
				abs(p[3]-p[1])<w && abs(p[3]-p[2])<(wvlt[1]-2))
			{
				if      ((p[3]-p[1])>5 && (p[2]-p[3]>=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[3]<=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[1]>=0)) { p[2]=p[1]; }
				else if ((p[3]-p[1])>5 && (p[2]-p[1]<=0)) { p[2]=p[1]; }
				else if ((p[3]-p[2])>0 && (p[2]-p[1]>0))  { }
				else if ((p[1]-p[2])>0 && (p[2]-p[3]>0))  { }
				else
				                                     p[2]=(p[3]+p[1])>>1;
						
					for (count=1;count<4;count++)
					{
						short *q = &pr[((scan+count)<<1)];

						short *q1= &q[IM_DIM];
						w = wvlt[5];

						if (abs(q1[0])   < w)   q1[0] = 0;
						if (abs(q1[1])   < w)   q1[1] = 0;
						if (abs(q1[s])   < w)   q1[s] = 0;
						if (abs(q1[s+1]) < w) q1[s+1] = 0;
						
						w += 6;
						if (abs(q[(2*IM_SIZE)])<w)              q[(2*IM_SIZE)]=0;
						if (abs(q[(2*IM_SIZE)+1])<w)            q[(2*IM_SIZE)+1]=0;
						if (abs(q[(2*IM_SIZE)+(2*IM_DIM)])<w)   q[(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(q[(2*IM_SIZE)+(2*IM_DIM)+1])<w) q[(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
						e=(2*IM_SIZE)+IM_DIM;
						w = wvlt[4];
						if (abs(q[e])<wvlt[4]) q[e]=0;
						if (abs(q[e+1])<wvlt[4]) q[e+1]=0;
						if (abs(q[e+(2*IM_DIM)])<wvlt[4]) q[e+(2*IM_DIM)]=0;
						if (abs(q[e+(2*IM_DIM)+1])<wvlt[4]) q[e+(2*IM_DIM)+1]=0;
					}
						
					if (quality<=LOW9)
					{
						for (count=1;count<4;count++)
						{
							if (abs(p[count+(IM_DIM>>1)])<11) p[count+(IM_DIM>>1)]=0;
							if (abs(p[count+IM_SIZE])<12) p[count+IM_SIZE]=0;
							if (abs(p[count+IM_SIZE+(IM_DIM>>1)])<13) p[count+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
						
			}
			else if (abs(p[4]-p[0])<(wvlt[1]+1) && abs(p[4]-p[3])<(wvlt[1]+1) && abs(p[1]-p[0])<(wvlt[1]+1))
			{
				if (abs(p[3]-p[1])<(wvlt[1]+6) && abs(p[3]-p[2])<(wvlt[1]+6))
				{
					if (((p[3]-p[2])>=0 && (p[2]-p[1])>=0) ||
						((p[3]-p[2])<=0 && (p[2]-p[1])<=0)) 
					{
						//p[2]=(p[3]+p[1]+1)>>1;
						
						for (count=1;count<4;count++)
						{
							short *q = &pr[((scan+count)<<1)];

							if (abs(q[IM_DIM])<wvlt[5]) q[IM_DIM]=0;
							if (abs(q[IM_DIM+1])<wvlt[5]) q[IM_DIM+1]=0;
							if (abs(q[(3*IM_DIM)])<wvlt[5]) q[(3*IM_DIM)]=0;
							if (abs(q[(3*IM_DIM)+1])<wvlt[5]) q[(3*IM_DIM)+1]=0;
						
							if (abs(q[(2*IM_SIZE)])<(wvlt[5]+6)) q[(2*IM_SIZE)]=0;
							if (abs(q[(2*IM_SIZE)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+1]=0;
							if (abs(q[(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)]=0;
							if (abs(q[(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
							e=(2*IM_SIZE)+IM_DIM;
							if (abs(q[e])<wvlt[4]) q[e]=0;
							if (abs(q[e+1])<wvlt[4]) q[e+1]=0;
							if (abs(q[e+(2*IM_DIM)])<wvlt[4]) q[e+(2*IM_DIM)]=0;
							if (abs(q[e+(2*IM_DIM)+1])<wvlt[4]) q[e+(2*IM_DIM)+1]=0;
						}
						
						if (quality<=LOW9)
						{
							for (count=1;count<4;count++)
							{
								if (abs(p[count+(IM_DIM>>1)])<11) p[count+(IM_DIM>>1)]=0;
								if (abs(p[count+IM_SIZE])<12) p[count+IM_SIZE]=0;
								if (abs(p[count+IM_SIZE+(IM_DIM>>1)])<13) p[count+IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
					}
				}
			}
		}
	}
	
	for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[1]-p[(4*IM_DIM)+1])<wvlt[2] && abs(p[(2*IM_DIM)]-p[(2*IM_DIM)+2])<wvlt[2])
			{
				if (abs(p[(2*IM_DIM)+1]-p[(2*IM_DIM)])<(wvlt[3]-1) && abs(p[1]-p[(2*IM_DIM)+1])<wvlt[3])
				{
					e=(p[1]+p[(4*IM_DIM)+1]+p[(2*IM_DIM)]+p[(2*IM_DIM)+2]+2)>>2;
						
					if (abs(e-p[(2*IM_DIM)])<5 || abs(e-p[(2*IM_DIM)+2])<5)
					{
						p[(2*IM_DIM)+1]=e;
					}
					
					count=scan+(2*IM_DIM)+1;

					short *q = &pr[(count<<1)];
						
					if (abs(q[IM_DIM])<wvlt[5]) q[IM_DIM]=0;
					if (abs(q[IM_DIM+1])<wvlt[5]) q[IM_DIM+1]=0;
					if (abs(q[(3*IM_DIM)])<wvlt[5]) q[(3*IM_DIM)]=0;
					if (abs(q[(3*IM_DIM)+1])<wvlt[5]) q[(3*IM_DIM)+1]=0;
						
					if (abs(q[(2*IM_SIZE)])<(wvlt[5]+6)) q[(2*IM_SIZE)]=0;
					if (abs(q[(2*IM_SIZE)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+1]=0;
					if (abs(q[(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)]=0;
					if (abs(q[(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
					e=(2*IM_SIZE)+IM_DIM;
					if (abs(q[e])<32) q[e]=0;
					if (abs(q[e+1])<32) q[e+1]=0;
					if (abs(q[e+(2*IM_DIM)])<32) q[e+(2*IM_DIM)]=0;
					if (abs(q[e+(2*IM_DIM)+1])<32) q[e+(2*IM_DIM)+1]=0;
					
					if (quality<=LOW9)
					{
						for (e=0;e<3;e++)
						{
							if (abs(pr[count+e-1+(IM_DIM>>1)])<11) pr[count+e-1+(IM_DIM>>1)]=0;
							if (abs(pr[count+e-1+IM_SIZE])<12) pr[count+e-1+IM_SIZE]=0;
							if (abs(pr[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) pr[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}
		}
	}
	
	for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[2]-p[1])<wvlt[2] && abs(p[1]-p[0])<wvlt[2])
			{
				if (abs(p[0]-p[(2*IM_DIM)])<wvlt[2] && abs(p[2]-p[(2*IM_DIM)+2])<wvlt[2])
				{
					if (abs(p[(4*IM_DIM)+1]-p[(2*IM_DIM)])<wvlt[2] && abs(p[(2*IM_DIM)]-p[(2*IM_DIM)+1])<wvlt[3]) 
					{
						e=(p[1]+p[(4*IM_DIM)+1]+p[(2*IM_DIM)]+p[(2*IM_DIM)+2]+1)>>2;
						
						if (abs(e-p[(2*IM_DIM)])<5 || abs(e-p[(2*IM_DIM)+2])<5)
						{
							p[(2*IM_DIM)+1]=e;
						}
						
						count=scan+(2*IM_DIM)+1;

						short *q = &pr[(count<<1)];
						
						if (abs(q[IM_DIM])<wvlt[5]) q[IM_DIM]=0;
						if (abs(q[IM_DIM+1])<wvlt[5]) q[IM_DIM+1]=0;
						if (abs(q[(3*IM_DIM)])<wvlt[5]) q[(3*IM_DIM)]=0;
						if (abs(q[(3*IM_DIM)+1])<wvlt[5]) q[(3*IM_DIM)+1]=0;
						
						if (abs(q[(2*IM_SIZE)])<(wvlt[5]+6)) q[(2*IM_SIZE)]=0;
						if (abs(q[(2*IM_SIZE)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+1]=0;
						if (abs(q[(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(q[(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) q[(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
						e=(2*IM_SIZE)+IM_DIM;
						if (abs(q[e])<32) q[e]=0;
						if (abs(q[e+1])<32) q[e+1]=0;
						if (abs(q[e+(2*IM_DIM)])<32) q[e+(2*IM_DIM)]=0;
						if (abs(q[e+(2*IM_DIM)+1])<32) q[e+(2*IM_DIM)+1]=0;
					}
					
					if (quality<=LOW9)
					{
						for (e=0;e<3;e++)
						{
							if (abs(pr[count+e-1+(IM_DIM>>1)])<11) pr[count+e-1+(IM_DIM>>1)]=0;
							if (abs(pr[count+e-1+IM_SIZE])<12) pr[count+e-1+IM_SIZE]=0;
							if (abs(pr[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) pr[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}
		}
	}
}
