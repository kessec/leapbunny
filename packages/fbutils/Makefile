CROSS_COMPILE:=arm-linux-
CC:=$(CROSS_COMPILE)gcc
CPP:=$(CROSS_COMPILE)g++

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6/include -I$(ROOTFS_PATH)/usr/include/libpng12 -I$(PROJECT_PATH)/packages/zlib/zlib-1.2.3 -L$(ROOTFS_PATH)/usr/lib
CFLAGS=-std=gnu99 -Wno-variadic-macros -Wall -pedantic -D_FILE_OFFSET_BITS
CPPFLAGS=-Wno-variadic-macros -Wall -pedantic -D_FILE_OFFSET_BITS
LIBS=-lz -lpng12
INSTALL_PATH:=$(ROOTFS_PATH)/usr/bin

APPS=imager-fb drawtext-fb flipbook-fb fbctrl fbtest flipbook2
APPS_DEPRECATED=imager dpc-control mlc-control layer-control drawtext flipbook

# TODO: remove deprecated apps
APPS+=$(APPS_DEPRECATED)

all: $(APPS)

install:
	mkdir -p $(INSTALL_PATH)
	cp -v $(APPS) $(INSTALL_PATH)

imager-fb: imager-fb.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

drawtext-fb: drawtext-fb.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

imager: imager.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

flipbook-fb: flipbook-fb.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

flipbook: flipbook.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

flipbook2: flipbook2.cpp readpng.c
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

fbtest: FrameBufferTest.cpp
	$(CPP) $(INCLUDE) -o $@ $^ $(LIBS)

%: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o lf1000fb.h $(APPS)
