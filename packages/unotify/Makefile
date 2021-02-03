CC:=$(CROSS_COMPILE)gcc

CFLAGS=-Wall -pedantic

all: unotify cnotify vnotify

unotify: unotify.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

cnotify: cnotify.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

vnotify: vnotify.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o unotify cnotify vnotify
