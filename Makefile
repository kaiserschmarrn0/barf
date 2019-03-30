PREFIX = /usr/local

.PHONY: barf

all: barf

barf:
	gcc src/*.c -o barf -I/usr/include/freetype2 -lX11 -lxcb -lxcb-randr -lxcb-ewmh -lXft -lfreetype -lX11-xcb

install: barf
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f barf $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/barf

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/barf $(OBJ)

clean:
	rm -f barf
