/* ICS612 proj6 jbeutel 2011-12-04 */

#include "babyfs.h"

PRIVATE time_t test_time = 0;	/* for deterministic test results */

PUBLIC void set_time(time_t t) {
	test_time = t;
}

PUBLIC time_t get_time() {
	return test_time ? test_time : time(NULL);
}

/* vim: set ts=4 sw=4 tags=tags: */
