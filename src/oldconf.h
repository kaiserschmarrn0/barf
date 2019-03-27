#include <string.h>
#include <unistd.h>
#include <time.h>
#include <alsa/asoundlib.h>
#include <signal.h>

#define ICON_MAX 16
#define TEXT_MAX 32

#define BAR_X 0
#define BAR_Y 0
#define BAR_W 1920
#define BAR_H 34

#define BKGCOL 0xc03b4252

typedef union {
	//percent:HHMM 0:1
	int battery_format;

	//disabled:enabled 0:1
	int lid_state;
} block_data;

typedef struct component {
	//specified

	void (*init) (component *this);
	void (*run) (component *this);
	void (*cleanup) (component *this);

	const int start;
	const int width;

	const int clickable;
	void (*click) (struct component *this);

	//handled

	XftColor fg;
	xcb_gcontext_t bg;

	block_data data;
	
	pthread_t thread_id;
} component;

//functions that you probably want to use here
static XftColor xft_color(uint32_t hex);
static xcb_gcontext_t change_gc(xcb_gcontext_t gc, uint32_t hex);
static void draw_block(component *block, char *icon, char *text);

static const char *icon_font_name = "Font Awesome:size=14:autohint=true:antialias=true";
static const char *text_font_name = "IBM Plex Sans:size=12:autohint=true:antialias=true";

static void *battery_run(void *ref) {
	static sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	component *this = (component *)ref;
	
	this->fg = xft_color(0xffeceff4);
	change_gc(this->bg, 0xffbf616a);
	
	this->data.battery_format = 0;

	FILE *current;

	int bat_energy_full;
	int bat_1_energy_full;
	
	get_sys_int("/sys/class/power_supply/BAT0/energy_full", &bat_energy_full);
	get_sys_int("/sys/class/power_supply/BAT1/energy_full", &bat_1_energy_full);
	
	bat_energy_full += bat_1_energy_full;
	
	for (;;) {
		int bat_energy_now;
		int bat_power_now;

		int bat_1_energy_now;
		int bat_1_power_now;
		
		get_sys_int("/sys/class/power_supply/BAT0/energy_now", &bat_energy_now);
		get_sys_int("/sys/class/power_supply/BAT0/power_now", &bat_power_now);
		
		get_sys_int("/sys/class/power_supply/BAT1/energy_now", &bat_1_energy_now);
		get_sys_int("/sys/class/power_supply/BAT1/power_now", &bat_1_power_now);
		
		bat_energy_now += bat_1_energy_now;
		bat_power_now += bat_1_power_now;

		char text[TEXT_MAX];
		int perc = 100 * (float)bat_energy_now / (float)bat_energy_full;
			
		if (this->data.battery_format) {
			int online;
			get_sys_int("/sys/class/power_supply/AC/online", &online);

			if (online) {
				snprintf(text, TEXT_MAX, "    charging");
			} else {
				float time_remaining = (float)bat_energy_now / (float)bat_power_now;
				int hours = time_remaining;
				int mins = (time_remaining - hours) * 60;

				snprintf(text, TEXT_MAX, "    %02.2d:%02.2d left", hours, mins);
			}
		} else {
			snprintf(text, TEXT_MAX, "    %d", perc);
		}

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

static void battery_click(component *this) {
	if (this->data.battery_format) {
		this->fg = xft_color(0xffeceff4);
		change_gc(this->bg, 0xffbf616a);
	} else {
		this->fg = xft_color(0xffbf616a);
		change_gc(this->bg, 0xffeceff4);
	}

	this->data.battery_format = !this->data.battery_format;

	pthread_kill(this->thread_id, SIGUSR1);	
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



static void *wifi_run(void *ref) {
	component *this = (component *)ref;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	for (;;) {
		char state[TEXT_MAX];
		get_sys_string("/sys/class/net/wlp3s0/operstate", state);

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
		int online;
		get_sys_int("/sys/class/power_supply/AC/online", &online);

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
		snd_mixer_t *handle = NULL;
		snd_mixer_open(&handle, 0);
		snd_mixer_attach(handle, "default");
		snd_mixer_selem_register(handle, NULL, NULL);
		snd_mixer_load(handle);

		snd_mixer_selem_id_t *sid;
		snd_mixer_selem_id_alloca(&sid);
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, "Master");
		snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

		if (!elem) {
			goto a;
		}

		long vol_min, vol_max;
		snd_mixer_selem_get_playback_volume_range(elem, &vol_min, &vol_max);
		long vol;
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol);
		long perc = 100 * (vol - vol_min) / vol_max;
		
		char text[TEXT_MAX];
		sprintf(text, "    %d", perc);

		int not_mute;
		snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &not_mute);

		char icon[ICON_MAX];
		if (not_mute) {
			sprintf(icon, "\uf028");
		} else {
			sprintf(icon, "\uf026");
		}

		draw_block(this, icon, text);
	
		a:

		if (handle) {
			snd_mixer_close(handle);
		}

		sleep(1);
	}
}

static void *lid_run(void *ref) {
	component *this = (component *)ref;

	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);
	
	for (;;) {
		char text[TEXT_MAX];
		char icon[ICON_MAX];
		get_sys_string("/etc/acpi/acpi_sleep.sh", text);
		if (strcmp(text, "zzz") == 0) {
			this->data.lid_state = 1;
			sprintf(icon, "\uf111");
		} else if (strcmp(text, "#music") == 0) {
			this->data.lid_state = 0;
			sprintf(icon, "\uf001");
		} else {
			this->data.lid_state = 1;
			sprintf(icon, "\uf059");	
		}

		draw_block(this, icon, "\0");
	
		pause();
	}
}

static void lid_click(component *this) {
	system("st -g 40x1+0+0 -e sudo acpi_sleep_helper /etc/acpi/acpi_sleep.sh");

	pthread_kill(this->thread_id, SIGUSR1);
}

static component blocks[] = {
	//draw         x                   wdth c? click          cln
	{ lid_run,     0,                  128, 1, lid_click,     NULL },
	{ wifi_run,    128,                32,  0, NULL,          NULL },
	{ ac_run,      128 + 32,           128, 0, NULL,          NULL },
	{ clock_run,   1920 / 2 - 256 / 2, 256, 0, NULL,          NULL },
	{ sound_run,   1920 - 200 - 200,   200, 0, NULL,          NULL },
	{ battery_run, 1920 - 200,         200, 1, battery_click, NULL },
	
	//{ bright_run,  0,                  128, 0, NULL,          NULL },
};
