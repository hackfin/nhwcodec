			if (abs(e)>=(ratio-1)) 
			{	
				if (abs(e)<y_wavelet)
				{
					// FIXME: simplify
					if (((abs(p[-1]))+2)>=8) scan++;
					if (((abs(p[1]))+2)>=8) scan++;
					if (((abs(p[-step]))+2)>=8) scan++;
					if (((abs(p[step]))+2)>=8) scan++;

					if (scan<3 && e<y_wavelet && e>-y_wavelet)
					{
						if (e<0) p[0]=-7;else p[0]=7;
					}
					/*else if (!scan && abs(pr[a])<11) 
					{
						if (pr[a]<0) pr[a]=-7;else pr[a]=7;
					}*/

					scan=0;
				}
				if (abs(e)>6)
				{
					e=p[0];

					if (e>=8 && (e&7)<2) 
					{
						if (p[1]>7 && p[1]<10000)
							p[1]--;
						//else if (pr[i+j+step]>7 && pr[i+j+step]<10000) pr[i+j+step]--;
					}


					else if (e==-7 && p[1]==8)
						p[0]=-8;
					else if (e==8 && p[1]==-7)
						p[1]=-8;
					else if (e<-7 && ((-e)&7)<2)
					{
						if (p[1]<-14 && p[1]<10000)
						{
							if (((-p[1])&7)==7) p[1]++;
							else if (((-p[1])&7)<2 && j<(step-2) &&
								p[2]<=0)          p[1]++;
						}
					}
				}
		

			}
			else p[0]=0;


