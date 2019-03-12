			if (abs(p[2]-p[1])<wvlt[6] && abs(p[2]-p[0])<wvlt[6]  && abs(p[1]-p[0])<wvlt[6])
			{
				count=scan+1;

				short *q = &pr[(count<<1)+IM_DIM];
					
				if (abs(q[0])<wvlt[5]) q[0]=0;
				if (abs(q[1])<wvlt[5]) q[1]=0;
				q += 2 * IM_DIM;
				if (abs(q[0])<wvlt[5]) q[0]=0;
				if (abs(q[1])<wvlt[5]) q[1]=0;

				q = &pr[(count<<1) + (2*IM_SIZE)];
					
				if (abs(q[0])<(wvlt[5]+6)) q[0]=0;
				if (abs(q[1])<(wvlt[5]+6)) q[1]=0;
				q += s;
				if (abs(q[0])<(wvlt[5]+6)) q[0]=0;
				if (abs(q[1])<(wvlt[5]+6)) q[1]=0;

				q = &pr[(count<<1)+ IM_DIM + (2*IM_SIZE)];
					
				if (abs(q[0])<34) q[0]=0;
				if (abs(q[1])<34) q[1]=0;
				q += s;
				if (abs(q[0])<34) q[0]=0;
				if (abs(q[1])<34) q[1]=0;
				
				//for (e=0;e<3;e++)
				//{
				p = &pr[count];

						if (abs(p[(IM_DIM>>1)])<11) p[(IM_DIM>>1)]=0;
						if (abs(p[IM_SIZE])<12) p[IM_SIZE]=0;
						if (abs(p[IM_SIZE+(IM_DIM>>1)])<13) p[IM_SIZE+(IM_DIM>>1)]=0;
				//}
			}
