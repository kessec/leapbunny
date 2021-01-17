#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>


#define PAGE_SIZE 512

char buf[PAGE_SIZE];

int getRandomNumber(void) {
	int fd;
	int value;

	fd = open("/dev/urandom", O_RDONLY );
	if( fd != -1 ) {
		read(fd, &value, sizeof(int));
		printf("use random number %ud\n", value);
	} else {
		value=0;
		printf("use random number %ud\n", 0);
	}
	close(fd);
	return value;
}

int main(int argc, char *argv[]) {
	int i, ret;
	FILE *fp;
	time_t now;
	struct tm *curtime;
	// char b1_year, b2_month, b3_day, b4_hour, b5_wafer_number, b6_x_cor, b7_ycor;
	
	if(argc != 2) {
		printf("This program add development serial number for OTP cartridge\n");
		printf("Usage: %s inoutfile \n", argv[0]);
		return 1;
	}
	
	if((fp= fopen(argv[1], "r+")) == NULL) {
		printf("error opening file\n");
	}
	
	if(fseek(fp, -PAGE_SIZE, SEEK_END) != 0) {
		printf("error fseek(...), errno = %d (%s), !\n", errno, strerror(errno));
	}

	if(fread(buf, sizeof(char), PAGE_SIZE, fp) != PAGE_SIZE) {
		printf("error fseek(...) file, errno = %d (%s), !\n", errno, strerror(errno));
	} else {
		if(strncmp(&buf[128], "by otp_program.sh", 16) == 0) {
			printf("Warning: pseudo serial number already in binary file\n");
			return 1;
		}
		
		for(i=0; i<PAGE_SIZE; i++) {
			if(buf[i] != '\xff') {
				printf("Error: last page not empty\n");
				return 1;
			}
		}
		time(&now);
		curtime=gmtime(&now);
		
		strftime(&buf[128], PAGE_SIZE-128, "by otp_program.sh, %m/%d/%y %R:%S", curtime);
		
		if(fseek(fp, -PAGE_SIZE, SEEK_END) != 0) {
			printf("error fseek(...) file, errno = %d (%s), !\n", errno, strerror(errno));
		}

		ret = fwrite(buf, sizeof(char), PAGE_SIZE, fp);
		if(ret != PAGE_SIZE) {
			printf("Error creating file \n");
			fclose(fp);
			return 1;
		}
		
		
	}
		
	
	fclose(fp);
	return 0;
}

