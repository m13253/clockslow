
.PHONY: all clean install uninstall

CC=gcc
PREFIX=/usr

all: libclockslow.so

clean:
	rm -f libclockslow.so

install: libclockslow.so clockslow.sh
	install -Dm0755 libclockslow.so $(PREFIX)/lib/libclockslow.so
	install -Dm0755 clockslow.sh $(PREFIX)/bin/clockslow

uninstall:
	rm -f $(PREFIX)/bin/clockslow $(PREFIX)/lib/libclockslow.so

libclockslow.so: clockslow.c
	$(CC) -o libclockslow.so -shared -fPIC -Wall $(CFLAGS) clockslow.c -ldl -lm
