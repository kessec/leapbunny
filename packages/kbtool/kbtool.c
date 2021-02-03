/* kbtool.c -- Check to see if a key is pressed.
 *
 * Please add scan codes to the kb_keys table should we need to support
 * additional keys.  See <linux/input.h> for the scan code definitions.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 */

#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define KEYBOARD_NAME	"LF1000 Keyboard"

struct kb_key {
	char code;
	char *name;
};

struct kb_key kb_keys[] = {
	{ KEY_A,		"A" },
	{ KEY_B,		"B" },
	{ KEY_H,		"H" },
	{ KEY_L,		"L" },
	{ KEY_M,		"M" },
	{ KEY_P,		"P" },
	{ KEY_R,		"R" },
	{ KEY_X,		"X" },
	{ KEY_UP,		"Up" },
	{ KEY_DOWN,		"Down" },
	{ KEY_RIGHT,		"Right" },
	{ KEY_LEFT,		"Left" },
	{ KEY_VOLUMEDOWN,	"VolumeDown" },
	{ KEY_VOLUMEUP,		"VolumeUp" },
};

int get_keycode(char* key)
{
	int i;

	for (i = 0; i < sizeof(kb_keys)/sizeof(kb_keys[0]); i++)
		if (!strcmp(key, kb_keys[i].name))
			return kb_keys[i].code;

	return -1;
}

static int get_kb_fd(char *input_name)
{
	int fd;
	struct dirent *dp;
	char name[32];
	char dev[32];
	DIR *dir;

	dir = opendir("/dev/input");
	if (!dir)
		return 1;

	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name && !strncmp(dp->d_name, "event", 5)) {
			snprintf(dev, sizeof(dev), "/dev/input/%s", dp->d_name);
			fd = open(dev, O_RDONLY);
			if (fd == -1)
				continue;
			if (ioctl(fd, EVIOCGNAME(32), name) < 0) {
				close(fd);
				continue;
			}

			if (!strcmp(name, input_name)) {
				closedir(dir);
				return fd;
			} else { /* not what we wanted, check another */
				close(fd);
				fd = -1;
			}
		}
	}

	closedir(dir);
	return -1;
}

static void check_key(int fd, int key)
{
	char key_b[KEY_MAX/8 + 1];

	memset(key_b, 0, sizeof(key_b));
	ioctl(fd, EVIOCGKEY(sizeof(key_b)), key_b);

	printf("%d\n", !!(key_b[key/8] & (1<<(key % 8))));
}

static void print_help(char *name)
{
	int i;

	printf("usage: %s <key>\n\nThe supported <key> names are:\n", name);

	for (i = 0; i < sizeof(kb_keys)/sizeof(kb_keys[0]); i++)
		printf("\t%s\n", kb_keys[i].name);
}

int main(int argc, char **argv)
{
	int fd = get_kb_fd(KEYBOARD_NAME);
	int code;

	if (argc < 2) {
		print_help(argv[0]);
		return 0;
	}

	if (fd < 0) {
		printf("Error: unable to find \"%s\"\n", KEYBOARD_NAME);
		return 1;
	}

	code = get_keycode(argv[1]);
	if (code < 0) {
		printf("Unsupported key\n");
		close(fd);
		return 2;
	}

	check_key(fd, code);

	close(fd);
	return 0;
}
