#include <errno.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "clock.h"

static component *this;

int clock_init(component *ref) {
	this = ref;
	
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if (fd == -1) {
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
		
	if (timerfd_settime(fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't arm timer: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	
	char text[TEXT_MAX];
	
	strftime(text, TEXT_MAX, "%b %d %H:%M", date);

	this->fg = xft_color(CLOCK_FG);
	change_gc(this->bg, CLOCK_BG);

	this->fd = fd;

	draw_block(this, "\0", text);
}

int clock_run(void) {
	uint64_t timer_count = 0;
	read(this->fd, &timer_count, 8);

	time_t t = time(NULL);
	struct tm *date = localtime(&t);

	char text[TEXT_MAX];
	strftime(text, TEXT_MAX, "%b %d %H:%M", date);
	
	draw_block(this, "\0", text);

	return 0;
}

void clock_clean(void) {
	struct itimerspec ts;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	if (timerfd_settime(this->fd, 0, &ts, NULL) < 0) {
		fprintf(stderr, "Clock: couldn't disarm timer: %s\n", strerror(errno));
	}
	
	close(this->fd);
}
