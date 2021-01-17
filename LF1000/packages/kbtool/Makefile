CC:=arm-linux-gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6/include
CFLAGS=-Wall
LIBS=-lrt

all: kbtool

kbtool: kbtool.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o kbtool
