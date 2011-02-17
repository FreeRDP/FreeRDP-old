#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/freerdp.h>

struct ext_test_plugin
{
	rdpExtPlugin plugin;

	int ext_test_data;
};
typedef struct ext_test_plugin extTestPlugin;

static int
exttest_init(rdpExtPlugin * plugin, rdpInst * inst)
{
	extTestPlugin * exttest = (extTestPlugin *) plugin;

	printf("exttest_init: data = %d\n", exttest->ext_test_data);
	return 0;
}

static int
exttest_uninit(rdpExtPlugin * plugin, rdpInst * inst)
{
	extTestPlugin * exttest = (extTestPlugin *) plugin;

	/* free local resoures if necessary */
	exttest->ext_test_data = 0;
	free(exttest);
	return 0;
}

static uint32 RDPEXT_CC
exttest_pre_connect(rdpExtPlugin * plugin, rdpInst * inst)
{
	extTestPlugin * exttest = (extTestPlugin *) plugin;

	printf("exttest_pre_connect: %d, %s:%d\n", exttest->ext_test_data,
		inst->settings->server, inst->settings->tcp_port_rdp);
	return 0;
}

static uint32 RDPEXT_CC
exttest_post_connect(rdpExtPlugin * plugin, rdpInst * inst)
{
	extTestPlugin * exttest = (extTestPlugin *) plugin;

	printf("exttest_post_connect: %d, %s:%d\n", exttest->ext_test_data,
		inst->settings->server, inst->settings->tcp_port_rdp);
	return 0;
}

int RDPEXT_CC
FreeRDPExtensionEntry(PFREERDP_EXTENSION_ENTRY_POINTS pEntryPoints)
{
	extTestPlugin * exttest;

	exttest = (extTestPlugin *) malloc(sizeof(extTestPlugin));
	memset(exttest, 0, sizeof(extTestPlugin));
	exttest->plugin.ext = pEntryPoints->ext;
	exttest->plugin.init = exttest_init;
	exttest->plugin.uninit = exttest_uninit;

	if (pEntryPoints->data)
	{
		exttest->ext_test_data = atoi(pEntryPoints->data);
	}
	else
	{
		exttest->ext_test_data = -1;
	}

	pEntryPoints->pRegisterExtension((rdpExtPlugin *) exttest);
	pEntryPoints->pRegisterPreConnectHook((rdpExtPlugin *) exttest, exttest_pre_connect);
	pEntryPoints->pRegisterPostConnectHook((rdpExtPlugin *) exttest, exttest_post_connect);

	return 0;
}

