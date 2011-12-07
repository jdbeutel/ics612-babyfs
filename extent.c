/* ICS612 proj6 jbeutel 2011-11-20 */

#include <stdio.h>
#include <errno.h>	/* ENOSPC */
#include "babyfs.h"

/* finds an unallocated key hole large enough to hold block_count */
PRIVATE blocknr_t find_free_extent(struct root *ext_rt, blocknr_t nearby,
									uint32_t block_count) {
	struct key first;
	struct key *key;
	struct path p;
	blocknr_t block_after_extent;
	int32_t free_blocks;	/* may be negative for overlapping extents */
	int ret;

	first.objectid = 0;		/* todo: start from nearby? */
	first.type = 0;
	first.offset = 0;
	ret = search_slot(ext_rt, &first, &p, 0);
	if (ret < 0) return ret;	/* todo: use a signed type for return value? */
	key = key_for(p.nodes[0], p.slots[0]);
	while (TRUE) {
		block_after_extent = key->objectid + key->offset;
		ret = step_to_next_slot(&p);
		if (ret < 0) return ret;
		if (ret > 0) {	/* no more items */
			free_blocks = ext_rt->fs_info->total_blocks - block_after_extent;
			if (free_blocks >= block_count) {
				break;
			} else {
				block_after_extent = -ENOSPC;
				break;
			}
		}
		key = key_for(p.nodes[0], p.slots[0]);
		free_blocks = key->objectid - block_after_extent;
		if (free_blocks >= block_count) {
			break;
		}
	}
	if (ret == 0) {		/* then the next slot is on the path */
		free_path(&p);
	}
	return block_after_extent;
}

PUBLIC blocknr_t mkfs_alloc_block(struct root *ext_rt, blocknr_t nearby,
									uint16_t type) {
	printf("debug: mkfs_alloc_block %d\n", nearby + 1);
	return nearby + 1;	/* just while making the extent tree */
}

static blocknr_t currently_allocating = 0;

PUBLIC blocknr_t normal_alloc_block(struct root *ext_rt, blocknr_t nearby,
									uint16_t type) {
	uint32_t block_count = 1;
	blocknr_t b, previously_allocating;

	printf("debug: normal_alloc_block near %d\n", nearby);
	b = find_free_extent(ext_rt, nearby, block_count);
	if (currently_allocating != b) {	/* not recursing on extent tree */
		previously_allocating = currently_allocating;
		currently_allocating = b;
		insert_extent(ext_rt, b, type, block_count);
		currently_allocating = previously_allocating;
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
