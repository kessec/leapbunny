CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6.28-lf1000/include
CFLAGS=-std=gnu99 -Wall -pedantic 
LIBS=-lrt

all: dumpcore 

dumpcore: dumpcore.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o dumpcore
