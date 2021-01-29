#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	int sleep_len = 30;
	
	if(argc > 1)
		sscanf(argv[1], "%d", &sleep_len);
	
	while(1)
	{
		sync();
		sleep(sleep_len);
	}
	return 0;
}
