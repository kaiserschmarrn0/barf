#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "ac.h"

static void ac_draw(component *this) {
	int charging;
	get_sys_int("/sys/class/power_supply/AC/online", &charging);
	
	char icon[ICON_MAX];
	if (charging) {
		sprintf(icon, "\uf0e7");
	} else {
		sprintf(icon, "\uf1e6");
	}

	draw_block(this, icon, "\0");
}

int ac_init(component *this) {
	int fd;
	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd == -1) {
		fprintf(stderr, "AC: couldn't create timer: %s\n", strerror(errno));
		return 1;
	}
	
	struct itimerspec ts;
	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;
		
	if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "AC: couldn't arm timer: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	this->fd = fd;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	ac_draw(this);
	
	return 0;
}

static void eat_fd(component *this) {
	uint64_t timer_count = 0;
	read(this->fd, &timer_count, 8);
}

int ac_run(component *this) {
	eat_fd(this);

	ac_draw(this);	

	return 0;
}

void ac_clean(component *this) {
	struct itimerspec ts;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	if (timerfd_settime(this->fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "AC: couldn't disarm timer: %s\n", strerror(errno));
	}
	
	close(this->fd);
}
