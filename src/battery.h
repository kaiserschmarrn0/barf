#ifndef BATTERY_H
#define BATTERY_H

#include "barf.h"

int battery_init(component *this);

int battery_run_perc(component *this);
int battery_run_time(component *this);

void battery_clean(component *this);
int battery_click(component *this);

#endif
