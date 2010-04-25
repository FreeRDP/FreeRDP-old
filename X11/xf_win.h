
#ifndef __XF_WIN_H
#define __XF_WIN_H

#include <freerdp/freerdp.h>

int
xf_pre_connect(rdpInst * inst);
int
xf_post_connect(rdpInst * inst);
void
xf_uninit(void * xf_info);
int
xf_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
xf_check_fds(rdpInst * inst);

#endif
