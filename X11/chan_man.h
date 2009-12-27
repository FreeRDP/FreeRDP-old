
#ifndef __CHAN_MAN_H
#define __CHAN_MAN_H

#include "freerdp.h"

int
chan_man_init(void);
int
chan_man_load_plugin(rdpSet * settings, const char * filename);

#endif
