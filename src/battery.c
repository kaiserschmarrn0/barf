#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "battery.h"

static component *this;

static int energy_full = 0;

static int get_battery_energy() {
	int energy = 0;
	get_sys_int("/sys/class/power_supply/BAT0/energy_now", &energy);
	int energy_now = energy;
	get_sys_int("/sys/class/power_supply/BAT1/energy_now", &energy);
	return energy_now + energy;
}

static void get_battery_icon(int perc, char *icon) {
	switch (perc / 20) {
		case 5: sprintf(icon, "\uf240"); break;
		case 4: sprintf(icon, "\uf240"); break;
		case 3: sprintf(icon, "\uf241"); break;
		case 2: sprintf(icon, "\uf242"); break;
		case 1: sprintf(icon, "\uf243"); break;
		case 0: sprintf(icon, "\uf244"); break;
		default: sprintf(icon, "\0");
	}
}

static void battery_draw_perc(void) {
	char icon[ICON_MAX];
	char text[TEXT_MAX];

	int perc = 100 * (float)get_battery_energy() / (float)energy_full;

	get_battery_icon(perc, icon);
	
	snprintf(text, TEXT_MAX, "    %d%%", perc);

	draw_block(this, icon, text);
}

static void battery_draw_time(void) {
	char icon[ICON_MAX];
	char text[TEXT_MAX];

	int energy_now = get_battery_energy();
	int perc = 100 * (float)energy_now / (float)energy_full;
	
	get_battery_icon(perc, icon);
	
	int charging;
	get_sys_int("/sys/class/power_supply/AC/online", &charging);

	if (charging) {
		snprintf(text, TEXT_MAX, "    charging");
	} else {
		int power;
		get_sys_int("/sys/class/power_supply/BAT0/power_now", &power);
		float power_now = power;
		get_sys_int("/sys/class/power_supply/BAT1/power_now", &power);
		power_now += power;
	
		float time_remaining = (float)energy_now / (float)power_now;
		int hours = time_remaining;
		int mins = (time_remaining - hours) * 60;
	
		snprintf(text, TEXT_MAX, "    %02.2d:%02.2d left", hours, mins);
	}
	
	draw_block(this, icon, text);
}

int battery_init(component *ref) {
	this = ref;
	
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if (this->fd == -1) {
		fprintf(stderr, "Clock: couldn't create timer: %s\n", strerror(errno));
		return 1;
	}

	struct itimerspec ts;
	ts.it_interval.tv_sec = 30;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 30;
	ts.it_value.tv_nsec = 0;
		
	if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't arm timer: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	int energy;
	get_sys_int("/sys/class/power_supply/BAT0/energy_full", &energy);
	energy_full += energy;
	get_sys_int("/sys/class/power_supply/BAT1/energy_full", &energy);
	energy_full += energy;

	this->fg = xft_color(0xffeceff4);
	change_gc(this->bg, 0xffbf616a);

	this->fd = fd;

	battery_draw_perc();

	return 0;
}

static void eat_fd(int fd) {
	uint64_t timer_count = 0;
	read(fd, &timer_count, 8);
}

int battery_run_perc(void) {
	eat_fd(this->fd);

	battery_draw_perc();

	return 0;
}

int battery_run_time(void) {
	eat_fd(this->fd);

	battery_draw_time();
	
	return 0;
}

void battery_clean() {
	struct itimerspec ts;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	int fd = this->fd;
	if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't disarm timer: %s\n", strerror(errno));
	}
	
	close(fd);
}

int battery_click() {
	if (this->run == battery_run_perc) {
		this->fg = xft_color(0xffbf616a);
		change_gc(this->bg, 0xffeceff4);
		battery_draw_time();
		this->run = battery_run_time;
	} else {
		this->fg = xft_color(0xffeceff4);
		change_gc(this->bg, 0xffbf616a);
		battery_draw_perc();
		this->run = battery_run_perc;
	}

	return 0;
}
