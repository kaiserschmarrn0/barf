#ifndef CONFIG_H
#define CONFIG_H

#include "bright.h"
#include "wifi.h"
#include "clock.h"
#include "battery.h"
#include "volume.h"
#include "ac.h"

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 34

#define BKGCOL 0xc03b4252

static const char *icon_font_name = "Font Awesome:size=14:autohint=true:antialias=true";
static const char *text_font_name = "IBM Plex Sans:size=12:autohint=true:antialias=true";

static component blocks[] = {
	//draw         x                   wdth c? click
	{ bright_init,  bright_run_icon,  bright_clean,  0,            128, bright_click  },
	{ wifi_init,    wifi_run,         wifi_clean,    128,          32,  NULL          },
	{ ac_init,      ac_run,           ac_clean,      128+32,       128, NULL          },
	{ clock_init,   clock_run,        clock_clean,   1920/2-256/2, 256, NULL          },
	{ volume_init,  volume_run,       volume_clean,  1920 - 400,   200, NULL          },
	{ battery_init, battery_run_perc, battery_clean, 1920 - 200,   200, battery_click },
};

#endif
