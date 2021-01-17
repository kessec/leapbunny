
all: oss vol

oss: oss.c
	$(CROSS_COMPILE)gcc -o oss oss.c

vol: vol.c
	$(CROSS_COMPILE)gcc -o vol vol.c

clean:
	rm -f ./vol ./oss *.o
