/* collect-pdata.cpp, derived from ts-rawtest.cpp
 *
 * Collects and summarizes pressure readings from a touchscreen.
 *
 * Displays a grid of points (9 rows, 5 columns by default).  
 *	  command line arg "-x <dd>" can be used to specify number of rows [2, 24]
 *	  command line arg "-y <dd>" can be used to specify number of cols [2, 13]
 * Collects at least min_data_count pressure readings at each point.  The
 *	  default minimum is 10.  
 *	  command line arg "-n <dd>" can be used to specify min number (must be
 *		> 0)
 * Current version (0.2) collects data at every point in two different stages.
 *	  In first stage, it expects touches with a 20g pin.  The grid points are
 *		displayed in blue.
 *	  In second stage, it expects touches with a 200g pin.  The grid points are
 *		displayed in red.
 *	  After min_data_count pressure readings have been collected at a grid
 *	    point during a stage, the point is re-displayed in black.
 *	  After all points have been re-displayed in black, user can continue to
 *		collect more readings, or can move to the next stage by pressing the
 *		Home button.
 *	  After leaving the last stage, the program computes pressure curves for
 *		each of the grid points.  The current version of the program computes
 *		the pressure curve as a straight line between the average reading at
 *		the point during the first stage and the average reading at the point
 *		during the second stage.
 *	  Then the program outputs (to the serial port) the pressure curves and
 *		information (min, max, range, standard deviation) about the data
 *		collected at each point during each stage. 
 *	  After outputting this info, the program clears the display, continues
 *		to receive touch screen data, and displays touches with dots and lines
 *		whose thickness is proportional to the touch pressure (using the
 *		pressure curves that were generated after the last stage).
 *	  During this drawing phase, pressing the Up quadrant of the Dpad (the part
 *		nearest the display) clears the display.  Pressing the Home button
 *		during this phase causes the program to exit.
 *
 * A future version might allow user to specify number of stages via a command
 *	  line argument.
 * We might also consider implementing additional methods for computing the
 *	  pressure curves and allow selection of the method via a command line
 *	  argument.
 */

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>


#define TSR_NAME	"LF1000 touchscreen raw"
#define KB_NAME		"LF1000 Keyboard"

char ts_dev_name[20];

#include "fb.h"
#include "drawtext.h"
#include "OpenInput.h"
#include "Stats.h"

#define WHITE 0xffffff
#define BLACK 0x000000
#define RED   0xff0000
#define BLUE  0x0000ff
#define GREEN 0x00ff00

#define VERSION "0.2"

#define SYSFS "/sys/devices/platform/lf1000-touchscreen/"


#define MAXGX	24
#define MAXGY	13

int GX = 9;
int GY = 5;

int gx[MAXGX], gy[MAXGY];

int min_data_count = 10;	/* must collect at least this many pressure
							 * readings at a point for each weight
							 * Override with command line argument "-n d",
							 * where 'd' is the new value of min_data_count.
							 */
#define NUM_STAGES	2
//#define NUM_STAGES	3
// TODO: FIXME:
// We might want to define MAX_NUM_STAGES and then make NUM_STAGES a variable
// that can be modified via a command line argument at runtime.

int cur_stage;

CStats stats[MAXGX][MAXGY][NUM_STAGES];

#define NUM_PCURVE_PTS	9
int	pcurves[MAXGX][MAXGY][NUM_PCURVE_PTS];

int tsd[20];		/* raw touch screen data values received from the driver */
int a[7];			/* affine transformation coefficients */
int tnt_plane[3];	/* parameters for position-dependent tnt adjustment */
int max_tnt_down;	/* the maximum unadjusted tnt value that's considered Down*/

struct pollfd polldat[2] = {{0, POLLIN}, {0, POLLIN }};

long get_stage_color();
void g_draw(long color);
void g_init();

int init_display() 
{
	if (init_drawtext())
		return 1;
	cls ();		// Clear screen

	g_draw(get_stage_color());

	return 0;
}

int init_globals() 
{
	FILE *FIN = fopen (SYSFS "pointercal", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "pointercal");
		return 1;
	}
	fscanf (FIN, "%d %d %d %d %d %d %d", 
				 &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]);
	fclose (FIN);

	FIN = fopen (SYSFS "tnt_plane", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "tnt_plane");
		return 1;
	}
	fscanf (FIN, "%d %d %d", &tnt_plane[0], &tnt_plane[1], &tnt_plane[2]);
	fclose (FIN);

	FIN = fopen (SYSFS "max_tnt_down", "r");
	if (!FIN)
	{
		perror("Can't open " SYSFS "max_tnt_down");
		return 1;
	}
	fscanf (FIN, "%d", &max_tnt_down);
	fclose (FIN);

	// Keyboard
	int kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Touchscreen
	int ts = open_input_device(TSR_NAME);
	if (ts < 0)
	{
		perror("Can't find " TSR_NAME);
		return 1;
	}
	polldat[0].fd = kb;
	polldat[1].fd = ts;

	cur_stage  = 0;
	memset (tsd, 0xff, sizeof(tsd));

		/* must call init_fb() before g_init(), because
		 * it inits hsize and vsize 
		 */
	if (init_fb ())	
		return 1;
	g_init();	/* initialize the grid coordinates */

	return 0;
}


void build_linear_pcurves()
{
	int x, y, p;

	for (x = 0; x < GX; ++x) {
		for (y = 0; y < GY; ++y) {
			int light  = (int)stats[x][y][0].Mean();
			int heavy  = (int)stats[x][y][1].Mean();
			int delta;
			int i;

			pcurves[x][y][0] = heavy;
			pcurves[x][y][NUM_PCURVE_PTS-1] = light;
			delta = light - heavy;

			for (i = 1; i < NUM_PCURVE_PTS-1; ++i) {
				pcurves[x][y][i] = heavy + (i * delta)/(NUM_PCURVE_PTS-1);
			}
		}
	}
}

void summarize_pressure_data()
{
	int x, y, p;

	printf("\nGrid coordinates\n");
	printf("x: ");
	for (x=0; x<GX; x++) printf("%3d ", gx[x]);
	printf("\ny: ");
	for (y=0; y<GY; y++) printf("%3d ", gy[y]);
	printf("\n");

	printf("\nPressure Curves\n");
	for (x = GX-1; x >= 0; --x) {
		for (y = 0; y < GY; ++y) {
			printf("[%d,%d]: ", x, y);
			for (p = 0; p < NUM_PCURVE_PTS; ++p) {
				printf("%4d ", pcurves[x][y][p]);
			}
			printf("\n");
		}
	}
	printf("\nSlopes of Pressure Curves\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int light  = (int)stats[x][y][0].Mean();
			int heavy  = (int)stats[x][y][1].Mean();
			int delta;

			delta = light - heavy;
			printf("%2d.%d  ", delta / (NUM_PCURVE_PTS-1), 
							   delta % (NUM_PCURVE_PTS-1));
		}
		printf("\n");
	}
	printf("\nIntercepts of Pressure Curves (Avg Readings for Heaviest Weight)\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int heavy  = (int)stats[x][y][1].Mean();

			printf("%3d  ", heavy);
		}
		printf("\n");
	}
	printf("\nAverage Readings for Lightest Weight\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int light  = (int)stats[x][y][0].Mean();

			printf("%3d  ", light);
		}
		printf("\n");
	}
	printf("\n# of Readings for Heaviest Weight at each point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int num = (int)stats[x][y][1].N();
			printf("%3d ", num);
		}
		printf("\n");
	}
	printf("\n# of Readings for Lightest Weight at each point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int num = (int)stats[x][y][0].N();
			printf("%3d ", num);
		}
		printf("\n");
	}
	printf("\nStdDev of Readings for Heaviest Weight (p-curve intercepts)\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			printf("%4.1f  ", stats[x][y][1].Stdev());
		}
		printf("\n");
	}
	printf("\nStdDev of Readings for Lightest Weight\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			printf("%4.1f  ", stats[x][y][0].Stdev());
		}
		printf("\n");
	}
	printf("\nMax-Min of Readings for Heaviest Weight\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int min = (int)stats[x][y][1].Min();
			int max = (int)stats[x][y][1].Max();
			printf("%3d  ",  max - min);
		}
		printf("\n");
	}
	printf("\nMax-Min of Readings for Lightest Weight\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int min = (int)stats[x][y][0].Min();
			int max = (int)stats[x][y][0].Max();
			printf("%3d  ",  max - min);
		}
		printf("\n");
	}
	printf("\nMax Reading for Heaviest Weight at Each Point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int max = (int)stats[x][y][1].Max();
			printf("%3d  ",  max);
		}
		printf("\n");
	}
	printf("\nMin Reading for Heaviest Weight at Each Point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int min = (int)stats[x][y][1].Min();
			printf("%3d  ",  min);
		}
		printf("\n");
	}
	printf("\nMax Reading for Lightest Weight at Each Point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int max = (int)stats[x][y][0].Max();
			printf("%3d  ", max);
		}
		printf("\n");
	}
	printf("\nMin Reading for Lightest Weight at Each Point\n");
	for (x = GX-1; x >= 0; --x) {
		printf("Row %2d:  ", x);
		for (y = 0; y < GY; ++y) {
			int min = (int)stats[x][y][0].Min();
			printf("%3d  ", min);
		}
		printf("\n");
	}
}

#if 0	// consider light, medium, and heavy; two linear fits
void build_pcurves()
{
	int x, y, p;

	for (x = 0; x < GX; ++x) {
		for (y = 0; y < GY; ++y) {
			int light  = (int)stats[x][y][0].Mean();
			int medium = (int)stats[x][y][1].Mean();
			int heavy  = (int)stats[x][y][2].Mean();
			int delta;
			int i;

			pcurves[x][y][0] = heavy;
			pcurves[x][y][NUM_PCURVE_PTS/2] = medium;
			pcurves[x][y][NUM_PCURVE_PTS-1] = light;
			delta = (medium - heavy > 7) 
						? (medium - heavy)/(NUM_PCURVE_PTS/2)
						: 1;
			for (i = 1; i < NUM_PCURVE_PTS/2; ++i) {
				pcurves[x][y][i] = heavy + i * delta;
			}
			if (medium < pcurves[x][y][i-1]) {
				medium = 1 + pcurves[x][y][i-1];
				pcurves[x][y][NUM_PCURVE_PTS/2] = medium;
			}					
			delta = (light - medium > 7) 
						? (light - medium)/(NUM_PCURVE_PTS/2)
						: 1;
			for (i = 1; i < NUM_PCURVE_PTS/2; ++i) {
				pcurves[x][y][i + NUM_PCURVE_PTS/2] = medium + i * delta;
			}
			if (light < pcurves[x][y][i-1]) {
				light = 1 + pcurves[x][y][i-1];
				pcurves[x][y][NUM_PCURVE_PTS-1] = light;
			}					
		}
	}

	printf("\ngx: ");
	for (x=0; x<GX; x++) printf("%3d ", gx[x]);
	printf("\ngy: ");
	for (y=0; y<GY; y++) printf("%3d ", gy[y]);
	printf("\n");

	printf("\nPressure Curves\n");
	for (x = 0; x < GX; ++x) {
		for (y = 0; y < GY; ++y) {
			printf("[%d,%d]: ", x, y);
			for (p = 0; p < NUM_PCURVE_PTS; ++p) {
				printf("%4d ", pcurves[x][y][p]);
			}
			printf("\n");
		}
	}
}
#endif

/*	The program collects data at a grid of GX*GY points arranged in GX rows of 
 *	GY columns (when the screen is viewed in portrait orientation).
 *	The first column (column 0) is nearest the power button.
 *	The first row (row 0) is nearest the dpad.  
 *  The pressure curve for the point in row 0 and column 0 is in pcurves[0][0].
 *  The pressure curves for the other points in row 0 are in pcurves[0][1], 
 *  pcurves[0][2], ..., pcurves[0][GY-1].
 *
 *  ppcurves[][] is a two-dimensional array of pointers to arrays that contain
 *	the pressure curves for the grid points.
 *
 *	The origin of the x- and y-coordinate system is the lower left corner,
 *	nearest the speaker and the DPad.  During bi-linear interpolation the code
 *	uses calculated screen coordinates to identify the nearest pressure curves
 *	on which the interpolation ought to be based.  Since screen coordinates
 *	are in the x- and y-coordinate system, it seems most natural to arrange
 *	ppcurves in order of increasing x and y.
 */
int * ppcurves[MAXGX][MAXGY] = {
	{pcurves[0][0], pcurves[0][1], pcurves[0][2], pcurves[0][3], 
	 pcurves[0][4], pcurves[0][5], pcurves[0][6], pcurves[0][7], 
	 pcurves[0][8], pcurves[0][9], pcurves[0][10], pcurves[0][11], 
	 pcurves[0][12]},
	{pcurves[1][0], pcurves[1][1], pcurves[1][2], pcurves[1][3], 
	 pcurves[1][4], pcurves[1][5], pcurves[1][6], pcurves[1][7], 
	 pcurves[1][8], pcurves[1][9], pcurves[1][10], pcurves[1][11], 
	 pcurves[1][12]},
	{pcurves[2][0], pcurves[2][1], pcurves[2][2], pcurves[2][3], 
	 pcurves[2][4], pcurves[2][5], pcurves[2][6], pcurves[2][7], 
	 pcurves[2][8], pcurves[2][9], pcurves[2][10], pcurves[2][11], 
	 pcurves[2][12]},
	{pcurves[3][0], pcurves[3][1], pcurves[3][2], pcurves[3][3], 
	 pcurves[3][4], pcurves[3][5], pcurves[3][6], pcurves[3][7], 
	 pcurves[3][8], pcurves[3][9], pcurves[3][10], pcurves[3][11], 
	 pcurves[3][12]},
	{pcurves[4][0], pcurves[4][1], pcurves[4][2], pcurves[4][3], 
	 pcurves[4][4], pcurves[4][5], pcurves[4][6], pcurves[4][7], 
	 pcurves[4][8], pcurves[4][9], pcurves[4][10], pcurves[4][11], 
	 pcurves[4][12]},
	{pcurves[5][0], pcurves[5][1], pcurves[5][2], pcurves[5][3], 
	 pcurves[5][4], pcurves[5][5], pcurves[5][6], pcurves[5][7], 
	 pcurves[5][8], pcurves[5][9], pcurves[5][10], pcurves[5][11], 
	 pcurves[5][12]},
	{pcurves[6][0], pcurves[6][1], pcurves[6][2], pcurves[6][3], 
	 pcurves[6][4], pcurves[6][5], pcurves[6][6], pcurves[6][7], 
	 pcurves[6][8], pcurves[6][9], pcurves[6][10], pcurves[6][11], 
	 pcurves[6][12]},
	{pcurves[7][0], pcurves[7][1], pcurves[7][2], pcurves[7][3], 
	 pcurves[7][4], pcurves[7][5], pcurves[7][6], pcurves[7][7], 
	 pcurves[7][8], pcurves[7][9], pcurves[7][10], pcurves[7][11], 
	 pcurves[7][12]},
	{pcurves[8][0], pcurves[8][1], pcurves[8][2], pcurves[8][3], 
	 pcurves[8][4], pcurves[8][5], pcurves[8][6], pcurves[8][7], 
	 pcurves[8][8], pcurves[8][9], pcurves[8][10], pcurves[8][11], 
	 pcurves[8][12]},
	{pcurves[9][0], pcurves[9][1], pcurves[9][2], pcurves[9][3], 
	 pcurves[9][4], pcurves[9][5], pcurves[9][6], pcurves[9][7], 
	 pcurves[9][8], pcurves[9][9], pcurves[9][10], pcurves[9][11], 
	 pcurves[9][12]},
	{pcurves[10][0], pcurves[10][1], pcurves[10][2], pcurves[10][3], 
	 pcurves[10][4], pcurves[10][5], pcurves[10][6], pcurves[10][7], 
	 pcurves[10][8], pcurves[10][9], pcurves[10][10], pcurves[10][11], 
	 pcurves[10][12]},
	{pcurves[11][0], pcurves[11][1], pcurves[11][2], pcurves[11][3], 
	 pcurves[11][4], pcurves[11][5], pcurves[11][6], pcurves[11][7], 
	 pcurves[11][8], pcurves[11][9], pcurves[11][10], pcurves[11][11], 
	 pcurves[11][12]},
	{pcurves[12][0], pcurves[12][1], pcurves[12][2], pcurves[12][3], 
	 pcurves[12][4], pcurves[12][5], pcurves[12][6], pcurves[12][7], 
	 pcurves[12][8], pcurves[12][9], pcurves[12][10], pcurves[12][11], 
	 pcurves[12][12]},
	{pcurves[13][0], pcurves[13][1], pcurves[13][2], pcurves[13][3], 
	 pcurves[13][4], pcurves[13][5], pcurves[13][6], pcurves[13][7], 
	 pcurves[13][8], pcurves[13][9], pcurves[13][10], pcurves[13][11], 
	 pcurves[13][12]},
	{pcurves[14][0], pcurves[14][1], pcurves[14][2], pcurves[14][3], 
	 pcurves[14][4], pcurves[14][5], pcurves[14][6], pcurves[14][7], 
	 pcurves[14][8], pcurves[14][9], pcurves[14][10], pcurves[14][11], 
	 pcurves[14][12]},
	{pcurves[15][0], pcurves[15][1], pcurves[15][2], pcurves[15][3], 
	 pcurves[15][4], pcurves[15][5], pcurves[15][6], pcurves[15][7], 
	 pcurves[15][8], pcurves[15][9], pcurves[15][10], pcurves[15][11], 
	 pcurves[15][12]},
	{pcurves[16][0], pcurves[16][1], pcurves[16][2], pcurves[16][3], 
	 pcurves[16][4], pcurves[16][5], pcurves[16][6], pcurves[16][7], 
	 pcurves[16][8], pcurves[16][9], pcurves[16][10], pcurves[16][11], 
	 pcurves[16][12]},
	{pcurves[17][0], pcurves[17][1], pcurves[17][2], pcurves[17][3], 
	 pcurves[17][4], pcurves[17][5], pcurves[17][6], pcurves[17][7], 
	 pcurves[17][8], pcurves[17][9], pcurves[17][10], pcurves[17][11], 
	 pcurves[17][12]},
	{pcurves[18][0], pcurves[18][1], pcurves[18][2], pcurves[18][3], 
	 pcurves[18][4], pcurves[18][5], pcurves[18][6], pcurves[18][7], 
	 pcurves[18][8], pcurves[18][9], pcurves[18][10], pcurves[18][11], 
	 pcurves[18][12]},
	{pcurves[19][0], pcurves[19][1], pcurves[19][2], pcurves[19][3], 
	 pcurves[19][4], pcurves[19][5], pcurves[19][6], pcurves[19][7], 
	 pcurves[19][8], pcurves[19][9], pcurves[19][10], pcurves[19][11], 
	 pcurves[19][12]},
	{pcurves[20][0], pcurves[20][1], pcurves[20][2], pcurves[20][3], 
	 pcurves[20][4], pcurves[20][5], pcurves[20][6], pcurves[20][7], 
	 pcurves[20][8], pcurves[20][9], pcurves[20][10], pcurves[20][11], 
	 pcurves[20][12]},
	{pcurves[21][0], pcurves[21][1], pcurves[21][2], pcurves[21][3], 
	 pcurves[21][4], pcurves[21][5], pcurves[21][6], pcurves[21][7], 
	 pcurves[21][8], pcurves[21][9], pcurves[21][10], pcurves[21][11], 
	 pcurves[21][12]},
	{pcurves[22][0], pcurves[22][1], pcurves[22][2], pcurves[22][3], 
	 pcurves[22][4], pcurves[22][5], pcurves[22][6], pcurves[22][7], 
	 pcurves[22][8], pcurves[22][9], pcurves[22][10], pcurves[22][11], 
	 pcurves[22][12]},
	{pcurves[23][0], pcurves[23][1], pcurves[23][2], pcurves[23][3], 
	 pcurves[23][4], pcurves[23][5], pcurves[23][6], pcurves[23][7], 
	 pcurves[23][8], pcurves[23][9], pcurves[23][10], pcurves[23][11], 
	 pcurves[23][12]}
};

/* This routine returns a value in the interval [1,10]. 
 * The number can be regarded as an indication of the pressure with which the
 * touchscreen is being pressed.  A value of 1 indicates a light touch.  A value
 * of 10 indicates a very heavy touch.
 *
 * The routine determines the returned value by comparing the 'adc_pressure'
 * argument to the values in a pressure curve.  
 * If adc_pressure is less than the first entry in the pressure curve, the 
 * routine returns 10.  If adc_pressure is larger than the first entry and 
 * smaller than the second entry, the routine returns 9.  And so on.  If 
 * adc_pressure is larger than all entries in the pressure curve, the routine
 * returns 1.
 *
 * The routine constructs the pressure curve using bilinear interpolation of
 * the pressure curves for the four corners of the grid's sub-rectangle in 
 * which the point ('x', 'y') lies.  The pressure curves for all the grid's 
 * points ought to be constructed before this routine is called.
 *
 * The bilinear interpolation code is functionally equivalent to the code that's
 * used in the touchscreen driver (lf1000_ts2.ko) when pressure adjustment is
 * enabled (/sys/devices/platform/lf1000-touchscreen/adjust_pressure contains 
 * 1).
 *
 * Here's a summary of the processing:
 *
 *  if x <= x[0], x >= x[GX-1], y <= y[0], or y >= y[GY-1],
 *		if in one of the four corners, use the corresponding pcurve value. 
 *		else if x <= x[0] or x >= x[GX-1]
 *			use linear interpolation (in y) of the two nearest pcurves
 *		else if y <= y[0] or y >= y[GY-1]
 *			use linear interpolation (in x) of the two nearest pcurves
 *			
 *
 *
 *	else {
 *		Let xl and xh be the indices such that x[xl] <= cur-x <= x[xh]
 *		and yl and yh be the indices such that y[yl] <= cur-y <= y[yh].
 *
 *			P(xl, yl)		P(xh, yl)		Q11 = (xl, yl)  Q21 = (xh, yl)
 *
 *					P(cur-x, cur-y)
 *
 *			P(xl, yh)		P(xh, yh)		Q12 = (xl, yh)  Q22 = (xh, yh)
 *		
 *			P(x,y) ~ (P(Q11)*(x2-x)(y2-y) + P(Q21)*(x-x1)(y2-y)
 *										  + P(Q12)*(x2-x)(y-y1)
 *										  + P(Q22)*(x-x1)*y-y1))
 *					  /((x2-x1)(y2-y1))
 *
 *			NOTE: (x2-x1)(y2-y1) is a constant.
 *	}
 */
int get_pressure(int adc_pressure, int x, int y, int printit) 
{
	int   interp_pcurve[NUM_PCURVE_PTS];
	int   p;
	int	  x_offset;		/* x-offset from start of interval */
	int	  x_interval;	/* length of x-interval */
	int	  y_offset;		/* y-offset from start of interval */
	int	  y_interval;	/* length of y-interval */
	int * pcurve1;
	int * pcurve2;
	int   ix, iy;
	int	  i;
	int * pcurve;

		/* if the touch is near the Dpad */
	if (x <= gx[0]) {
		if (y <= gy[0]) {
			pcurve = ppcurves[0][0];
		}
		else if (y >= gy[GY-1]) {
			pcurve = ppcurves[0][GY-1];
		}
		else {
			/* construct a p-curve by linear interpolation
			 * from ppcurves[0][yl] and ppcurves[0][yh].
			 * First identify the interval in which y lies.
			 * We already know it's not in the first or the last.
			 */
			for (iy = 1; iy < GY-2; ++iy) {
				if (y < gy[iy+1])
					break;
			}
			if (iy > GY-2)
				iy = GY-2;
			pcurve1    = ppcurves[0][iy];
			pcurve2    = ppcurves[0][iy + 1];
			y_offset   = y - gy[iy];
			y_interval = gy[iy+1] - gy[iy];
			for (p = 0; p < NUM_PCURVE_PTS; ++p) {
				interp_pcurve[p] = pcurve1[p] 
									+  y_offset * (pcurve2[p]-pcurve1[p])
										/ y_interval;
			}
			pcurve = interp_pcurve;
		}
	}
		/* if the touch is near "LeapPad" */
	else if (x >= gx[GX-1]) {
		if (y <= gy[0]) {
			pcurve = ppcurves[GX-1][0];
		}
		else if (y >= gy[GY-1]) {
			pcurve = ppcurves[GX-1][GY-1];
		}
		else {
			/* construct a p-curve by linear interpolation
			 * from ppcurves[GX-1][yl] and ppcurves[GX-1][yh].
			 * First identify the interval in which y lies.
			 * We already know it's not in the first or the last.
			 */
			for (iy = 1; iy < GY-2; ++iy) {
				if (y < gy[iy+1])
					break;
			}
			if (iy > GY-2)
				iy = GY-2;
			pcurve1    = ppcurves[GX-1][iy];
			pcurve2    = ppcurves[GX-1][iy + 1];
			y_offset   = y - gy[iy];
			y_interval = gy[iy+1] - gy[iy];
			for (p = 0; p < NUM_PCURVE_PTS; ++p) {
				interp_pcurve[p] = pcurve1[p] 
									+  y_offset * (pcurve2[p]-pcurve1[p])
										/ y_interval;
			}
			pcurve = interp_pcurve;
		}
	}
		/* if the touch is near the Power side 
		 * and isn't in one of the corners
		 */
	else if (y <= gy[0]) {
		/* construct a p-curve by linear interpolation
		 * from ppcurves[xl][0] and ppcurves[xh][0].
		 * First identify the interval in which x lies.
		 * We already know it's not in the first or the last.
		 */
		for (ix = 1; ix < GX-2; ++ix) {
			if (x < gx[ix+1])
				break;
		}
		if (ix > GX-2)
			ix = GX-2;
		pcurve1    = ppcurves[ix][0];
		pcurve2    = ppcurves[ix+1][0];
		x_offset   = x - gx[ix];
		x_interval = gx[ix+1] - gx[ix];
		for (p = 0; p < NUM_PCURVE_PTS; ++p) {
			interp_pcurve[p] = pcurve1[p] 
								+  x_offset * (pcurve2[p]-pcurve1[p])
									/ x_interval;
		}
		pcurve = interp_pcurve;
	}
		/* if the touch is near the Volume side
		 * and isn't in one of the corners
		 */
	else if (y >= gy[GY-1]) {
		/* construct a p-curve by linear interpolation
		 * from ppcurves[xl][GX-1] and ppcurves[xh][GX-1].
		 * First identify the interval in which x lies.
		 * We already know it's not in the first or the last.
		 */
		for (ix = 1; ix < GX-2; ++ix) {
			if (x < gx[ix+1])
				break;
		}
		if (ix > GX-2)
			ix = GX-2;
		pcurve1    = ppcurves[ix][GY-1];
		pcurve2    = ppcurves[ix+1][GY-1];
		x_offset   = x - gx[ix];
		x_interval = gx[ix+1] - gx[ix];
		for (p = 0; p < NUM_PCURVE_PTS; ++p) {
			interp_pcurve[p] = pcurve1[p] 
								+  x_offset * (pcurve2[p]-pcurve1[p])
									/ x_interval;
		}
		pcurve = interp_pcurve;
	}
	else {	/* point is not in a corner or near an edge
			 * use bi-linear interpolation to construct the pressure
			 * curve from the curves at the four nearest grid points.
			 */
			/*
			f(x,y) ~ (f(Q11)*(x2-x)(y2-y) + f(Q21)*(x-x1)(y2-y)
										  + f(Q12)*(x2-x)(y-y1)
										  + f(Q22)*(x-x1)*y-y1))
					  /((x2-x1)(y2-y1))
			 */
		int * pcurve11;
		int * pcurve12;
		int * pcurve21;
		int * pcurve22;
		int   xy_interval;
		int	  x_offset2;
		int	  y_offset2;

		for (ix = 1; ix < GX-2; ++ix) {
			if (x < gx[ix+1])
				break;
			if (ix > GX-2)
				ix = GX-2;
		}
		for (iy = 1; iy < GY-2; ++iy) {
			if (y < gy[iy+1])
				break;
		}
		if (iy > GY-2)
			iy = GY-2;
		pcurve11   = ppcurves[ix][iy];
		pcurve12   = ppcurves[ix][iy+1];
		pcurve21   = ppcurves[ix+1][iy];
		pcurve22   = ppcurves[ix+1][iy+1];

		x_offset   = x - gx[ix];
		x_offset2  = gx[ix+1] - x;
		x_interval = gx[ix+1] - gx[ix];

		y_offset   = y - gy[iy];
		y_offset2  = gy[iy+1] - y;
		y_interval = gy[iy+1] - gy[iy];
		xy_interval = x_interval * y_interval;
				
		for (p = 0; p < NUM_PCURVE_PTS; ++p) {
			interp_pcurve[p] = (pcurve11[p] * x_offset2 * y_offset2
								+ pcurve21[p] * x_offset * y_offset2
								+ pcurve12[p] * x_offset2 * y_offset
								+ pcurve22[p] * x_offset * y_offset)
									/ xy_interval;
		}
		pcurve = interp_pcurve;
	}
	for (i=0; i<NUM_PCURVE_PTS; i++)
	{
		if (adc_pressure < pcurve[i])
		{
			break;
		}
	}
	if (printit) {
		int j;
		printf("Interpolated pcurve: ");
		for (j=0; j<NUM_PCURVE_PTS; j++)
			printf("%3d ", pcurve[j]);
		printf("\n  adc_pressure %d, i %d, p %d\n",
				adc_pressure, i, 10 - i);
	
	}
	return 10 - i;
}

/* This routine receives data from the touchscreen and draws points and lines
 * whose thickness is proportional to the pressure of the touch, as returned
 * by get_pressure().
 */
void test_calibration()
{
	int x, y, t, p, fancy_tnt, color;
	int tlx=-1, tly=-1, tlf = 0;
	int ix, iy;
	int count = 0;

	printf("\ntest_calibration\n");
	cls ();
	while (1) {
		struct input_event ev[64];
		int wx, wy;

		// poll input event system
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[1].revents) // Touchscreen
			{
				int rd = read( polldat[1].fd, ev, 
								sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					switch (ev[0].type)
					{
					case EV_ABS:
						tsd[ev[0].code] = ev[0].value;
						break;
					case EV_SYN:
						// Do pointercal transfrom from raw (wx, wy) to screen (x,y)
						wx = (tsd[0]+tsd[1])/2;
						wy = (tsd[2]+tsd[3])/2;
						x = (a[0]*wx + a[1]*wy + a[2])/a[6];
						y = (a[3]*wx + a[4]*wy + a[5])/a[6];

						// Compute TNT and up/down status
						t = (tsd[6]+tsd[7])/2;
						fancy_tnt = max_tnt_down + 
									(tnt_plane[0]*wx + tnt_plane[1]*wy 
									 + tnt_plane[2] + 32768)/65536;
						if (t <= fancy_tnt) {
						
							p = tsd[8];
							/* This code is similar to code in mb.cpp */
							int fatness = get_pressure(p, x, y, 
														0 == (count % 100));
							if (!fatness)
								fatness = tlf;
							/* Enable this code to get output you can use
							 * to check the bilinear interpolation in
							 * get_pressure()
							 * /
							if (0 == (count++ % 100)) 
								printf("x %d, y %d, fatness %d, p %d\n",
											x, y, fatness, p);
							/* */
							if (tlx != -1 && tly != -1)
								fullroundline (tlx, tly, x, y,
											 tlf, fatness, BLACK);
							else
								fullrounddot (x, y, fatness, BLACK);

							tlx = x;
							tly = y;
							tlf = fatness;
						}
						else {
							/* Enable this code to help you keep track
							 * of the output.
							 * /
							if (count) {
								printf("UP\n");
								count = 0;
							}
							/* */
							tlx = tly = -1;
							tlf = 0;
						}
					}
				}
			}
			if (polldat[0].revents) // Keyboard
			{
				int rd = read( polldat[0].fd, ev, 
								sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && 
					    ev[0].value==1)
					{
						if (ev[0].code == KEY_ESC)
						{
							break;
						}
						else cls();
					}
				}
			}
		}
	}
}

/* This routine returns a color that's associated with the current value of
 * cur_stage. 
 * TODO: FIXME: to deal with a larger number of pin weights, we could define
 *				an array of colors and return the cur_stage-th entry of the
 * array.
 */
long get_stage_color() {
	long color;

	if      (cur_stage == 0) color = BLUE;
#if 0	// 26may11
	else if (cur_stage == 1) color = GREEN;
#endif	// 26may11
	else					 color = RED;

	return color;
}

/* This routine initializes the gx[] and gy[] arrays, which contain the x- and
 * y-coordinates of the grid points.
 */
void g_init ()
{
	int i;

	for (i=0; i<GX; i++)
		gx[i] = 10 + (int)(1.0*i*(hsize-20)/(GX-1));
	for (i=0; i<GY; i++)
		gy[i] = 10 + (int)(1.0*i*(vsize-20)/(GY-1));
}


	/* draw all the grid points in the specified color */
void g_draw(long color)
{
	int x, y;

	for (x = 0; x < GX; ++x) {
		for (y = 0; y < GY; ++y) {
			fullfatdot (gx[x], gy[y], 5, color);
		}
	}
}


/* This routine checks if at least min_data_count pressure readings have been
 * taken at every grid point.
 * Returns 0 if more data is needed for at least one point.
 */
int data_for_all_points() 
{
	int x, y;

	for (x = 0; x < GX; ++x) {
		for (y = 0; y < GY; ++y) {
			if (stats[x][y][cur_stage].N() < min_data_count) {
				printf("Need data for x %d, y %d, stage %d\n", x, y, cur_stage);
				return 0;
			}
		}
	}
	printf("Data for all points in stage %d\n", cur_stage);
	return 1;
}


int do_avg = 0;

void help (char **argv)
{
	printf ("Usage:\n   %s [-n <dd>] [-h]\n", argv[0]);
	printf ("where\n");
	printf ("    -n <dd>: min number of pressure readings "
			  "for each point and weight\n");
	printf ("    -x <dd>: number of grid rows (2 <= dd <= 24)\n");
	printf ("    -y <dd>: number of grid columns (2 <= dd <= 13)\n");
	printf ("    -h: help\n");
}


int parse_args( int argc, char **argv)
{
	int i;
	int errors = 0;

	for (i=1; i<argc; i++)
	{
		if (!strcmp (argv[i], "-h"))
			++errors;
		else if (!strcmp (argv[i], "-n")) {
			int num;
			if (   (++i < argc) 
				&& (1 == sscanf( argv[i], "%d", &num))
				&& (num > 0)) {

				min_data_count = num;
			}
			else {
				printf("  a positive integer ought to follow '-n'\n");
				++errors;
			}
		}
		else if (!strcmp (argv[i], "-x")) {
			int num;
			if (   (++i < argc) 
				&& (1 == sscanf( argv[i], "%d", &num))
				&& (num >= 2)
				&& (num <= MAXGX)) {

				GX = num;
			}
			else {
				printf("  a positive integer in [2, %d] ought to follow '-x'\n",
						MAXGX);
				++errors;
			}
		}
		else if (!strcmp (argv[i], "-y")) {
			int num;
			if (   (++i < argc) 
				&& (1 == sscanf( argv[i], "%d", &num))
				&& (num >= 2)
				&& (num <= MAXGY)) {

				GY = num;
			}
			else {
				printf("  a positive integer in [2, %d] ought to follow '-y'\n",
						MAXGY);
				++errors;
			}
		}
		else {
			printf("  Unrecognized argument: '%s'\n", argv[i]);
			++errors;
		}
	}
	return errors;
}

int main (int argc, char **argv)
{
	int status;

	printf ("%s Version %s\n", argv[0], VERSION);

	if (parse_args(argc, argv)) {
		help (argv);
		exit (0);
	}
	if (0 != (status = init_globals())) {
		return status;
	}
	if (0 != (status = init_display())) {
		return status;
	}

	system ("echo 5 > " SYSFS "report_events");
	system ("echo 50 > " SYSFS "sample_rate_in_hz");

	// The main event loop
	while (1) {
		int ix, iy;
		int wx, wy;
		int x, y, t, p, fancy_tnt;
		struct input_event ev[64];

		// poll input event system
		if (poll (polldat, 2, 0) > 0)
		{
			if (polldat[1].revents) // Touchscreen
			{
				int rd = read( polldat[1].fd, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					switch (ev[0].type)
					{
					case EV_ABS:
						tsd[ev[0].code] = ev[0].value;
						break;
					case EV_SYN:
						// Do pointercal transfrom from raw (wx, wy) to screen (x,y)
						wx = (tsd[0]+tsd[1])/2;
						wy = (tsd[2]+tsd[3])/2;
						x = (a[0]*wx + a[1]*wy + a[2])/a[6];
						y = (a[3]*wx + a[4]*wy + a[5])/a[6];

						// Compute TNT and check up/down status
						t = (tsd[6]+tsd[7])/2;
						fancy_tnt = max_tnt_down + 
									(tnt_plane[0]*wx + tnt_plane[1]*wy 
									 + tnt_plane[2] + 32768)/65536;
						if (t <= fancy_tnt) 
						{
							/* tnt indicates Down.
							 * Check if the touch point is inside one of the
							 * grid markers.
							 * If it is, add the pressure to the marker's
							 * statistics.  If the required # of readings
							 * has now been collected, display a black marker.
							 */
							p = tsd[8];	
							for (ix = 0; ix < GX; ++ix) {
								if (   (gx[ix] - 4 <= x) 
									&& (gx[ix] + 4 >= x)) {
									for (iy = 0; iy < GY; ++iy) {
										if (   (gy[iy] - 4 <= y) 
											&& (gy[iy] + 4 >= y)) {
											stats[ix][iy][cur_stage].Add((double)p);
											if (stats[ix][iy][cur_stage].N()
												== min_data_count) 
											{
												fullfatdot (gx[ix], gy[iy], 5,
															 BLACK);
											}
										}
											/* break out if we know the point
											 * doesn't lie in any marker
											 */
										else if (y < gy[iy]) {
											break;
										}
									}	
								}
									/* break out if we know the point
									 * doesn't lie in any marker
									 */
								else if (x < gx[ix]) {
									break;
								}
							}	
						}	
					}
				}
			}
			if (polldat[0].revents) // Keyboard
			{
				int rd = read( polldat[0].fd, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && 
					    ev[0].value==1)
					{
						/* When the Home key is pressed, check if all needed
						 * data has been collected for the current stage.
						 * If it has, move to the next stage (if there is one)
						 * or leave the pressure data collection loop.
						 */
						if (ev[0].code == KEY_ESC)
						{
							if (data_for_all_points()) {
								if (++cur_stage == NUM_STAGES)
									break;
								else 	
									g_draw(get_stage_color());
							}
						}
					}
				}
			}
		}
	}
	// NOTE: This is the place to check if we want to construct pressure curves
	//		 using a technique besides drawing a line from 200g to 20g readings.
	build_linear_pcurves();

	summarize_pressure_data();

	test_calibration();

	system ("echo 1 > " SYSFS "report_events");
	system ("echo 50 > " SYSFS "sample_rate_in_hz");
	cls ();

	return 0;
}

