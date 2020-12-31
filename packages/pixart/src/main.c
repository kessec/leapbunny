/*
 * Application for reading/writing PixArt PAC7332 EEPROM using
 * its UVC Extension Unit controls.
 *
 * Includes uvcvideo.h from the Linux sources, so compile with:
 * gcc -I/path/to/linux-kernel-src/drivers/media/video/uvc -o pixart_fw main.c
 *
 * See also: http://www.quickcamteam.net/software/libwebcam
 */

/*
 * This program takes PixArt-provided Intel HEX firmware image files for its
 * second and third arguments.  PixArt supplies two files a FW.hex file,
 * containing an opaque firmware blob, and UI.hex, a file that contains
 * modifiable parameters, such as camera image saturation and USB product id.
 * The FW.hex files should be valid Intel HEX format, e.g.,
 * # srec_info PAC7332C_20100128_AsiUSA_60hz_initdark_9553_FW.hex -Intel
 * ...
 * The UI.hex files are not valid, so they are handled specially.
 *
 * The srec_* utilities can be obtained by installing srecord:
 * http://srecord.sourceforge.net
 */

#include "common.h"
#include "pixart_hex.h"
#include "pixart_ctrls.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

static int check_inputs(const char *dev, const char *fw, const char *ui)
{
	int ret;
	struct stat buf;

	ret = stat(dev, &buf);

	if(ret != 0 || !S_ISCHR(buf.st_mode))
		return -1;

	ret = stat(fw, &buf);
	
	if(ret != 0)
		return -1;

	ret = stat(ui, &buf);
	
	if(ret != 0)
		return -1;

	return 0;
}

static int usage(FILE *stream, char *name)
{
	fprintf(stream, "usage:\n" \
	                "\t%1$s device FW.hex UI.hex\n" \
	                "\te.g.,\n\t# %1$s /dev/video0 PAC7332C_20100701_AsiUSA_60hz_VGA_IQ15_D40D_FW.hex PAC7332C_20100701_AsiUSA_60hz_VGA_IQ15_D40D_UI.hex\n",
			name );
	return 0;
}

int main(int argc, char *argv[])
{
	int dev, fw_img, ui_img, i, ret = -1;
	FW_STATUS status;

	struct fw_chunk chunks[3] = { {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0} };
	struct fw_chunk *id = &chunks[0],
			*ui = &chunks[1],
			*fw = &chunks[2];

	if(argc != 4 || (check_inputs(argv[1], argv[2], argv[3]) != 0))
	{
		ret = -EINVAL;
		usage(stderr, argv[0]);
		goto out;
	}

	dev = open(argv[1], O_RDWR);
	if(dev == -1)
	{
		ret = errno;
		fprintf(stderr, "Failed to open %s (%s)\n", argv[1], strerror(errno));
		goto out;
	}

	fw_img = open(argv[2], O_RDONLY);
	if(fw_img == -1)
	{
		ret = errno;
		fprintf(stderr, "Failed to open %s (%s)\n", argv[2], strerror(errno));
		goto err;
	}

	ui_img = open(argv[3], O_RDONLY);
	if(ui_img == -1)
	{
		ret = errno;
		fprintf(stderr, "Failed to open %s (%s)\n", argv[3], strerror(errno));
		goto err1;
	}

	ret = install_xu_ctrls(dev);
	if(ret != 0)
		goto err2;

	/*
	 * Must parse FW.hex first.  The UI.hex parsing routine computes a
	 * checksum using its contents.
	 */
	ret = parse_fw_hex(fw_img, fw);
	if(ret != 0)
		goto err2;

	ret = parse_ui_hex(ui_img, id, ui, fw);
	if(ret != 0)
		goto err3;

	fprintf(stdout, "Checking camera firmware version...");
	status = check_cam_fw(dev, id);
	switch(status)
	{
	case FW_CHECK_MATCH:
		fprintf(stdout, "OK!\nFirmware is current; no update needed.\n");
		break;
	case FW_CHECK_MISMATCH:
		fprintf(stdout, "OK!\nFirmware is out-of-date; updating...");
		fflush(stdout);
		for(i = 0; (ret == 0) && (i < NUM_ELEMS(chunks)); i++)
		{
			ret = update_fw(dev, &chunks[i]);
		}
		if(i == NUM_ELEMS(chunks))
		{
			fprintf(stdout, "OK!\n");
		}
		break;
	case FW_CHECK_ERR:
		fprintf(stderr, "Error!\nCould not determine firmware version.");
		break;
	}

err3:
	for(i = 0; i < NUM_ELEMS(chunks); i++)
	{
		if(chunks[i].data)
		{
			free(chunks[i].data);
			chunks[i].data = NULL;
			chunks[i].len = 0;
		}
	}
err2:
	close(ui_img);
err1:
	close(fw_img);
err:
	close(dev);
out:
	return ret;
}

