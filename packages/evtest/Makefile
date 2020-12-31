INCLUDES=-I$(PROJECT_PATH)/linux-2.6/include

evtest: evtest.c
	arm-linux-gcc $(INCLUDES) -o evtest evtest.c

install: evtest
	cp ./evtest $(ROOTFS_PATH)/usr/bin

clean:
	rm -f ./evtest *.o
