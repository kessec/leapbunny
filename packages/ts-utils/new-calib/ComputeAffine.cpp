#include "ComputeAffine.h"
// screen_x = (a1 * adc_x + a2 * adc_y + a0) / a6;
// screen_y = (a4 * adc_x + a5 * adc_y + a3) / a6;
// pointercal:  a1 a2 a0 a4 a5 a3 a6
// compute_aff: c1 c2 c3 c4 c5 c6 c7
// screen_x = (c1 * adc_x + c2 * adc_y + c3) / c7;
// screen_y = (c4 * adc_x + c5 * adc_y + c6) / c7;
void compute_affine (int screen_x[3], int screen_y[3], int adc_x[3], int adc_y[3], int pointercal[7])
{
	// Remove offset of 0,0 point; get screen width and height
	int sx1 = screen_x[PT10] - screen_x[PT00];
	int sy1 = screen_y[PT01] - screen_y[PT00];

	// x0=362; y0=175; # target=1,0
	// x1=710; y1=871; # target=0,1
	// x2=361; y2=872; # target=0,0
	int x0=adc_x[PT10], y0=adc_y[PT10];
	int x1=adc_x[PT01], y1=adc_y[PT01];
	int x2=adc_x[PT00], y2=adc_y[PT00];

	int detA=x0*(y1-y2) - x1*(y0-y2) + x2*(y0-y1);
	int c7 = 65536;
	double s = (double)c7 / detA;

	int c1=(int)(sx1*(y1-y2)*s);
	int c2=(int)(sx1*(x2-x1)*s);
	int c3=(int)(sx1*(x1*y2 - x2*y1)*s);
	int c4=(int)(sy1*(y2-y0)*s);
	int c5=(int)(sy1*(x0-x2)*s);
	int c6=(int)(sy1*(x2*y0 - x0*y2)*s);

	// offset again
	c3 += c7 * screen_x[PT00];
	c6 += c7 * screen_y[PT00];

	pointercal[0] = c1;
	pointercal[1] = c2;
	pointercal[2] = c3;
	pointercal[3] = c4;
	pointercal[4] = c5;
	pointercal[5] = c6;
	pointercal[6] = c7;
}

#ifdef DEBUG
#include <stdio.h>
main ()
{
	int screen_x[3], screen_y[3];
	int adc_x[3], adc_y[3];
	int pointercal[7];
	
	screen_x[PT00] = 10;	screen_y[PT00] = 10;
	screen_x[PT10] = 490;	screen_y[PT10] = 10;
	screen_x[PT01] = 10;	screen_y[PT01] = 290;

	adc_x[PT00] = 1100;	adc_y[PT00] = 1100;
	adc_x[PT10] = 1900;	adc_y[PT10] = 1100;
	adc_x[PT01] = 1100;	adc_y[PT01] = 1900;

	compute_affine (screen_x, screen_y, adc_x, adc_y, pointercal);

	int c1 = pointercal[0];
	int c2 = pointercal[1];
	int c3 = pointercal[2];
	int c4 = pointercal[3];
	int c5 = pointercal[4];
	int c6 = pointercal[5];
	int c7 = pointercal[6];

	int x0 = adc_x[PT10];	int y0 = adc_y[PT10];
	int x1 = adc_x[PT01];	int y1 = adc_y[PT01];
	int x2 = adc_x[PT00];	int y2 = adc_y[PT00];

	printf ("%d %d %d %d %d %d %d\n", c1, c2, c3, c4, c5, c6, c7);

	printf ("1,0=%d,%d\n", 
		(c1*x0 + c2*y0 + c3)/c7,
		(c4*x0 + c5*y0 + c6)/c7);
	printf ("0,1=%d,%d\n", 
		(c1*x1 + c2*y1 + c3)/c7,
		(c4*x1 + c5*y1 + c6)/c7);
	printf ("0,0=%d,%d\n", 
		(c1*x2 + c2*y2 + c3)/c7,
		(c4*x2 + c5*y2 + c6)/c7);
	return 0;
}
#endif
