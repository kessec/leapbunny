#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>

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
	char buf[256];
	for (;;)
	{
		int fd = open ("/proc/stat", O_RDONLY);
		if (!fd)
		{
			perror ("Can't open /proc/stat");
			exit (1);
		}
		int len = read (fd, buf, 256);
		if (len < 1)
		{
			perror ("Can't open /proc/stat");
			exit (1);
		}
		sscanf (buf, "cpu %d %d %d %d", &user, &nice, &system, &idle);
		int duser = user-user0;
		int dnice = nice-nice0;
		int dsystem = system-system0;
		int didle = idle-idle0;
		// printf ("U=%02d N=%02d S=%02d I=%02d\n",
		// 	duser, dnice, dsystem, didle);
		int tot = duser+ dnice+ dsystem+ didle;
		if (XL)
			printf ("%d %d %d %d %d\n", 
				duser, dnice, dsystem, didle, tot);
		else if (tot <12)
			printf ("%s%s%s\n",
				"uuUUUUUUUUUU"+12-duser,
				"ssSSSSSSSSSS"+12-dsystem,
				"iiIIIIIIIIII"+12-didle);
		fflush (stdout);
		user0 = user;
		nice0 = nice;
		system0 = system;
		idle0 = idle;
		usleep (91000);
	}
}
