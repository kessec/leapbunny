CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(KERNELDIR)/drivers/media/video/uvc
#CFLAGS=-std=gnu99 -Wall -pedantic 
#LIBS=-lrt

all: pixart

pixart:main.c pixart_hex.c pixart_ctrls.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o pixart
