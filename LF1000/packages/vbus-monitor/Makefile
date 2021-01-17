CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6/include
CFLAGS=-std=gnu99 -Wno-variadic-macros -Wall -pedantic -DDEBUG_PRINT
LIBS=-lrt

all: vbus-monitor

vbus-monitor: vbus-monitor.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o vbus-monitor
