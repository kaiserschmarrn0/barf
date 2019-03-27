#include <string.h>
#include <unistd.h>
#include <time.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <sys/inotify.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <X11/Xft/Xft.h>

#define ICON_MAX 16
#define TEXT_MAX 32

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 34

#define BKGCOL 0xc03b4252

typedef union {
	/* brightness */
	struct {
		xcb_connection_t *conn;
		xcb_randr_output_t monitor;
		xcb_atom_t backlight_atom;

		int max_bright;
		int min_bright;
	};
} block_data;

typedef struct component {
	/* specified */

	int (*init) (struct component *this);
	void (*run) (struct component *this);
	void (*clean) (struct component *this);

	const int start;
	const int width;

	void (*click) (struct component *this);

	/* handled */

	int fd;

	XftColor fg;
	xcb_gcontext_t bg;

	block_data data;
} component;

/* likely to be used */
XftColor xft_color(uint32_t hex);
xcb_gcontext_t change_gc(xcb_gcontext_t gc, uint32_t hex);
void draw_block(component *block, char *icon, char *text);

int bright_init(component *this);
void bright_run_icon(component *this);
void bright_clean(component *this);
void bright_click(component *this);

int clock_init(component *this);
void clock_run(component *this);
void clock_clean(component *this);

static const char *icon_font_name = "Font Awesome:size=14:autohint=true:antialias=true";
static const char *text_font_name = "IBM Plex Sans:size=12:autohint=true:antialias=true";

static void get_sys_int(char *path, int *value) {
	FILE *batfile = NULL; 
	while (!(batfile = fopen(path, "r"))) {
		fprintf(stderr, "Couldn't open file: %s\n", strerror(errno));
		sleep(1);
	}

	fscanf(batfile, "%d", value);
	fclose(batfile);
}

static void get_sys_string(char *path, char *string) {
	FILE *batfile = NULL; 
	while (!(batfile = fopen(path, "r"))) {
		fprintf(stderr, "Couldn't open file: %s\n", strerror(errno));
		sleep(1);
	}

	fscanf(batfile, "%15s", string);
	string[15] = '\0';
	fclose(batfile);
}

static component blocks[] = {
	//draw         x                   wdth c? click
	{ bright_init, bright_run_icon, bright_clean, 0, 128, bright_click },
	{ clock_init, clock_run, clock_clean, 1920/2-256/2, 256, NULL },
};
