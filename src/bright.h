#ifndef BRIGHT_H
#define BRIGHT_H

#include "barf.h"

#define DEF_FG 0xff242223
#define DEF_BG 0xff86879a

int bright_init(component *this);

int bright_run_icon(void);

void bright_clean(void);

int bright_click(void);

#endif
