	short *p = &pr[scan];
			if (abs(p[1]-p[(4*IM_DIM)+1])<wvlt[2] && abs(p[s]-p[s+2])<wvlt[2])
			{
				if (abs(p[s+1]-p[s])<(wvlt[3]-1) && abs(p[1]-p[s+1])<wvlt[3])
				{
					e=(p[1]+p[(4*IM_DIM)+1]+p[s]+p[s+2]+2)>>2;
						
					if (abs(e-p[s])<5 || abs(e-p[s+2])<5)
					{
						p[s+1]=e;
					}
					
					count=scan+s+1;
					short *q = &pr[count << 1];
					short *t = &q[IM_DIM];
						
					if (abs(t[0])<wvlt[5])            t[0]=0;
					if (abs(t[1])<wvlt[5])            t[1]=0;
					t = &t[2 * IM_DIM];
					if (abs(t[0])<wvlt[5])            t[0]=0;
					if (abs(t[1])<wvlt[5])            t[1]=0;
						
					t = &q[2 * IM_SIZE];
					if (abs(t[0])<(wvlt[5]+6))   t[0]=0;
					if (abs(t[1])<(wvlt[5]+6))   t[1]=0;
					if (abs(t[s])<(wvlt[5]+6))   t[s]=0;
					if (abs(t[s+1])<(wvlt[5]+6)) t[s+1]=0;
						
					t += IM_DIM;
					if (abs(t[0])<32)   t[0]=0;
					if (abs(t[1])<32)   t[1]=0;
					if (abs(t[s])<32)   t[s]=0;
					if (abs(t[s+1])<32) t[s+1]=0;
					
					if (quality<=LOW9)
					{
						q = &pr[count-1];

						for (e=0;e<3;e++)
						{
							if (abs(q[e+(IM_DIM>>1)])<11)         q[e+(IM_DIM>>1)]=0;
							if (abs(q[e+IM_SIZE])<12)             q[e+IM_SIZE]=0;
							if (abs(q[e+IM_SIZE+(IM_DIM>>1)])<13) q[e+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}

