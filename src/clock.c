#include "barf.h"

int clock_init(component *this) {
	this->fg = xft_color(0xffffffff);
	change_gc(this->bg, 0xc03b4252);

	this->fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if (this->fd == -1) {
		fprintf(stderr, "Clock: couldn't create timer: %s\n", strerror(errno));
		return 1;
	}

	time_t t = time(NULL);
	struct tm *date;
	date = localtime(&t);

	struct itimerspec ts;
	ts.it_interval.tv_sec = 60 - date->tm_sec;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 60;
	ts.it_value.tv_nsec = 0;
		
	if (timerfd_settime(this->fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't arm timer: %s\n", strerror(errno));
		close(this->fd);
		return 1;
	}
	
	char text[TEXT_MAX];
	
	strftime(text, TEXT_MAX, "%b %d %H:%M", date);

	draw_block(this, "\0", text);
}

void clock_run(component *this) {
	uint64_t timer_count = 0;
	read (this->fd, &timer_count, 8);

	time_t t = time(NULL);
	struct tm *date = localtime(&t);

	char text[TEXT_MAX];
	strftime(text, TEXT_MAX, "%b %d %H:%M", date);
	
	draw_block(this, "\0", text);
}

void clock_clean(component *this) {
	close(this->fd);
	
	struct itimerspec ts;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	if (timerfd_settime(this->fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't disarm timer: %s\n", strerror(errno));
		close(this->fd);
	}
}
