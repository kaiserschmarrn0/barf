#ifndef WIFI_H
#define WIFI_H

#include "barf.h"

int wifi_init(component *this);

int wifi_run(void);

void wifi_clean(void);

#endif
