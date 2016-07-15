/* Note, the pl2303 serial port seems to bounce for around 5ms */
/* when the device is opened, so this code does some debouncing */
/* to aid in automatically finding the starts of the traces */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#define STATE_NONE	0
#define STATE_DTR_START	1
#define STATE_DTR_STOP	2
#define STATE_IN_TRACE	3

int main(int argc, char **argv) {

	int i,result,done=0;
	int fd,temp_int,rate,channels;
	float points[4];
	int ticks=0,trace_ticks=0,sub_ticks=0;
	double watts;
	long long temp64;
	int trace=0;
	int state=STATE_NONE;
	double trace_joules=0.0;
	int time_in_state=0;
	int threshold=1;
	double total_joules=0.0;
	double total_time=0.0;
	double gain=300.0;
	double resistor=0.00333;

	int rate_divider=1;

	if (argc>1) {

		rate_divider=atoi(argv[1]);
	}
	else {
		rate_divider=1;
	}

	/* stdin */
	fd=0;


	read(fd,&temp_int,4);
	printf("(* Version %d *)\n",temp_int);

	read(fd,&temp64,8);
	printf("(* Start Time %lld.",temp64);
	read(fd,&temp64,8);
	printf("%06lld)\n",temp64);

	read(fd,&temp_int,4);
	rate=temp_int;
	printf("(* Rate %d Hz *)\n",rate);

	printf("(* Gain %lf *)\n",gain);
	printf("(* Resistor %lf *)\n",resistor);

	printf("(* Sample divider %d *)\n",rate_divider);

	threshold=rate/500;
	if (threshold<1) threshold=1;

	read(fd,&temp_int,4);
	channels=temp_int;
	printf("(* Channels %d *)\n",channels);

	while(!done) {

		for(i=0;i<channels;i++) {

			result=read(fd,&points[i],sizeof(float));

			if (isinf(points[i])) {
				done=1;
				break;
			}

			if (result<sizeof(float)) {
				done=1;
				break;
			}

//			printf("%lf\t",points[i]);

		}

		if (done) break;

		if (state==STATE_NONE) {
			if (points[3]>4.0) {
				time_in_state++;
				if (time_in_state>threshold) {
					state=STATE_DTR_START;
					time_in_state=0;
				}
			}
		} else if (state==STATE_DTR_START) {

			if (points[3]<-4.0) {

//				time_in_state++;
//				if (time_in_state>threshold) {

				state=STATE_IN_TRACE;
				printf("Starting Trace %d at %lf\n",
					trace,
					(double)ticks/(double)rate);
				trace_ticks=0;
				sub_ticks=0;
				trace_joules=0.0;
				time_in_state=0;
//				}
			}
		} else if (state==STATE_IN_TRACE) {
			if (points[3]>4.0) {
				time_in_state++;
				if (time_in_state>threshold) {
					state=STATE_DTR_STOP;
					printf("Ending Trace %d at %lfs, %lf %lfs Elapsed\n",
						trace,
						(double)ticks/(double)rate,
						(double)trace_ticks,
						(double)trace_ticks/(double)rate*(double)rate_divider);
					printf("Total Energy: %lfJ Average Power: %lfW\n",
						trace_joules,
						trace_joules/((double)trace_ticks/(double)rate*(double)rate_divider));
					total_joules+=trace_joules;
					total_time+=(double)trace_ticks/(double)rate*(double)rate_divider;
					trace++;
					time_in_state=0;
				}
			}
		}
		else if (state==STATE_DTR_STOP) {
			if (points[3]<-4.0) {
//				time_in_state++;
//				if (time_in_state>threshold) {
					state=STATE_NONE;
					time_in_state=0;
//				}

			}
		}



		if (state==STATE_IN_TRACE) {

			/* P=IV */
			/* V=points[1] */
			/* I=V/R = (V/AMP)/R = V/300/.0033 */

			watts=((points[0]/gain)/resistor)*points[1];

#if 0
			printf("%lf\t%lf\t(* %lf %lf *)\n",
				(double)ticks/(double)rate*(double)rate_divider,
				watts,
				points[0],points[1]);
#endif
//			printf("%d %d\n",sub_ticks,rate_divider);
			if (sub_ticks==0) {
				trace_joules+=(watts/(double)rate*(double)rate_divider);
				trace_ticks++;
			}
			sub_ticks++;
			if (sub_ticks==rate_divider) {
				sub_ticks=0;
			}

		}

		ticks++;
	}

	read(fd,&temp64,8);
	printf("(* Stop Time %lld.",temp64);
	read(fd,&temp64,8);
	printf("%06lld)\n",temp64);

	printf("Average Joules=%lf\tAverage Watts=%lf\n",total_joules/(double)trace,total_joules/total_time);

	return 0;
}
