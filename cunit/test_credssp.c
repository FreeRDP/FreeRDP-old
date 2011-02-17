
#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include "credssp.h"
#include "test_credssp.h"

int init_credssp_suite(void)
{
	return 0;
}

int clean_credssp_suite(void)
{
	return 0;
}

int add_credssp_suite(void)
{
	add_test_suite(credssp);

	return 0;
}

