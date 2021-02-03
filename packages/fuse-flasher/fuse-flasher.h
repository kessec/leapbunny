#ifndef _FUSE_FLASHER_H_
#define _FUSE_FLASHER_H_

#include <stdio.h>

extern const char* SD_DIR;
extern const char* SD_PARTITION_DIR;
extern const char* SD_EXT_DIR;
extern const char* SD_RAW_DIR;
extern const char* NORBOOT_DIR;

void init (void);

#if 1 // Debugging info
#include <errno.h>
#include <string.h>
#include <time.h>
extern FILE *debug_out;
extern time_t debug_t0;
#define TIME	time(NULL)-debug_t0
#define ENTER	init(), fprintf (debug_out, "+++ %ld %s: Enter path=%s\n", TIME, __func__, path)
#define EXIT	init(), fprintf (debug_out, "+++ %ld %s: Exit path=%s\n", TIME, __func__, path)
#define RETURN(s)	init(), fprintf (debug_out, "+++ %ld %s: Return '%s' path=%s\n", TIME, __func__, s, path)
#define NOTE1(s,x)	init(), fprintf (debug_out, "+++ %ld %s: Note " s " path=%s\n", TIME, __func__, x, path)
#define NOTE2(s,x,y)	init(), fprintf (debug_out, "+++ %ld %s: Note " s " path=%s\n", TIME, __func__, x, y, path)
#define PERROR(s)	init(), fprintf (debug_out, "+++ %ld %s: Error %s: %s path=%s\n", TIME, __func__, s, strerror(errno), path)
#else
#define ENTER
#define EXIT
#define RETURN(s)
#define NOTE1(s,x)
#define NOTE2(s,x,y)
#define PERROR(s)
#endif

#endif //_FUSE_FLASHER_H_
