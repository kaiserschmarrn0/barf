#include <stdio.h>
#include <string.h>

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xft/Xft.h>

#define ICON_MAX 16
#define TEXT_MAX 16

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 32

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

typedef struct {
	void (*function) (char *, char*);

	int start;
	int width;

	uint32_t fg_col;
	uint32_t bg_col;

	XftColor fg;
	xcb_gcontext_t bg;
} Block;

static void star(char *icon, char *text) {
	strncpy(icon, "", ICON_MAX);
	strncpy(text, "star", TEXT_MAX);
}

static void bruh(char *icon, char *text) {
	strncpy(icon, "\0", ICON_MAX);
	strncpy(text, "bruh momentum", TEXT_MAX);
}

static XftColor xft_color(uint32_t hex);
static xcb_gcontext_t xcb_gc(uint32_t hex);

static Block blocks[] = {
	{ star, 0, 256, 0xff3b4252, 0xff81a1c1 },
	{ bruh, 256, 512, 0xff81a1c1, 0xff3b4252 },
};

static Display *dpy;
static xcb_connection_t *conn;
static xcb_screen_t *scr;
static xcb_window_t bar;
static xcb_pixmap_t pm;

static xcb_gcontext_t bgc;
static xcb_visualid_t vis;
static Visual *vis_ptr;
static xcb_colormap_t cm;
static uint32_t depth;

static char *icon_font_name = "Font Awesome:size=16:autohint=true:antialias=true";
static char *text_font_name = "IBM Plex Sans:size=16:autohint=true:antialias=true";

static XftFont *icon_font;
static XftFont *text_font;

//static XftColor fg;
static XftDraw *draw;

static void update() { 
	xcb_copy_area(conn, pm, bar, bgc, 0, 0, 0, 0, BAR_W, BAR_H);
}

static uint32_t xcb_color(uint32_t hex) {
	rgba col = (rgba) hex;

	if (!col.a)
		return 0U;

	col.r = (col.r * col.a) / 255;
	col.g = (col.g * col.a) / 255;
	col.b = (col.b * col.a) / 255;

	return col.v;
}

static XftColor xft_color(uint32_t hex) {
	rgba col = (rgba)xcb_color(hex);

	XRenderColor rc;
	rc.red = col.r * 65535 / 255;
	rc.green = col.g * 65535 / 255;
	rc.blue = col.b * 65535 / 255;
	rc.alpha = col.a * 65535 / 255;

	XftColor ret;
	XftColorAllocValue(dpy, vis_ptr, cm, &rc, &ret);

	return ret;
}

static xcb_gcontext_t xcb_gc(uint32_t hex) {
	uint32_t vals[2];
	vals[1] = 0;
	vals[0] = xcb_color(hex);

	xcb_gcontext_t ret = xcb_generate_id(conn);
	xcb_create_gc(conn, ret, bar, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, vals);

	return ret;
}

static void die(void) {
	xcb_disconnect(conn);

	XftFontClose(dpy, icon_font);
	XftFontClose(dpy, text_font);

	xcb_destroy_window(conn, bar);
	xcb_free_pixmap(conn, pm);

	for (int i = 0; i < LEN(blocks); i++) {
		xcb_free_gc(conn, blocks[i].bg);
	}
		
	xcb_free_gc(conn, bgc);
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

static inline void draw_text(XftColor *fg, int x, char *icon, char *text) {
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

	int location = x - (icon_width + text_width) / 2;

	if (icon[0])
		xft_string(location, fg, icon_font, icon, icon_len);
	if (text[0])
		xft_string(location + icon_width, fg, text_font, text, text_len);
}

static inline void fill_back(Block block) {
	xcb_rectangle_t rect;

	rect.x = block.start;
	rect.y = 0;
	rect.width = block.width;
	rect.height = BAR_H;

	xcb_poly_fill_rectangle(conn, pm, block.bg, 1, &rect);
}

static inline void draw_block(Block block) {
	fill_back(block);

	char icon[ICON_MAX];
	char text[TEXT_MAX];

	block.function(icon, text);

	draw_text(&block.fg, block.start + block.width / 2, icon, text);
}

static inline void draw_blocks(void) {
	xcb_rectangle_t rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = BAR_W;
	rect.height = BAR_H;

	xcb_poly_fill_rectangle(conn, pm, bgc, 1, &rect);

	for (int i = 0; i < LEN(blocks); i++)
		draw_block(blocks[i]);
}

static inline void connect(void) {
	dpy = XOpenDisplay(0);
	conn = XGetXCBConnection(dpy);
	XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
	scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
}

static inline void load_fonts(void) {
	icon_font = XftFontOpenName(dpy, 0, icon_font_name);
	text_font = XftFontOpenName(dpy, 0, text_font_name);
}

static inline void get_visual(void) {
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
}

static inline void load_colormap(void) {
	cm = xcb_generate_id(conn);
	xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, cm, scr->root, vis);
}

static inline void calculate_depth(void) {
	depth = (vis == scr->root_visual) ? XCB_COPY_FROM_PARENT : 32;
}

static inline void load_bar(void) {
	bar = xcb_generate_id(conn);

	uint32_t vals[5];
	vals[0] = xcb_color(0xC03b4252);
	vals[1] = 0xFFFFFFFF;
 	vals[2] = 0;
	vals[3] = XCB_EVENT_MASK_EXPOSURE;
	vals[4] = cm;

	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT |
			XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	xcb_create_window(conn, depth, bar, scr->root, BAR_X, BAR_Y, BAR_W, BAR_H, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, vis, mask, vals);
}

static inline void get_atoms(const char **names, xcb_atom_t *atoms, unsigned int count) {
	xcb_intern_atom_cookie_t cookies[count];
	for (int i = 0; i < count; i++)
		cookies[i] = xcb_intern_atom(conn, 0, strlen(names[i]), names[i]);
	for (int i = 0; i < count; i++) {
		xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookies[i], NULL);
		if (reply) {
			atoms[i] = reply->atom;
			free(reply);
		}
	}
}

static inline void register_atoms(void) {
	const char *names[2];
	names[0] = "_NET_WM_WINDOW_TYPE";
	names[1] = "_NET_WM_WINDOW_TYPE_DOCK";
	xcb_atom_t atoms[LEN(names)];
	get_atoms(names, atoms, LEN(names));
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, bar, atoms[0], XCB_ATOM_ATOM, 32, 1,
			&atoms[1]);
}

static inline void load_backbuffer(void) {
	pm = xcb_generate_id(conn);
	xcb_create_pixmap(conn, depth, pm, bar, BAR_W, BAR_H);
}

static inline void load_bgc(void) {
	bgc = xcb_generate_id(conn);
	uint32_t vals[2];
	vals[1] = 0;
	vals[0] = xcb_color(0xc03b4252);
	xcb_create_gc(conn, bgc, bar, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, vals);
}

static inline void load_draw(void) {
	draw = XftDrawCreate(dpy, pm, vis_ptr, cm);
}

static inline void load_blocks(void) {
	for (int i = 0; i < LEN(blocks); i++) {
		blocks[i].fg = xft_color(blocks[i].fg_col);
		blocks[i].bg = xcb_gc(blocks[i].bg_col);
	}
}

int main(void) {
	atexit(die);

	connect();
	load_fonts();
	get_visual();
	load_colormap();
	calculate_depth();
	load_bar();
	register_atoms();	
	xcb_map_window(conn, bar);
	load_backbuffer();
	load_bgc();
	load_draw();
	load_blocks();

	xcb_generic_event_t *ev = NULL;
	for (; !xcb_connection_has_error(conn);) {
		xcb_flush(conn);
		ev = xcb_wait_for_event(conn);
		draw_blocks();
 		update();
		free(ev);
	}

 	return 0;
}
