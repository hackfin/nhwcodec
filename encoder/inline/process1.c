				res= pr[scan]-res256[count];res256[count]=0;

				if (!res || res==1)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (pr[stage]==-7 || pr[stage]==-8) 
					{
						if (pr[stage-1]<2 && pr[stage-1]>-8) pr[stage]=-9;
					}
				}
				else if (res==2)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (pr[stage]>15 && !(pr[stage]&7)) pr[stage]--;
					else if (pr[stage]==-7 || pr[stage]==-8) 
					{
						if (pr[stage-1]<=1) pr[stage]=-9;
					}
					else if (pr[stage]==-6) 
					{
						if (pr[stage-1]<=-1 && pr[stage-1]>-8) pr[stage]=-9;
					}
				}
				else if (res==3)
				{
					if (quality>=HIGH1) {res256[count]=144;enc->nhw_res5_word_len++;}
					else
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (pr[stage]>15 && !(pr[stage]&7)) pr[stage]--;
						else if (pr[stage]<=0 && ((((-pr[stage])+2)&65532)==8)) 
						{
							if (pr[stage-1]<=2) pr[stage]=-10;
						}
					}
				}
				else if (res>res_setting) 
				{
					res256[count]=141;enc->nhw_res1_word_len++;

					if (res==4)
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (pr[stage]==7 || (pr[stage]&65534)==8)
						{
							if (pr[stage-1]>=0 && pr[stage-1]<8) pr[stage]+=2;
						}
					}
					else if (res>6) 
					{
						if (res>7 && quality>=HIGH1) 
						{
							res256[count]=148;enc->nhw_res5_word_len++;enc->nhw_res1_word_len++;
						}
						else
						{
							stage = (j<<9)+(i>>9)+(IM_DIM);

							if (pr[stage]>15 && !(pr[stage]&7)) pr[stage]--;
							else if (pr[stage]==-6 || pr[stage]==-7 || pr[stage]==-8) 
							{
								if (pr[stage-1]<0 && pr[stage-1]>-8) pr[stage]=-9;
							}
						}
					}
				}

