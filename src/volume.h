#ifndef VOLUME_H
#define VOLUME_H

#include "barf.h"

#define DEF_FG 0xff242223
#define DEF_BG 0xff86879a

int volume_init(component *this);

int volume_run(void);

void volume_clean(void);

#endif
