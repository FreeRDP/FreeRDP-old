
#include "CUnit/Basic.h"

#include "test_credssp.h"
#include "test_ntlmssp.h"
#include "test_libfreerdpgdi.h"
#include "test_freerdp.h"

int main(int argc, char* argv[])
{
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	add_credssp_suite();
	add_ntlmssp_suite();
	//add_libfreerdpgdi_suite();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}

