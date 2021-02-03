//============================================================================
// Name        : FrameBufferTest.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <linux/lf1000/lf1000fb.h>

enum PatternType {
	VERTICAL_STRIPES,
	HORIZONTAL_STRIPES,
	YUV_DIAGONAL_STRIPES
};

void DrawPattern(const struct fb_fix_screeninfo &finfo,
		const struct fb_var_screeninfo &vinfo, char *fbp, enum PatternType pattern_type) {

	unsigned long int x = 0, y = 0;
	long int location = 0;

	char red[4], green[4], blue[4], alpha[4];

	for(int i = 0; i < 4; ++i) {
		red[i] = rand() % 256;
		green[i] = rand() % 256;
		blue[i] = rand() % 256;
		alpha[i] = rand() % 256;
	}

	// Figure out where in memory to put the pixel
	for (y = 0; y < vinfo.yres; y++) {
		for (x = 0; x < vinfo.xres; x++) {

			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
					   (y + vinfo.yoffset) * finfo.line_length;

			int r, g, b, a;
			switch(pattern_type) {
			case VERTICAL_STRIPES:
				if(x < vinfo.xres/4) {
					r = red[0];
					g = green[0];
					b = blue[0];
					a = alpha[0];
				} else if(x < vinfo.xres/2) {
					r = red[1];
					g = green[1];
					b = blue[1];
					a = alpha[1];
				} else if(x < vinfo.xres * 3 / 4) {
					r = red[2];
					g = green[2];
					b = blue[2];
					a = alpha[2];
				} else {
					r = red[3];
					g = green[3];
					b = blue[3];
					a = alpha[3];
				}
				break;
			case HORIZONTAL_STRIPES:
				if(y < vinfo.yres/4) {
					r = red[0];
					g = green[0];
					b = blue[0];
					a = alpha[0];
				} else if(y < vinfo.yres/2) {
					r = red[1];
					g = green[1];
					b = blue[1];
					a = alpha[1];
				} else if(y < vinfo.yres * 3 / 4) {
					r = red[2];
					g = green[2];
					b = blue[2];
					a = alpha[2];
				} else {
					r = red[3];
					g = green[3];
					b = blue[3];
					a = alpha[3];
				}
				break;

			case YUV_DIAGONAL_STRIPES:
				if(x + y < vinfo.xres/2) {
					r = red[0];
					g = green[0];
					b = blue[0];
					a = alpha[0];
				} else if(x + y < vinfo.xres) {
					r = red[1];
					g = green[1];
					b = blue[1];
					a = alpha[1];
				} else if(x + y < (vinfo.xres * 3) / 2) {
					r = red[2];
					g = green[2];
					b = blue[2];
					a = alpha[2];
				} else {
					r = red[3];
					g = green[3];
					b = blue[3];
					a = alpha[3];
				}
				break;
			}

			if(vinfo.nonstd) {
				int y2, u, v;
				y2 = 66 * r + 129 * g + 25 * b;
				u = -28 * r - 74 * g + 112 * b;
				v = 112 * r - 94 * g - 18 * b;

				y2 = (y2 + 128) >> 8;
				u = (u + 128) >> 8;
				v = (v + 128) >> 8;

				y2 += 16;
				u += 128;
				v += 128;

				location = (x + vinfo.xoffset) +
						   (y + vinfo.yoffset) * 4096;
				*(fbp + location) = y2;
				if(!(x & 0x1) && !(y & 0x1)) {
					location = (x/2 + vinfo.xoffset) + 2048 +
							   (y/2 + vinfo.yoffset) * 4096;
					*(fbp + location) = u;
					*(fbp + location + (vinfo.yres / 2) * 4096) = v;
				}
			} else {
				switch(vinfo.bits_per_pixel) {
				case 24:
					*(fbp + location + (char)(vinfo.blue.offset >> 3)) = b;
					*(fbp + location + (char)(vinfo.green.offset >> 3)) = g;
					*(fbp + location + (char)(vinfo.red.offset >> 3)) = r;
					break;

				case 32:
					*(fbp + location + (char)(vinfo.blue.offset >> 3)) = b;
					*(fbp + location + (char)(vinfo.green.offset >> 3)) = g;
					*(fbp + location + (char)(vinfo.red.offset >> 3)) = r;
					*(fbp + location + (char)(vinfo.transp.offset >> 3)) = a;
					break;

				default:
					r = r * 5 / 8;
					g = g * 6 / 8;
					b = b * 5 / 8;
					{
						unsigned short int t = r<<11 | g << 5 | b;
						*((unsigned short int*)(fbp + location)) = t;
					}
				}
			}
		}
	}
}

void DrawCheckerBoard(const struct fb_fix_screeninfo &finfo,
		const struct fb_var_screeninfo &vinfo, char *fbp, int square_size, unsigned int pattern_xsize, unsigned int pattern_ysize) {

	unsigned long int x = 0, y = 0;
	long int location = 0;

	char red[4], green[4], blue[4], alpha[4];

	for(int i = 0; i < 4; ++i) {
		red[i] = rand() % 256;
		green[i] = rand() % 256;
		blue[i] = rand() % 256;
		alpha[i] = rand() % 256;
	}

	int current_color = 0;
	// Figure out where in memory to put the pixel
	for (y = 0; y < pattern_ysize; y++) {
		if(y % square_size == 0) {
			if(y % (square_size * 2) == 0)
				current_color &= 0x1;
			else
				current_color |= 0x2;
		}

		for (x = 0; x < pattern_xsize; x++) {
			if(x % square_size == 0) {
				if(x % (square_size * 2) == 0)
					current_color &= 0x2;
				else
					current_color |= 0x1;
			}
			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
					   (y + vinfo.yoffset) * finfo.line_length;

			int r, g, b, a;
			r = red[current_color];
			g = green[current_color];
			b = blue[current_color];
			a = alpha[current_color];
			if(vinfo.nonstd) {
				int y2, u, v;
				y2 = 66 * r + 129 * g + 25 * b;
				u = -28 * r - 74 * g + 112 * b;
				v = 112 * r - 94 * g - 18 * b;

				y2 = (y2 + 128) >> 8;
				u = (u + 128) >> 8;
				v = (v + 128) >> 8;

				y2 += 16;
				u += 128;
				v += 128;

				location = (x + vinfo.xoffset) +
						   (y + vinfo.yoffset) * 4096;
				*(fbp + location) = y2;
				if(!(x & 0x1) && !(y & 0x1)) {
					location = (x/2 + vinfo.xoffset) + 2048 +
							   (y/2 + vinfo.yoffset) * 4096;
					*(fbp + location) = u;
					*(fbp + location + (vinfo.yres / 2) * 4096) = v;
				}
			} else {
				switch(vinfo.bits_per_pixel) {
				case 24:
					*(fbp + location + (char)(vinfo.blue.offset >> 3)) = b;
					*(fbp + location + (char)(vinfo.green.offset >> 3)) = g;
					*(fbp + location + (char)(vinfo.red.offset >> 3)) = r;
					break;

				case 32:
					*(fbp + location + (char)(vinfo.blue.offset >> 3)) = b;
					*(fbp + location + (char)(vinfo.green.offset >> 3)) = g;
					*(fbp + location + (char)(vinfo.red.offset >> 3)) = r;
					*(fbp + location + (char)(vinfo.transp.offset >> 3)) = a;
					break;

				default:
					r = r * 5 / 8;
					g = g * 6 / 8;
					b = b * 5 / 8;
					{
						unsigned short int t = r<<11 | g << 5 | b;
						*((unsigned short int*)(fbp + location)) = t;
					}
				}
			}
		}
	}
}

int main(int argc, char *arv[]) {
	int fbfd0 = 0;
	int fbfd1 = 0;
	int fbfd2 = 0;
	struct fb_var_screeninfo vinfo0;
	struct fb_var_screeninfo vinfo1;
	struct fb_var_screeninfo vinfo2;
	struct fb_fix_screeninfo finfo0;
	struct fb_fix_screeninfo finfo1;
	struct fb_fix_screeninfo finfo2;
	char *fbp0 = 0;
	char *fbp1 = 0;
	char *fbp2 = 0;
	int ret;

	srand( time(0) );

	// Open the file for reading and writing
	fbfd0 = open("/dev/fb0", O_RDWR);
	if (!fbfd0) {
		printf("Error: cannot open framebuffer 0 device.\n");
		exit(1);
	}
	printf("The framebuffer 0 device was opened successfully.\n");

	fbfd1 = open("/dev/fb1", O_RDWR);
	if (!fbfd1) {
		printf("Error: cannot open framebuffer 1 device.\n");
		exit(1);
	}
	printf("The framebuffer 1 device was opened successfully.\n");

	fbfd2 = open("/dev/fb2", O_RDWR);
	if (!fbfd2) {
		printf("Error: cannot open framebuffer 2 device.\n");
		exit(1);
	}
	printf("The framebuffer 2 device was opened successfully.\n");

	// Get fixed screen information
	if (ioctl(fbfd0, FBIOGET_FSCREENINFO, &finfo0)) {
		printf("Error reading fixed information 0.\n");
		exit(2);
	}
	if (ioctl(fbfd1, FBIOGET_FSCREENINFO, &finfo1)) {
		printf("Error reading fixed information 1.\n");
		exit(2);
	}
	if (ioctl(fbfd2, FBIOGET_FSCREENINFO, &finfo2)) {
		printf("Error reading fixed information 2.\n");
		exit(2);
	}

	// Get variable screen information
	if (ioctl(fbfd0, FBIOGET_VSCREENINFO, &vinfo0)) {
		printf("Error reading variable information 0.\n");
		exit(3);
	}
	if (ioctl(fbfd1, FBIOGET_VSCREENINFO, &vinfo1)) {
		printf("Error reading variable information 1.\n");
		exit(3);
	}

	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}

	printf("0 %dx%d, %dbpp, nonstd=%x, red=%d-%d, green=%d-%d, blue=%d-%d, alpha=%d-%d\n",
			vinfo0.xres, vinfo0.yres, vinfo0.bits_per_pixel, vinfo0.nonstd,
			vinfo0.red.length, vinfo0.red.offset, vinfo0.green.length, vinfo0.green.offset,
			vinfo0.blue.length, vinfo0.blue.offset, vinfo0.transp.length, vinfo0.transp.offset);
	printf("1 %dx%d, %dbpp, nonstd=%x, red=%d-%d, green=%d-%d, blue=%d-%d, alpha=%d-%d\n",
			vinfo1.xres, vinfo1.yres, vinfo1.bits_per_pixel, vinfo1.nonstd,
			vinfo1.red.length, vinfo1.red.offset, vinfo1.green.length, vinfo1.green.offset,
			vinfo1.blue.length, vinfo1.blue.offset, vinfo1.transp.length, vinfo1.transp.offset);
	printf("2 %dx%d, %dbpp, nonstd=%x, red=%d-%d, green=%d-%d, blue=%d-%d, alpha=%d-%d\n",
			vinfo2.xres, vinfo2.yres, vinfo2.bits_per_pixel, vinfo2.nonstd,
			vinfo2.red.length, vinfo2.red.offset, vinfo2.green.length, vinfo2.green.offset,
			vinfo2.blue.length, vinfo2.blue.offset, vinfo2.transp.length, vinfo2.transp.offset);

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("A ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("B ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("C ret=%d\n", ret);

	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 0);
	if(ret < 0)
		printf("C2 ret=%d\n", ret);

	// Map the device to memory
	fbp0 = (char *)mmap(0, finfo0.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd0, 0);
	if ((int)fbp0 == -1) {
		printf("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully. fbp0=%p\n", fbp0);

	DrawPattern(finfo0, vinfo0, fbp0, VERTICAL_STRIPES);

	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("D ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 0);
	if(ret < 0)
		printf("E ret=%d\n", ret);

	// Map the device to memory
	fbp1 = (char *)mmap(0, finfo1.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd1, 0);
	if ((int)fbp1 == -1) {
		printf("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully. fbp1=%p\n", fbp1);

	DrawPattern(finfo1, vinfo1, fbp1, HORIZONTAL_STRIPES);
	sleep(5);

	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("F ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 0);
	if(ret < 0)
		printf("G ret=%d\n", ret);

	// Map the device to memory
	fbp2 = (char *)mmap(0, finfo2.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd2, 0);
	if ((int)fbp2 == -1) {
		printf("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully. fbp2=%p\n", fbp2);
	memset(fbp2, 0, finfo2.smem_len);
	DrawPattern(finfo2, vinfo2, fbp2, YUV_DIAGONAL_STRIPES);
	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 0);
	if(ret < 0)
		printf("I ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("J ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("K ret=%d\n", ret);
	printf("Only layer 0\n");
	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("K ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 0);
	if(ret < 0)
		printf("L ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("M ret=%d\n", ret);
	printf("Only layer 1\n");
	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 0);
	if(ret < 0)
		printf("P ret=%d\n", ret);
	printf("Only layer 2\n");
	sleep(5);

	ret = ioctl(fbfd0, FBIOBLANK, 0);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 0);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 0);
	if(ret < 0)
		printf("P ret=%d\n", ret);

	struct lf1000fb_blend_cmd blend_cmd;
	blend_cmd.enable = true;
	blend_cmd.alpha = 8;
	ret = ioctl(fbfd0, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend A0");
	blend_cmd.enable = true;
	blend_cmd.alpha = 8;
	ret = ioctl(fbfd1, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend A1");
	blend_cmd.enable = true;
	blend_cmd.alpha = 8;
	ret = ioctl(fbfd2, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend A2");
	printf("Layer 0T & 1T & 2T\n");
	sleep(5);

	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}
	vinfo2.nonstd &= ~(3<<24);
	vinfo2.nonstd |= (1<<24); //middle
	if (ioctl(fbfd2, FBIOPUT_VSCREENINFO, &vinfo2)) {
		printf("Error writing variable information 2.\n");
		exit(3);
	}
	printf("Layer 0T & 2T & 1T\n");
	sleep(5);

	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}
	vinfo2.nonstd &= ~(3<<24);
	vinfo2.nonstd |= (0<<24); //top
	if (ioctl(fbfd2, FBIOPUT_VSCREENINFO, &vinfo2)) {
		printf("Error writing variable information 2.\n");
		exit(3);
	}
	printf("Layer 2T & 0T & 1T\n");
	sleep(5);

	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}
	vinfo2.nonstd &= ~(3<<24);
	vinfo2.nonstd |= (2<<24); // bottom
	if (ioctl(fbfd2, FBIOPUT_VSCREENINFO, &vinfo2)) {
		printf("Error writing variable information 2.\n");
		exit(3);
	}

	blend_cmd.enable = false;
	blend_cmd.alpha = 15;
	ret = ioctl(fbfd0, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend B0");
	blend_cmd.enable = false;
	blend_cmd.alpha = 15;
	ret = ioctl(fbfd1, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend B1");
	blend_cmd.enable = false;
	blend_cmd.alpha = 15;
	ret = ioctl(fbfd2, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend B2");

	ret = ioctl(fbfd0, FBIOBLANK, 0);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("P ret=%d\n", ret);

	DrawPattern(finfo0, vinfo0, fbp0, YUV_DIAGONAL_STRIPES);
	printf("Layer 0, double tall, top\n");
	sleep(5);

	vinfo0.yoffset = vinfo0.yres;
	if (ioctl(fbfd0, FBIOPAN_DISPLAY, &vinfo0)) {
		printf("Error panning 0.\n");
		exit(3);
	}
	DrawPattern(finfo0, vinfo0, fbp0, YUV_DIAGONAL_STRIPES);
	printf("Layer 0, double tall, bottom\n");
	sleep(5);

	vinfo0.yoffset = vinfo0.yres/2;
	if (ioctl(fbfd0, FBIOPAN_DISPLAY, &vinfo0)) {
		printf("Error panning 0.\n");
		exit(3);
	}
	printf("Layer 0, double tall, middle\n");
	sleep(5);

	vinfo0.yoffset = 0;
	if (ioctl(fbfd0, FBIOPAN_DISPLAY, &vinfo0)) {
		printf("Error panning 0.\n");
		exit(3);
	}

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 0);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("P ret=%d\n", ret);

	DrawPattern(finfo1, vinfo1, fbp1, YUV_DIAGONAL_STRIPES);
	printf("Layer 1, double tall, top\n");
	sleep(5);

	vinfo1.yoffset = vinfo1.yres;
	if (ioctl(fbfd1, FBIOPAN_DISPLAY, &vinfo1)) {
		printf("Error panning 1.\n");
			exit(3);
	}
	DrawPattern(finfo1, vinfo1, fbp1, YUV_DIAGONAL_STRIPES);
	printf("Layer 1, double tall, bottom\n");
	sleep(5);

	vinfo1.yoffset = vinfo1.yres/2;
	if (ioctl(fbfd1, FBIOPAN_DISPLAY, &vinfo1)) {
		printf("Error panning 1.\n");
			exit(3);
	}
	printf("Layer 1, double tall, middle\n");
	sleep(5);

	vinfo1.yoffset = 0;
	if (ioctl(fbfd1, FBIOPAN_DISPLAY, &vinfo1)) {
		printf("Error panning 1.\n");
		exit(3);
	}

	ret = ioctl(fbfd0, FBIOBLANK, 1);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 1);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 0);
	if(ret < 0)
		printf("P ret=%d\n", ret);

	DrawPattern(finfo2, vinfo2, fbp2, YUV_DIAGONAL_STRIPES);
	printf("Layer 2, double wide, left\n");
	sleep(5);

	vinfo2.xoffset = vinfo2.xres;
	if (ioctl(fbfd2, FBIOPAN_DISPLAY, &vinfo2)) {
		printf("Error panning 2.\n");
		exit(3);
	}
	DrawPattern(finfo2, vinfo2, fbp2, YUV_DIAGONAL_STRIPES);
	printf("Layer 2, double wide, right\n");
	sleep(5);

	/*vinfo2.xoffset = vinfo2.xres/2;
	if (ioctl(fbfd2, FBIOPAN_DISPLAY, &vinfo2)) {
		printf("Error panning 1.\n");
			exit(3);
	}
	printf("Layer 2, double wide, middle\n");
	sleep(5);*/

	vinfo2.xoffset = 0;
	if (ioctl(fbfd2, FBIOPAN_DISPLAY, &vinfo2)) {
		printf("Error panning 2.\n");
		exit(3);
	}

	DrawCheckerBoard(finfo2, vinfo2, fbp2, 20, 100, 100);
	printf("Layer 2, drawing checkerboard test\n");
	sleep(1);

	lf1000fb_vidscale_cmd vidscale_cmd;
	if (ioctl(fbfd2, LF1000FB_IOCGVIDSCALE, &vidscale_cmd)) {
		printf("Error getting vidscale 2.\n");
		exit(3);
	}
	vidscale_cmd.sizex = 100;
	vidscale_cmd.sizey = 100;
	vidscale_cmd.apply = 1;
	if (ioctl(fbfd2, LF1000FB_IOCSVIDSCALE, &vidscale_cmd)) {
		printf("Error setting vidscale 2.\n");
		exit(3);
	}
	printf("Layer 2, scale test\n");
	sleep(5);

	DrawCheckerBoard(finfo2, vinfo2, fbp2, 20, 600, 100);
	if (ioctl(fbfd2, LF1000FB_IOCGVIDSCALE, &vidscale_cmd)) {
		printf("Error getting vidscale 2.\n");
		exit(3);
	}
	vidscale_cmd.sizex = 600;
	vidscale_cmd.sizey = 100;
	vidscale_cmd.apply = 1;
	if (ioctl(fbfd2, LF1000FB_IOCSVIDSCALE, &vidscale_cmd)) {
		printf("Error setting vidscale 2.\n");
		exit(3);
	}
	printf("Layer 2, drawing checkerboard wide test\n");
	sleep(5);

	//Crashes
	/*munmap(fbp2, finfo2.smem_len);
	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}
	vinfo2.xres = 100;
	vinfo2.yres = 300;
	if (ioctl(fbfd2, FBIOPUT_VSCREENINFO, &vinfo2)) {
		printf("Error writing variable information 2.\n");
		exit(3);
	}
	if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
		printf("Error reading variable information 2.\n");
		exit(3);
	}
	fbp2 = (char *)mmap(0, finfo2.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd2, 0);

	DrawCheckerBoard(finfo2, vinfo2, fbp2, 20, 100, 300);
	if (ioctl(fbfd2, LF1000FB_IOCGVIDSCALE, &vidscale_cmd)) {
		printf("Error getting vidscale 2.\n");
		exit(3);
	}
	vidscale_cmd.sizex = 100;
	vidscale_cmd.sizey = 300;
	vidscale_cmd.apply = 1;
	if (ioctl(fbfd2, LF1000FB_IOCSVIDSCALE, &vidscale_cmd)) {
		printf("Error setting vidscale 2.\n");
		exit(3);
	}
	printf("Layer 2, drawing checkerboard tall test\n");
	sleep(5);*/

	if (ioctl(fbfd2, LF1000FB_IOCGVIDSCALE, &vidscale_cmd)) {
		printf("Error getting vidscale 2.\n");
		exit(3);
	}
	vidscale_cmd.sizex = vinfo2.xres;
	vidscale_cmd.sizey = vinfo2.yres;
	vidscale_cmd.apply = 1;
	if (ioctl(fbfd2, LF1000FB_IOCSVIDSCALE, &vidscale_cmd)) {
		printf("Error setting vidscale 2.\n");
		exit(3);
	}

	ret = ioctl(fbfd0, FBIOBLANK, 0);
	if(ret < 0)
		printf("N ret=%d\n", ret);
	ret = ioctl(fbfd1, FBIOBLANK, 0);
	if(ret < 0)
		printf("O ret=%d\n", ret);
	ret = ioctl(fbfd2, FBIOBLANK, 1);
	if(ret < 0)
		printf("P ret=%d\n", ret);

	DrawPattern(finfo0, vinfo0, fbp0, VERTICAL_STRIPES);
	DrawPattern(finfo1, vinfo1, fbp1, HORIZONTAL_STRIPES);
	blend_cmd.enable = true;
	blend_cmd.alpha = 8;
	ret = ioctl(fbfd0, LF1000FB_IOCSALPHA, &blend_cmd);
	if (ret)
		perror("ioctl set alpha blend A0\n");

	struct lf1000fb_position_cmd position_cmd;
	position_cmd.left = vinfo0.xres/2;
	position_cmd.top = vinfo0.yres/2;
	position_cmd.right = vinfo0.xres;
	position_cmd.bottom = vinfo0.yres;
	position_cmd.apply = true;
	ret = ioctl(fbfd0, LF1000FB_IOCSPOSTION, &position_cmd);
	if (ret)
		perror("ioctl set position\n");
	printf("position test lower right\n");
	sleep(5);

	position_cmd.left = -vinfo0.xres/2;
	position_cmd.top = -vinfo0.yres/2;
	position_cmd.right = vinfo0.xres/2;
	position_cmd.bottom = vinfo0.yres/2;
	position_cmd.apply = true;
	ret = ioctl(fbfd0, LF1000FB_IOCSPOSTION, &position_cmd);
	if (ret)
		perror("ioctl set position\n");
	printf("position test upper left\n");
	sleep(5);

	position_cmd.left = 0;
	position_cmd.top = 0;
	position_cmd.right = vinfo0.xres;
	position_cmd.bottom = vinfo0.yres;
	position_cmd.apply = true;
	ret = ioctl(fbfd0, LF1000FB_IOCSPOSTION, &position_cmd);
	if (ret)
		perror("ioctl set position\n");

	munmap(fbp2, finfo2.smem_len);
	munmap(fbp1, finfo1.smem_len);
	munmap(fbp0, finfo0.smem_len);

	close(fbfd2);
	close(fbfd1);
	close(fbfd0);
	return 0;
}
