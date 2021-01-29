OBJS=ts-myinput ts-tstest ts-input-noise ts-input-fuzz ts-input-bandwidth \
	mpvl ts-ts-input ts-myinputraw ts-ts-noise ts-both-noise ts-gestures \
	list-input-devices ts-polltest mpvl-layers mpvl-fb mpvl-fb-split \
	ts-var-data ts-var-send ac-avg-raw ac-avg ac-unit-circle
INCLUDES=-I$(PROJECT_PATH)/linux-2.6/include -I./usr/include
CPPFLAGS=$(INCLUDES)
LDFLAGS=-L./usr/lib
LDLIBS=-lts -lm

CALIB=new-calib

.PHONY: all $(CALIB)

all: $(OBJS) $(CALIB)

CC=arm-linux-gcc
CXX=arm-linux-g++ -g

$(CALIB):
	$(MAKE) --directory=$@

install: all
	$(MAKE) --directory=$(CALIB) install
	mkdir -p $(ROOTFS_PATH)/usr/bin
	cp $(OBJS) $(ROOTFS_PATH)/usr/bin

post: all
	$(MAKE) --directory=$(CALIB) post
	echo cp -v $(OBJS) ~lfu
	rm -f ~lfu/tslib.tar ~lfu/ts.tar
	tar -cf ~lfu/tslib.tar usr/lib
	tar -cf ~lfu/ts.tar $(OBJS)

clean:
	$(MAKE) --directory=$(CALIB) clean
	rm -f $(OBJS) *.o *~
