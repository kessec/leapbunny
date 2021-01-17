CROSS_COMPILE ?= arm-linux-
CFLAGS=-I$(PROJECT_PATH)/linux-2.6/include -Wall

threewire: threewire.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $^

clean:
	rm ./threewire
