OBJS=new-calib mb batt mph pfun acc-calib acc-show acc-bubble ts-rawtest \
	 collect-pdata
SCRIPTS=acc-raw.sh acc-cooked.sh
FBINCS=fb.h drawtext.h 
STATINCS=MaxMin.h Histo.h Stats.h 
INCLUDES=-I$(PROJECT_PATH)/linux-2.6/include -I./usr/include -I../../mfgdata/lib
CPPFLAGS=$(INCLUDES)
LDFLAGS=-L./usr/lib -L../../mfgdata
LDLIBS=-lts -lMfgData

CC=arm-linux-gcc
CXX=arm-linux-g++ -g

all: $(OBJS)

install: all
	mkdir -p $(ROOTFS_PATH)/usr/bin
	cp $(OBJS) $(ROOTFS_PATH)/usr/bin
	cp $(SCRIPTS) $(ROOTFS_PATH)/usr/bin

post: all
	echo cp -v $(OBJS) ~lfu
	rm -f ~lfu/tslib.tar ~lfu/ts.tar
	tar -cf ~lfu/tslib.tar usr/lib
	tar -cf ~lfu/ts.tar $(OBJS) $(SCRIPTS)

clean:
	rm -f $(OBJS) *.o *~

##############################################################################

fb.o: fb.h fb.cpp

drawtext.o: drawtext.h drawtext.cpp

OpenInput.o: OpenInput.h OpenInput.cpp

MaxMin.o: MaxMin.h MaxMin.cpp

Histo.o: Histo.h Histo.cpp

Stats.o: Stats.h Stats.cpp

ComputeAffine.o: ComputeAffine.h ComputeAffine.cpp

###

new-calib.o: new-calib.cpp $(FBINCS) $(STATINCS) ComputeAffine.h 

# calib-delay.o: calib-delay.cpp $(FBINCS) $(STATINCS)

batt.o: batt.cpp $(FBINCS)

mb.o: mb.cpp $(FBINCS) OpenInput.h

ts-rawtest.o: ts-rawtest.cpp $(FBINCS) OpenInput.h

collect-pdata.o: collect-pdata.cpp $(FBINCS) $(STATINCS) OpenInput.h

pfun.o: pfun.cpp $(FBINCS) OpenInput.h

mph.o: mph.cpp $(FBINCS) $(STATINCS)

new-calib: new-calib.o MaxMin.o Histo.o fb.o OpenInput.o Stats.o drawtext.o \
	ComputeAffine.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

acc-calib: acc-calib.o MaxMin.o Histo.o fb.o OpenInput.o Stats.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

acc-show: acc-show.o fb.o OpenInput.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS) -lm

acc-bubble: acc-bubble.o fb.o OpenInput.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS) -lm

ts-rawtest: ts-rawtest.o fb.o OpenInput.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

collect-pdata: collect-pdata.o fb.o OpenInput.o Stats.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

mb: mb.o fb.o OpenInput.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

pfun: pfun.o fb.o OpenInput.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

mph: mph.o fb.o OpenInput.o drawtext.o Stats.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS) -lm

batt: batt.o fb.o drawtext.o
	$(CXX) -o $@ $^ $(LDFLAGS) 

calib-delay: calib-delay.o MaxMin.o Histo.o fb.o OpenInput.o Stats.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ts-press-demo: ts-press-demo.o MaxMin.o Histo.o fb.o OpenInput.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

