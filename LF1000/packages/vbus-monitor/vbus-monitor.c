/* vbus-monitor -- monitor vbus and refresh avahi IP address as needed
 * 
 * Scott Esters <sesters@leapfrog.com>
 *
 * based on monitord
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <linux/input.h>

#ifdef DEBUG_PRINT
#define dbprintf(...)	fprintf(console, "vbus-monitor: " __VA_ARGS__)
FILE *console;	/* for debug printing */
#else
#define dbprintf(...)
#endif

int	running=0;

/*
 * handle a change in the USB Vbus
 */
void event_vbus(unsigned int value)
{
	dbprintf("vbus is %s\n", value == 0 ? "low" : "high");

	if (value) { /* VBUS high */
		system("/usr/bin/vbus-actions 1");
	}
	else {	/* VBUS low */
		system("/usr/bin/vbus-actions 0");
	}
		
}

/* Handle the case of synchronizing with existing VBUS state of system */
void synchronize_with_vbus (void)
{
	/* Just poll driver */
	FILE *f = fopen ("/sys/devices/platform/lf1000-usbgadget/vbus", "r");
	if (f) {
		int c = fgetc (f);
		fclose (f);
		/* Send fake event to get us in sync */
		event_vbus (c == '1');
	}
}

/*
 * daemonize our process by losing the controlling shell and disconnecting from
 * the file descriptors
 */
#ifndef DEBUG_NO_DAEMON
void daemonize(const char *cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit r1;
	struct sigaction sact;

	umask(0);

	if(getrlimit(RLIMIT_NOFILE, &r1) < 0) {
		exit(1);
	}

	/*
	 * become a session leader to lose controlling TTY
	 */

	if((pid = fork()) < 0) {
		exit(1);
	}
	else if(pid != 0) /* parent */
		exit(0);
	setsid();

	/*
	 * ensure future opens won't allocate controlling TTYs
	 */

	sact.sa_handler = SIG_IGN;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	if(sigaction(SIGHUP, &sact, NULL) == -1) {
		exit(1);
	}
	if((pid = fork()) < 0) {
		exit(1);
	}
	else if(pid != 0) /* parent */
		exit(0);

	/*
	 * change current working directory to the root so that we won't
	 * prevent file systems from being unmounted
	 */

	if(chdir("/") < 0) {
		exit(1);
	}

	/*
	 * close all open file descriptors
	 */
	if(r1.rlim_max == RLIM_INFINITY)
		r1.rlim_max = 20; /* XXX */
	for(i = 0; i < r1.rlim_max; i++)
		close(i);

	/*
	 * attach file descriptors 0, 1, 2 to /dev/null
	 */

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	/*
	 * initiate log file
	 */

	if(fd0 != 0 || fd1 != 1 || fd2 != 2) {
		exit(1);
	}
}
#endif /* DEBUG_NO_DAEMON */

/*
 * find and open an input device by name
 */
int open_input_device(char *input_name)
{
	struct dirent *dp;
	DIR *dir;
	char dev[20];
	char name[32];
	int fd;

	dir = opendir("/dev/input/");
	if (!dir)
		return -1;

	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name && !strncmp(dp->d_name, "event", 5)) {
			sprintf(dev, "/dev/input/%s", dp->d_name);

			fd = open(dev, O_RDONLY);
			if(fd == -1)
				continue;

			if(ioctl(fd, EVIOCGNAME(32), name) < 0) {
				close(fd);
				continue;
			}

			if(!strcmp(name, input_name)) { /* found */
				closedir(dir);
				return fd;
			}

			close(fd);
		}
	}

	closedir(dir);
	return -1;
}

/*
 * handle signals
 */
void handle_signal(int sig)
{
	switch(sig) {
		/* the daemon needs to shut down */
		case SIGTERM:
		case SIGINT:
			running = 0;
		break;

		default:
		dbprintf("unknown signal\n");
		break;
	}
}

#define FD_USB	0

int main(int argc, char **argv)
{
	int ret, size;
	int fd_usb;
	struct input_event ev;
	struct pollfd fds[1];
	int num_fds = 0;
	struct sigaction sa_int;

	/* 
	 * initialize our state
	 */

	running=0;

#ifndef DEBUG_NO_DAEMON
	daemonize(argv[0]);	
#endif

#ifdef DEBUG_PRINT
	console = fopen("/dev/console", "w");
	if(console == NULL)
		exit(1);
#endif
	/* 
	 * grab the USB input device so we can monitor VBUS 
	 */

	fd_usb = open_input_device("LF1000 USB");
	if(fd_usb < 0) {
		dbprintf("can't open USB input device\n");
		goto fail_usb;
	}
	fds[FD_USB].fd = fd_usb;
	fds[FD_USB].events = POLLIN;
	num_fds++;

	/*
	 * trap SIGTERM so that we clean up before exiting
	 */

	running = 1;
        sigemptyset(&sa_int.sa_mask);
	sa_int.sa_handler = handle_signal;
	sa_int.sa_flags = 0;
	if(sigaction(SIGTERM, &sa_int, NULL) == -1) {
		dbprintf("can't trap SIGTERM\n");
		goto fail_sig;
	}
#ifdef DEBUG_NO_DAEMON
	if(sigaction(SIGINT, &sa_int, NULL) == -1) {
		dbprintf("can't trap SIGINT\n");
		goto fail_sig;
	}
#endif

	/*
	 * monitor
	 */

	synchronize_with_vbus ();
	while(running) {
		/* monitor our file descriptors for activity */
		ret = poll(fds, num_fds, 1000);
		if(ret > 0) {
			/* did we get a USB event? */
			if(fds[FD_USB].revents & POLLIN) {
				dbprintf("USB event\n");
				size = read(fd_usb, &ev, sizeof(ev));
				/* USB vbus switch event */
				if(ev.type == EV_SW && ev.code == SW_LID)
					event_vbus(ev.value);
			}
		}
	}

	dbprintf("exiting...\n");

	close(fd_usb);
#ifdef DEBUG_PRINT
	fclose(console);
#endif
	exit(0);

fail_sig:
	close(fd_usb);
fail_usb:
#ifdef DEBUG_PRINT
	fclose(console);
#endif
	exit(1);
}
