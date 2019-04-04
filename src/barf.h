#ifndef BARF_H
#define BARF_H

#include <xcb/xcb.h>
#include <X11/Xft/Xft.h>

#define ICON_MAX 16
#define TEXT_MAX 32

typedef struct component {
	int (*init) (struct component *ref);
	int (*run) (void);
	void (*clean) (void);

	const int start;
	const int width;

	int (*click) (void);

	int fd;            //no touchy

	XftColor fg;       //no touchy
	xcb_gcontext_t bg; //no touchy
} component;

XftColor xft_color(uint32_t hex);
xcb_gcontext_t change_gc(xcb_gcontext_t gc, uint32_t hex);

void draw_block(component *block, char *icon, char *text);

void get_sys_int(char *path, int *value);
void get_sys_string(char *path, char *string);

#endif
