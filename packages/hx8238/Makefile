CROSS_COMPILE ?= arm-linux-
CFLAGS=-I$(PROJECT_PATH)/linux-2.6/include -Wall

hx8238: hx8238.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $^

clean:
	rm -f ./hx8238
