// New Calibration

#define _FILE_OFFSET_BITS	64

#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>

#include <tslib.h>
#include <libMfgData.h>

#define TS_NAME	 "LF1000 touchscreen interface"
#define TSR_NAME "LF1000 touchscreen raw"
#define KB_NAME	 "LF1000 Keyboard"

#define TSP_VERSION     4
#define TSP_SUBVERSION  0

#include "fb.h"
#include "OpenInput.h"
#include "MaxMin.h"
#include "Histo.h"
#include "Stats.h"
#include "drawtext.h"
#include "ComputeAffine.h"

#define ABS(X)	 ((X)<0?-(X):(X))
#define MAX(A,B) ((A)>(B)?(A):(B))
#define MIN(A,B) ((A)<(B)?(A):(B))

enum board_id { BID_EXPLORER, BID_MADRID };

// TODO: NOTE: values are assigned to this struct's 'bid', 'max_noise_x',
//				and 'max_noise_y' members, but those members are not used
// anyplace else.  Only the struct's 'tnt_mode_to_use' member is used.
struct board_model
{
	enum board_id bid;
	double	max_noise_x;
	double	max_noise_y;
	int		tnt_mode_to_use;
} board;

#define RED		0xff0000
#define GRN		0x00ff00
#define BLU		0x0000ff
#define BLK		0x000000
#define YEL		0xDFDF00
#define NO  	0x3c3c3c

#define	C20		0xff9f30
#define C25		0xdf0000
#define C30		0x00df00
#define C40		0x0000df
#define C50		0xffdf00
#define C75	    0xdfdfdf
#define C100	0x60bfff
#define C150	0xdf00df
#define C200	0x010101

#define	N_PCURVE_PTS	9   // With nine control points, we get 10 bins
#define NOMINAL_WEIGHT 40	// Weight of pin we set TNT threshold on
#define CNOMINAL	C40
int MINIMAL_WEIGHT=40;	// Weight of pin we set TNT threshold on
int CMINIMAL=C40;

#define RAD			75

#define MIN_TNT_PADDING	5.0
#define MAX_DIFF_AVG_TO_CENTER_TNT	3
int avg_of_corner_tnts;	

int setting_delay_in_us		= -1;
int setting_y_delay_in_us	= -1;
int setting_tnt_delay_in_us = -1;
int setting_min_tnt_up		= -1;
int setting_max_tnt_down	= -1;
int v2_max_tnt_down			= -1;
int v3_7_max_tnt_down		= -1;
int v3_8_max_delta_tnt		= -1;
int setting_max_delta_tnt	= -1;
int setting_pointercal[7]   = { -1, -1, -1, -1, -1, -1, -1 };
int setting_pressure[N_PCURVE_PTS] = { 
	468, 471, 487, 495, 509, 524, 553, 575, 612 };
int setting_tnt_mode		= -1;
int setting_averaging		= -1;
int setting_tnt_plane[3];
int default_pressure[N_PCURVE_PTS] = { 
	468, 471, 487, 495, 509, 524, 553, 575, 612 };

// To help UI a bit, remember first point so we can detect dropped pins
int calib_delay_x_point;
int calib_delay_y_point;

char ts_dev_name[20];
char tsr_dev_name[20];

struct tsdev *tsl;
int kb, kb_i, tsr, tsr_i;
struct pollfd polldat[3];
FILE *flog = NULL;

CMfgData mfgData;

// Define this and test_drive really slows down, but you can view testdrive data for anomalies, like 0,0 points
// #define LOG_TESTDRIVE

int loose = 0; // For bringing up new hardware, assume less

#define STANDARD_PRESSURE_UPPER_LIMIT 900
#define GOWORLD_PRESSURE_UPPER_LIMIT 1500
static int pressure_upper_limit = STANDARD_PRESSURE_UPPER_LIMIT;

#if 1	// 17may11
#define STANDARD_PEN_DOWN_THRESHOLD 50
#define GOWORLD_PEN_DOWN_THRESHOLD  50
#define MAX_TNT_DIFF_WHEN_DOWN		 3
#else
#define STANDARD_PEN_DOWN_THRESHOLD 30
#define GOWORLD_PEN_DOWN_THRESHOLD  40
#endif	// 17may11

#if 1	// 9aug11
#define GOWORLD_MAX_DELTA_TNT	 25
#define GOWORLD_MAX_TNT_DOWN	200
#define GOWORLD_MIN_TNT_UP		400
#endif	// 9aug11

static int pen_down_threshold = STANDARD_PEN_DOWN_THRESHOLD;

const int MAX_PI = 9; // N_PCURVE_PTS;
const int PI_MINIMAL = 2;
const int PI_NOMINAL = 3;
const int weight_lut[MAX_PI] =  { 20, 25, 30, 40, 50, 75, 100, 150, 200 };
const int enough_good[MAX_PI] = { 11, 12, 13, 20, 25, 30,  50,  50,  50 };
const long color_lut[MAX_PI] =  { C20,C25,C30,C40,C50,C75,C100,C150,C200 };

long color_for_weight (int w)
{
	int i;
	for (i=0; i<MAX_PI; i++)
	{
		if (w==weight_lut[i])
			return color_lut[i];
	}
	return BLK;
}

int get_board_model (struct board_model *board)
{
	char buf[100];
	sprintf (buf, "/sys/devices/platform/lf1000-gpio/board_id");
	FILE *f;
	f = fopen (buf, "r");
	if (!f) { 
		perror ("board_id read"); 
		fclose(f);
		return 1; 
	}
	int x;
	if (fscanf (f, "%x", &x) < 1) { 
		printf ("scanf failed\n"); 
		fclose(f);
		return 2; 
	}
	fclose (f);
	switch (x)
	{
	case 0x2:
	case 0xa:
		printf ("Board ID: 0x%X=Explorer\n", x);
		board->bid = BID_EXPLORER;
		board->max_noise_x = 5.0;
		board->max_noise_y = 3.0;
		board->tnt_mode_to_use = 0;
		return 0;
	case 0xb:
	case 0xd:
		printf ("Board ID: 0x%X=Madrid\n", x);
		board->bid = BID_MADRID;
		board->max_noise_x = 2.0;
		board->max_noise_y = 4.0;
		board->tnt_mode_to_use = 1;
		return 0;
	}
	printf ("Unexpected board ID: 0x%x\n", x); 
	return 3;
}

int get_sysfs (const char *sysfs)
{
	char buf[100];
	sprintf (buf, "/sys/devices/platform/lf1000-touchscreen/%s", sysfs);
	FILE *f;
	f = fopen (buf, "r");
	if (!f) { 
		perror ("get_sysfs read"); 
		printf("Failed to open %s for reading\n", buf);
		exit (1); 
	}
	int x;
	if (fscanf (f, "%d", &x) < 1) { 
		printf ("scanf failed on %s\n", buf); 
		exit (1); 
	}
	fclose (f);
	return x;
}

void set_sysfs (const char *sysfs, int value)
{
	char buf[100];
	sprintf (buf, "/sys/devices/platform/lf1000-touchscreen/%s", sysfs);
	FILE *f;
	f = fopen (buf, "w");
	if (!f) { 
		perror ("set_sysfs write"); 
		printf("Failed to open %s for writing\n", buf);
		exit (1); 
	}
	fprintf (f, "%d\n", value);
	fclose (f);
}

void report_to_mfg (int pass)
{
	FILE *f;
	f = fopen ("/tmp/TSCalResult", "w");
	if (!f) { 
		perror ("report_to_mfg write"); 
		printf("Failed to open /tmp/TSCalResult for writing\n");
		exit (1); 
	}
	fprintf (f, "%s\n", pass ? "PASS" : "FAIL");
	fclose (f);
}

void set_raw_events (int enable)
{
	int x = get_sysfs ("report_events");
	x &= ~4; if (enable) x |= 4;
	set_sysfs ("report_events", x);
}

int init_input ()
{
	// Keyboard
	kb = open_input_device(KB_NAME);
	if (kb < 0)
	{
		perror("Can't find " KB_NAME);
		return 1;
	}
	// Input Event 
	if (find_input_device (TSR_NAME, tsr_dev_name) < 0)
	{
		perror ("Can't find " TSR_NAME);
		return 1;
	}
	tsr = open(tsr_dev_name, O_RDONLY);
	if (tsr < 0)
	{
		perror("failed to open ts raw device");
		return 1;
	}
	kb_i = 0;
	polldat[0].fd = kb;
	polldat[0].events = POLLIN;
	tsr_i = 1;
	polldat[1].fd = tsr;
	polldat[1].events = POLLIN;

	// TSLIB
	tsl = ts_open(tsr_dev_name, 1);
	if (!tsl) {
		perror("ts_open");
		return 1;
	}

	if (ts_config(tsl)) {
		perror("ts_config");
		return 1;
	}
	return 0;
}

int fit_line (int n, double *x, double *y, double *m, double *b)
{
	if (n < 2)
	{
		printf ("You can't fit a line to %d points, Jack\n", n);
		return -1;
	}
	int i;
	double sx, sy, sxx, sxy, s1;
	// Compute M
	sx = sy = sxx = sxy = s1 = 0;
	for (i=0; i<n; i++)
	{
		s1 += 1;
		sx += x[i];
		sy += y[i];
		sxx += x[i]*x[i];
		sxy += x[i]*y[i];
	}
	// Thanks www.pdc.kth.se/publications/talks/mpi/pt2pt-i-lab/least-squares.c
	// who credits Dora Abdullah (Fortran version, 11/96)
	double dm = (double)(sx*sy - s1*sxy) / (sx*sx - s1*sxx);
	double db = (sy - dm*sx) / s1;

	// Verify our fit...
	printf ("mx=%9.4f b=%9.4f\n", dm, db);
	for (i=0; i<n; i++)
	{
		printf ("Fit x=%9.5f y=%9.5f -> %9.5f\n",
			x[i], y[i], (.5+dm*x[i]+db));
	}
	*m = dm;
	*b = db;
	return 0;
}

// @x = (0,  0, 1, 1);
// @y = (0,  1, 0, 1);
// @z = (10, 20, 30, 40); # Expect Ax+By+C: A=20, B=10, C=10
void fit_plane (int n, int *x, int *y, int *z, int *a, int *b, int *c, int *z_offset)
{
	int i;
	int sx, sy, sxx, syy, sxy, s1, sz, sxz, syz;
	double m[3][3], in[3][3], d;

	int cx;
	int cy;
	int cz;

	if (n <= 0) {
		cx = cy = cz = 0;
	}
	else {
		for (i = cx = cy = cz = 0; i < n; ++i) {
			cx += x[i];
			cy += y[i];
			cz += z[i];
		}
		cx = (cx + n/2)/n;
		cy = (cy + n/2)/n;
		cz = (cz + n/2)/n;
	}
	*z_offset = cz;

	printf ("Using TNT center offset = %d\n", *z_offset); 
	if (flog)
		fprintf (flog, "Using TNT center offset = %d\n", *z_offset); 

	// Compute M
	sx = sy = sxx = syy = sxy = s1 = 0;
	sxz = syz = sz = 0;
	for (i=0; i<n; i++)
	{
		s1 += 1;
		sx += x[i];
		sy += y[i];
		sz += z[i]-cz;
		sxx += x[i]*x[i];
		syy += y[i]*y[i];
		sxy += x[i]*y[i];
		sxz += x[i]*(z[i]-cz);
		syz += y[i]*(z[i]-cz);
	}

	// m_ij will be m[i][j]
	m[0][0] = sxx; m[0][1] = sxy; m[0][2] = sx;
	m[1][0] = sxy; m[1][1] = syy; m[1][2] = sy;
	m[2][0] = sx;  m[2][1] = sy;  m[2][2] = s1;
	// printf ("s x=%d y=%d z=%d xx=%d yy=%d xy=%d xz=%d yz=%d\n",
	// 	sx, sy, sz, sxx, syy, sxy, sxz, syz);

	// solution is p(x,y) = Ax+Bx+C
	// Solve mu=v, where u=[A, B, C] v=[sxz, syz, sz] and m=above
	// so u=m^-1 v

	// determinant
	d = m[0][0] * (m[1][1]*m[2][2]-m[1][2]*m[2][1]) -
	    m[1][0] * (m[0][1]*m[2][2]-m[0][2]*m[2][1]) +
	    m[2][0] * (m[0][1]*m[1][2]-m[0][2]*m[1][1]);

	// m^-1 = 1/d (C_ij)^T
	in[0][0]= (m[1][1]*m[2][2]-m[2][1]*m[1][2])/d;
	in[0][1]=-(m[1][0]*m[2][2]-m[1][2]*m[2][0])/d;
	in[0][2]= (m[1][0]*m[2][1]-m[2][0]*m[1][1])/d;

	in[1][0]=-(m[0][1]*m[2][2]-m[0][2]*m[2][1])/d;
	in[1][1]= (m[0][0]*m[2][2]-m[0][2]*m[2][0])/d;
	in[1][2]=-(m[0][0]*m[2][1]-m[2][0]*m[0][1])/d;

	in[2][0]= (m[0][1]*m[1][2]-m[0][2]*m[1][1])/d;
	in[2][1]=-(m[0][0]*m[1][2]-m[1][0]*m[0][2])/d;
	in[2][2]= (m[0][0]*m[1][1]-m[1][0]*m[0][1])/d;

	// Compute U
	double A = in[0][0]*sxz + in[0][1]*syz + in[0][2]*sz;
	double B = in[1][0]*sxz + in[1][1]*syz + in[1][2]*sz;
	double C = in[2][0]*sxz + in[2][1]*syz + in[2][2]*sz;

	if (0)
	{
		printf ("d=%9.6f\n", d);
		printf ("A=%9.6f\n", A);
		printf ("B=%9.6f\n", B);
		printf ("C=%9.6f\n", C);

		for (i=0; i<n; i++)
		{
			printf ("At %4d,%4d: Real=%4d Fit=%11.6f\n",
				x[i], y[i], z[i], cz+A*x[i]+B*y[i]+C);
		}
	}

	*a=(int)(A*65536+.5);
	*b=(int)(B*65536+.5);
	*c=(int)(C*65536+.5);

	printf ("TNT Plane Coef: a=%5d b=%5d c=%7d\n", *a, *b, *c);
	if (flog)
	{
		fprintf (flog, "TNT Plane Coef: a=%5d b=%5d c=%7d\n", *a, *b, *c);
		for (i=0; i<n; i++)
		{
			int fit = (*a*x[i]+ *b*y[i]+ *c + 32768)/65536;
			fprintf (flog, "At %4d,%4d: Real=%4d Fit=%4d Offset=%4d\n",
				 x[i], y[i], z[i], cz+fit, fit);
		}
	}

	if (n == 5)
	{
		int xx[] = {512, 0, 1023, 1023, 0};
		int yy[] = {512, 0, 0, 1023, 1023};
		printf ("Fit Points: ");
		for (i = 0; i < n; ++i) {
			printf( "%d, ", cz+(*a*xx[i]+ *b*y[i]+ *c+32768)/65536);
		}
		printf (" center: %d\n",
				cz+(*a*cx + *b*cy + *c + 32768)/65536);
		if (flog)
		{
			fprintf (flog, "Fit Points: ");
			for (i = 0; i < n; ++i) {
				fprintf(flog, "%d, ", cz+(*a*xx[i]+ *b*y[i]+ *c+32768)/65536);
			}
			fprintf (flog, " center: %d\n",
					cz+(*a*cx + *b*cy + *c + 32768)/65536);
		}
	}
	else if (n == 4) {
		int xx[] = {0, 1023, 1023, 0};
		int yy[] = {0, 0, 1023, 1023};
		printf ("Fit Corners: %d, %d, %d, %d  Center: %d\n",
			cz+(*a*xx[0]+ *b*y[0]+ *c+32768)/65536,
			cz+(*a*xx[1]+ *b*y[1]+ *c+32768)/65536,
			cz+(*a*xx[2]+ *b*y[2]+ *c+32768)/65536,
			cz+(*a*xx[3]+ *b*y[3]+ *c+32768)/65536,
			cz+(setting_tnt_plane[0]*cx +
				setting_tnt_plane[1]*cy +
				setting_tnt_plane[2]    + 32768)/65536);
		if (flog)
			fprintf (flog, "Fit Corners: %d, %d, %d, %d  Center: %d\n",
				 cz+(*a*xx[0]+ *b*y[0]+ *c+32768)/65536,
				 cz+(*a*xx[1]+ *b*y[1]+ *c+32768)/65536,
				 cz+(*a*xx[2]+ *b*y[2]+ *c+32768)/65536,
				 cz+(*a*xx[3]+ *b*y[3]+ *c+32768)/65536,
				 cz+(setting_tnt_plane[0]*cx +
					 setting_tnt_plane[1]*cy +
					 setting_tnt_plane[2]    + 32768)/65536);
	}
	else {
		printf ("Unexpected number of points (%d) passed to fit_plane()\n", n);
		fprintf(flog,
				"Unexpected number of points (%d) passed to fit_plane()\n", n);
	}
}

#define N_RAW_DELTA IX_P
#define RAW_0 0
#define SDL_N 8
enum indices { 
	IX_X,  IX_X2, IX_Y,  IX_Y2, 
	IX_P1, IX_P2, IX_T1, IX_T2, IX_P, IX_Delay, N_RAW };
static char *LABEL[] = {
	" X", "X2", " Y", "Y2", "P1", "P2", "T1", "T2", " P", " D" };
static int raw[N_RAW] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, };
int scan_delay_lut[SDL_N] = { 5, 10, 15, 20, 30, 40, 60, 80 };
#define IX_QUIET 7

double target_noise_x, target_noise_y, target_noise_t;
double actual_noise_t;

	/* this routine returns 'true' if the tnt values indicate the pen is not
	 * definitely up.  If either t1 or t2 is greater than the threshold value,
 	 * the routine returns 'false'.  Only if both are less than the threshold
	 * does it return 'true'.
	 * On most units, at least one of the tnt values is near 1023 when the
	 * screen is not touched.  
	 * The 'loose' threshold is lower to deal with the possibility that a
	 * unit's tnt values might not be that high when the screen is untouched.
	 */
inline int pen_is_down ()
{
	int thresh = loose ? 800 : 1000;
	return raw[IX_T1]<=thresh && raw[IX_T2]<=thresh;
}
inline int pen_is_really_down ()
{
	int thresh = loose ? 100 : pen_down_threshold;
	if (board.bid == BID_MADRID)
		return pen_is_down() && (raw[IX_T1]<=thresh)
			&& (abs(raw[IX_T1] - raw[IX_T2]) 
			    <= MAX_TNT_DIFF_WHEN_DOWN);
	else
		return pen_is_down() && raw[IX_T1]<=thresh;
}
inline int pen_is_up () { return !pen_is_down(); }

// Return 1 for bad data, 0 for ok
static char hc_reason[1024];
#define HI_THRESH 25
#define AVG_THRESH 20
#define RANGE_THRESH 10

int fail_heuristic_checks (int center, int corner1, int corner2, int corner3, int corner4)
{
	int avg = (corner1 + corner2 + corner3 + corner4 + 2) / 4;
	int hi = corner1;
#if 1	// 9aug11pm Changed to fix an apparent bug
	int lo = corner1;
#else
	int lo = corner2;
#endif	// 9aug11
	if (hi < corner2)	hi = corner2;
	if (hi < corner3)	hi = corner3;
	if (hi < corner4)	hi = corner4;
	if (lo > corner2)	lo = corner2;
	if (lo > corner3)	lo = corner3;
	if (lo > corner4)	lo = corner4;
	
	printf ("Data for heuristic checks: cen=%d corners=%d,%d,%d,%d avg=%d\n",
		center, corner1, corner2, corner3, corner4, avg);
	if (flog)
		fprintf (flog,
			 "Data for heuristic checks: cen=%d corners=%d,%d,%d,%d avg=%d\n",
			 center, corner1, corner2, corner3, corner4, avg);

	
	if (center > hi)
	{
		sprintf (hc_reason, "%d=center>hi=%d", center, hi);
		return 1;	// center>hi is automatic failure
	}
	// Check for 2 of these 3 tests failing
	strcpy (hc_reason, "");
	int fails = 0;
	if (hi > HI_THRESH)
		sprintf (hc_reason+strlen(hc_reason), "hi:%d>%d ", hi, HI_THRESH), fails++;
	if (avg > AVG_THRESH)
		sprintf (hc_reason+strlen(hc_reason), "avg:%d>%d ", avg, AVG_THRESH), fails++;
	if (hi-lo > RANGE_THRESH)
		sprintf (hc_reason+strlen(hc_reason), "hi-lo:%d>%d", hi-lo, RANGE_THRESH), fails++;
	return fails > 1;
}


int test_drive ()
{
	if (flog)
		fprintf (flog, "Entering test_drive\n");
	struct tsdev *my_tsl;
	if (find_input_device (TS_NAME, ts_dev_name) < 0)
	{
		perror ("Can't find " TS_NAME);
		return 1;
	}
	my_tsl = ts_open(ts_dev_name, 1);
	if (!my_tsl) {
		perror("ts_open");
		return 1;
	}

	if (ts_config(my_tsl)) {
		perror("ts_config");
		return 1;
	}

#ifdef LOG_TESTDRIVE
	int ts_i = 2;
	polldat[2].fd = open_input_device (TS_NAME);
	polldat[2].events = POLLIN;
#endif
	cls ();
	char text[100];
	sprintf (text, "Test Drive: Touch and Draw...");
	drawtext (hsize/2-8*strlen(text)/2, vsize/2, text);
	sprintf (text, "Press A to accept, B to redo");
	drawtext (hsize/2-8*strlen(text)/2, vsize/2+20, text);
	// The main event loop
	while (1) {
		static int tlx=-1, tly=-1, tlf=0;;
		static int x=-1, y=-1, p=-1, state=-1;
		struct ts_sample samp[64];
		struct input_event ev[64];
		int i;
		
		// tslib have something for us?  If so, draw it
		int ret = ts_read(my_tsl, samp, 64);
		for (i=0; i<ret; i++)
		{
#ifdef LOG_TESTDRIVE
			if (flog)
			{
				fprintf (flog, "testdrive: X=%4d Y=%4d P=%4d\n",
					 samp[i].x, samp[i].y, samp[i].pressure);
			}
#endif
			// Pen down?  (but skip if user didn't want it)
			int fatness = (samp[i].pressure+9)/10;
			if (!fatness)
				fatness = tlf;
			if (tlx != -1 && tly != -1)
				fullroundline (tlx, tly, samp[i].x, samp[i].y, tlf, fatness, 0x0);
			else
				fullrounddot (samp[i].x, samp[i].y, fatness, 0x0);
			tlx = samp[i].x;
			tly = samp[i].y;
			tlf = fatness;
			if (!samp[i].pressure)
			{
				tlx = tly = -1;
				tlf = 0;
			}
		}
		
		// input event system: poll keyboard[1] and ts[0]
#ifdef LOG_TESTDRIVE
		if (poll (polldat, 3, 0) > 0)
#else
		if (poll (polldat, 2, 0) > 0)
#endif
		{
			if (polldat[kb_i].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && ev[0].value ==1)
					{
						if (ev[0].code == KEY_ESC)
						{
							cls ();
						}
						if (ev[0].code == KEY_A || ev[0].code == KEY_ESC)
						{
							cls ();
							ts_close (my_tsl);
							return 0; // Happy
						}
						else if (ev[0].code == KEY_B)
						{
							cls ();
							ts_close (my_tsl);
							return 1; // Sad
						}
					}
				}
			}
		}

		if (polldat[tsr_i].revents) // TS raw
		{
			// Must be TS input...
			int rd = read(tsr, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("\nmyinput: error reading");
				return -1;
			}
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				int j;
				switch (ev[i].type)
				{
				case EV_SYN:
					if (flog)
					{
						for (j=0; j<N_RAW; j++)
							fprintf (flog, "%s=%4d ", LABEL[j], raw[j]);
						fprintf (flog, "\n");
					}
					break;
				case EV_ABS:
					j = ev[i].code-RAW_0;
					if (j>=0 && j<N_RAW)
						raw[j] = ev[i].value;
					break;
				}
			}
		}
#ifdef LOG_TESTDRIVE
		if (polldat[ts_i].revents) // TS cooked
		{
			static int x=-1, y=-1, p=-1;
			int rd = read(polldat[ts_i].fd, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("\nmyinput: error reading");
				return -1;
			}
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				int j;
				switch (ev[i].type)
				{
				case EV_SYN:
					if (flog)
					{
						fprintf (flog, "                  i: X=%4d Y=%4d P=%4d\n",
							 x, y, p);
					}
					break;
				case EV_ABS:
					switch (ev[i].code)
					{
					case ABS_X: x=ev[i].value; break;
					case ABS_Y: y=ev[i].value; break;
					case ABS_PRESSURE: p=ev[i].value; break;
					}
					break;
				}
			}
		}
#endif
	}
}

static void default_delays(int just_delays)
{
	// Pick TNT channel
	setting_tnt_mode = board.tnt_mode_to_use;
	if (!just_delays)
		printf ("Using tnt_mode=%d\n", setting_tnt_mode);
	if (flog)
		fprintf (flog, "Using tnt_mode=%d\n", setting_tnt_mode);

		// Changed these delays from 5 to 1 after finding very little
		// difference between results for delays of 1, 3, 5, or 7.
		// While we're at in, ensure that the touchscreen is being sampled 
		// 100 times per second.
	setting_delay_in_us     = 1;
	setting_y_delay_in_us   = 1;
	setting_tnt_delay_in_us = 1;
	set_sysfs ("sample_rate_in_hz", 100);
#if 1		
	setting_max_delta_tnt   = 5;	// 11may11  Changed back to 5 after
									//			discussions with Rob
									// Probably very little difference in
									// performance between 3 and 5.  
									// If we see units with lots of tails,
									// we can change this back to 3.
#else
	setting_max_delta_tnt   = 3;	// 2may11  Changed from 5 to 3 after
									// seeing fewer tails on pm#2 w/
									// max_delta_tnt == 3.
#endif
	target_noise_x = 2.5;
	target_noise_y = 2.5;
	target_noise_t = 2.5;

	if (flog)
	{
		fprintf (flog, "Setting for delay_in_us:     %4d\n",
						setting_delay_in_us);
		fprintf (flog, "Setting for y_delay_in_us:   %4d\n",
						setting_y_delay_in_us);
		fprintf (flog, "Setting for tnt_delay_in_us: %4d\n",
						setting_tnt_delay_in_us);
		fprintf (flog, "Setting for max_delta_tnt:   %4d\n",
						setting_max_delta_tnt);
		fprintf (flog, "Setting for tnt_mode:        %4d\n", setting_tnt_mode);
		fprintf (flog, "Setting for averaging:       %4d\n", setting_averaging);
	}
	if (!just_delays)
	{
		printf ("Setting for delay_in_us:     %4d\n", setting_delay_in_us);
		printf ("Setting for y_delay_in_us:   %4d\n", setting_y_delay_in_us);
		printf ("Setting for tnt_delay_in_us: %4d\n", setting_tnt_delay_in_us);
		printf ("Setting for max_delta_tnt:   %4d\n", setting_max_delta_tnt);
		printf ("Setting for tnt_mode:        %4d\n", setting_tnt_mode);
		printf ("Setting for averaging:       %4d\n", setting_averaging);
	}

	calib_delay_x_point = 0;
	calib_delay_y_point = 0;
}

// Return <0 for abort, >0 for retry, and 0 for Success
// Set just_delays to 1 to reduce logging and expect more input w/o pen-up
int calib_delay (int just_delays, int mfg_mode)
{
	printf ("Entering calib_delay\n");
	if (flog && !just_delays)
		fprintf (flog, "Entering calib_delay\n");
	int cx=hsize/2;
	int cy=vsize/2;
	cls ();
	if (mfg_mode)
		fullrounddot (cx, cy, RAD*RAD, CNOMINAL);
	line (cx-5, cy, cx+5, cy, 0);
	line (cx, cy-5, cx, cy+5, 0);
	char text[100];
	int want=(int)((just_delays ? 8 : 3)*12.5);
	CStats sax[SDL_N], say[SDL_N], sat[SDL_N], 
		   snx[SDL_N], sny[SDL_N], 
		   sdx[SDL_N], sdy[SDL_N], sdt[SDL_N];
	CStats snx2[SDL_N], sny2[SDL_N];
	CStats snt1[SDL_N], snt2[SDL_N];

	int i,j;

	int wait_for_release = 0;
	int in_loop = 1;
	int last_seconds = 0;
	int too_noisy = 0;
	double stdev_thresh = loose ? 1000.0 : 5.0;
	while (in_loop) {
		int seconds = 1+(int)((want-snx[0].N())/12.5);
		if (!wait_for_release && seconds != last_seconds)
		{
			if (mfg_mode)
				sprintf (text, "Insert %dg Pin: %d seconds",
					 NOMINAL_WEIGHT, seconds);
			else
				sprintf (text, "Press and hold here %d seconds",
					 seconds);
			drawtext (cx-8*strlen(text)/2, cy-100, text);
			seconds = last_seconds;
		}

		struct input_event ev[64];
		if (poll (polldat, 2, 0) <= 0)
			continue;
		if (polldat[kb_i].revents) // Keyboard
		{
			int rd = read(kb, ev, sizeof(struct input_event));
			if (rd == sizeof (struct input_event))
			{
				if (ev[0].type == EV_KEY && ev[0].value ==1)
				{
					if (ev[0].code == KEY_B || ev[0].code == KEY_ESC)
					{
						return -2; // Abort
					}
				}
			}
		}
		if (polldat[tsr_i].revents) // TS raw
		{
			// Must be TS input...
			int rd = read(tsr, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("\nmyinput: error reading");
				return -1;
			}
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				int ix, dx, dy, dt, ax, ay, at;
				switch (ev[i].type)
				{
				case EV_ABS:
					j = ev[i].code-RAW_0;
					if (j>=0 && j<N_RAW)
						raw[j] = ev[i].value;
					break;

				case EV_SYN:
					// Make sure we see every raw change once before we go
					for (j=0; j<N_RAW_DELTA; j++)
						if (raw[j] == -1)
							break;
					if (j<N_RAW_DELTA)
						break; // Not ready yet
					if (flog && !just_delays)
					{
						for (j=0; j<N_RAW; j++)
							fprintf (flog, "%s=%4d ", LABEL[j], raw[j]);
						fprintf (flog, "tn=%d wfr=%d sd=%d\n", too_noisy, wait_for_release);
					}
					if (wait_for_release)
					{
						if (pen_is_up ())
							in_loop = 0; // Done
					}
					else
					{
						// Only count down poins
						if (!pen_is_really_down ())
							break;
						ix=0;
						switch (raw[IX_Delay])
						{
						case 5: ix=0; break;
						case 10: ix=1; break;
						case 15: ix=2; break;
						case 20: ix=3; break;
						case 30: ix=4; break;
						case 40: ix=5; break;
						case 60: ix=6; break;
						case 80: ix=7; break;
						}
						dx = (raw[IX_X] - raw[IX_X2]);
						dy = (raw[IX_Y] - raw[IX_Y2]);
						dt = (raw[IX_T1] - raw[IX_T2]);
						ax = abs(raw[IX_X] - raw[IX_X2]);
						ay = abs(raw[IX_Y] - raw[IX_Y2]);
						at = abs(raw[IX_T1] - raw[IX_T2]);
						snx[ix].Add(raw[IX_X]);
						sny[ix].Add(raw[IX_Y]);
						snx2[ix].Add(raw[IX_X2]);
						sny2[ix].Add(raw[IX_Y2]);
						sax[ix].Add(ax);
						say[ix].Add(ay);
						sat[ix].Add(at);
						sdx[ix].Add(dx);
						sdy[ix].Add(dy);
						sdt[ix].Add(dt);
						snt1[ix].Add(raw[IX_T1]);
						snt2[ix].Add(raw[IX_T2]);
						if (snx[0].N() >= want)
						{
							if (just_delays)
							{
								in_loop = 0;
								continue;
							}
							cls ();
							printf ("Release\n");
							if (mfg_mode)
							{
								sprintf (text, "Remove Pin");
								drawtext (cx-8*strlen(text)/2, cy-80, text);
							}
							else
							{
								sprintf (text, "Release");
								drawtext (cx-8*strlen(text)/2, cy, text);
							}
							if (flog)
								fprintf (flog, "%s\n", text);
							wait_for_release = 1; // Done with down
						}
						if (!just_delays && ix==IX_QUIET &&
						    (snx[IX_QUIET].Stdev() > stdev_thresh) ||
						    (sny[IX_QUIET].Stdev() > stdev_thresh))
						{
							cls ();
							printf ("Too Noisy!  %f > %f or %f > %f.  Try again.\n",
								snx[IX_QUIET].Stdev(), stdev_thresh,
								sny[IX_QUIET].Stdev(), stdev_thresh);
							sprintf (text, "Too Noisy!  Try again.");
							drawtext (cx-8*strlen(text)/2, cy-60, text);
							if (flog)
								fprintf (flog, "Too Noisy!  %f > %f or %f > %f.  Try again.\n",
									 snx[IX_QUIET].Stdev(), stdev_thresh,
									 sny[IX_QUIET].Stdev(), stdev_thresh);
							too_noisy = 1;
							wait_for_release = 1; // Done with down
						}
					}
					break;
				}
			}
		}
	}
	if (too_noisy)
		return 1;   // Not same as other returns... This one is > 0

	if (flog)
	{
		fprintf (flog, "                 ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7d", scan_delay_lut[i]);
		fprintf (flog, "\n");

		fprintf (flog, "X Posn:          ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snx[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "X2 Posn:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snx2[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "X Noise:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snx[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "X2 Noise:        ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snx2[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "X Abs Mean Delta:");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", ABS(sdx[i].Mean()));
		fprintf (flog, "\n");
		fprintf (flog, "X Mean Abs Delta:");
		for (i=0; i<SDL_N; i++)
				fprintf (flog, " %7.2f", sax[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "\n");

		fprintf (flog, "Y Posn:          ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", sny[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "Y2 Posn:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", sny2[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "Y Noise:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", sny[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "Y2 Noise:        ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", sny2[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "Y Abs Mean Delta:");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", ABS(sdy[i].Mean()));
		fprintf (flog, "\n");
		fprintf (flog, "Y Mean Abs Delta:");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", say[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "\n");

		fprintf (flog, "T1 Posn:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snt1[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "T2 Posn:         ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snt2[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "T1 Noise:        ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snt1[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "T2 Noise:        ");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", snt2[i].Stdev());
		fprintf (flog, "\n");
		fprintf (flog, "T Abs Mean Delta:");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", ABS(sdt[i].Mean()));
		fprintf (flog, "\n");
		fprintf (flog, "T Mean Abs Delta:");
		for (i=0; i<SDL_N; i++)
			fprintf (flog, " %7.2f", sat[i].Mean());
		fprintf (flog, "\n");
		fprintf (flog, "\n");
	}

	double noise_x1 = 1.5 * (snx[SDL_N-1].Stdev() + snx[SDL_N-2].Stdev()
												  + snx[SDL_N-3].Stdev())/3;
	double noise_y1 = 1.5 * (sny[SDL_N-1].Stdev() + sny[SDL_N-2].Stdev()
												  + sny[SDL_N-3].Stdev())/3;
	double noise_x2 = 1.5 * (snx2[SDL_N-1].Stdev() + snx2[SDL_N-2].Stdev()
												   + snx2[SDL_N-3].Stdev())/3;
	double noise_y2 = 1.5 * (sny2[SDL_N-1].Stdev() + sny2[SDL_N-2].Stdev()
												   + sny2[SDL_N-3].Stdev())/3;
	double noise_t1 = 1.5 * (snt1[SDL_N-1].Stdev() + snt1[SDL_N-2].Stdev()
												   + snt1[SDL_N-3].Stdev())/3;
	double noise_t2 = 1.5 * (snt2[SDL_N-1].Stdev() + snt2[SDL_N-2].Stdev()
												   + snt2[SDL_N-3].Stdev())/3;
	double noise_x = noise_x1 > noise_x2 ? noise_x1 : noise_x2;
	double noise_y = noise_y1 > noise_y2 ? noise_y1 : noise_y2;
	double noise_t = noise_t1 > noise_t2 ? noise_t1 : noise_t2;
	
	actual_noise_t = noise_t/1.5;

	if (!just_delays)
		printf ("actual noise  x=%5.2f y=%5.2f t=%5.2f\n", noise_x/1.5, noise_y/1.5, noise_t/1.5);
	if (flog)
	{
		fprintf (flog, "actual noise  x=%5.2f y=%5.2f t=%5.2f\n", noise_x/1.5, noise_y/1.5, noise_t/1.5);
		fprintf (flog, "target_noise  x=%5.2f y=%5.2f t=%5.2f\n", noise_x, noise_y, noise_t);
	}

	target_noise_x = MAX (2.5, noise_x); // Yet another fudge factor, if the original data is too quiet
	target_noise_y = MAX (2.5, noise_y);
	target_noise_t = MAX (2.5, noise_t);

	// Pick TNT channel
	setting_tnt_mode = board.tnt_mode_to_use;
	// if (noise_x > noise_y)
	// 	setting_tnt_mode = 1; // Y channel is better
	// else
	// 	setting_tnt_mode = 0; // X channel is better
	if (!just_delays)
		printf ("Using tnt_mode=%d\n", setting_tnt_mode);
	if (flog)
		fprintf (flog, "Using tnt_mode=%d\n", setting_tnt_mode);

	// Set settings using MAD < noise
	int set_x_mad_noise=-1, set_y_mad_noise=-1, set_t_mad_noise=-1;
	for (i=0; i<SDL_N; i++)
		if (sax[i].Mean()<noise_x)
		{
			set_x_mad_noise = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (say[i].Mean()<noise_y)
		{
			set_y_mad_noise = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (sat[i].Mean()<noise_t)
		{
			set_t_mad_noise = scan_delay_lut[i];
			break;
		}
	// Set settings using AMD < MAD
	int set_x_amd_mad=-1, set_y_amd_mad=-1, set_t_amd_mad=-1;
	for (i=0; i<SDL_N; i++)
		if (ABS(sdx[i].Mean()) < sax[i].Mean())
		{
			set_x_amd_mad = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (ABS(sdy[i].Mean()) < say[i].Mean())
		{
			set_y_amd_mad = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (ABS(sdt[i].Mean()) < sat[i].Mean())
		{
			set_t_amd_mad = scan_delay_lut[i];
			break;
		}
	// Set settings using MAD < noise
	int set_x_amd=-1, set_y_amd=-1, set_t_amd=-1;
	double converged=2.5;
	for (i=0; i<SDL_N; i++)
		if (ABS(sdx[i].Mean()) < converged)
		{
			set_x_amd = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (ABS(sdy[i].Mean()) < converged)
		{
			set_y_amd = scan_delay_lut[i];
			break;
		}
	for (i=0; i<SDL_N; i++)
		if (ABS(sdt[i].Mean()) < converged)
		{
			set_t_amd = scan_delay_lut[i];
			break;
		}
	// Set looking for MAD change decreasing below converged
	int set_x_mad_conv=-1, set_y_mad_conv=-1, set_t_mad_conv=-1;
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = sax[i].Mean() - sax[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_x_mad_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = say[i].Mean() - say[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_y_mad_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = sat[i].Mean() - sat[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_t_mad_conv = scan_delay_lut[i];
			break;
		}
	}
	// Set looking for channel change decreasing below converged
	int set_x1_conv=-1, set_y1_conv=-1, set_t1_conv=-1;
	int set_x2_conv=-1, set_y2_conv=-1, set_t2_conv=-1;
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = snx[i].Mean() - snx[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_x1_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = snx2[i].Mean() - snx2[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_x2_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = sny[i].Mean() - sny[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_y1_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = sny2[i].Mean() - sny2[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_y2_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = snt1[i].Mean() - snt1[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_t1_conv = scan_delay_lut[i];
			break;
		}
	}
	for (i=0; i<SDL_N-1; i++)
	{
		double ad = snt2[i].Mean() - snt2[i+1].Mean();
		if (ad < 0) ad = -ad;
		if (ad < converged)
		{
			set_t2_conv = scan_delay_lut[i];
			break;
		}
	}
	if (flog)
	{
		fprintf (flog, "   MAD<noise AMD<MAD AMD<Cnv MAD Cnv  Cnv#1   Cnv#2\n");
		fprintf (flog, "X:     %3d     %3d     %3d     %3d     %3d     %3d\n",
			 set_x_mad_noise, set_x_amd_mad, set_x_amd, set_x_mad_conv, set_x1_conv, set_x2_conv);
		fprintf (flog, "Y:     %3d     %3d     %3d     %3d     %3d     %3d\n",
			 set_y_mad_noise, set_y_amd_mad, set_y_amd, set_y_mad_conv, set_y1_conv, set_y2_conv);
		fprintf (flog, "T:     %3d     %3d     %3d     %3d     %3d     %3d\n",
			 set_t_mad_noise, set_t_amd_mad, set_t_amd, set_t_mad_conv, set_t1_conv, set_t2_conv);
	}
	setting_delay_in_us = MAX(set_x_mad_conv, MAX(set_x1_conv, set_x2_conv));
	setting_y_delay_in_us = MAX(set_y_mad_conv, MAX(set_y1_conv, set_y2_conv));
	setting_tnt_delay_in_us = MAX(set_t_mad_conv, MAX(set_t1_conv, set_t2_conv));
	for (i=0; i<SDL_N; i++) {
		if (scan_delay_lut[i] == setting_tnt_delay_in_us)
		{
			setting_max_delta_tnt = (int)(2 * sat[i].Mean());
			break;
		}
	}
	if (setting_max_delta_tnt < 20) // Added 25apr11 for easy comparison
		v3_8_max_delta_tnt = 20;

		// 11may11	Changed back from 3 to 5; if we see too many tails,
		//			we can change it back to 3.
		// 2may11 changed from 5 to 3 after seeing many fewer tails w/ 3.
	if (setting_max_delta_tnt < 5) // Just in case, give us some headroom
	{
		fprintf (flog, "Changed max_delta_int from %d to 5\n", 
				 setting_max_delta_tnt);
		setting_max_delta_tnt = 5;
	}

	// Decide to enable Median mode if all delays are short
	// if (setting_delay_in_us + setting_y_delay_in_us + setting_tnt_delay_in_us < 
	//     4 * scan_delay_lut[0])
	// {
	// 	setting_averaging = -1; // Median mode on
	// }

	if (flog)
	{
		fprintf (flog, "Setting for delay_in_us:     %4d\n", setting_delay_in_us);
		fprintf (flog, "Setting for y_delay_in_us:   %4d\n", setting_y_delay_in_us);
		fprintf (flog, "Setting for tnt_delay_in_us: %4d\n", setting_tnt_delay_in_us);
		fprintf (flog, "Setting for max_delta_tnt:   %4d\n", setting_max_delta_tnt);
		fprintf (flog, "3.8 setting for max_delta_tnt:   %4d\n",
				 		v3_8_max_delta_tnt);
		fprintf (flog, "Setting for tnt_mode:        %4d\n", setting_tnt_mode);
		fprintf (flog, "Setting for averaging:       %4d\n", setting_averaging);
	}
	if (!just_delays)
	{
		printf ("Setting for delay_in_us:     %4d\n", setting_delay_in_us);
		printf ("Setting for y_delay_in_us:   %4d\n", setting_y_delay_in_us);
		printf ("Setting for tnt_delay_in_us: %4d\n", setting_tnt_delay_in_us);
		printf ("Setting for max_delta_tnt:   %4d\n", setting_max_delta_tnt);
		printf ("Setting for tnt_mode:        %4d\n", setting_tnt_mode);
		printf ("Setting for averaging:       %4d\n", setting_averaging);
	}

	calib_delay_x_point = (int)snx[SDL_N-1].Mean();
	calib_delay_y_point = (int)sny[SDL_N-1].Mean();

	return 0;
}

void calib_pressure_mfg_redraw (long color, int weight)
{
	const int cx=hsize/2;
	const int cy=vsize/2;
	char text[100];

	cls ();
	fullrounddot (cx, cy, RAD*RAD, color);
	line (cx-5, cy, cx+5, cy, 0);
	line (cx, cy-5, cx, cy+5, 0);
	sprintf (text, "%dg", weight);
	drawtext (cx-8*strlen(text)/2, cy-80, text);
}

static void store_default_pressure_curve() {
	int pi;

	for (pi=0; pi<N_PCURVE_PTS; pi++)
		setting_pressure[pi] = default_pressure[pi];
	printf ("Using default pressure curve\n");
	fprintf (flog, "Using default pressure curve\n");
	for (pi=0; pi<N_PCURVE_PTS; pi++)
		fprintf (flog, "%d ", setting_pressure[pi]);
	fprintf (flog, "\n");
}

int calib_pressure_mfg (int use_default_pressure, int skip_pressure_data_step)
{
	int ret = 0;
	printf ("Entering calib_pressure_mfg\n");
	if (flog)
		fprintf (flog, "Entering calib_pressure_mfg\n");

	if (skip_pressure_data_step) {
		store_default_pressure_curve();
		set_sysfs ("max_tnt_down", setting_max_tnt_down);
		return 0;
	}

	const int TNT_DOWN=500;
	set_sysfs ("max_tnt_down", TNT_DOWN);

	const int NO_PVAL=9999;
	const int BN=2000;
	int bucket[BN];
	int tnt_bucket[BN];
	const int cx=hsize/2;
	const int cy=vsize/2;
	int p_val[MAX_PI];
	int tnt_val[MAX_PI];
	const int ENOUGH_TRIES = 700;	// 7 seconds
	const int DEAD_TIME = 50; // 1/2 second
	int pi;
	char text[100];

	for (pi=0; pi<MAX_PI; pi++)
	{
		// Clear out p_val
		p_val[pi] = NO_PVAL;

		// Clear out buckets
		memset (bucket, 0, BN*sizeof(int));
		memset (tnt_bucket, 0, BN*sizeof(int));

		// Draw screen
		calib_pressure_mfg_redraw (color_lut[pi], weight_lut[pi]);

		const int PROGRESS_LEN=10;
		char progress_bar[PROGRESS_LEN+3] = "[          ]";
		int progress_level = 0;
		char tries_bar[PROGRESS_LEN+3] = "[          ]";
		int tries = 0;
		int seen_one = 0;
		int never_seen_counter = 0;
		int need_redraw = 0;
		
		int in_loop = 1;
		int finished_collecting = 0;
		int p=0, t1=0, i;
		int t2 = 0;
		int x1 = 0;
		int x2 = 0;
		int y1 = 0;
		int y2 = 0;
		int p1 = 0;
		int p2 = 0;

		while (in_loop) {
			struct input_event ev[64];
			if (poll (polldat, 2, 0) <= 0)
				continue;
			if (polldat[kb_i].revents) // Keyboard
			{
				int rd = read(kb, ev, sizeof(struct input_event));
				if (rd == sizeof (struct input_event))
				{
					if (ev[0].type == EV_KEY && ev[0].value ==1)
					{
						if (ev[0].code == KEY_A || ev[0].code == KEY_ESC)
						{
							cls ();
							// in_loop = 0;
							return -2; // Abort
						}
					}
				}
			}
			if (polldat[tsr_i].revents) // TS raw
			{
				// Must be TS input...
				int rd = read(tsr, ev, sizeof(struct input_event) * 64);
				if (rd < (int) sizeof(struct input_event)) {
					perror("\nmyinput: error reading");
					return -1;
				}
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					int where;
					switch (ev[i].type)
					{
					case EV_ABS:
						if (ev[i].code == IX_T1) t1 = ev[i].value;
						switch (ev[i].code) {
						case IX_T2: t2 = ev[i].value; break;
						case IX_X:  x1 = ev[i].value; break;
						case IX_X2: x2 = ev[i].value; break;
						case IX_Y:  y1 = ev[i].value; break;
						case IX_Y2: y2 = ev[i].value; break;
						case IX_P1: p1 = ev[i].value; break;
						case IX_P2: p2 = ev[i].value; break;
						}
						if (ev[i].code == IX_P)
						{
							p = ev[i].value;
							// Quantize a bit and bound
							if (p>=BN) p=BN-1;
							if (p<0) p=0;
							p &= ~1;
						}
						break;

					case EV_SYN:
						// Log what we got
						if (flog)
							if (!finished_collecting && t1 < TNT_DOWN)
							{
								fprintf (flog, "calib_p %dg: Tries=%3d "
											   "x1=%4d x2=%4d "
											   "y1=%4d y2=%4d p1=%4d p2=%4d "
											   "T1=%4d T2=%4d P=%4d %c%c%c%c\n", 
									 weight_lut[pi], tries, 
									 x1, x2, y1, y2, p1, p2, t1, t2, p,
									 (abs(x1 - x2) > 3) ? 'x' : '_',
									 (abs(y1 - y2) > 3) ? 'y' : '_',
									 (abs(p1 - p2) > 3) ? 'p' : '_',
									 (abs(t1 - t2) > 3) ? 't' : '_');
							}

						// Show Tries
						where=1+(PROGRESS_LEN*tries)/ENOUGH_TRIES;
						if (tries_bar[where] != '-')
						{
							tries_bar[where] = '-';
							drawtext (cx-8*strlen(tries_bar)/2, cy-100, tries_bar);
						}

						// Give up if we cant' get enough data
						if (seen_one==1 && finished_collecting==0 && ++tries > ENOUGH_TRIES)
						{
						give_up:
							p=NO_PVAL;
							printf ("Mode %3dg: P=%4d T=%2d\n", weight_lut[pi], p, 1023);
							if (flog)
								fprintf (flog, "Mode %3dg: P=%4d T=%2d\n", weight_lut[pi], p, 1023);
							p_val[pi] = p;
							finished_collecting=1;
							// wait_for_menu=1;
							cls ();
							sprintf (text, "Can't Sense");
							drawtext (cx-8*strlen(text)/2, cy-90, text);
							sprintf (text, "Remove Pin"); //  and Press MENU");
							drawtext (cx-8*strlen(text)/2, cy-60, text);
							break;
						}
						// Collect a good point
						if (t1 < TNT_DOWN && p>0 && p<BN && finished_collecting==0)
						{
							if (need_redraw)
							{
								need_redraw = 0;
								calib_pressure_mfg_redraw (color_lut[pi], weight_lut[pi]);
							}
							seen_one = 1;
							bucket[p]++;
							tnt_bucket[t1]++;
							// Show progress
							if (bucket[p] > progress_level)
							{
								progress_level = bucket[p];
								int where=1+(PROGRESS_LEN*progress_level)/enough_good[pi];
								progress_bar[where] = 'X';
								drawtext (cx-8*strlen(progress_bar)/2, cy-80, progress_bar);
							}
							// Success, take "p" as the mode
							if (bucket[p] >= enough_good[pi])
							{
								// Find the TNT mode, hopefully this is easy!
								int i, tm=0, m=0;
								for (i=10; i<30; i++)
									printf ("%2d: %3d\n", i, tnt_bucket[i]);
								for (i=0; i<BN; i++)
									if (tnt_bucket[i]>m)
										tm=i, m=tnt_bucket[i];

								printf ("Mode %3dg: P=%4d T=%2d\n", weight_lut[pi], p, tm);
								if (flog)
									fprintf (flog, "Mode %3dg: P=%4d T=%2d\n", weight_lut[pi], p, tm);

								tnt_val[pi] = tm;
								p_val[pi] = p;
								finished_collecting=1;

								if (   (pi == PI_NOMINAL)
									&& (tm > avg_of_corner_tnts 
											 + MAX_DIFF_AVG_TO_CENTER_TNT))
								{
									printf("FAIL: TNT for 40g at center (%d)"
										   " is not within %d of avg tnt"
										   " of corners (%d)\n",
											tm, MAX_DIFF_AVG_TO_CENTER_TNT,
											avg_of_corner_tnts);
									fprintf(flog,
										   "FAIL: TNT for 40g at center (%d)"
										   " is not within %d of avg tnt"
										   " of corners (%d)\n",
											tm, MAX_DIFF_AVG_TO_CENTER_TNT,
											avg_of_corner_tnts);
									cls ();
									return -4;
								}
								cls ();
								sprintf (text, "Remove Pin");
								drawtext (cx-8*strlen(text)/2, cy-60, text);
							}
						}
						// If we never see a pin down, give up eventually
						if (t1 > TNT_DOWN && !seen_one)
						{
							never_seen_counter++;
							// If we've already blinked long enough, give up
							if (never_seen_counter > ENOUGH_TRIES*2)
							{
								// Pretend to have seen one, then given up
								seen_one = 1;
								goto give_up;
							}
							// Try blinking to attract attention
							if (never_seen_counter > ENOUGH_TRIES)
							{
								// Only update once in a while, so we don't slow down UI
								if (((never_seen_counter-ENOUGH_TRIES) % 25)==0)
								{
									if (((never_seen_counter-ENOUGH_TRIES) / 25) & 1)
									{
										cls ();
										need_redraw = 1;
									}
									else
									{
										calib_pressure_mfg_redraw (color_lut[pi], weight_lut[pi]);
									}
								}
							}
						}
						// Count a few pen-up points to be sure the pin has been removed
						if (t1 > TNT_DOWN && finished_collecting>0)
						{
							finished_collecting++;
							if (finished_collecting > DEAD_TIME)
								in_loop = 0;
						}
						break;
					}
				}
			}
		}
	}
	// Test for failure
	for (pi=0; pi<MAX_PI; pi++)
	{
		if (p_val[pi]==NO_PVAL)
		{
			// ret=-3; // Failure
			printf ("FAILED to measure %dg weight\n", weight_lut[pi]);
			if (flog)
				fprintf (flog, "FAILED to measure %dg weight\n", weight_lut[pi]);
		}
	}
	// convert p_val[] to setting_pressure[]; check for failures
	// Fit a 7-point curve through 30g to 200g.  
	// Fit another 2 points at 200g *2/3 and *1/3 to capture finger pressure
#if 1	// 29apr11	Experiment: fit a straight line to only 4 of the points
		//			If tests show that this fitting is close to the 9-point 
		// fitting, we can shorten the pressure calibration step.
//	const int weight_lut[MAX_PI] =      { 20, 25, 30, 40, 50, 75, 100, 150, 200 };
	if (1)
	{
		// We're going to fit (1/x, y) with a line, chucking outliers
		int i, n = 0;
		double x[MAX_PI], y[MAX_PI];
		// EXPERIMENT: use 30, 50, 100, 200
		int s, subsam[4] = {2, 4, 6, 8};

			// try to ensure there's a valid reading for the first weight

		for (i = subsam[0]; i < subsam[1]; ++i) {
			if (p_val[i] > 0 && p_val[i] < pressure_upper_limit)
				break;
		}
		subsam[0] = ((i < subsam[1]) ? i : -1);

		for (s = 0; s < 4; ++s) {
			i = subsam[s];
			if (i < 0) {
				if (flog)
					fprintf (flog, "ExpFitting: no valid reading < 50g\n");
				continue;
			}
			if (p_val[i] > 0 && p_val[i] < pressure_upper_limit)
			{
				if (flog)
					fprintf (flog, "ExpFitting  %dg at %d\n", 
									weight_lut[i], p_val[i]);
				x[n] = 1.0/weight_lut[i];
				y[n] = p_val[i];
				n++;
			}
			else
			{
				if (flog)
					fprintf (flog, "ExpSkipping %dg at %d\n", 
									weight_lut[i], p_val[i]);
			}
		}
		double m, b;
		if (fit_line (n, x, y, &m, &b))
		{
			// Trouble!
			printf ("Exp.fit_line returned failure.  No pressure curve!!\n");
			if (flog)
				fprintf (flog, "Exp.fit_line returned failure.  No pressure curve!!\n");
			ret = -3;
		}
		else
		{
			// Success
			// Verify our fit...
			if (flog)
				fprintf (flog, "Exp.fit_line: mx=%9.4f b=%9.4f\n", m, b);
			int i;
			for (i=0; i<n; i++)
			{
				if (flog)
					fprintf (flog, "Fit x=%9.5f y=%9.5f -> %9.5f\n",
						 x[i], y[i], (.5+m*x[i]+b));
			}

			// Generate 9 points now
			// Paint in first 7: 200g, 150g, 100g, 75g, 50g, 40g, 30g
			// Note magic: x=1/weight_lut[] so don't expect to see "mx+b" exactly below
			for (i=0; i<7; i++)
				setting_pressure[i+2] = (int)(.5+m/weight_lut[MAX_PI-1-i]+b);
			// Then, pitch in a couple of very heavy points for finger presses
			setting_pressure[1] = (int)(.5+m/800+b);
			setting_pressure[0] = (int)(.5+m/2000+b);

			printf ("Exp.Setting pressure: ");
			for (pi=0; pi<N_PCURVE_PTS; pi++)
				printf ("%d ", setting_pressure[pi]);
			printf ("\n");
			if (flog)
			{
				fprintf (flog, "Exp.Setting pressure: ");
				for (pi=0; pi<N_PCURVE_PTS; pi++)
					fprintf (flog, "%d ", setting_pressure[pi]);
				fprintf (flog, "\n");
			}
		}
	}
#endif

	if (1)
	{
		// We're going to fit (1/x, y) with a line, chucking outliers
		int i, n = 0;
		double x[MAX_PI], y[MAX_PI];
		for (i=0; i<MAX_PI; i++)
		{
			// Accept good points only -- chuck wild outliers and non-reads
			if (p_val[i] > 0 && p_val[i] < pressure_upper_limit)
			{
				if (flog)
					fprintf (flog, "Fitting  %dg at %d\n", weight_lut[i], p_val[i]);
				x[n] = 1.0/weight_lut[i];
				y[n] = p_val[i];
				n++;
			}
			else
			{
				if (flog)
					fprintf (flog, "Skipping %dg at %d\n", weight_lut[i], p_val[i]);
			}
		}
		double m, b;
		if (fit_line (n, x, y, &m, &b))
		{
			// Trouble!
			printf ("fit_line returned failure.  No pressure curve!!\n");
			if (flog)
				fprintf (flog, "fit_line returned failure.  No pressure curve!!\n");
			ret = -3;
		}
		else
		{
			// Success
			// Verify our fit...
			if (flog)
				fprintf (flog, "fit_line: mx=%9.4f b=%9.4f\n", m, b);
			int i;
			for (i=0; i<n; i++)
			{
				if (flog)
					fprintf (flog, "Fit x=%9.5f y=%9.5f -> %9.5f\n",
						 x[i], y[i], (.5+m*x[i]+b));
			}

			// Generate 9 points now
			// Paint in first 7: 200g, 150g, 100g, 75g, 50g, 40g, 30g
			// Note magic: x=1/weight_lut[] so don't expect to see "mx+b" exactly below
			for (i=0; i<7; i++)
				setting_pressure[i+2] = (int)(.5+m/weight_lut[MAX_PI-1-i]+b);
			// Then, pitch in a couple of very heavy points for finger presses
			setting_pressure[1] = (int)(.5+m/800+b);
			setting_pressure[0] = (int)(.5+m/2000+b);
		}
	}

		// Added for easy comparison of 3.8/3.9 with 3.7
	v3_7_max_tnt_down = setting_max_tnt_down;
	if (p_val[PI_MINIMAL] != NO_PVAL)
	{
		v3_7_max_tnt_down = tnt_val[PI_MINIMAL];
		printf ("3.7 setting for max_tnt_down: %d\n", v3_7_max_tnt_down);
		if (flog)
		{
			fprintf (flog, "3.7 setting for max_tnt_down: %d\n",
							v3_7_max_tnt_down);
		}
	}

	// Report pressure curve
	if (ret==0)
	{
		printf ("Setting pressure: ");
		for (pi=0; pi<N_PCURVE_PTS; pi++)
			printf ("%d ", setting_pressure[pi]);
		printf ("\n");
		if (flog)
		{
			fprintf (flog, "Setting pressure: ");
			for (pi=0; pi<N_PCURVE_PTS; pi++)
				fprintf (flog, "%d ", setting_pressure[pi]);
			fprintf (flog, "\n");
		}
	}
		// near the start of the routine, we set it to TNT_DOWN
	set_sysfs ("max_tnt_down", setting_max_tnt_down);

	if (use_default_pressure) 
	{
		store_default_pressure_curve();
	}
	// Clean up
	cls ();
	return ret;
}

int calib_pressure ()
{
	int ret = 0;
	printf ("Entering calib_pressure\n");
	if (flog)
		fprintf (flog, "Entering calib_pressure\n");

	const int B0=200, B1=1200, BN=40;
	const int BD=B1-B0, BW=BD/BN;
	int bucket[BN];
	memset (bucket, 0, BN*sizeof(int));

	int cx=hsize/2;
	int cy=vsize/2;
	cls ();
	char text[100];
	sprintf (text, "Pressure Calibration:");
	drawtext (cx-8*strlen(text)/2, cy-60, text);
	sprintf (text, "Do some typical drawing with");
	drawtext (cx-8*strlen(text)/2, cy-40, text);
	sprintf (text, "both finger and stylus.");
	drawtext (cx-8*strlen(text)/2, cy-40+16, text);
	sprintf (text, "Watch the histogram on the right grow!");
	drawtext (cx-8*strlen(text)/2, cy, text);
	sprintf (text, "Top=High pressure; Bottom=Low");
	drawtext (cx-8*strlen(text)/2, cy+20, text);
	sprintf (text, "Press A to Test Drive");
	drawtext (cx-8*strlen(text)/2, cy+60, text);

	int i,j,x,y;
	CMakeHisto sp;
	int in_loop = 1;
	int pen_state = 0;
	x=y=10;
	while (in_loop) {
		struct input_event ev[64];
		if (poll (polldat, 2, 0) <= 0)
			continue;
		if (polldat[kb_i].revents) // Keyboard
		{
			int rd = read(kb, ev, sizeof(struct input_event));
			if (rd == sizeof (struct input_event))
			{
				if (ev[0].type == EV_KEY && ev[0].value ==1)
				{
					if (ev[0].code == KEY_A || ev[0].code == KEY_ESC)
				{
						sp.Partition (setting_pressure);
						printf ("Setting pressure: ");
						for (i=0; i<N_PCURVE_PTS; i++)
							printf ("%d ", setting_pressure[i]);
						printf ("\n");
						cls ();
						ret = 0;
						in_loop = 0;
					}
					else if (ev[0].code == KEY_B)
					{
						cls ();
						ret = -2; // Abort
						in_loop = 0;
					}
				}
			}
		}
		if (polldat[tsr_i].revents) // TS raw
		{
			// Must be TS input...
			int rd = read(tsr, ev, sizeof(struct input_event) * 64);
			if (rd < (int) sizeof(struct input_event)) {
				perror("\nmyinput: error reading");
				return -1;
			}
			for (i = 0; i < rd / sizeof(struct input_event); i++)
			{
				switch (ev[i].type)
				{
				case EV_ABS:
					if (ev[i].code == IX_P)
						if (flog)
							fprintf (flog, "calib_pressure: P=%4d\n", ev[i].value);
					if (ev[i].code == IX_P && ev[i].value)
					{
						sp.Add(ev[i].value);
						// if (flog)
						// 	fprintf (flog, "calib_pressure: P=%4d\n", ev[i].value);
						int BSW=vsize/BN;
						int ix = (ev[i].value - B0)/BW;
						if (ix < 0) ix = 0;
						if (ix >= BN) ix = BN-1;
						bucket[ix]++;
						line (hsize-bucket[ix], ix*BSW, hsize-bucket[ix], (ix+1)*BSW-1, 0xff8030);
					}
				}
			}
		}
	}
	return ret;
}

int get_five_points (int mfg_mode, int heuristic_checks)
{
	printf ("Entering get_five_points\n");
	if (flog)
		fprintf (flog, "Entering get_five_points\n");
#define NPOINTS	5
#define DOWN_COUNT	(20+8)	// Total kept is 4 less
#define UP_COUNT	3
#define MM_CORNER_HISTORY	6
	int i,j;
	int state = 0;
	enum target_ix { TC, T00, T10, T11, T01 };
	int target_x[NPOINTS] = { hsize/2, 23, hsize-1-23, hsize-1-23, 23 };
	int target_y[NPOINTS] = { vsize/2, 22, 22, vsize-1-22, vsize-1-22 };
	int label_x[NPOINTS] = { hsize/2 - 20, 30, hsize-70, hsize-70, 30}; // label=40 dots
	int label_y[NPOINTS] = { 23-8, 23-8, 23-8, vsize-24-8, vsize-24-8 };
	char text[100];

	int prev_x = calib_delay_x_point;
	int prev_y = calib_delay_y_point;

	int down_to_go, up_to_go;

	CMaxMin hit_t[NPOINTS];
	CStats hit_x[NPOINTS], hit_y[NPOINTS];
	struct input_event ev[64];

	for (i=0; i<NPOINTS; i++)
		hit_t[i].Init(MM_CORNER_HISTORY);

	printf ("target_noise  x=%5.2f y=%5.2f t=%5.2f\n", 
			target_noise_x, target_noise_y, target_noise_t);
	if (flog)
		fprintf (flog, "target_noise  x=%5.2f y=%5.2f t=%5.2f\n", 
			 target_noise_x, target_noise_y, target_noise_t);
	while (state < NPOINTS)
	{
		const int ENOUGH_TRIES = 700;	// 7 seconds
		int never_seen_counter = 0;
		int tries = 0;

		// Draw target
		cls ();
		int cx = target_x[state];
		int cy = target_y[state];
		if (mfg_mode)
			fullrounddot (cx, cy, RAD*RAD, CMINIMAL);
		line (cx-5, cy, cx+5, cy, 0);
		line (cx, cy-5, cx, cy+5, 0);

		if (mfg_mode)
		{
			sprintf (text, "Insert %dg Pin", MINIMAL_WEIGHT);
			int cx2 = hsize/2;
			int cy2 = vsize/2;

			drawtext (cx2-8*strlen(text)/2, cy2-100, text);
		}
		else
			drawtext (label_x[state], label_y[state], "Touch");
		// Clear stats
		hit_x[state].Init();
		hit_y[state].Init();
		hit_t[state].Init(MM_CORNER_HISTORY);

		down_to_go = DOWN_COUNT;
		up_to_go = UP_COUNT;

		if (flog)
			fprintf (flog, "target state=%d point=%d,%d\n",
				 state, cx, cy);
		int in_loop = 1;
		int too_noisy = 0;
		while (in_loop) {
			if (poll (polldat, 2, 0) > 0)
			{
				if (polldat[kb_i].revents) // Keyboard
				{
					int rd = read(kb, ev, sizeof(struct input_event));
					if (rd == sizeof (struct input_event))
					{
						if (ev[0].type == EV_KEY && ev[0].value ==1)
						{
							if (ev[0].code == KEY_B || ev[0].code == KEY_ESC)
							{
								// in_loop = 0;
								return -1;
							}
						}
					}
				}
				if (polldat[tsr_i].revents) // TS raw
				{
					// Must be TS input...
					int rd = read(tsr, ev, sizeof(struct input_event) * 64);
					if (rd < (int) sizeof(struct input_event)) {
						perror("error reading");
						break;
					}
					for (i = 0; 
						 in_loop && (i < rd / sizeof(struct input_event)); i++)
					{
						switch (ev[i].type)
						{
						case EV_SYN:
							if (flog)
							{
								for (j=0; j<N_RAW; j++)
									fprintf (flog, "%s=%4d ", LABEL[j], raw[j]);
								if (pen_is_really_down ()) fprintf (flog, "* %.0f", 
												    hit_x[state].N());
								fprintf (flog, "\n");
							}
								// if we've already seen the pen down or if
								// it's now down, increment tries
							if ((tries > 0) || pen_is_down()) 
							{
								if (++tries >= ENOUGH_TRIES)
								{
									// timeout; fail the touch screen
									printf("Timeout: pin not really down\n");
									fprintf(flog,
											"Timeout: pin not really down\n");
									cls ();
									sprintf (text, "FAILED !!");
									int cx2 = hsize/2;
									int cy2 = vsize/2;
									drawtext (cx2-8*strlen(text)/2, cy2-100, text);
									return 1;
								}
							}
							if (too_noisy)
							{
								if (pen_is_up())
								{
									in_loop = 0;
									too_noisy =0;
									break;
								}
								break;
							}
							if (down_to_go && pen_is_really_down())
							{
								// Collecting a down point, but not enough yet
								--down_to_go;
								if (! down_to_go)
								{
									// Got enough, ask for release
									int cx = hsize/2;
									int cy = vsize/2;
									cls ();
									if (mfg_mode)
									{
										sprintf (text, "Remove Pin");
										drawtext (cx-8*strlen(text)/2, cy-80, text);
									}
									else
									{
										sprintf (text, "Release");
										drawtext (cx-8*strlen(text)/2, cy, text);
									}
									if (flog)
										fprintf (flog, "%s\n", text);
									prev_x = ((raw[IX_X]+raw[IX_X2])/2);
									prev_y = ((raw[IX_Y]+raw[IX_Y2])/2);
								}
							}
							if (!down_to_go && up_to_go)
							{
								if (pen_is_up())
									--up_to_go;
							}
							if (!down_to_go && !up_to_go)
							{
								// Done
								if (in_loop) {
									state++;
									in_loop = 0;
								}
							}
							if (pen_is_really_down() && down_to_go &&
							    down_to_go+3 < DOWN_COUNT) // throw out first 3 points)
							{
								// Down point
								hit_t[state].Add((raw[IX_T1]+raw[IX_T2])/2);
								hit_x[state].Add((raw[IX_X]+raw[IX_X2])/2);
								hit_y[state].Add((raw[IX_Y]+raw[IX_Y2])/2);
								// Check for previous point
								if (abs(prev_x - ((raw[IX_X]+raw[IX_X2])/2)) < 10 &&
								    abs(prev_y - ((raw[IX_Y]+raw[IX_Y2])/2)) < 10)
								{
									// too close, ignore this
									too_noisy = 1;
								}


								// Sanity
								if (hit_x[state].Stdev() > target_noise_x ||
								    hit_y[state].Stdev() > target_noise_y)
								{
									int cx = (hsize/2 + label_x[state])/2;
									int cy = (vsize/2 + label_y[state])/2;
									cls ();
									printf ("Too Noisy!  %f > %f or %f > %f.  Try again.\n",
										hit_x[state].Stdev(), target_noise_x,
										hit_y[state].Stdev(), target_noise_y);
									sprintf (text, "Too noisy!  Try again.");
									drawtext (cx-8*strlen(text)/2, cy, text);
									too_noisy = 1;
									if (flog)
										fprintf (flog, 
											 "Too Noisy!  %f > %f or %f > %f.  Try again.\n",
											hit_x[state].Stdev(), target_noise_x,
											hit_y[state].Stdev(), target_noise_y);

								}
							}
								// Flash a circle until pen is inserted
							if ((tries == 0) && !pen_is_down())
							{
								never_seen_counter++;
								// If we've already blinked long enough, give up
								if (never_seen_counter > ENOUGH_TRIES*2)
								{
									// timeout; fail the touch screen
									printf("Timeout: pin not inserted\n");
									fprintf(flog,
											"Timeout: pin not inserted\n");
									cls ();
									sprintf (text, "FAILED !!");
									int cx2 = hsize/2;
									int cy2 = vsize/2;
									drawtext (cx2-8*strlen(text)/2, cy2-100, text);
									return 2;
								}
								// Try blinking to attract attention
								if (never_seen_counter > ENOUGH_TRIES)
								{
									// Only update once in a while, so we don't slow down UI
									if (((never_seen_counter-ENOUGH_TRIES) % 25)==0)
									{
										if (((never_seen_counter-ENOUGH_TRIES) / 25) & 1)
										{
											cls ();
										}
										else
										{
											if (mfg_mode)
												fullrounddot (cx, cy, RAD*RAD, CMINIMAL);
											line (cx-5, cy, cx+5, cy, 0);
											line (cx, cy-5, cx, cy+5, 0);
										}
										if (mfg_mode)
										{
											sprintf (text, "Insert %dg Pin", MINIMAL_WEIGHT);
											int cx2 = hsize/2;
											int cy2 = vsize/2;
											drawtext (cx2-8*strlen(text)/2, cy2-100, text);
										}
										else
											drawtext (label_x[state], label_y[state],
													 "Touch");
									}
								}
							}
							break;
						case EV_ABS:
							j = ev[i].code-RAW_0;
							if (j>=0 && j<N_RAW)
							{
								raw[j] = ev[i].value;
							}
							break;
						}
					}
				}
			}
		}
	}
	cls ();

	// Pointercal
	int screen_x[3], screen_y[3];
	int adc_x[3], adc_y[3];
	screen_x[PT00] = target_x[T00]; screen_y[PT00] = target_y[T00];
	screen_x[PT10] = target_x[T10]; screen_y[PT10] = target_y[T10];
	screen_x[PT01] = target_x[T01]; screen_y[PT01] = target_y[T01];
	adc_x[PT00] = (int)hit_x[T00].Mean(); adc_y[PT00] = (int)hit_y[T00].Mean();
	adc_x[PT10] = (int)hit_x[T10].Mean(); adc_y[PT10] = (int)hit_y[T10].Mean();
	adc_x[PT01] = (int)hit_x[T01].Mean(); adc_y[PT01] = (int)hit_y[T01].Mean();
	compute_affine (screen_x, screen_y, adc_x, adc_y, setting_pointercal);

	if (flog)
	{
		fprintf (flog, "Target Noise  x=%5.2f y=%5.2f\n", 
						target_noise_x, target_noise_y);
		for (i=0; i<NPOINTS; i++)
		{
			fprintf (flog, "X[%d]: Target=%3d  %4.0f/%4.0f/%4.0f Stdev=%5.2f N=%3.0f  ", 
				 i, target_x[i],
				 hit_x[i].Min(), hit_x[i].Mean(), hit_x[i].Max(), 
				 hit_x[i].Stdev(), hit_x[i].N());
			fprintf (flog, "Y[%d]: Target=%3d  %4.0f/%4.0f/%4.0f Stdev=%5.2f N=%3.0f\n", 
				 i, target_y[i],
				 hit_y[i].Min(), hit_y[i].Mean(), hit_y[i].Max(), 
				 hit_y[i].Stdev(), hit_y[i].N());
		}
	}

	// TNT plane
	int xx[NPOINTS], yy[NPOINTS], zz[NPOINTS], a, b, c, tnt_offset;
	xx[TC] = (int)hit_x[TC].Mean(); yy[TC] = (int)hit_y[TC].Mean();	
									zz[TC] = hit_t[TC].Max(MM_CORNER_HISTORY-1);
	xx[T00] = (int)hit_x[T00].Mean(); yy[T00] = (int)hit_y[T00].Mean();	
									zz[T00] = hit_t[T00].Max(MM_CORNER_HISTORY-1);
	xx[T10] = (int)hit_x[T10].Mean(); yy[T10] = (int)hit_y[T10].Mean();	
									zz[T10] = hit_t[T10].Max(MM_CORNER_HISTORY-1);
	xx[T11] = (int)hit_x[T11].Mean(); yy[T11] = (int)hit_y[T11].Mean();	
									zz[T11] = hit_t[T11].Max(MM_CORNER_HISTORY-1);
	xx[T01] = (int)hit_x[T01].Mean(); yy[T01] = (int)hit_y[T01].Mean();	
									zz[T01] = hit_t[T01].Max(MM_CORNER_HISTORY-1);
	for (i=0; i<NPOINTS; i++)
	{
		hit_t[i].Dump(stdout);
		if (flog)
			hit_t[i].Dump(flog);
	}

	fit_plane (NPOINTS, xx, yy, zz, 
		   &setting_tnt_plane[0], 
		   &setting_tnt_plane[1], 
		   &setting_tnt_plane[2],
		   &tnt_offset);

	// Do heuristic checks on TNT, if desired
	if (heuristic_checks && fail_heuristic_checks (zz[TC], zz[T00], zz[T10], zz[T11], zz[T01]))
	{
		printf("Failed heuristic checks: %s\n", hc_reason);
		if (flog)
			fprintf(flog, "Failed heuristic checks: %s\n", hc_reason);
		cls ();
		sprintf (text, "FAILED !!");
		int cx2 = hsize/2;
		int cy2 = vsize/2;
		drawtext (cx2-8*strlen(text)/2, cy2-100, text);
		return 3;
	}

	// Pick a threshold for TNT in the center.
	double tnt_padding = 3*actual_noise_t; // 3 sigmas good enough?
		// Fudge it up if there is too little noise
	if (tnt_padding < MIN_TNT_PADDING) 
		tnt_padding = MIN_TNT_PADDING;
		// Save this for comparison with tnt of 40g during pressure cal
	avg_of_corner_tnts = (zz[T00] + zz[T01] + zz[T10] + zz[T11] + 2)/4;

	setting_min_tnt_up = (1024 + tnt_offset)/2;
	setting_max_tnt_down = tnt_offset + (int)tnt_padding;
	v2_max_tnt_down = (int)(1.5*setting_max_tnt_down);
	printf ("Setting for min_tnt_up:   %d\n", setting_min_tnt_up);
	printf ("Setting for max_tnt_down: %d (V2:%d)\n", 
			setting_max_tnt_down, v2_max_tnt_down);
	if (flog)
	{
		fprintf (flog, "Setting for min_tnt_up:   %d\n", setting_min_tnt_up);
		fprintf (flog, "Setting for max_tnt_down: %d (V2:%d)\n", 
						setting_max_tnt_down, v2_max_tnt_down);
	}

	return 0;
}

int get_four_points (int mfg_mode, int heuristic_checks)
{
	printf ("Entering get_four_points\n");
	if (flog)
		fprintf (flog, "Entering get_four_points\n");
#define N_POINTS	4
#define DOWN_COUNT	(20+8)	// Total kept is 4 less
#define UP_COUNT	3
#define MM_CENTER_HISTORY	20
#define MM_CORNER_HISTORY	6
	int i,j;
	int state = 0;
	enum target_ix { T00, T10, T11, T01 };
	int target_x[N_POINTS] = { 23, hsize-1-23, hsize-1-23, 23 };
	int target_y[N_POINTS] = { 22, 22, vsize-1-22, vsize-1-22 };
	int label_x[N_POINTS] = { 30, hsize-70, hsize-70, 30}; // label=40 dots
	int label_y[N_POINTS] = { 23-8, 23-8, vsize-24-8, vsize-24-8 };
	char text[100];

	int prev_x = calib_delay_x_point;
	int prev_y = calib_delay_y_point;

	int down_to_go, up_to_go;

	// CMaxMin mmt1; // , mmt2;
	CMaxMin hit_t[N_POINTS];
	CStats hit_x[N_POINTS], hit_y[N_POINTS];
	struct input_event ev[64];

	for (i=0; i<N_POINTS; i++)
		hit_t[i].Init(MM_CORNER_HISTORY);

	printf ("target_noise  x=%5.2f y=%5.2f t=%5.2f\n", 
			target_noise_x, target_noise_y, target_noise_t);
	if (flog)
		fprintf (flog, "target_noise  x=%5.2f y=%5.2f t=%5.2f\n", 
			 target_noise_x, target_noise_y, target_noise_t);
	while (state < N_POINTS)
	{
		const int ENOUGH_TRIES = 700;	// 7 seconds
		int never_seen_counter = 0;
		int tries = 0;

		// Draw target
		cls ();
		int cx = target_x[state];
		int cy = target_y[state];
		if (mfg_mode)
			fullrounddot (cx, cy, RAD*RAD, CMINIMAL);
		line (cx-5, cy, cx+5, cy, 0);
		line (cx, cy-5, cx, cy+5, 0);

		if (mfg_mode)
		{
			sprintf (text, "Insert %dg Pin", MINIMAL_WEIGHT);
			int cx2 = hsize/2;
			int cy2 = vsize/2;
			drawtext (cx2-8*strlen(text)/2, cy2-80, text);
		}
		else
			drawtext (label_x[state], label_y[state], "Touch");
		// Clear stats
		hit_x[state].Init();
		hit_y[state].Init();
		hit_t[state].Init(MM_CORNER_HISTORY);

		down_to_go = DOWN_COUNT;
		up_to_go = UP_COUNT;

		if (flog)
			fprintf (flog, "target state=%d point=%d,%d\n",
				 state, cx, cy);
		int in_loop = 1;
		int too_noisy = 0;
		while (in_loop) {
			if (poll (polldat, 2, 0) > 0)
			{
				if (polldat[kb_i].revents) // Keyboard
				{
					int rd = read(kb, ev, sizeof(struct input_event));
					if (rd == sizeof (struct input_event))
					{
						if (ev[0].type == EV_KEY && ev[0].value ==1)
						{
							if (ev[0].code == KEY_B || ev[0].code == KEY_ESC)
							{
								// in_loop = 0;
								return -1;
							}
						}
					}
				}
				if (polldat[tsr_i].revents) // TS raw
				{
					// Must be TS input...
					int rd = read(tsr, ev, sizeof(struct input_event) * 64);
					if (rd < (int) sizeof(struct input_event)) {
						perror("error reading");
						break;
					}
					for (i = 0; 
						 in_loop && (i < rd / sizeof(struct input_event)); i++)
					{
						switch (ev[i].type)
						{
						case EV_SYN:
							if (flog)
							{
								for (j=0; j<N_RAW; j++)
									fprintf (flog, "%s=%4d ", LABEL[j], raw[j]);
								if (pen_is_really_down ()) fprintf (flog, "* %.0f", 
												    hit_x[state].N());
								fprintf (flog, "\n");
							}
								// if we've already seen the pen down or if
								// it's now down, increment tries
							if ((tries > 0) || pen_is_down()) 
							{
								if (++tries >= ENOUGH_TRIES)
								{
									// timeout; fail the touch screen
									printf("Timeout: pin not really down\n");
									fprintf(flog,
											"Timeout: pin not really down\n");
									cls ();
									sprintf (text, "FAILED !!");
									int cx2 = hsize/2;
									int cy2 = vsize/2;
									drawtext (cx2-8*strlen(text)/2, cy2-100, text);
									return 1;
								}
							}
							if (too_noisy)
							{
								if (pen_is_up())
								{
									in_loop = 0;
									too_noisy =0;
									break;
								}
								break;
							}
							if (down_to_go && pen_is_really_down())
							{
								// Collecting a down point, but not enough yet
								--down_to_go;
								if (! down_to_go)
								{
									// Got enough, ask for release
									int cx = hsize/2;
									int cy = vsize/2;
									cls ();
									if (mfg_mode)
									{
										sprintf (text, "Remove Pin");
										drawtext (cx-8*strlen(text)/2, cy-80, text);
									}
									else
									{
										sprintf (text, "Release");
										drawtext (cx-8*strlen(text)/2, cy, text);
									}
									if (flog)
										fprintf (flog, "%s\n", text);
									prev_x = ((raw[IX_X]+raw[IX_X2])/2);
									prev_y = ((raw[IX_Y]+raw[IX_Y2])/2);
								}
							}
							if (!down_to_go && up_to_go)
							{
								if (pen_is_up())
									--up_to_go;
							}
							if (!down_to_go && !up_to_go)
							{
								// Done
								if (in_loop) {
									state++;
									in_loop = 0;
								}
							}
							if (pen_is_really_down() && down_to_go &&
							    down_to_go+3 < DOWN_COUNT) // throw out first 3 points)
							{
								// Down point
								hit_t[state].Add((raw[IX_T1]+raw[IX_T2])/2);
								hit_x[state].Add((raw[IX_X]+raw[IX_X2])/2);
								hit_y[state].Add((raw[IX_Y]+raw[IX_Y2])/2);
								// Check for previous point
								if (abs(prev_x - ((raw[IX_X]+raw[IX_X2])/2)) < 10 &&
								    abs(prev_y - ((raw[IX_Y]+raw[IX_Y2])/2)) < 10)
								{
									// too close, ignore this
									too_noisy = 1;
								}


								// Sanity
								if (hit_x[state].Stdev() > target_noise_x ||
								    hit_y[state].Stdev() > target_noise_y)
								{
									int cx = (hsize/2 + label_x[state])/2;
									int cy = (vsize/2 + label_y[state])/2;
									cls ();
									printf ("Too Noisy!  %f > %f or %f > %f.  Try again.\n",
										hit_x[state].Stdev(), target_noise_x,
										hit_y[state].Stdev(), target_noise_y);
									sprintf (text, "Too noisy!  Try again.");
									drawtext (cx-8*strlen(text)/2, cy, text);
									too_noisy = 1;
									if (flog)
										fprintf (flog, 
											 "Too Noisy!  %f > %f or %f > %f.  Try again.\n",
											hit_x[state].Stdev(), target_noise_x,
											hit_y[state].Stdev(), target_noise_y);

								}
							}
								// Flash a circle until pen is inserted
							if ((tries == 0) && !pen_is_down())
							{
								never_seen_counter++;
								// If we've already blinked long enough, give up
								if (never_seen_counter > ENOUGH_TRIES*2)
								{
									// timeout; fail the touch screen
									printf("Timeout: pin not inserted\n");
									fprintf(flog,
											"Timeout: pin not inserted\n");
									cls ();
									sprintf (text, "FAILED !!");
									int cx2 = hsize/2;
									int cy2 = vsize/2;
									drawtext (cx2-8*strlen(text)/2, cy2-100, text);
									return 2;
								}
								// Try blinking to attract attention
								if (never_seen_counter > ENOUGH_TRIES)
								{
									// Only update once in a while, so we don't slow down UI
									if (((never_seen_counter-ENOUGH_TRIES) % 25)==0)
									{
										if (((never_seen_counter-ENOUGH_TRIES) / 25) & 1)
										{
											cls ();
										}
										else
										{
											if (mfg_mode)
												fullrounddot (cx, cy, RAD*RAD, CMINIMAL);
											line (cx-5, cy, cx+5, cy, 0);
											line (cx, cy-5, cx, cy+5, 0);
										}
										if (mfg_mode)
										{
											sprintf (text, "Insert %dg Pin", MINIMAL_WEIGHT);
											int cx2 = hsize/2;
											int cy2 = vsize/2;
											drawtext (cx2-8*strlen(text)/2, cy2-100, text);
										}
										else
											drawtext (label_x[state], label_y[state],
													 "Touch");
									}
								}
							}
							break;
						case EV_ABS:
							j = ev[i].code-RAW_0;
							if (j>=0 && j<N_RAW)
							{
								raw[j] = ev[i].value;
							}
							break;
						}
					}
				}
			}
		}
	}
	cls ();

	// Pointercal
	int screen_x[3], screen_y[3];
	int adc_x[3], adc_y[3];
	screen_x[PT00] = target_x[T00]; screen_y[PT00] = target_y[T00];
	screen_x[PT10] = target_x[T10]; screen_y[PT10] = target_y[T10];
	screen_x[PT01] = target_x[T01]; screen_y[PT01] = target_y[T01];
	adc_x[PT00] = (int)hit_x[T00].Mean(); adc_y[PT00] = (int)hit_y[T00].Mean();
	adc_x[PT10] = (int)hit_x[T10].Mean(); adc_y[PT10] = (int)hit_y[T10].Mean();
	adc_x[PT01] = (int)hit_x[T01].Mean(); adc_y[PT01] = (int)hit_y[T01].Mean();
	compute_affine (screen_x, screen_y, adc_x, adc_y, setting_pointercal);

	if (flog)
	{
		fprintf (flog, "Target Noise  x=%5.2f y=%5.2f\n", 
						target_noise_x, target_noise_y);
		for (i=0; i<N_POINTS; i++)
		{
			fprintf (flog, "X[%d]: Target=%3d  %4.0f/%4.0f/%4.0f Stdev=%5.2f N=%3.0f  ", 
				 i, target_x[i],
				 hit_x[i].Min(), hit_x[i].Mean(), hit_x[i].Max(), 
				 hit_x[i].Stdev(), hit_x[i].N());
			fprintf (flog, "Y[%d]: Target=%3d  %4.0f/%4.0f/%4.0f Stdev=%5.2f N=%3.0f\n", 
				 i, target_y[i],
				 hit_y[i].Min(), hit_y[i].Mean(), hit_y[i].Max(), 
				 hit_y[i].Stdev(), hit_y[i].N());
		}
	}

	// TNT plane
	int xx[4], yy[4], zz[4], a, b, c, tnt_offset;
	xx[0] = (int)hit_x[T00].Mean(); yy[0] = (int)hit_y[T00].Mean();
									zz[0] = hit_t[T00].Max(MM_CORNER_HISTORY-1);
	xx[1] = (int)hit_x[T10].Mean(); yy[1] = (int)hit_y[T10].Mean();
									zz[1] = hit_t[T10].Max(MM_CORNER_HISTORY-1);
	xx[2] = (int)hit_x[T11].Mean(); yy[2] = (int)hit_y[T11].Mean();
									zz[2] = hit_t[T11].Max(MM_CORNER_HISTORY-1);
	xx[3] = (int)hit_x[T01].Mean(); yy[3] = (int)hit_y[T01].Mean();
									zz[3] = hit_t[T01].Max(MM_CORNER_HISTORY-1);
	for (i=0; i<N_POINTS; i++)
	{
		hit_t[i].Dump(stdout);
		if (flog)
			hit_t[i].Dump(flog);
	}

	fit_plane (4, xx, yy, zz, 
		   &setting_tnt_plane[0], 
		   &setting_tnt_plane[1], 
		   &setting_tnt_plane[2],
		   &tnt_offset);

	// Do heuristic checks on TNT, if desired
	if (heuristic_checks && fail_heuristic_checks (-1, zz[T00], zz[T10], zz[T11], zz[T01]))
	{
		printf("Failed heuristic checks: %s\n", hc_reason);
		if (flog)
			fprintf(flog, "Failed heuristic checks: %s\n", hc_reason);
		cls ();
		sprintf (text, "FAILED !!");
		int cx2 = hsize/2;
		int cy2 = vsize/2;
		drawtext (cx2-8*strlen(text)/2, cy2-100, text);
		return 3;
	}

	// Pick a threshold for TNT in the center.
	double tnt_padding = 3*actual_noise_t; // 3 sigmas good enough?
		// Fudge it up if there is too little noise
	if (tnt_padding < MIN_TNT_PADDING) 
		tnt_padding = MIN_TNT_PADDING;
		// Save this for comparison with tnt of 40g during pressure cal
	avg_of_corner_tnts = tnt_offset;

	setting_min_tnt_up = (1024 + tnt_offset)/2;
	setting_max_tnt_down = tnt_offset + (int)tnt_padding;
	v2_max_tnt_down = (int)(1.5*setting_max_tnt_down);
	printf ("Setting for min_tnt_up:   %d\n", setting_min_tnt_up);
	printf ("Setting for max_tnt_down: %d (V2:%d)\n", 
			setting_max_tnt_down, v2_max_tnt_down);
	if (flog)
	{
		fprintf (flog, "Setting for min_tnt_up:   %d\n", setting_min_tnt_up);
		fprintf (flog, "Setting for max_tnt_down: %d (V2:%d)\n",
				 setting_max_tnt_down, v2_max_tnt_down);
	}
	return 0;
}

int write_mfgdata_tspr (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_mfgdata_tspr: Can't open for output");
		return 1;
	}
	// See libMfgData.h for order of this file
	fprintf (f, "%d ", TSP_VERSION); // version
	fprintf (f, "%d ", setting_max_tnt_down);
	fprintf (f, "%d ", setting_min_tnt_up);
	fprintf (f, "%d ", setting_max_delta_tnt);
	fprintf (f, "%d ", setting_delay_in_us);
	fprintf (f, "%d ", setting_y_delay_in_us);
	fprintf (f, "%d ", setting_tnt_delay_in_us);
	int i;
	for (i=0; i<9; i++)
		fprintf (f, "%d ", setting_pressure[i]);
	fprintf (f, "%d ", setting_tnt_mode);
	fprintf (f, "%d ", setting_averaging);
	fprintf (f, "%d %d %d\n", setting_tnt_plane[0], setting_tnt_plane[1], setting_tnt_plane[2]);
	fchmod (fileno(f), 0666);
	fclose (f);
	return 0;
}

int write_settssh (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_settssh: Can't open for output");
		return 1;
	}
	fprintf (f, "#!/bin/sh\n");
	fprintf (f, "SYS=/sys/devices/platform/lf1000-touchscreen\n");
	fprintf (f, "# TSP Version=%d\n", TSP_VERSION);
	fprintf (f, "echo %d > $SYS/max_tnt_down\n", setting_max_tnt_down);
	fprintf (f, "#2 echo %d > $SYS/max_tnt_down\n", v2_max_tnt_down);
	fprintf (f, "#3_7 echo %d > $SYS/max_tnt_down\n", v3_7_max_tnt_down);
	fprintf (f, "echo %d > $SYS/min_tnt_up\n", setting_min_tnt_up);
	fprintf (f, "echo %d > $SYS/max_delta_tnt\n", setting_max_delta_tnt);
	fprintf (f, "#3_8 echo %d > $SYS/max_delta_tnt\n", v3_8_max_delta_tnt);
	fprintf (f, "echo %d > $SYS/delay_in_us\n", setting_delay_in_us);
	fprintf (f, "echo %d > $SYS/y_delay_in_us\n", setting_y_delay_in_us);
	fprintf (f, "echo %d > $SYS/tnt_delay_in_us\n", setting_tnt_delay_in_us);
	// fprintf (f, "echo %d %d %d %d %d %d %d > $SYS/pointercal\n",
	// 	setting_pointercal[0], setting_pointercal[1], setting_pointercal[2], 
	// 	setting_pointercal[3], setting_pointercal[4], setting_pointercal[5], 
	// 	setting_pointercal[6]);
	fprintf (f, "echo %d %d %d %d %d %d %d %d %d > $SYS/pressure_curve\n",
		 setting_pressure[0], setting_pressure[1], setting_pressure[2], 
		 setting_pressure[3], setting_pressure[4], setting_pressure[5], 
		 setting_pressure[6], setting_pressure[7], setting_pressure[8]);
	fprintf (f, "echo %d > $SYS/tnt_mode\n", setting_tnt_mode);
	fprintf (f, "echo %d > $SYS/averaging\n", setting_averaging);
	fprintf (f, "echo %d %d %d > $SYS/tnt_plane\n", 
		 setting_tnt_plane[0], setting_tnt_plane[1], setting_tnt_plane[2]);
	fprintf (f, "#2 echo %d %d %d > $SYS/tnt_plane\n", 0, 0, 0);
	fchmod (fileno(f), 0777);
	fclose (f);
	return 0;
}

int write_pointercal (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_pointercal: Can't open for output");
		return 1;
	}
	fprintf (f, "%d %d %d %d %d %d %d\n",
		setting_pointercal[0], setting_pointercal[1], setting_pointercal[2], 
		setting_pointercal[3], setting_pointercal[4], setting_pointercal[5], 
		setting_pointercal[6]);
	fchmod (fileno(f), 0777);
	fclose (f);
	return 0;
}

int write_pressure (const char *filename)
{
	FILE *f = fopen (filename, "w");
	if (!f)
	{
		perror ("write_pressure: Can't open for output");
		return 1;
	}
	fprintf (f, "%d %d %d %d %d %d %d %d %d\n",
		 setting_pressure[0], setting_pressure[1], setting_pressure[2], 
		 setting_pressure[3], setting_pressure[4], setting_pressure[5], 
		 setting_pressure[6], setting_pressure[7], setting_pressure[8]);
	fchmod (fileno(f), 0777);
	fclose (f);
	return 0;
}

int write_mfgdata_to_NOR ()
{
	// Write out pointercal
	int r1 = mfgData.SetTSCalData (setting_pointercal);
	if (r1)
	{
		printf ("Failure of setTSCalData() returning %d\n", r1);
		return r1;
	}
	// Write out pressure curve
	int i;
	struct ts_pressure_data tsp;
	tsp.version = TSP_VERSION;
	tsp.u.data3.max_tnt_down = setting_max_tnt_down;
	tsp.u.data3.min_tnt_up = setting_min_tnt_up;
	tsp.u.data3.max_delta_tnt = setting_max_delta_tnt;
	tsp.u.data3.delay_in_us = setting_delay_in_us;
	tsp.u.data3.y_delay_in_us = setting_y_delay_in_us;
	tsp.u.data3.tnt_delay_in_us = setting_tnt_delay_in_us;
	for (i=0; i<N_PCURVE_PTS; i++)
		tsp.u.data3.pressure_curve[i] = setting_pressure[i];
	tsp.u.data3.tnt_mode = setting_tnt_mode;
	tsp.u.data3.averaging = setting_averaging;
	for (i=0; i<3; i++)
		tsp.u.data3.tnt_plane[i] = setting_tnt_plane[i];
	for (i=0; i<3; i++)
		tsp.u.data3.reserved[i] = 0;
	int r2 = mfgData.SetTSPressure (&tsp);
	if (r2)
	{
		printf ("Failure of setTSPressure() returning %d\n", r2);
		return r2;
	}

	if (mfgData.SetMfgMode (true))
	{
		printf ("Failure setting MfgMode on\n");
		return 1;
	}
	else
	{
		if (mfgData.Update ())
		{
			printf ("Failure updating mfgData\n");
			return 1;
		}
		else
		{
			if (mfgData.SetMfgMode (false))
			{
				printf ("Failure setting MfgMode back off\n");
				return 1;
			}
		}
	}
	return 0;
}

static void write_summary_to_log(FILE * flog)
{
	if (flog)
	{
		fprintf (flog, "Summary:\n");
		fprintf (flog, "Setting for max_tnt_down:    %4d\n", 
						setting_max_tnt_down);
		fprintf (flog, "3.7 setting for max_tnt_down:   %4d\n",
				 		v3_7_max_tnt_down);
		fprintf (flog, "Setting for min_tnt_up:      %4d\n", setting_min_tnt_up);
		fprintf (flog, "Setting for max_delta_tnt:   %4d\n",  
						setting_max_delta_tnt);
		fprintf (flog, "3.8 setting for max_delta_tnt:   %4d\n",
				 		v3_8_max_delta_tnt);
		fprintf (flog, "Setting for delay_in_us:     %4d\n", 
						setting_delay_in_us);
		fprintf (flog, "Setting for y_delay_in_us:   %4d\n", 
						setting_y_delay_in_us);
		fprintf (flog, "Setting for tnt_delay_in_us: %4d\n", 
						setting_tnt_delay_in_us);
		fprintf (flog, "Setting for pressure_curve:  %d %d %d %d %d %d %d %d %d\n",
			 setting_pressure[0], setting_pressure[1], setting_pressure[2], 
			 setting_pressure[3], setting_pressure[4], setting_pressure[5], 
			 setting_pressure[6], setting_pressure[7], setting_pressure[8]);
		fprintf (flog, "Setting for tnt_mode:        %4d\n", setting_tnt_mode);
		fprintf (flog, "Setting for averaging:       %4d\n", setting_averaging);
		fprintf (flog, "Setting for pointercal:      %d %d %d %d %d %d %d\n",
			setting_pointercal[0], setting_pointercal[1], setting_pointercal[2],
			setting_pointercal[3], setting_pointercal[4], setting_pointercal[5],
			setting_pointercal[6]);
		fprintf (flog, "Setting for tnt_plane:       %d %d %d\n", 
			 setting_tnt_plane[0], setting_tnt_plane[1], setting_tnt_plane[2]);
	}
}

void help (char **argv)
{
	printf ("Usage:\n   %s [-l] [-p] [-d] [-m] [-h]\n", argv[0]);
	printf ("-l: Loosen tolerance\n");
	printf ("-p: Just measure pressure\n");
	printf ("-d: Just measure delays and noise assuming mechanical finger\n");
	printf ("-z: Disable median mode in driver\n");
	printf ("-m: Manufacturing Mode\n");
	printf ("-w: On successful calib, write data to NOR\n");
	printf ("-x: eXperimental: use default delays\n");
	printf ("-gw: Use GoWorld parameters\n");
	printf ("-S: Don't collect pressure data; store standard pressure curve\n");
	printf ("-s: Collect pressure data, but use standard pressure curve\n");
	printf ("-5: Fit TNT plane to 5 points\n");
	printf ("-c: Perform some heuristic checks on data to reject bad screens\n");
	printf ("-M#: Use mass #g for TNT plane fitting; # in 20,25,30,50,75,100,150,200\n");
	printf ("-h: help\n");
}

#define RETURN(x)	do { report_to_mfg (!x); return x; } while (0)

struct cmdline_args {
	int just_pressure;
	int just_delays;
	int mfg_mode;
	int write_nor;
	int loose;
	int averaging;
	int experimental;
	int goworld;
	int skip_pressure_data_step;
	int use_default_pressure_curve;
	int use_five_points;
	int heuristic_checks;
};

static int get_delays( struct cmdline_args * pArgs ) 
{
	int r = 1;

	if (pArgs->experimental)
	{
		default_delays(pArgs->just_delays);
		set_sysfs ("scanning", 0);
	}
	else {
		while (r)
		{
			set_sysfs ("scanning", 1);
			r = calib_delay (pArgs->just_delays, pArgs->mfg_mode);
			set_sysfs ("scanning", 0);
			if (r<0)
			{
				set_raw_events (0);
				cls ();
			}
		}
	}
	if (r >= 0) {	// Update the delays
		set_sysfs ("max_tnt_down",    setting_max_tnt_down);
		set_sysfs ("min_tnt_up",      setting_min_tnt_up);
		set_sysfs ("delay_in_us",     setting_delay_in_us);
		set_sysfs ("y_delay_in_us",   setting_y_delay_in_us);
		set_sysfs ("tnt_delay_in_us", setting_tnt_delay_in_us);
		set_sysfs ("tnt_mode",        setting_tnt_mode);
	}
	return r;
}

/* returns nonzero if command line syntax ought to be displayed
 * otherwise returns 0
 */
static int parse_args( int argc, char **argv, struct cmdline_args * pArgs)
{
	int i;
	int numBadArgs = 0;

	for (i=1; i<argc; i++)
	{
		if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
		{
			return 1;
		}
		if (!strcmp ("-l", argv[i]))
		{
			printf ("Setting loose=1\n");
			pArgs->loose = 1;
		}
		else if (!strcmp ("-p", argv[i]))
		{
			printf ("Just pressure\n");
			pArgs->just_pressure = 1;
		}
		else if (!strcmp ("-d", argv[i]))
		{
			printf ("Just delays (mechanical finger)\n");
			pArgs->just_delays = 1;
		}
		else if (!strcmp ("-z", argv[i]))
		{
			printf ("Disable median\n");
			pArgs->averaging = 1;
		}
		else if (!strcmp ("-m", argv[i]))
		{
			printf ("Manufacturing Mode\n");
			pArgs->mfg_mode = 1;
		}
		else if (!strcmp ("-w", argv[i]))
		{
			printf ("On success, write to NOR\n");
			pArgs->write_nor = 1;
		}
		else if (!strcmp ("-x", argv[i]))
		{
			printf ("Experimental: delays == 5, get_five_points()\n");
			pArgs->experimental = 1;
		}
		else if (!strcmp ("-gw", argv[i]))
		{
			printf ("GoWorld thresholds for TnT and Pressure\n");
			pArgs->goworld = 1;
			pressure_upper_limit = GOWORLD_PRESSURE_UPPER_LIMIT;
			pen_down_threshold   = GOWORLD_PEN_DOWN_THRESHOLD;
		}
		else if (!strcmp ("-S", argv[i]))
		{
			printf ("Skip pressure data step; use default pressure curve\n");
			pArgs->skip_pressure_data_step    = 1;
			pArgs->use_default_pressure_curve = 1;
		}

		else if (!strcmp ("-s", argv[i]))
		{
			printf ("Use standard (default) pressure curve\n");
			pArgs->use_default_pressure_curve = 1;
		}
		else if (!strcmp ("-5", argv[i]))
		{
			printf ("Use 5 points for tnt plane fitting\n");
			pArgs->use_five_points = 1;
		}
		else if (!strcmp ("-c", argv[i]))
		{
			printf ("Perform some heuristic checks on data to reject bad screens\n");
			pArgs->heuristic_checks = 1;
		}
		else if (!strncmp ("-M", argv[i], 2))
		{
			int mass;
			mass = atoi (argv[i]+2);
			long color = color_for_weight (mass);
			if (color == BLK)
			{
				printf ("Unknown mass %dg for -M.  Use 20, 25, 30...200\n");
				exit (1);
			}
			printf ("Use mass %dg for TNT plane fitting\n", mass);
			MINIMAL_WEIGHT=mass;
			CMINIMAL=color;
		}
		else
		{
			printf ("Unknown argument: %s\n", argv[i]);
			++numBadArgs;	// alternatively we could just ignore unknown args
		}
	}
	return numBadArgs;
}

enum {
	RETURN_OK = 0,
	RETURN_INIT_FB_ERROR,
	RETURN_INIT_INPUT_ERROR,
	RETURN_INIT_DRAWTEXT_ERROR,
	RETURN_OPEN_LOG_ERROR,
	RETURN_GET_DELAYS_ERROR,
	RETURN_GET_POINTS_ERROR,
	RETURN_CALIB_PRESSURE_ERROR,
	RETURN_WRITE_TO_NOR_ERROR,
	RETURN_BOARD_ID_ERROR
};
int main (int argc, char **argv)
{
	// Only Averaging is set, and it's set to -1 to indicate "use median3"
	struct cmdline_args cmdlineArgs = {0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0};

	printf ("%s: New touch screen calibration program v%d.%d\n", 
		argv[0], TSP_VERSION, TSP_SUBVERSION);
	if (0 == parse_args(argc, argv, &cmdlineArgs)) {
		setting_averaging = cmdlineArgs.averaging;
		loose			  = cmdlineArgs.loose;
	}
	else {
		help (argv);
		RETURN (RETURN_OK);
	}
	if (init_fb ())
		RETURN (RETURN_INIT_FB_ERROR);
	if (init_input ())
		RETURN (RETURN_INIT_INPUT_ERROR);
	if (init_drawtext ())
		RETURN (RETURN_INIT_DRAWTEXT_ERROR);

	if (get_board_model (&board))
		RETURN (RETURN_BOARD_ID_ERROR);

	if (cmdlineArgs.just_delays)
		flog = stdout;
	else
		flog = fopen ("/var/log/new-calib.log", "w");
	if (!flog)
	{
		perror ("Can't open /var/log/new-calib.log");
		RETURN (RETURN_OPEN_LOG_ERROR);
	}
	fprintf (flog, "%s: New touch screen calibration program v%d.%d\n", 
		argv[0], TSP_VERSION, TSP_SUBVERSION);
	
	// Tack on serial number of device
	char sn[128];
	mfgData.Init ();
	if (mfgData.GetSerialNumber (sn))
		sn[0] = 0;
	fprintf (flog, "Serial Number='%s'\n", sn);

	int not_done = 1;
	while (not_done)
	{
		set_sysfs ("averaging", setting_averaging);
#if 1	// 9aug11
		if (cmdlineArgs.goworld) {
			set_sysfs ("min_tnt_up", GOWORLD_MIN_TNT_UP); 
		} else {
			set_sysfs ("min_tnt_up", 1000);
		}
			// Tell ts driver not to enable pull-ups on x and y
			// If this is running on a unit with an obsolete kernel,
			// set_sysfs() will fail and will exit.
		set_sysfs ("tails", 0); 
#else
		set_sysfs ("min_tnt_up", 500); // 1000 won't work for GW.  Needs to be lower
#endif
		set_sysfs ("sample_rate_in_hz", 100);
			/* NOTE: tests indicated no difference between calibrations
			 * 		 when sample rate was 100 and calibrations when 
			 * sample rate was 50.  Here we set sample rate to 100
			 * because that rate will be used in drawing applications
			 * and because it might allow calibrations to be completed
			 * slightly faster.
			 */
		set_raw_events (1);
		int r;
		if (cmdlineArgs.just_pressure)
			goto skip_to_pressure;

		r = get_delays( &cmdlineArgs );
		if (r < 0) {
			RETURN (RETURN_GET_DELAYS_ERROR);
					// Aborting here leaves calibration
		}
		if (cmdlineArgs.just_delays)
			RETURN (RETURN_OK);

		// Get control points
		if (cmdlineArgs.use_five_points) {
			r = get_five_points (cmdlineArgs.mfg_mode,
								 cmdlineArgs.heuristic_checks);
		}
		else {
			r = get_four_points (cmdlineArgs.mfg_mode,
								 cmdlineArgs.heuristic_checks);
		}

		if (r)
			RETURN (RETURN_GET_POINTS_ERROR);
				// Aborted or failed on get_X_points

#if 1	// 9aug11
		if (cmdlineArgs.goworld) {
			setting_min_tnt_up	  = GOWORLD_MIN_TNT_UP;
			setting_max_tnt_down  = GOWORLD_MAX_TNT_DOWN;
			setting_max_delta_tnt = GOWORLD_MAX_DELTA_TNT;

			printf ("GoWorld setting for min_tnt_up:   %d\n", 
					setting_min_tnt_up);
			printf ("GoWorld setting for max_tnt_down: %d\n", 
					setting_max_tnt_down);
			printf ("GoWorld setting for max_delta_tnt: %d\n", 
					setting_max_delta_tnt);
			if (flog)
			{
				fprintf (flog, "GoWorld setting for min_tnt_up:   %d\n",
						 setting_min_tnt_up);
				fprintf (flog, "GoWorld setting for max_tnt_down: %d\n",
						 setting_max_tnt_down);
				fprintf (flog, "GoWorld setting for max_delta_tnt: %d\n", 
						setting_max_delta_tnt);
			}
		}
#endif	// 9aug11
		
	skip_to_pressure:
		r = cmdlineArgs.mfg_mode
				  ? calib_pressure_mfg (cmdlineArgs.use_default_pressure_curve,
										cmdlineArgs.skip_pressure_data_step) 
				  : calib_pressure ();

		if (cmdlineArgs.just_pressure && r)
		{
			set_raw_events (0);
			cls ();
			RETURN (RETURN_OK);	// Done
		}
		if (r)
			RETURN (RETURN_CALIB_PRESSURE_ERROR);
					// Aborted or failed on pressure curve

		write_summary_to_log(flog);

			// Test drive with new params
		if (cmdlineArgs.just_pressure)
		{
			if (0 != write_pressure ("/sys/devices/platform/"
									 "lf1000-touchscreen/pressure_curve")) 
			{
				printf("Could not write pressure to sysfs\n");
				fprintf(flog, "Could not write pressure to sysfs\n");
			}
		}
		else
		{
			if (0 != (r = write_settssh ("/tmp/set-ts.sh"))) {
				printf("Could not write settings to /tmp/set-ts.sh\n");
				fprintf(flog, "Could not write settings to /tmp/set-ts.sh\n");
			}
			if (0 != write_pointercal("/sys/devices/platform/"
									  "lf1000-touchscreen/pointercal")) {
				printf("Could not write affine xform to pointercal in sysfs\n");
				fprintf(flog, "Could not write affine xform "
							  "to pointercal in sysfs\n");
			}
			if (0 == r) {					// if we wrote the script ok,
				system ("/tmp/set-ts.sh");	// run it
			}
		}
#ifdef LOG_TESTDRIVE
		set_raw_events (1);
#else
		set_raw_events (0);
#endif
		if (cmdlineArgs.mfg_mode || !test_drive ())
		{
			// Likes it
			not_done = 0;
		}
#ifdef LOG_TESTDRIVE
		set_raw_events (0);
#endif
	}
	if (cmdlineArgs.just_pressure) {
		printf ("echo %d %d %d %d %d %d %d %d %d > $SYS/pressure_curve\n",
			setting_pressure[0], setting_pressure[1], setting_pressure[2], 
			setting_pressure[3], setting_pressure[4], setting_pressure[5], 
			setting_pressure[6], setting_pressure[7], setting_pressure[8]);
	}
	else
	{
		if (0 != write_settssh ("/flags/set-ts.sh")) {
			printf("Could not write settings to /flags/set-ts.sh\n");
			fprintf(flog, "Could not write settings to /flags/set-ts.sh\n");
		}
		if (0 != write_pointercal ("/flags/pointercal")) {
			printf("Could not write affine xform to /flags/pointercal\n");
			fprintf(flog,"Could not write affine xform to /flags/pointercal\n");
		}
		if (0 != write_mfgdata_tspr ("/tmp/mfgdata-tspr-data"))
		{
			printf("Could not write to /tmp/mfgdata-tspr-data\n");
			fprintf(flog,"Could not write to /tmp/mfgdata-tspr-data\n");
		}
		if (cmdlineArgs.write_nor)
		{
			if (write_mfgdata_to_NOR ())
			{
				mfgData.Exit ();
				RETURN (RETURN_WRITE_TO_NOR_ERROR);
			}
		}
	}
	mfgData.Exit ();
	RETURN (RETURN_OK);
}

/*
 UI:                          
  Phase 1: calib_delay       B: return -2 (abort)
  Phase 2: get_four_points   B: return -1 (abort)
  Phase 3: calib_pressure    A: return 0, accept;  B: return -2 (abort)
  Phase 4: test_drive        Home: cls;  A: Save return 0;  B: return 1 (abort)
*/
