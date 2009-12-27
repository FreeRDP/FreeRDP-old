
#ifndef __CHAN_MAN_H
#define __CHAN_MAN_H

#include "freerdp.h"

int
chan_man_init(void);
int
chan_man_load_plugin(rdpSet * settings, const char * filename);
int
chan_man_pre_connect(struct rdp_inst * inst);
int
chan_man_post_connect(struct rdp_inst * inst);
int
chan_man_data(struct rdp_inst * inst, int chan_id, char * data,
	int data_size, int flags, int total_size);

#endif
