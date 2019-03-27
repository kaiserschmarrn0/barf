PREFIX = /usr/local

.PHONY: barf

all: barf

barf:
	gcc -g src/barf.c src/bright.c src/clock.c -o barf -I/usr/include/freetype2 -lX11 -lxcb -lxcb-randr -lxcb-keysyms -lxcb-ewmh -lxcb-icccm -lXft -lfreetype -lX11-xcb -lpthread -lasound

install: barf
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f barf $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/barf

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/barf $(OBJ)

clean:
	rm -f barf
