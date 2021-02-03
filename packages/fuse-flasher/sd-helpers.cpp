#include <stdio.h>
#include "sysfs.h"
#include "fuse-flasher.h"

#define SYS_BUS_MMC_DEVICES "/sys/bus/mmc/devices"

#define NAME_LEN 1024
static char mmcdev[NAME_LEN] = {0};  // The chip device: mmc0:b368
static char mmcblk[NAME_LEN] = {0};  // The mmcblk device: /dev/mmcblkN

// Return NULL if we fail, or a pointer to our static buffer if we succeed
// Usually, get "mmc0:b368"
char *get_mmcdev (void)
{
	if (get_pipe_str ("ls " SYS_BUS_MMC_DEVICES " | fgrep mmc | head -1", mmcdev, NAME_LEN) < 0)
	{
		const char *path = SYS_BUS_MMC_DEVICES;
		PERROR ("failed pipe for fetching");
		return NULL;
	}
	return mmcdev;
}

// Return NULL if we fail, or a pointer to our static buffer if we succeed
// Usually, get "mmcblk0"
char *get_mmcblk (const char *dev)
{
	// Allow NULL to fetch dev automatically
	if (dev == NULL)
		dev = get_mmcdev ();
	if (dev == NULL)
		return NULL; // Trouble
	// Find the actual device we bound back onto
	sprintf (mmcblk, "ls %s/%s | fgrep block: | cut -d : -f 2", SYS_BUS_MMC_DEVICES, dev);
	if (get_pipe_str (mmcblk, mmcblk, NAME_LEN) < 0)
	{
		const char *path = SYS_BUS_MMC_DEVICES;
		NOTE1 ("cmd=%s", mmcblk);
		PERROR ("failed pipe for fetching 'block'");
		return NULL;
	}
	return mmcblk;
}

