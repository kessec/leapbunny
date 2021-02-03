CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6/include
CFLAGS=-std=gnu99 -Wall -pedantic 
LIBS=-lrt

all: otpwrite addsn

addsn:addsn.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

otpwrite:otpwrite.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o otpwrite addsn
