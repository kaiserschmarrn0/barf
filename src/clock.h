#ifndef CLOCK_H
#define CLOCK_H

#include "barf.h"

int clock_init(component *this);

int clock_run(void);

void clock_clean(void);

#endif
