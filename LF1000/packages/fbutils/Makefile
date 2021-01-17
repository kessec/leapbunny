CROSS_COMPILE:=arm-linux-
CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6/include -I$(ROOTFS_PATH)/usr/include/libpng12 -I$(PROJECT_PATH)/packages/zlib/zlib-1.2.3 -L$(ROOTFS_PATH)/usr/lib
CFLAGS=-std=gnu99 -Wno-variadic-macros -Wall -pedantic
LIBS=-lz -lpng12
INSTALL_PATH:=$(ROOTFS_PATH)/usr/bin

APPS=imager-fb imager dpc-control mlc-control layer-control drawtext flipbook

all: $(APPS)

install:
	mkdir -p $(INSTALL_PATH)
	for a in "$(APPS)"; do \
		cp $$a $(INSTALL_PATH); \
	done

imager-fb: imager-fb.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

imager: imager.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

flipbook: flipbook.c readpng.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

%: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(APPS)
