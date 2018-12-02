#include <xcb/xcb.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <string.h>
#include <stdio.h>

#define FONTS_MAX 4

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 32

#define LEN(A) sizeof(A)/sizeof(*A)

typedef struct font_t {
 XftFont *font;

 int ascent,
     descent,
     width,
     height;
} font_t;

static Display *dpy;
static xcb_connection_t *conn;
static xcb_screen_t *scr;
static xcb_window_t bar;
static xcb_pixmap_t pm;

static xcb_gcontext_t fgc, bgc;
static xcb_visualid_t vis;
static Visual *vis_ptr;
static xcb_colormap_t cm;

static XftFont *fonts[FONTS_MAX];
static int fonts_len = 0;

static XftColor fg;
static XftDraw *draw;

int fuc = 0;

static void update() { 
 xcb_copy_area(conn, pm, bar, fgc, 0, 0, 0, 0, fuc, fonts[0]->height);
}

static void die() {
 xcb_disconnect(conn);
}

int main(void) {
 dpy = XOpenDisplay(0);
 conn = XGetXCBConnection(dpy);
 XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
 scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

 fonts[0] = XftFontOpenName(dpy, 0, "IBM Plex Sans:size=16:autohint=true:antialias=true");

 XVisualInfo xv, *res = NULL;
 int flag;
 res = XGetVisualInfo(dpy, VisualDepthMask, &xv, &flag);
 if (flag) {
  vis_ptr = res->visual;
  vis = res->visualid;
 } else {
  vis_ptr = DefaultVisual(dpy, 0);
  vis = scr->root_visual;
 }
 
 cm = xcb_generate_id(conn);
 xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, cm, scr->root, vis);

 XRenderColor rc = { 5 / 255 * 65535, 21 / 255 * 65535, 25 / 255 * 65535, 65535 };
 XftColorAllocValue(dpy, vis_ptr, cm, &rc, &fg);
 
 bar = xcb_generate_id(conn);
 xcb_create_window(conn, scr->root_depth, bar, scr->root, BAR_X, BAR_Y, BAR_W, BAR_H, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, scr->root_visual,
                   XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                   (uint32_t[]){ 0xFFFFFF, 1, XCB_EVENT_MASK_EXPOSURE });

 const char *names[] = {
  "_NET_WM_WINDOW_TYPE",
  "_NET_WM_WINDOW_TYPE_DOCK"
 };
 
 xcb_intern_atom_cookie_t cookies[LEN(names)];
 xcb_atom_t atoms[LEN(names)];
 xcb_intern_atom_reply_t *reply;

 for (int i = 0; i < LEN(names); i++) cookies[i] = xcb_intern_atom(conn, 0, strlen(names[i]), names[i]);
 for (int i = 0; i < LEN(names); i++) {
  reply = xcb_intern_atom_reply(conn, cookies[i], NULL);
  if (reply) {
   atoms[i] = reply->atom;
   free(reply);
  }
 }
 
 xcb_change_property(conn, XCB_PROP_MODE_REPLACE, bar, atoms[0], XCB_ATOM_ATOM, 32, 1, &atoms[1]);

 xcb_map_window(conn, bar);
 xcb_flush(conn);

 fgc = xcb_generate_id(conn);
 bgc = xcb_generate_id(conn);
 xcb_create_gc(conn, fgc, bar, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, (uint32_t[]){ 0x00051519, 0 });
 xcb_create_gc(conn, bgc, bar, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, (uint32_t[]){ 0x00FFFFFF, 0 });
 
 pm = xcb_generate_id(conn);
 XGlyphInfo ret;
 XftTextExtentsUtf8(dpy, fonts[0], (XftChar8 *)"omegalul", 8, &ret);
 fuc = ret.width;
 
 xcb_create_pixmap(conn, scr->root_depth, pm, bar, ret.width, fonts[0]->height);

 xcb_poly_fill_rectangle(conn, pm, bgc, 1, (xcb_rectangle_t[]){ { 0, 0, ret.width, fonts[0]->height } });
 
 draw = XftDrawCreate(dpy, pm, vis_ptr, cm);
 XftDrawStringUtf8(draw, &fg, fonts[0], ret.x, fonts[0]->ascent, (XftChar8 *)"omegalul", 8); 
 
 atexit(die);

 xcb_generic_event_t *ev;
 for (; !xcb_connection_has_error(conn);) {
  printf("hmm\n");
  xcb_flush(conn);
  ev = xcb_wait_for_event(conn);
  update();
 }

 return 0;
}
