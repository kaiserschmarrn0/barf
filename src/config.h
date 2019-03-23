#include <string.h>

#include "barf.h"

#define ICON_MAX 16
#define TEXT_MAX 16

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 32

#define BKGCOL 0xc03b4252

static const char *icon_font_name = "Font Awesome:size=14:autohint=true:antialias=true";
static const char *text_font_name = "IBM Plex Sans:size=12:autohint=true:antialias=true";

static void star(char *icon, char *text) {
	strncpy(icon, "", ICON_MAX);
	strncpy(text, " star", TEXT_MAX);
}

static void bruh(char *icon, char *text) {
	strncpy(icon, "\0", ICON_MAX);
	strncpy(text, "Mar 22 12:00", TEXT_MAX);
}

static Block blocks[] = {
	{ star, 0, 256, 0xff3b4252, 0xff81a1c1 },
	{ bruh, 256, 512, 0xffffffff, 0xc03b4252 },
};
