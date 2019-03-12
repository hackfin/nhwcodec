	short *p = &pr[scan];
			if (abs(p[2]-p[1])<wvlt[2]
			 && abs(p[1]-p[0])<wvlt[2])
			{
				if (abs(p[0]-p[s])<wvlt[2]
				 && abs(p[2]-p[s+2])<wvlt[2])
				{
					if (abs(p[(4*IM_DIM)+1]-p[s])<wvlt[2]
					 && abs(p[s]-p[s+1])<wvlt[3]) 
					{
						e=(p[1]+p[(4*IM_DIM)+1]+p[s]+p[s+2]+1)>>2;
						
						if (abs(e-p[s])<5 || abs(e-p[s+2])<5)
						{
							p[s+1]=e;
						}
						
						count=scan+s+1;
						short *q = &pr[count << 1];
						
						if (abs(q[IM_DIM])      <wvlt[5]) q[IM_DIM]=0;
						if (abs(q[IM_DIM+1])    <wvlt[5]) q[IM_DIM+1]=0;
						if (abs(q[(3*IM_DIM)])  <wvlt[5]) q[(3*IM_DIM)]=0;
						if (abs(q[(3*IM_DIM)+1])<wvlt[5]) q[(3*IM_DIM)+1]=0;

						q += 2 * IM_SIZE;

						if (abs(q[0])  <(wvlt[5]+6))     q[0]=0;
						if (abs(q[1])  <(wvlt[5]+6))     q[1]=0;
						if (abs(q[s])  <(wvlt[5]+6))     q[s]=0;
						if (abs(q[s+1])<(wvlt[5]+6))     q[s+1]=0;
						
						q += IM_DIM;
						if (abs(q[0])<32)     q[0]=0;
						if (abs(q[1])<32)     q[1]=0;
						if (abs(q[s])<32)     q[s]=0;
						if (abs(q[s+1])<32)   q[s+1]=0;
					}
					
					if (quality<=LOW9)
					{
						short *q = &pr[count-1];
						for (e=0;e<3;e++)
						{
							if (abs(q[e+(IM_DIM>>1)])<11)
								q[e+(IM_DIM>>1)]=0;
							if (abs(q[e+IM_SIZE])<12)
								q[e+IM_SIZE]=0;
							if (abs(q[e+IM_SIZE+(IM_DIM>>1)])<13)
								q[e+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}
