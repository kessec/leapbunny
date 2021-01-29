C_FLAGS=-D_FILE_OFFSET_BITS=64
CXX=arm-linux-g++
LIBS=-L usr/lib -lfuse
INCLUDE=-I usr/include/ -I usr/include/fuse
# ROOTFS_PATH=.

all: fuse-flasher

install: all
	mkdir -p $(ROOTFS_PATH)/usr/sbin
	cp -v fuse-flasher $(ROOTFS_PATH)/usr/sbin
	tar -c usr/lib | tar -C $(ROOTFS_PATH) -xv 

fuse-flasher: fuse-flasher.cpp usr/include/fuse.h \
	sd-partition-fuse.cpp sysfs.cpp sd-helpers.cpp \
	sd-ext3-fuse.cpp sd-raw-fuse.cpp filenames.cpp norboot-fuse.cpp
	$(CXX) $(C_FLAGS) $(INCLUDE) -o $@ $^ $(LIBS)
	# tar -cf ~lfu/fuse-flasher.tar usr/bin usr/lib

usr/include/fuse.h: fuse-arm-dev.tar.gz
	tar -zxf fuse-arm-dev.tar.gz

clean:
	rm -rf fuse-flasher usr etc
