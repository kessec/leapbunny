#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <jpeglib.h>
}

#define TAGS 20
struct timeval t0[TAGS];
unsigned char output[640*480*3];

#include "Stats.cpp"

void start_sw (int tag)
{
	gettimeofday (&t0[tag], NULL);
}

double stop_sw (int tag)
{
	struct timeval t1;
	gettimeofday (&t1, NULL);
	return (t1.tv_sec - t0[tag].tv_sec) +
		(t1.tv_usec - t0[tag].tv_usec) * 1e-6;
}

double unpack (const char *filename, int ratio, int method)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			err;
	struct stat sinfo;
	cinfo.err = jpeg_std_error(&err);
	unsigned char *out = output;

	FILE *f = fopen(filename, "rb");
	if (f==NULL)
	{
		perror (filename);
		exit (1);
	}
	fstat (fileno (f), &sinfo);
	jpeg_create_decompress (&cinfo);
	jpeg_stdio_src(&cinfo, f);

	(void) jpeg_read_header(&cinfo, TRUE);

	cinfo.out_color_space = JCS_YCbCr;

	// cinfo.out_color_space = JCS_RGB;

	/*
	printf ("Info: size=%d = %d x %d with %d components \n", 
		sinfo.st_size,
		cinfo.image_width, cinfo.image_height,
		cinfo.num_components);
	*/
	
	cinfo.scale_num = 1;
	cinfo.scale_denom = ratio;
	cinfo.dct_method = (J_DCT_METHOD) method;

start_sw (0);
	jpeg_start_decompress (&cinfo);
	int row_stride = cinfo.output_width * cinfo.output_components;
	// printf ("row_stride=%d\n", row_stride); // 3*640=1920
	JSAMPLE **buf = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		(void) jpeg_read_scanlines(&cinfo, buf, 1);
		memcpy (out, *buf, row_stride);
		out += row_stride;
	}
	/* Step 7: Finish decompression */
	(void) jpeg_finish_decompress(&cinfo);
double t0 = stop_sw (0);
	jpeg_destroy_decompress(&cinfo);
	fclose(f);
	// printf ("time=%f\n", t0);
	return t0;
}

int size;
double pack (char *filename, int w, int h, int ratio, int quality, int method)
{
	struct jpeg_compress_struct	cinfo;
	struct jpeg_error_mgr			err;
	struct stat sinfo;
	cinfo.err = jpeg_std_error(&err);
	unsigned char *in = output;

	char fn[512];
	sprintf (fn, "%s-%d-%03d.jpg", filename, ratio, quality);
	FILE *f = fopen(fn, "wb");
	if (f==NULL)
	{
		perror (filename);
		exit (1);
	}
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest(&cinfo, f);

	cinfo.image_width = w;
	cinfo.image_height = h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	cinfo.dct_method = (J_DCT_METHOD) method;
	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, false);

start_sw (0);
	jpeg_start_compress (&cinfo, TRUE);
	int row_stride = cinfo.image_width * cinfo.input_components;
	// printf ("row_stride=%d\n", row_stride); // 3*640=1920
	JSAMPROW row_pointer[1];
	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = in;//&output[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
		in += row_stride;
	}
	/* Step 7: Finish decompression */
	(void) jpeg_finish_compress(&cinfo);
double t0 = stop_sw (0);
	jpeg_destroy_compress(&cinfo);
	fclose(f);
	stat (fn, &sinfo);
	size = sinfo.st_size;
	// printf ("time=%f\n", t0);
	return t0;
}

int main (int argc, char **argv)
{

	int i, j;
#if 1
	// Decode speeds, different algorithms, different decimates
	for (j=0; j<3; j++)
	{
		CStats s1, s2, s4;
		for (i=1; i<argc; i++)
		{
			// printf ("name=%s\n", argv[i]);
			s1.Add(unpack (argv[i], 1, j));
			s2.Add(unpack (argv[i], 2, j));
			s4.Add(unpack (argv[i], 4, j));
		}
		printf ("j=%d\n", j);
		printf ("  S1: %.3f/%.3f/%.3f N=%.0f\n", 
			s1.Min(), s1.Mean(), s1.Max(), s1.N());
		printf ("  S2: %.3f/%.3f/%.3f N=%.0f\n", 
			s2.Min(), s2.Mean(), s2.Max(), s2.N());
		printf ("  S4: %.3f/%.3f/%.3f N=%.0f\n", 
			s4.Min(), s4.Mean(), s4.Max(), s4.N());
	}
#endif
#if 0
	CStats in;
	for (i=1; i<argc; i++)	
	{
		struct stat sinfo;
		stat (argv[i], &sinfo);
		in.Add(sinfo.st_size);
	}
	printf ("Input size: %.3f/%.3f/%.3f N=%ld\n", 
			in.Min(), in.Mean(), in.Max(), in.N());
#endif
#if 0

	// Decode, then reencode
	int q = 50;
	j = 0;
	{
		CStats u1, u2, u4;
		CStats p1, p2, p4;
		CStats s1, s2, s4;
		for (i=1; i<argc; i++)
		{
			// printf ("name=%s\n", argv[i]);
			u1.Add(unpack (argv[i], 1, 0));
			p1.Add(pack (argv[i], 640, 480, 1, q, j));
			s1.Add(size);
			u2.Add(unpack (argv[i], 2, 0));
			p2.Add(pack (argv[i], 640/2, 480/2, 2, q, j));
			s2.Add(size);
			u4.Add(unpack (argv[i], 4, 0));
			p4.Add(pack (argv[i], 640/4, 480/4, 4, q, j));
			s4.Add(size);
		}
		printf ("Q=%3d j=%d", q, j);
		// printf ("  u1: %.3f/%.3f/%.3f N=%ld\n",
		// 	u1.Min(), u1.Mean(), u1.Max(), u1.N());
		printf ("  P1: %.3f/%.3f/%.3f N=%ld ",
			p1.Min(), p1.Mean(), p1.Max(), p1.N());

		// printf ("  u2: %.3f/%.3f/%.3f N=%ld\n",
		//	u2.Min(), u2.Mean(), u2.Max(), u2.N());
		printf ("  P2: %.3f/%.3f/%.3f N=%ld ",
			p2.Min(), p2.Mean(), p2.Max(), p2.N());

		// printf ("  u4: %.3f/%.3f/%.3f N=%ld\n",
		//	u4.Min(), u4.Mean(), u4.Max(), u4.N());
		printf ("  P4: %.3f/%.3f/%.3f N=%ld\n",
			p4.Min(), p4.Mean(), p4.Max(), p4.N());

#if 0
		printf ("  S1: %6.0f/%6.0f/%6.0f N=%ld ",
			s1.Min(), s1.Mean(), s1.Max(), s1.N());
		printf ("  S2: %6.0f/%6.0f/%6.0f N=%ld ",
			s2.Min(), s2.Mean(), s2.Max(), s2.N());
		printf ("  S4: %6.0f/%6.0f/%6.0f N=%ld\n",
			s4.Min(), s4.Mean(), s4.Max(), s4.N());
#endif
	}
#endif
}
