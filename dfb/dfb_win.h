
#ifndef __DFB_WIN_H
#define __DFB_WIN_H

#include <freerdp/freerdp.h>

int
dfb_pre_connect(rdpInst * inst);
int
dfb_post_connect(rdpInst * inst);
void
dfb_uninit(void * dfb_info);
int
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
dfb_check_fds(rdpInst * inst);

#endif /* __DFB_WIN_H */
