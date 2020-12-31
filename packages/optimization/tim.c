// Replacement for tim in boot scripts

// /sys/devices/platform/lf1000-nand # cat write
// Read  N=3295 346/871/3285
// Write N=8 1699/1804/1851
// Erase N=0 0/0/0
// Lock  N=6605 165/550/133770
//
// /proc/uptime
// 203.27 0.01
//
// /proc/stat
// cpu  27 0 197 22987 0 2 0 0 0
// cpu0 27 0 197 22987 0 2 0 0 0
// intr 24994 0 0 0 0 23213 0 0 0 0 0 445 0 180 0 0 0 0 0 0 0 53 0 0 0 0 926 0 0 0 0 0 0 177 0 0 0 0 0 0 0 0 0
// ctxt 52457
// btime 1201498622
// processes 77
// procs_running 1
// procs_blocked 0
//
//////////////////////////////////////////////////////////////////////////////
//
// Generate a line of output like this
//  Mark Tag Uptime UserTicks SysTicks IdleTicks Reads Writes Erases Locks
//  +++ Begin 353.52 125 351 442 3295 8 0 6600


#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

main (int c, char **v)
{
	int user, nice, system, idle;
	char buf[128];
	if (c<3)
	{
		printf ("Usage tim {tag} {message}\n");
		exit (1);
	}
	char *tag =
		tolower(v[1][0]) == 'b' ? "Bgn" :
		tolower(v[1][0]) == 's' ? "Bgn" :
		tolower(v[1][0]) == 'm' ? "Mid" :
		tolower(v[1][0]) == 'e' ? "End" :
		tolower(v[1][0]) == 'd' ? "End" : v[1];
	char *msg = v[2];
	int fd = open ("/proc/uptime", O_RDONLY);
	if (fd<0)
	{
		perror ("Can't open /proc/uptime");
		exit (1);
	}
	int len = read (fd, buf, 128);
	close (fd);
	if (len < 1)
	{
		perror ("Can't read /proc/uptime");
		exit (1);
	}
	char *space = strchr (buf, ' ');
	space[0] = 0;
	printf ("+++ %7s ", buf);

	fd = open ("/proc/stat", O_RDONLY);
	if (fd<0)
	{
		perror ("Can't open /proc/stat");
		exit (1);
	}

	len = read (fd, buf, 128);
	close (fd);
	if (len < 1)
	{
		perror ("Can't read /proc/stat");
		exit (1);
	}
	sscanf (buf, "cpu %d %d %d %d", &user, &nice, &system, &idle);
	printf ("%5d %5d %5d ", user, system, idle);

	fd = open ("/sys/devices/platform/lf1000-nand/write", O_RDONLY);
	if (fd<0)
	{
		printf ("%s %s\n", tag, msg);
		return 0;
	}
	len = read (fd, buf, 128);
	close (fd);
	if (len < 1)
	{
		perror ("Can't read lf1000-nand/write");
		exit (1);
	}
	int read_, write_, erase, lock;
	char *nl;
	sscanf (buf, "Read  N=%d", &read_);
	nl = strchr (buf, '\n');
	sscanf (nl+1, "Write N=%d", &write_);
	nl = strchr (nl+1, '\n');
	sscanf (nl+1, "Erase N=%d", &erase);
	nl = strchr (nl+1, '\n');
	sscanf (nl+1, "Lock  N=%d", &lock);
	printf ("%5d %5d %5d %5d %s %s\n", read_, write_, erase, lock, tag, msg);
}
