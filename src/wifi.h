#ifndef WIFI_H
#define WIFI_H

#include "barf.h"

#define DEF_FG 0xff242223
#define DEF_BG 0xff86879a

int wifi_init(component *this);

int wifi_run(void);

void wifi_clean(void);

#endif
