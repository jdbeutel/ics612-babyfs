/* ICS612 proj6 jbeutel 2011-11-20 */

#include <stdio.h>
#include <assert.h>	/* assert() */
#include <errno.h>	/* ENOSPC */
#include "babyfs.h"

#define BLOCK_RESERVES	100
PRIVATE struct block_reserve {
	blocknr_t start;
	uint16_t type;
	uint32_t block_count;
} block_reserves[BLOCK_RESERVES];
PRIVATE int n_block_reserves = 0;

/* finds the reserved extent of blocks following the given.
 */
PRIVATE struct block_reserve *find_reserve(blocknr_t start_inclusive) {
	struct block_reserve *ret = NULL;
	int i;
	for (i = 0; i < n_block_reserves && i < BLOCK_RESERVES; i++) {
		if (block_reserves[i].start >= start_inclusive) {	/* candidate */
			if (!ret || block_reserves[i].start < ret->start ) {  /* closer */
				ret = &block_reserves[i];
			}
		}
	}
	return ret;
}

PRIVATE struct block_reserve *pop_reserve() {
	return n_block_reserves ? &block_reserves[--n_block_reserves] : NULL;
}

PRIVATE int push_reserve(blocknr_t start, uint16_t type, uint32_t block_count) {
	if (n_block_reserves >= BLOCK_RESERVES) {
		return -ENOSPC;
	}
	printf("debug: reserving block %d type %x\n", start, type);
	block_reserves[n_block_reserves].start = start;
	block_reserves[n_block_reserves].type = type;
	block_reserves[n_block_reserves].block_count = block_count;
	n_block_reserves++;
	return SUCCESS;
}

/* inserts extents that were deferred to avoid infinite recursion
 * on the extent tree itself.
 */
PUBLIC int insert_extents_for_reserves(struct root *ext_rt) {
	struct block_reserve *br = NULL;
	int ret = 0;

	while (!ret && (br = pop_reserve())) {
		printf("debug: inserting extent for reserve block %d (%d remaining)\n",
				br->start, n_block_reserves);
		ret = insert_extent(ext_rt, br->start, br->type, br->block_count);
	}
	return ret;
}

/* finds an unreserved extent of blocks between the given.
 * returns starting block of sufficient size, or 0 if none.
 */
PRIVATE blocknr_t find_unreserved_blocks(uint32_t block_count,
						blocknr_t start_inclusive, blocknr_t end_exclusive) {
	blocknr_t block_in_question = start_inclusive;
	while (TRUE) {
		struct block_reserve *br = find_reserve(block_in_question);
		int enough;
		if (!br || end_exclusive < br->start) {	/* no more reserves */
			enough = end_exclusive - block_in_question >= block_count;
			return enough ? block_in_question : 0;
		}
		enough = br->start - block_in_question >= block_count;
		if (enough) {
			return block_in_question;
		}
		block_in_question = br->start + br->block_count;
	}
}

/* finds an unallocated key hole large enough to hold block_count */
PRIVATE blocknr_t find_free_extent(struct root *ext_rt, blocknr_t nearby,
									uint32_t block_count) {
	struct key first;
	struct key *key;
	struct path p;
	blocknr_t block_in_question, found;
	int32_t free_blocks;	/* may be negative for overlapping extents */
	int ret;

	first.objectid = 0;		/* block 0 is always allocated for superblock */
	first.type = 0;			/* todo: start from nearby or end? */
	first.offset = 0;
	ret = search_slot(ext_rt, &first, &p, 0);
	if (ret < 0) {
		errno = -ret;
		return 0;
	}
	key = key_for(p.nodes[0], p.slots[0]);
	while (TRUE) {
		block_in_question = key->objectid + key->offset;
		ret = step_to_next_slot(&p);
		if (ret < 0) {
			errno = -ret;
			return 0;
		}
		if (ret > 0) {	/* no more items */
			found = find_unreserved_blocks( block_count,
						block_in_question, ext_rt->fs_info->total_blocks);
			if (found) {
				break;
			} else {
				errno = ENOSPC;
				found = 0;
				break;
			}
		}
		key = key_for(p.nodes[0], p.slots[0]);
		found = find_unreserved_blocks( block_count,
						block_in_question, key->objectid);
		if (found) {
			break;
		}
	}
	if (ret == 0) {		/* then the next slot is on the path */
		free_path(&p);
	}
	return found;
}

PUBLIC blocknr_t mkfs_alloc_block(struct root *ext_rt, blocknr_t nearby,
									uint16_t type) {
	printf("debug: mkfs_alloc_block %d\n", nearby + 1);
	return nearby + 1;	/* just while making the extent tree */
}

PRIVATE int recursion = 0;

PUBLIC blocknr_t normal_alloc_block(struct root *ext_rt, blocknr_t nearby,
									uint16_t type) {
	uint32_t block_count = 1;
	blocknr_t b;
	int ret;

	printf("debug: normal_alloc_block near %d (recursion level %d)\n",
			nearby, recursion);
	b = find_free_extent(ext_rt, nearby, block_count);
	printf("debug: normal_alloc_block got %d (for type %x)\n", b, type);
	if (!b) {
		assert(errno);
		return 0;
	}
	if (recursion && EXT_TYPE(type)) {	/* to avoid infinite recursion */
		push_reserve(b, type, block_count);	/* insert_extent later */
	} else {
		recursion++;
		ret = insert_extent(ext_rt, b, type, block_count);
		recursion--;
		if (ret < 0) {
			errno = -ret;
			return 0;
		}
	}
	return b;
}

PUBLIC blocknr_t alloc_block(struct fs_info *fs_info, struct cache *nearby,
							uint16_t type) {
	blocknr_t hint = nearby->will_write ? nearby->write_blocknr
						: nearby->was_read ? nearby->read_blocknr : 0;
	return fs_info->alloc_block(&fs_info->extent_root, hint, type);
}

PUBLIC int insert_extent(struct root *ext_rt, uint32_t blocknr, uint16_t type,
						uint32_t block_count) {
	struct path p;
	struct key key;
	int ret;

	key.objectid = blocknr;
	key.type = type;
	key.offset = block_count;
	ret = insert_empty_item(ext_rt, &key, &p, sizeof(struct item));
	if (ret) return ret;
	/* no metadata for project 6 extents */
	free_path(&p);
	return SUCCESS;
}

/* vim: set ts=4 sw=4 tags=tags: */
