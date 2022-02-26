/* tsc.c
 * Access the pentium "time stamp counter"
 */

#include "intel.h"

static long long tsdata;

long long
rd_tsc ( void )
{
    	rdtscll ( tsdata );
	return tsdata;
}

void
rd_tsc_p ( long long *tp )
{
    	rdtscll ( tsdata );
	*tp = tsdata;
}

/* THE END */
