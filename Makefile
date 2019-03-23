all: barf

barf: clean
	g++ src/barf.c -o barf -I/usr/include/freetype2 -lX11 -lxcb -lxcb-keysyms -lxcb-ewmh -lxcb-icccm -lXft -lfreetype -lX11-xcb

clean:
	touch barf
	rm barf
