/* ICS612 proj6 jbeutel 2011-11-15 */
/* device block caching and shadowing */

#include <assert.h>
#include "cache.h"

#define CACHE_COUNT 100
struct cache caches[CACHE_COUNT];
struct cache *lru;	/* least-recently-used (free) doubly linked list */
struct cache *mru;	/* most-recently-used; the other end of the list */

#define HASH_COUNT 0x100
#define HASH_MASK (HASH_COUNT - 1)
struct cache *read_hashes[HASH_COUNT];	/* hashed on read_blocknr */
struct cache *write_hashes[HASH_COUNT];	/* hashed on write_blocknr */

int caches_initialized = FALSE;
void init_caches() {
	int i;
	if (!caches_initialized) {
		lru = &caches[0];
		lru->lru_prev = NULL;
		lru->lru_next = &caches[1];
		for (i = 1; i < CACHE_COUNT - 1; i++) {
			caches[i].lru_prev = &caches[i - 1];
			caches[i].lru_next = &caches[i + 1];
		}
		caches[CACHE_COUNT].lru_prev = &caches[CACHE_COUNT - 1];
		caches[CACHE_COUNT].lru_next = NULL;

		for (i = 0; i < HASH_COUNT; i++) {
			read_hashes[i] = NULL;
			write_hashes[i] = NULL;
		}
		dev_open();
		caches_initialized = TRUE;
	}
}

/* gets a block from the device or cache. */
struct cache *get_block(blocknr_t blocknr) {
	init_caches();
	assert(dev_open() > blocknr);
}

/* initializes a new block for writing for the first time.
 * write_blocknr - allocated in extent tree by caller
 */ 
struct cache *init_block(blocknr_t write_blocknr) {
	init_caches();
	assert(dev_open() > write_blocknr);
}

/* marks a block for writing, so it represents both the read and write block.
 * write_blocknr - allocated in extent tree by caller
 */ 
int shadow_block_to(struct cache *c, blocknr_t write_blocknr) {	
	assert(caches_initialized);
	assert(dev_open() > write_blocknr);
}

/* decrements users count, to allow flushing */
int put_block(struct cache *c) {
	assert(caches_initialized);
}

/* writes all blocks that will_write and are not in use. */
int flush_all() {
	assert(caches_initialized);
}
