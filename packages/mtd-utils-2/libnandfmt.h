#ifndef LIBNANDFMT_H
#define LIBNANDFMT_H
#ifdef __cplusplus
extern "C"
{
#endif
// Return 0 for OK, non-zero for trouble.
int nandfmt_open (const char *dev, unsigned long start, unsigned long length,
		 int in_ubi_mode, int verbose);
int nandfmt_close (void (*pBusyCallback)(int percent));
int nandfmt_write (char *buf, int length);
// Just display erase counts and close
void nandfmt_show_erase_counts ();
#ifdef __cplusplus
}
#endif
#endif // LIBNANDFMT_H
