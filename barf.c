#include <stdio.h>
#include <string.h>

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xft/Xft.h>

#include "config.h"

#define LEN(A) sizeof(A)/sizeof(*A)

typedef union {
	struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	uint32_t v;
} rgba;

static xcb_connection_t *conn;
static Display *dpy;
static xcb_screen_t *scr;
static xcb_window_t bar;
static xcb_pixmap_t pm;

static xcb_gcontext_t bgc;
static xcb_visualid_t vis;
static Visual *vis_ptr;
static xcb_colormap_t cm;
static uint32_t depth;

static XftFont *icon_font;
static XftFont *text_font;

static XftDraw *draw;

static pthread_mutex_t lock;

static uint32_t xcb_color(uint32_t hex) {
	rgba col;
	col.v = hex;

	if (!col.a)
		return 0U;

	col.r = (col.r * col.a) / 255;
	col.g = (col.g * col.a) / 255;
	col.b = (col.b * col.a) / 255;

	return col.v;
}

static XftColor xft_color(uint32_t hex) {
	rgba col;
	col.v = xcb_color(hex);

	XRenderColor rc;
	rc.red = col.r * 65535 / 255;
	rc.green = col.g * 65535 / 255;
	rc.blue = col.b * 65535 / 255;
	rc.alpha = col.a * 65535 / 255;

	XftColor ret;
	XftColorAllocValue(dpy, vis_ptr, cm, &rc, &ret);

	return ret;
}

static xcb_gcontext_t create_gc(uint32_t hex) {
	uint32_t vals[2];
	vals[1] = 0;
	vals[0] = xcb_color(hex);

	xcb_gcontext_t ret = xcb_generate_id(conn);
	xcb_create_gc(conn, ret, bar, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, vals);

	return ret;
}

static xcb_gcontext_t change_gc(xcb_gcontext_t gc, uint32_t hex) {
	hex = xcb_color(hex);
	xcb_change_gc(conn, gc, XCB_GC_FOREGROUND, &hex);
}

static void flip_block(component *block) {
	xcb_copy_area(conn, pm, bar, bgc, block->start, 0, block->start, 0, block->width, BAR_H);
}

static void flip() {
	for (int i = 0; i < LEN(blocks); i++) {
		flip_block(&blocks[i]);	
	}
}

static void xft_string(int x, XftColor *fg, XftFont *font, char *str, int len) {
	int y = (BAR_H + font->ascent - font->descent) / 2;
	XftDrawStringUtf8(draw, fg, font, x, y, (XftChar8 *)str, len);
}

static int xft_width(XftFont *font, char *str, int len) {
	XGlyphInfo ret;
	XftTextExtentsUtf8(dpy, font, (XftChar8 *)str, len, &ret);
	return ret.width;
}

static void draw_block(component *block, char *icon, char *text) {
	pthread_mutex_lock(&lock);

	xcb_rectangle_t rect;

	rect.x = block->start;
	rect.y = 0;
	rect.width = block->width;
	rect.height = BAR_H;

	xcb_poly_fill_rectangle(conn, pm, block->bg, 1, &rect);
	
	int text_len; 
	int icon_len;

	XGlyphInfo ret;
	int icon_width = 0;
	int text_width = 0;

	if (icon[0]) {
		icon_len = strlen(icon);
		icon_width = xft_width(icon_font, icon, icon_len);
	}
	if (text[0]) {
		text_len = strlen(text);
		text_width = xft_width(text_font, text, text_len);
	}

	int location = block->start + (block->width - (icon_width + text_width)) / 2;

	if (icon[0]) {
		xft_string(location, &block->fg, icon_font, icon, icon_len);
	}
	if (text[0]) {
		xft_string(location + icon_width, &block->fg, text_font, text, text_len);
	}

	flip_block(block);

	xcb_flush(conn);
	
	pthread_mutex_unlock(&lock);
}

static inline void connect(void) {
	dpy = XOpenDisplay(0);
	conn = XGetXCBConnection(dpy);
	XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
	scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	
	XVisualInfo xv, *res = NULL;
	xv.depth = 32;
	int flag = 0;
	res = XGetVisualInfo(dpy, VisualDepthMask, &xv, &flag);
	if (flag > 0) {
		vis_ptr = res->visual;
		vis = res->visualid;
	} else {
		vis_ptr = DefaultVisual(dpy, 0);
		vis = scr->root_visual;
	}
	
	cm = xcb_generate_id(conn);
	xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, cm, scr->root, vis);
	
	depth = (vis == scr->root_visual) ? XCB_COPY_FROM_PARENT : 32;
}

static inline void create_window(void) {
	bar = xcb_generate_id(conn);

	uint32_t vals[5];
	vals[0] = xcb_color(0xC03b4252);
	vals[1] = 0xFFFFFFFF;
 	vals[2] = 0;
	vals[3] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS;
	vals[4] = cm;

	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT |
			XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	xcb_create_window(conn, depth, bar, scr->root, BAR_X, BAR_Y, BAR_W, BAR_H, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, vis, mask, vals);
	
	const char *names[2];
	names[0] = "_NET_WM_WINDOW_TYPE";
	names[1] = "_NET_WM_WINDOW_TYPE_DOCK";
	
	xcb_atom_t atoms[LEN(names)];
	xcb_intern_atom_cookie_t cookies[LEN(names)];
	for (int i = 0; i < LEN(names); i++)
		cookies[i] = xcb_intern_atom(conn, 0, strlen(names[i]), names[i]);
	for (int i = 0; i < LEN(names); i++) {
		xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookies[i], NULL);
		if (reply) {
			atoms[i] = reply->atom;
			free(reply);
		}
	}
	
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, bar, atoms[0], XCB_ATOM_ATOM, 32, 1,
			&atoms[1]);
	
	xcb_map_window(conn, bar);
}

static inline void load_resources(void) {
	pm = xcb_generate_id(conn);
	xcb_create_pixmap(conn, depth, pm, bar, BAR_W, BAR_H);
	
	bgc = create_gc(BKGCOL);
	
	icon_font = XftFontOpenName(dpy, 0, icon_font_name);
	text_font = XftFontOpenName(dpy, 0, text_font_name);

	draw = XftDrawCreate(dpy, pm, vis_ptr, cm);
}

static void die(void) {
	XftFontClose(dpy, icon_font);
	XftFontClose(dpy, text_font);

	xcb_destroy_window(conn, bar);
	xcb_free_pixmap(conn, pm);

	for (int i = 0; i < LEN(blocks); i++) {
		xcb_free_gc(conn, blocks[i].bg);
		if(blocks[i].cleanup) {
			blocks[i].cleanup(&blocks[i]);
		}
	}
		
	xcb_free_gc(conn, bgc);
	
	xcb_disconnect(conn);

	pthread_mutex_destroy(&lock);
}

static void handle_expose(xcb_generic_event_t *ev) {
	flip();
}

static void handle_button_press(xcb_generic_event_t *ev) {
	xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;

	for (int i = 0; i < LEN(blocks); i++) {
		if (e->event_x > blocks[i].start && e->event_x < blocks[i].start + blocks[i].width) {
			printf("block %d pressed.\n", i);
		}
	}
}

int main(void) {
	atexit(die);

	connect();
	create_window();
	load_resources();

	if (pthread_mutex_init(&lock, NULL)) {
		printf("failed to lock drawing.\n");
		return 1;
	}
	
	pthread_t component_id[LEN(blocks)];
	for (int i = 0; i < LEN(blocks); i++) {
		blocks[i].bg = create_gc(0xffffffff);	
		pthread_create(&component_id[i], NULL, blocks[i].run, (void *)&blocks[i]);
	}

	void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *event);
	
	for (int i = 0; i < XCB_NO_OPERATION; i++) {
		events[i] = NULL;
	}

	events[XCB_EXPOSE]       = handle_expose;
	events[XCB_BUTTON_PRESS] = handle_button_press;

	xcb_generic_event_t *ev = NULL;
	for (; !xcb_connection_has_error(conn);) {
		xcb_flush(conn);
		ev = xcb_wait_for_event(conn);

		int index = ev->response_type & ~0x80;
		if (events[index]) {
			events[index](ev);
		}

		free(ev);
	}
	
	return 0;
}
