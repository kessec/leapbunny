#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <stdlib.h>

double t0;

#define head(N) \
  void N () { \
    struct timeval tv;  \
    gettimeofday (&tv, NULL); \
    double t1 = tv.tv_sec + tv.tv_usec * 1e-6 - t0; \
    printf ("+++ %.3f Start %s...\n", t1, #N);

#define tail(N) \
    gettimeofday (&tv, NULL); \
    double t2 = tv.tv_sec + tv.tv_usec * 1e-6 - t0; \
    printf ("+++ %.3f ..... %s Done.  Took %.3f\n", t2, #N, t2-t1); \
  }

head(bootflags)
{
	int r = mount ("/dev/mtdblock1", "/flags", "jffs2", MS_NOATIME | MS_SYNCHRONOUS, "");
	if (r)
	{
		printf ("*** Mount /dev/mtdblock1 /flags failed, returning %d\n", r);
	}
}  
tail(bootflags)

head(mfgdata)
{
	int r = mount ("/dev/mtdblock2", "/mfgdata", "jffs2", MS_NOATIME | MS_SYNCHRONOUS, "");
	if (r)
	{
		printf ("*** Mount /dev/mtdblock2 /mfgdata failed, returning %d\n", r);
	}
}  
tail(mfgdata)

head(ubi)
{
	system ("/etc/init.d/ubi start");
}
tail(ubi)

head(setcal)
{
	system ("/etc/init.d/setcal start");
}
tail(setcal)

head(mass_storage)
{
	system ("/etc/init.d/mass_storage start");
}
tail(mass_storage)

head(monitord)
{
	system ("/etc/init.d/monitord start");
}
tail(monitord)

head(bootvideo)
{
	system ("/etc/init.d/bootvideo start");
}
tail(bootvideo)

head(lightning)
{
	system ("/etc/init.d/lightning start");
}
tail(lightning)

// boot video early
main (int argc, char **argv)
{
  int p1, p2, p3;
  struct timeval tv;

  gettimeofday (&tv, NULL);
  t0 = tv.tv_sec + tv.tv_usec * 1e-6;
  if (argc>1) 
  {
	  double d = atof(argv[1]);
	  printf ("Offsetting fling start time by %.3f\n", d);
	  t0 -= d;
  }

  if (!(p1=fork ()))
  {
    // child
    mfgdata ();
  }
  else
  {
    // parent
    bootflags ();
    if (!(p2=fork ()))
    {
      // child
      bootvideo ();
    }
    else
    {
      // parent
      if (!(p3=fork ()))
      {
	// child
	setcal ();
      }
      else
      {
	// parent
	wait (p1); // wait for mfgdata
        ubi ();
	mass_storage ();
	monitord ();
	wait (p2); // wait for bootvideo
	wait (p3); // wait for setcal
	lightning ();
      }
    }
  }
}

/*
main_ubi_bootvideo (int argc, char **argv)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  t0 = tv.tv_sec + tv.tv_usec / 1e6;
  int p1, p2, p3;
  if (!(p1=fork ()))
  {
    // child
    mfgdata ();
  }
  else
  {
    // parent
    bootflags ();
    if (!(p2=fork ()))
    {
      // child
      setcal ();
    }
    else
    {
      // parent
      ubi ();
      if (!(p3=fork ()))
      {
	// child
	bootvideo ();
      }
      else
      {
	// parent
	wait (p1); // wait for mfgdata
	mass_storage ();
	monitord ();
	wait (p2); // wait for setcal
	wait (p3); // wait for bootvideo
	lightning ();
      }
    }
  }
}
*/

/*
  bootflags .18	   ->  2
  ubi 1.88	   -> 18
  bootvideo 8.00   -> 80
  mfgdata .46      ->  5
  setcal .11       ->  1
  mass_storage 1.33-> 13
  monitord .24     ->  3
  lightning        

                      bootflags  
                 /       |         \
              ubi      mfgdata      setcal
           /        \    |
bootvideo             mass_storage
          \             |
           \          monitord
            \        /
            lightning
*/
