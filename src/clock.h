#ifndef CLOCK_H
#define CLOCK_H

#include "barf.h"

#define CLOCK_FG 0xffd7d7d7
#define CLOCK_BG 0xff302e2f

int clock_init(component *this);

int clock_run(void);

void clock_clean(void);

#endif
