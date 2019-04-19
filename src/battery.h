#ifndef BATTERY_H
#define BATTERY_H

#include "barf.h"

#define BAT_BG 0xffd65f5f
#define BAT_FG 0xffececec

int battery_init(component *this);

int battery_run_perc(void);
int battery_run_time(void);

void battery_clean(void);
int battery_click(void);

#endif
