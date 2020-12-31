CC=arm-linux-gcc
INCLUDES=-I$(PROJECT_PATH)/linux-2.6/include

gpio-control: gpio-control.c
	$(CC) $(INCLUDES) -o gpio-control gpio-control.c

install: gpio-control
	cp gpio-control $(ROOTFS_PATH)/usr/bin

clean:
	rm -f ./gpio-control *.o
