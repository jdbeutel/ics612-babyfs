/* ICS612 proj6 jbeutel 2011-12-06 */

#include <assert.h>
#include "p6.h"

int main (int argc, char ** argv) {
	int ret;

	set_time(0xaaaaaaaa);
	my_mkfs();
	ret = my_mkdir("/foo");
	assert(!ret);
	return ret;
}

/* vim: set ts=4 sw=4 tags=tags: */
