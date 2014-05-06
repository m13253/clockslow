
.PHONY: all clean install install32 uninstall

CC=gcc
PREFIX=/usr

all: libclockslow.so

clean:
	rm -f libclockslow.so

install: libclockslow.so clockslow.sh
	install -Dm0755 libclockslow.so $(PREFIX)/lib/libclockslow.so
	install -Dm0755 clockslow.sh $(PREFIX)/bin/clockslow

install32: libclockslow32.so
	install -Dm0755 libclockslow32.so $(PREFIX)/lib32/libclockslow.so

uninstall:
	rm -f $(PREFIX)/bin/clockslow $(PREFIX)/lib/libclockslow.so  $(PREFIX)/lib32/libclockslow.so


libclockslow.so: libclockslow.c
	$(CC) -o libclockslow.so -shared -fPIC -Wall $(CFLAGS) libclockslow.c -ldl -lm

libclockslow32.so: libclockslow.c
	$(CC) -o libclockslow32.so -shared -fPIC -m32 -Wall $(CFLAGS) libclockslow.c -ldl -lm
