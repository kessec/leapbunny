#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define POWER_INPUT "/dev/input/event0"

// Inject a power button press key event
int main (int argc, char **argv)
{
	struct input_event ev;
	
	// O_DSYNC idea stolen from Dan's JIOHandler
	int fd = open (POWER_INPUT, O_WRONLY | O_DSYNC);
	if (fd < 0)
	{
		perror("opening " POWER_INPUT);
		return 1;
	}

	// Press key
	ev.type = EV_KEY;
	ev.code = KEY_POWER;
	ev.value = 1;
	write (fd, &ev, sizeof(ev));

	// Release key
	ev.value = 0;
	write (fd, &ev, sizeof(ev));

	return close (fd);
}
