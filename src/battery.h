#ifndef BATTERY_H
#define BATTERY_H

#include "barf.h"

int battery_init(component *this);

int battery_run_perc(void);
int battery_run_time(void);

void battery_clean(void);
int battery_click(void);

#endif
