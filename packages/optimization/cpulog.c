#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

#define HZ			200	// sysconf(_SC_CLK_TCK) 
#define USEC_PER_HZ		(1000000/HZ)
#define TARGET_HZ		10
#define JIFFIES_PER_TARGET	(HZ/TARGET_HZ)
#define USEC_PER_TARGET		(1000000/TARGET_HZ)

int XL=0;

main (int c, char **v)
{
	if (c>1)
	{
		if (v[1][0] == 'x')
			XL=1;
	}
	int user, nice, system, idle;
	int user0=0, nice0=0, system0=0, idle0=0;
	int did0=-1;
	char buf[64];
	int fd = open ("/proc/stat", O_RDONLY);
	int fdu = open ("/proc/uptime", O_RDONLY);
	if (fd<0)
	{
		perror ("Can't open /proc/stat");
		exit (1);
	}
	if (fdu<0)
	{
		perror ("Can't open /proc/uptime");
		exit (1);
	}
	for (;;)
	{
		int seek = lseek (fdu, 0, 0);
		if (seek < 0)
		{
			perror ("Can't seek /proc/uptime");
			exit (1);
		}
		int len = read (fdu, buf, 64);
		if (len < 1)
		{
			perror ("Can't read /proc/uptime");
			exit (1);
		}
		const char *dot = strchr (buf, '.');
		if (dot[1] != did0)
		{
			did0 = dot[1];
			char *space = strchr (buf, ' ');
			space[0] = 0;
			printf ("%s ", buf);
			int seek = lseek (fd, 0, 0);
			if (seek < 0)
			{
				perror ("Can't seek /proc/stat");
				exit (1);
			}
			int len = read (fd, buf, 64);
			if (len < 1)
			{
				perror ("Can't read /proc/stat");
				exit (1);
			}
			sscanf (buf, "cpu %d %d %d %d", &user, &nice, &system, &idle);
			int duser = user-user0;
			int dnice = nice-nice0;
			int dsystem = system-system0;
			int didle = idle-idle0;
			int tot = duser+ dnice+ dsystem+ didle;
			// int xuser = 100*duser/tot;
			// int xnice = 100*dnice/tot;
			// int xsystem = 100*dsystem/tot;
 			// int xidle = 100*didle/tot;
			// printf ("U=%3d N=%3d S=%3d I=%3d T=%3d\n",
			// 	xuser, xnice, xsystem, xidle, tot);
			int xuser = 10*duser/tot;
			int xsystem = 10*dsystem/tot;
			int xidle = 10*didle/tot;
			if (XL)
				printf ("%d %d %d\n", xuser, xsystem, xidle);
			else
				printf ("%s%s%s\n",
					"uuUUUUUUUUUU"+12-xuser,
					"ssSSSSSSSSSS"+12-xsystem,
					"iiIIIIIIIIII"+12-xidle);
			fflush (stdout);
			user0 = user;
			nice0 = nice;
			system0 = system;
			idle0 = idle;
		}
		usleep (USEC_PER_HZ);
	}
}
