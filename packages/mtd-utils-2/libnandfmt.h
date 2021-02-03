#ifndef LIBNANDFMT_H
#define LIBNANDFMT_H
#ifdef __cplusplus
extern "C"
{
#endif
// Return 0 for OK, non-zero for trouble.
// Dev is "/dev/mtd3" etc.
// Start and length are the partition in NAND, given in bytes, not blocks
// Pass in_ubi_mode=1 for fancy UBI erase block formatting.... You can always
// just pass 0 here, and you'll only lose some wear leveling data
int nandfmt_open (const char *dev, unsigned long start, unsigned long length,
		 int in_ubi_mode, int verbose);

// Return 0 for OK, non-zero for trouble.
// Note!  You must provide a callback!  NULL is currently not supported.
// If in_ubi_mode was 1, then this callback will get called periodically with
// a percent of completion.  This lets the PC know how much longer
// nandfmt_close will take to return (could take minutes on large NAND
// chip), and help it report status to user.
int nandfmt_close (void (*pBusyCallback)(int percent));

// Return 0 for OK, non-zero for trouble.
// Send in a chunk of data.  Entire chunk is taken and 0 or more NAND blocks
// will get flashed.  Extra bytes will be buffered for you, so you don't need
// to send in chunk that is a multiple of the block size.   Very convenient
// for a stream coming from USB.
int nandfmt_write (const char *buf, int length);

// Just display erase counts and close
void nandfmt_show_erase_counts ();
#ifdef __cplusplus
}
#endif
#endif // LIBNANDFMT_H
