
.PHONY: all clean

CC=gcc

all: libclockslow.so

clean:
	rm -f libclockslow.so

libclockslow.so: clockslow.c
	$(CC) -o libclockslow.so -shared -fPIC -Wall clockslow.c -ldl -lm
