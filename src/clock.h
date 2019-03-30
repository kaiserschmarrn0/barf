#ifndef CLOCK_H
#define CLOCK_H

#include "barf.h"

int clock_init(component *this);

int clock_run(component *this);

void clock_clean(component *this);

#endif
