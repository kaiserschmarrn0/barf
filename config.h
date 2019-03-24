#include <string.h>
#include <unistd.h>
#include <time.h>
#include <alsa/asoundlib.h>

#define ICON_MAX 16
#define TEXT_MAX 16

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 34

#define BKGCOL 0xc03b4252

typedef union {

} block_data;

typedef struct component {
	//specified

	void *(*run) (void *ref);

	const int start;
	const int width;

	const int clickable;
	void (*click) (xcb_button_press_event_t *ev);
	
	void (*cleanup) (struct component *ref);

	//handled

	XftColor fg;
	xcb_gcontext_t bg;

	block_data data;
} component;

//functions that you probably want to use here
static XftColor xft_color(uint32_t hex);
static xcb_gcontext_t change_gc(xcb_gcontext_t gc, uint32_t hex);
static void draw_block(component *block, char *icon, char *text);

static const char *icon_font_name = "Font Awesome:size=14:autohint=true:antialias=true";
static const char *text_font_name = "IBM Plex Sans:size=12:autohint=true:antialias=true";

static void *battery_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xffeceff4);
	change_gc(this->bg, 0xffbf616a);

	FILE *current;

	int bat_energy_full;
	int bat_1_energy_full;
	
	current = fopen("/sys/class/power_supply/BAT0/energy_full", "r");
	fscanf(current, "%d", &bat_energy_full);
	fclose(current);
	
	current = fopen("/sys/class/power_supply/BAT1/energy_full", "r");
	fscanf(current, "%d", &bat_1_energy_full);
	fclose(current);

	bat_energy_full += bat_1_energy_full;
	
	for (;;) {
		int bat_energy_now;
		int bat_power_now;

		int bat_1_energy_now;
		int bat_1_power_now;

		current = fopen("/sys/class/power_supply/BAT0/energy_now", "r");
		fscanf(current, "%d", &bat_energy_now);
		fclose(current);
		current = fopen("/sys/class/power_supply/BAT0/power_now", "r");
		fscanf(current, "%d", &bat_power_now);
		fclose(current);
		
		current = fopen("/sys/class/power_supply/BAT1/energy_now", "r");
		fscanf(current, "%d", &bat_1_energy_now);
		fclose(current);
		current = fopen("/sys/class/power_supply/BAT1/power_now", "r");
		fscanf(current, "%d", &bat_1_power_now);
		fclose(current);
		
		bat_energy_now += bat_1_energy_now;
		bat_power_now += bat_1_power_now;

		int perc = 100 * (float)bat_energy_now / (float)bat_energy_full;

		char text[TEXT_MAX];

		snprintf(text, TEXT_MAX, "    %d", perc);

		char icon[ICON_MAX];

		if (perc > 80) {
			sprintf(icon, "\uf240");
		} else if (perc > 60) {
			sprintf(icon, "\uf241");
		} else if (perc > 40) {
			sprintf(icon, "\uf242");
		} else if (perc > 20) {
			sprintf(icon, "\uf243");
		} else {
			sprintf(icon, "\uf244");
		}

		draw_block(this, icon, text);	
		sleep(60);
	}
}

static void *clock_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xffffffff);
	change_gc(this->bg, 0xc03b4252);
	
	for(;;) {
		time_t t = time(NULL);
		struct tm *date = localtime(&t);

		char text[TEXT_MAX];
		strftime(text, TEXT_MAX, "%b %d %H:%M", date);

		draw_block(this, "\0", text);

		sleep(5);
	}
}

static void *bright_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	int max_bright;

	FILE *current = fopen("/sys/class/backlight/intel_backlight/max_brightness", "r");
	fscanf(current, "%d", &max_bright);
	fclose(current);

	for (;;) {
		int bright;

		current = fopen("/sys/class/backlight/intel_backlight/brightness", "r");
		fscanf(current, "%d", &bright);
		fclose(current);

		char icon[ICON_MAX];
		float frac = bright / max_bright;
		if (frac > 0.7) {
			sprintf(icon, "\uf005");
		} else if (frac > 0.6) {
			sprintf(icon, "\uf006");
		} else {
			sprintf(icon, "\uf123");
		}
		
		draw_block(this, icon, "\0");
		
		sleep(1);
	}
}

static void *wifi_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	for (;;) {
		char state[TEXT_MAX];
		FILE *current = fopen("/sys/class/net/wlp3s0/operstate", "r");
		fscanf(current, "%s", &state);
		fclose(current);

		char icon[ICON_MAX];
		if (strcmp(state, "up")) {
			sprintf(icon, "\uf08a");
		} else {
			sprintf(icon, "\uf004");
		}
		
		draw_block(this, icon, "\0");

		sleep(1);
	}
}

static void *ac_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	for (;;) {
		FILE *ac = fopen("/sys/class/power_supply/AC/online", "r");
		int online;
		fscanf(ac, "%d", &online);

		char icon[ICON_MAX];

		if (online) {
			sprintf(icon, "\uf0e7");
		} else {
			sprintf(icon, "\uf1e6");
		}

		draw_block(this, icon, "\0");

		sleep(1);
	}
}

static void *sound_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);
	
	for (;;) {
		snd_mixer_t *handle;
		snd_mixer_open(&handle, 0);
		snd_mixer_attach(handle, "default");
		snd_mixer_selem_register(handle, NULL, NULL);
		snd_mixer_load(handle);

		snd_mixer_selem_id_t *sid;
		snd_mixer_selem_id_alloca(&sid);
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, "Master");
		snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

		long vol_min, vol_max;
		snd_mixer_selem_get_playback_volume_range(elem, &vol_min, &vol_max);
		long vol;
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol);
		long perc = 100 * (vol - vol_min) / vol_max;
		
		char text[TEXT_MAX];
		sprintf(text, "    %d", perc);

		int not_mute;
		snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &not_mute);
		
		snd_mixer_close(handle);

		char icon[ICON_MAX];
		if (not_mute) {
			sprintf(icon, "\uf028");
		} else {
			sprintf(icon, "\uf026");
		}

		draw_block(this, icon, text);

		sleep(1);
	}
}

static component blocks[] = {
	//draw         x                   w    c  clk   cln
	{ bright_run,  0,                  128, 0, NULL, NULL },
	{ wifi_run,    128,                32,  0, NULL, NULL },
	{ ac_run,      128 + 32,           128, 0, NULL, NULL },
	{ clock_run,   1920 / 2 - 256 / 2, 256, 0, NULL, NULL },
	{ sound_run,   1920 - 200 - 200,   200, 0, NULL, NULL },
	{ battery_run, 1920 - 200,         200, 0, NULL, NULL },
};
