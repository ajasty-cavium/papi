/* This file tests uncore events on perf_event kernels
*/

#include "papi_test.h"

#define MAX_CYCLE_ERROR 30

char uncore_event[]="snbep_unc_imc0::UNC_M_CLOCKTICKS";

int
main( int argc, char **argv )
{
	int retval;
	int EventSet = PAPI_NULL;
	long long values[1];

	/* Set TESTS_QUIET variable */
	tests_quiet( argc, argv );	

	/* Init the PAPI library */
	retval = PAPI_library_init( PAPI_VER_CURRENT );
	if ( retval != PAPI_VER_CURRENT ) {
	   test_fail( __FILE__, __LINE__, "PAPI_library_init", retval );
	}

	retval = PAPI_create_eventset(&EventSet);
	if (retval != PAPI_OK) {
	   test_fail(__FILE__, __LINE__, "PAPI_create_eventset",retval);
	}

	/* we need to set a component for the EventSet */

	retval = PAPI_assign_eventset_component(EventSet, 0);

	/* we need to set to a certain cpu for uncore to work */

	PAPI_cpu_option_t cpu_opt;

	cpu_opt.eventset=EventSet;
	cpu_opt.cpu_num=0;

	retval = PAPI_set_opt(PAPI_CPU_ATTACH,&cpu_opt);
	if (retval != PAPI_OK) {
	   test_fail(__FILE__, __LINE__, "PAPI_CPU_ATTACH",retval);
	}

	/* we need to set the granularity to system-wide for uncore to work */

	PAPI_granularity_option_t gran_opt;
	
	gran_opt.def_cidx=0;
	gran_opt.eventset=EventSet;
	gran_opt.granularity=PAPI_GRN_SYS;

	retval = PAPI_set_opt(PAPI_GRANUL,&gran_opt);
	if (retval != PAPI_OK) {
	   test_fail(__FILE__, __LINE__, "PAPI_GRANUL",retval);
	}
	
	/* we need to set domain to be as inclusive as possible */
	
	PAPI_domain_option_t domain_opt;

	domain_opt.def_cidx=0;
	domain_opt.eventset=EventSet;
	domain_opt.domain=PAPI_DOM_ALL;

	retval = PAPI_set_opt(PAPI_DOMAIN,&domain_opt);
	if (retval != PAPI_OK) {
	   test_fail(__FILE__, __LINE__, "PAPI_DOMAIN",retval);
	}

	/* Add our uncore event */

	retval = PAPI_add_named_event(EventSet, uncore_event);

	if (retval != PAPI_OK) {
	  test_fail(__FILE__, __LINE__, "Error with event \n",retval);
	}

	/* Start PAPI */
	retval = PAPI_start( EventSet );
	if ( retval != PAPI_OK ) {
	   test_fail( __FILE__, __LINE__, "PAPI_start", retval );
	}

	/* our work code */
	do_flops( NUM_FLOPS );

	/* Stop PAPI */
	retval = PAPI_stop( EventSet, values );
	if ( retval != PAPI_OK ) {
	   test_fail( __FILE__, __LINE__, "PAPI_stop", retval );
	}

	if ( !TESTS_QUIET ) {
	   printf("Uncore test:\n");
	   printf("\t%s: %lld\n",uncore_event,values[0]);
	}

	test_pass( __FILE__, NULL, 0 );
	
	return 0;
}