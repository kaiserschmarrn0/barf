#ifndef WIFI_H
#define WIFI_H

#include "barf.h"

int wifi_init(component *this);

int wifi_run(component *this);

void wifi_clean(component *this);

#endif
