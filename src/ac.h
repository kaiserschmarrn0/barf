#ifndef AC_H
#define AC_H

#include "barf.h"

#define DEF_FG 0xff242223
#define DEF_BG 0xff86879a

int ac_init(component *this);

int ac_run();

void ac_clean();

#endif
