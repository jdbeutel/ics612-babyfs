/* ICS612 proj6 jbeutel 2011-11-15 */
/* device block caching and shadowing */

#include <assert.h>	/* assert() */
#include <string.h>	/* memset() */
#include <errno.h>	/* ENOBUFS */
#include "cache.h"

#define CACHE_COUNT 100
static struct cache caches[CACHE_COUNT];
static struct cache *lru;	/* least-recently-used (free) list */
static struct cache *mru;	/* most-recently-used; other end of the list */

static int caches_initialized = FALSE;
PRIVATE void init_caches() {
	int i;
	if (!caches_initialized) {
		lru = &caches[0];
		lru->less_recently_used = NULL;
		lru->more_recently_used = &caches[1];
		for (i = 1; i < CACHE_COUNT - 1; i++) {
			caches[i].less_recently_used = &caches[i - 1];
			caches[i].more_recently_used = &caches[i + 1];
		}
		caches[CACHE_COUNT - 1].less_recently_used = &caches[CACHE_COUNT - 2];
		caches[CACHE_COUNT - 1].more_recently_used = NULL;
		mru = &caches[CACHE_COUNT - 1];

		dev_open();
		caches_initialized = TRUE;
	}
}

PRIVATE struct cache *find_cache_for(blocknr_t n) {
	struct cache *c;
	int i;

	for (i = 0; i < CACHE_COUNT; i++) {
		c = &caches[i];
		if ((c->read_blocknr == n && c->was_read)
		|| (c->write_blocknr == n && c->will_write)) {
			return c;
		}
	}
	return NULL;
}

PRIVATE void remove_lru(struct cache *c) {
	assert(!c->users);
	if (!c->less_recently_used) {
		assert(c == lru);
		lru = c->more_recently_used;
	} else {
		c->less_recently_used->more_recently_used = c->more_recently_used;
	}
	if (!c->more_recently_used) {
		assert(c == mru);
		mru = c->less_recently_used;
	} else {
		c->more_recently_used->less_recently_used = c->less_recently_used;
	}
}

/* gets a block from the device or cache. */
PUBLIC struct cache *get_block(blocknr_t blocknr) {
	init_caches();
	assert(dev_open() > blocknr);
}

/* initializes a new block for writing for the first time.
 * write_blocknr - already allocated in extent tree by caller
 */ 
PUBLIC struct cache *init_block(blocknr_t write_blocknr) {
	struct cache *c;
	int hash_code;

	init_caches();
	assert(dev_open() > write_blocknr);
	c = find_cache_for(write_blocknr);
	if (c) {
		/* caller couldn't have allocated that block were it in use */
		assert(!c->users);
	} else {
		if (!lru) {
			flush_all();	/* try to free up some dirty ones */
			if (!lru) {
				errno = ENOBUFS;	/* all are in use */
				return NULL;
			}
		}
		c = lru;
	}
	remove_lru(c);
	memset(c, 0, sizeof(*c));
	c->was_read = FALSE;	/* just being explicit */
	c->will_write = TRUE;
	c->write_blocknr = write_blocknr;
	c->users = 1;
	return c;
}

/* marks a block for writing, so it represents both the read and write block.
 * write_blocknr - already allocated in extent tree by caller
 */ 
PUBLIC int shadow_block_to(struct cache *c, blocknr_t write_blocknr) {	
	assert(caches_initialized);
	assert(dev_open() > write_blocknr);
}

/* decrements users count, to allow flushing */
PUBLIC int put_block(struct cache *c) {
	assert(caches_initialized);
}

/* writes all blocks that will_write and are not in use. */
PUBLIC int flush_all() {
	assert(caches_initialized);
}

/* vim: set ts=4 sw=4: */
