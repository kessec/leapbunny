CC:=$(CROSS_COMPILE)gcc
CXX:=$(CROSS_COMPILE)g++
STRIP:=$(CROSS_COMPILE)strip
CFLAGS:=-O3
CXXFLAGS:=-O3
INSTALL_PATH:=$(ROOTFS_PATH)/usr/bin

ALL=iospeed iospeed2 iospeed3 iospeed4 cpuspeed cpulog tim memspeed \
    press-power iospeed6 iospeed8

MORE=faster-nand-timing nand-speed-timing-test 

all: $(ALL)
	# 
	#jpegspeed

INCLUDES=-I${BRIO_PATH}/ThirdParty/LibJPEG/Include
LIBS=-L${BRIO_PATH}/ThirdParty/LibJPEG/Libs/arm

iospeed7: iospeed7.cpp
	if [ -e $(BRIO_PATH)/System/Utility/AtomicFile.cpp ]; then \
		ln -sf $(BRIO_PATH)/System/Include/AtomicFile.h .; \
		$(CXX) $(CXXFLAGS) -DNOBRIO -I $(BRIO_PATH)/System/Include $(BRIO_PATH)/System/Utility/AtomicFile.cpp -o $@ $^; \
	else \
		echo "Trouble!!"; exit 1; \
	fi

jpegspeed: jpegspeed.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -ljpeg

# strip: 
# 	$(STRIP) get-bootstrap-version

clean:
	rm -fv *.o *~ $(ALL)

install: all
	install -d $(INSTALL_PATH)
	cp -pv $(ALL) $(MORE) $(INSTALL_PATH)
