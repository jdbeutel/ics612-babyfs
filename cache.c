/* ICS612 proj6 jbeutel 2011-11-15 */
/* device block caching and shadowing */

#include <assert.h>	/* assert() */
#include <string.h>	/* memset() */
#include <errno.h>	/* ENOBUFS */
#include "babyfs.h"	/* struct cache, etc */

/* Babyfs needs enough cache to hold all modified tree nodes for a single op,
 * to avoid the risk of thrashing while updating the extent tree.
 * This compilation unit has no knowledge of trees, but the change required
 * for one operation is generally at least one path from root to leaf
 * on both extent and FS tree, which have a maximum depth of 6 nodes each.
 * In addition to those 12 nodes, branches from the paths (up to 5 nodes)
 * may be required for proactive balancing of the nodes along the paths,
 * deallocation of blocks, and ops that change multiple keys that span
 * multiple nodes (e.g., when deleting a file with many extents).
 * 200 nodes of cache seems like more than enough for a single operation
 * on a tree with bounds 20..60.
 *
 * File data must be written after the modifications
 * of all trees are completed, so that the amount of
 * file data may exceed the cache.  The cache size
 * is limited by the project 6 limitation of using a fixed amount of RAM
 * "on the order of 1MB or (much) less."  (That would be on the order of 1000
 * nodes of cache.)  The superblock must be updated last, after all blocks
 * are put back and flushed, so none of the changes will become effective
 * if the write is interrupted.
 */

#define CACHE_COUNT 200
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

/* finds a cache that was read from or will write to the given block number.
 * It is the responsibility of the caller of the functions in this
 * compilation unit to not request a block for reading or writing
 * that isn't already allocated (in the extent tree), so the same
 * block number will never be read from and written to at the same time
 * (except for the superblock).
 *
 * Future enhancements to this project could use additional RAM
 * for caches while it's available, and use hash lists for read
 * and write block numbers to avoid the linear search of a large
 * number of caches.
 */
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

/* removes the given cache from anywhere in the least-recently-used list */
PRIVATE void remove_from_lru(struct cache *c) {
	struct cache *original_less_recently_used;

	assert(!c->users && !c->will_write);
	original_less_recently_used = c->less_recently_used;	/* before NULLing */
	if (!c->less_recently_used) {
		assert(c == lru);
		lru = c->more_recently_used;
	} else {
		c->less_recently_used->more_recently_used = c->more_recently_used;
		c->less_recently_used = NULL;
	}
	assert(!c->less_recently_used);
	if (!c->more_recently_used) {
		assert(c == mru);
		mru = original_less_recently_used;
	} else {
		c->more_recently_used->less_recently_used = original_less_recently_used;
		c->more_recently_used = NULL;
	}
	assert(!c->less_recently_used && !c->more_recently_used);
}

/* adds the given cache to the most-recently-used end of the LRU list */
PRIVATE void add_to_mru(struct cache *c) {
	assert(!c->users && !c->will_write);
	assert(!c->less_recently_used && !c->more_recently_used);
	if (!mru) {
		assert(!lru);	/* can't have one end of the list without the other */
		mru = c;
		lru = c;
	} else {
		assert(lru);	/* must have the other end of the list too */
		c->less_recently_used = mru;
		assert(!mru->more_recently_used);
		mru->more_recently_used = c;
		mru = c;
	}
}

/* gets a block from the cache or device.
 * blocknr - matching either read or write block.
 */ 
PUBLIC struct cache *get_block(blocknr_t blocknr) {
	struct cache *c;

	init_caches();
	assert(dev_open() > blocknr);
	c = find_cache_for(blocknr);
	if (c) {
		if (!c->users) {
			remove_from_lru(c);		/* will be used now */
		}
	} else {	/* get a free cache and read the block from the device */
		if (!lru) {
			flush_all();	/* try to free up some dirty ones */
			if (!lru) {
				errno = ENOBUFS;	/* all are in use */
				return NULL;
			}
		}
		c = lru;
		remove_from_lru(c);
		c->read_blocknr = blocknr;
		if (read_block(blocknr, c->u.contents) == FAILURE) {
			return NULL;
		}
		c->was_read = TRUE;
	}
	c->users++;
	return c;
}

/* initializes a new block for writing for the first time.
 * write_blocknr - already allocated in extent tree by caller
 */ 
PUBLIC struct cache *init_block(blocknr_t write_blocknr) {
	struct cache *c;

	init_caches();
	assert(dev_open() > write_blocknr);
	c = find_cache_for(write_blocknr);
	if (c) {	/* must have been freed by a previous generation */
		/* caller couldn't have allocated that block were it in use now */
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
	remove_from_lru(c);
	memset(c, 0, sizeof(*c));
	c->was_read = FALSE;	/* just being explicit */
	c->will_write = TRUE;
	c->write_blocknr = write_blocknr;
	c->users = 1;
	return c;
}

/* marks a block for writing, so it represents both the read and write block.
 * This is how a block is modified on disk, by copying changes to a new block.
 * c - previously gotten cache to shaddow
 * write_blocknr - already allocated in extent tree by caller
 */ 
PUBLIC void shadow_block_to(struct cache *c, blocknr_t write_blocknr) {	
	assert(caches_initialized);
	assert(dev_open() > write_blocknr);
	assert(c->users && !c->will_write);
	if (write_blocknr != SUPERBLOCK_NR) {
		/* always shadow to a new block (except for the superblock) */
		assert(!c->was_read || write_blocknr != c->read_blocknr);
	}
	c->will_write = TRUE;
	c->write_blocknr = write_blocknr;
}

/* decrements users count, to allow flushing and reuse of cache */
PUBLIC void put_block(struct cache *c) {
	assert(caches_initialized);
	assert(c->users);
	c->users--;
	if (!c->users && !c->will_write) {
		add_to_mru(c);	/* ready for reuse */
	}
}

/* writes all blocks that will_write and are not in use. */
PUBLIC int flush_all() {
	struct cache *c;
	int i, ret;

	assert(caches_initialized);
	for (i = 0; i < CACHE_COUNT; i++) {
		c = &caches[i];
		if (!c->users && c->will_write) {
			ret = write_block(c->write_blocknr, c->u.contents);
			if (ret)	return ret;
			c->will_write = FALSE;
			c->was_read = TRUE;
			c->read_blocknr = c->write_blocknr;
			add_to_mru(c);	/* ready for reuse */
		}
	}
	/* Future enhancements to this project could use an elevator algorithm
	 * along a write hash list with a scattered write.
	 */
	return SUCCESS;
}

PRIVATE int any_dirty() {
	int i;

	for (i = 0; i < CACHE_COUNT; i++) {
		if (caches[i].will_write) {
			return TRUE;
		}
	}
	return FALSE;
}

PUBLIC int write_superblock(struct fs_info fs_info) {
	struct cache *c;
	struct superblock *sb;

	init_caches();
	assert(!any_dirty());	/* flush_all() is called before this function */
	c = init_block(SUPERBLOCK_NR);	/* not shadowed, for my_mkfs() */
	sb = &c->u.superblock;
	sb->super_magic = SUPER_MAGIC;
	sb->version = BABYFS_VERSION;
	sb->extent_tree_blocknr = fs_info.extent_root.blocknr;
	sb->fs_tree_blocknr = fs_info.fs_root.blocknr;
	sb->total_blocks = fs_info.total_blocks;
	sb->lower_bounds = fs_info.lower_bounds;
	put_block(c);
	return flush_all();
}

/* vim: set ts=4 sw=4: */
