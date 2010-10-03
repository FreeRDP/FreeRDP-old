
#include "CUnit/Basic.h"

#define add_test_suite(name) \
	CU_pSuite pSuite; \
	pSuite = CU_add_suite(#name, init_##name##_suite, clean_##name##_suite); \
	if (pSuite == NULL) { \
		CU_cleanup_registry(); return CU_get_error(); \
	}

#define add_test_function(name) \
	if (CU_add_test(pSuite, #name, test_##name) == NULL) { \
		CU_cleanup_registry(); return CU_get_error(); \
	}

