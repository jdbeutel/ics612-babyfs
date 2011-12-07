/* ICS612 proj6 jbeutel 2011-12-06 */

#include "p6.h"

int main (int argc, char ** argv) {
	set_time(0xaaaaaaaa);
	my_mkfs();
	my_mkdir("/foo");
}

/* vim: set ts=4 sw=4 tags=tags: */
