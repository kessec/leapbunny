#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static inline int have_framebuffer(void)
{
	struct stat st;

	return stat("/sys/class/graphics", &st) == 0;
}
