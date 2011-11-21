/* ICS612 proj6 jbeutel 2011-11-20 */

#include <errno.h>	/* errno */
#include <string.h>	/* memset() */
#include <assert.h>	/* assert() */
#include "babyfs.h"

PUBLIC struct cache *init_node(
		blocknr_t blocknr, uint16_t type, uint16_t level) {
	struct cache *c = init_block(blocknr);
	if (c) {
		c->u.node.header.header_magic = HEADER_MAGIC;
		c->u.node.header.blocknr = blocknr;
		c->u.node.header.type = type;
		c->u.node.header.nritems = 0;
		c->u.node.header.level = level;
	}
	return c;
}

PUBLIC blocknr_t mkfs_alloc_block(struct root *extent_root, blocknr_t nearby) {
	return nearby + 1;	/* just while making the extent tree */
}

PRIVATE struct key *key_for(struct cache *node, int slot) {
	assert(slot < node->u.node.header.nritems);
	if (INDEX_TYPE(node->u.node.header.type)) {
		return &node->u.node.u.key_ptrs[slot].key;
	} else if (LEAF_TYPE(node->u.node.header.type)) {
		return &node->u.node.u.items[slot].key;
	} else {
		assert(FALSE);
	}
}

PRIVATE blocknr_t ptr_for(struct cache *node, int slot) {
	assert(slot < node->u.node.header.nritems);
	assert(INDEX_TYPE(node->u.node.header.type));
	return node->u.node.u.key_ptrs[slot].blocknr;
}

PRIVATE int compare_keys(struct key *k1, struct key *k2) {
	if (k1->objectid < k2->objectid) return -1;
	if (k1->objectid > k2->objectid) return 1;
	assert(k1->objectid == k2->objectid);
	if (k1->type < k2->type) return -1;
	if (k1->type > k2->type) return 1;
	assert(k1->type == k2->type);
	if (k1->offset < k2->offset) return -1;
	if (k1->offset > k2->offset) return 1;
	assert(k1->offset == k2->offset);
	return 0;
}

/* searches a tree for a key, or the slot where it should be inserted.
 * (This function is modeled after the one in Btrfs.)
 * r - the root of the tree to search
 * key - the key to search for
 * p - path result found/prepared
 * ins_len - number of bytes needed for the item and its metadata in leaf
 *	(if inserting), or negative if deleting.  When inserting,
 *	index nodes along the path and the leaf node are proactively split
 *	if at the upper bounds, or to provide the required space in the leaf.
 *	When deleting, index nodes (below root) at the lower bounds
 *	along the path are proactively fixed.
 * returns 0 if the key is found, 1 if not, or a negative errno.
 */
PUBLIC int search_slot(struct root *r, struct key *key, struct path *p,
						int ins_len) {
	blocknr_t bnr = r->blocknr;

	memset(p, 0, sizeof(*p));	/* to have NULLs after the root node */
	while (TRUE) {
		int i, level, comparison;
		struct cache *node = get_block(bnr);
		struct header *hdr;
		if (!node) return -errno;
		hdr = &node->u.node.header;
		assert(hdr->header_magic == HEADER_MAGIC);
		level = hdr->level;
		p->nodes[level] = node;
		if (!hdr->nritems) {
			/* special case; empty root node during mkfs */
			struct cache *leaf;
			blocknr_t leafnr;
			assert(level == 1);
			assert(!p->nodes[2]);
			assert(ins_len > 0);
			p->slots[1] = 0;
			leafnr = r->fs_info->alloc_block(&r->fs_info->extent_root, bnr);
			leaf = init_node(leafnr, LEAF_TYPE_FOR(hdr->type), 0);
			if (!leaf) return -errno;
			insert_key_ptr(r, node, p->slots[1], key, leafnr);
			p->nodes[0] = leaf;
			p->slots[0] = 0;
			return KEY_NOT_FOUND;
		}
		/* otherwise a node is never empty */
		for (i = 0; i < hdr->nritems; i++) {
			comparison = compare_keys(key_for(node, i), key);
			if (comparison > 0) break;
		}
		p->slots[level] = i ? i - 1 : 0;
		if (level == 0) {		/* leaf node */
			if (ins_len > 0) {
				ensure_leaf_space(r, p, ins_len);
			}
			comparison = compare_keys(key_for(node, p->slots[level]), key);
			return comparison ? KEY_NOT_FOUND : KEY_FOUND;
		} else {				/* index node */
			bnr = ptr_for(node, p->slots[level]);
			if (ins_len > 0) {		/* inserting */
				if (i == 0) {		/* key goes leftmost */
					update_key_ptr(r, node, p->slots[level], key, bnr);
				}
				if (hdr->nritems >= UPPER_BOUNDS(r->fs_info->lower_bounds)) {
					split_node(r, node);
				}
			}
			if (ins_len < 0) {		/* deleting */
				if (hdr->nritems <= r->fs_info->lower_bounds) {
					fix_node(r, node);
				}
			}
		}
	}
}

/* vim: set ts=4 sw=4: */
