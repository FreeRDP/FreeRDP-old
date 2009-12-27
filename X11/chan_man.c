
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freerdp.h"
#include "chan_man.h"
#include "vchan.h"

struct lib_data
{
	int fd;
	PVIRTUALCHANNELENTRY entry;
};

struct chan_data
{
	char name[CHANNEL_NAME_LEN + 1]
};

static struct lib_data g_libs[CHANNEL_MAX_COUNT];
static int g_num_libs;
static struct chan_data g_chans[CHANNEL_MAX_COUNT];
static int g_num_chans;

/* this is called shortly after the application starts and
   before any other function in the file */
int
chan_man_init(void)
{
	memset(g_libs, 0, sizeof(g_libs));
	memset(g_chans, 0, sizeof(g_chans));
	g_num_libs = 0;
	g_num_chans = 0;
	return 0;
}

/* this is called when processing the command line parameters */
int
chan_man_load_plugin(rdpSet * settings, const char * filename)
{
	return 0;
}
