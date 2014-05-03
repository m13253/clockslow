
.PHONY: all clean

all: libclockslow.so

clean:
	rm -f libclockslow.so

libclockslow.so: clockslow.c
	gcc -o libclockslow.so -shared -fPIC clockslow.c -ldl
