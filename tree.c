/* ICS612 proj6 jbeutel 2011-11-20 */

#include <errno.h>	/* errno */
#include <string.h>	/* memset() */
#include <assert.h>	/* assert() */
#include "babyfs.h"

PUBLIC struct cache *init_node( blocknr_t blocknr,
								uint16_t type, uint16_t level) {
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

PRIVATE blocknr_t do_alloc(struct root *r, struct cache *nearby) {
	blocknr_t hint = nearby->will_write ? nearby->write_blocknr
						: nearby->was_read ? nearby->read_blocknr : 0;
	return r->fs_info->alloc_block(&r->fs_info->extent_root, hint);
}

PRIVATE int is_root_level(int level, struct path *p) {
	return level == MAX_LEVEL - 1 || !p->nodes[level + 1];
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

PRIVATE int ensure_will_write(struct root *r, struct cache *node) {
	blocknr_t shadow;

	if (!node->will_write) {
		assert(node->was_read);
		shadow = do_alloc(r, node);
		if (!shadow) return -ENOSPC;
		shadow_block_to(node, shadow);
	}
	return SUCCESS;
}

PRIVATE int update_key_ptr(struct root *r, struct cache *node, int slot,
							struct key *key, blocknr_t blocknr) {
	struct key_ptr *kp = &node->u.node.u.key_ptrs[slot];
	int ret;

	ret = ensure_will_write(r, node);
	if (ret) return ret;
	kp->key.objectid = key->objectid;
	kp->key.type = key->type;
	kp->key.offset = key->offset;
	kp->blocknr = blocknr;
	return SUCCESS;
}

PRIVATE int insert_key_ptr(struct root *r, struct cache *node, int slot,
							struct key *key, blocknr_t blocknr) {
	int ret;
	int ub = UPPER_BOUNDS(r->fs_info->lower_bounds);
	struct header *hdr = &node->u.node.header;
	int move_count = hdr->nritems - slot;
	assert(move_count >= 0);
	assert(hdr->nritems < ub);	/* proactive splits guarantee room to insert */

	ret = ensure_will_write(r, node);
	if (ret) return ret;
	if (move_count) {	/* memmove() looks like it needs this guard */
		memmove(&node->u.node.u.key_ptrs[slot + 1],
				&node->u.node.u.key_ptrs[slot],
				move_count * sizeof(struct key_ptr));
	}
	return update_key_ptr(r, node, slot, key, blocknr);
}

/* splits the index node at the given level of the given path */
PRIVATE int split_index_node(struct root *r, struct path *p, int level) {
	int ret;
	int slot = p->slots[level];
	struct cache *left = p->nodes[level];
	int nritems = left->u.node.header.nritems;
	int nrmoving = nritems - nritems/2;	/* larger half moves to the right */
	struct cache *right;
	blocknr_t rightnr;

	ret = ensure_will_write(r, left);
	if (ret) return ret;
	rightnr = do_alloc(r, left);
	if (!rightnr) return -ENOSPC;
	right = init_node(rightnr, left->u.node.header.type, level);
	if (!right) return -errno;
	if(is_root_level(level, p)) {	/* no node above, so need to grow tree */
		blocknr_t new_rootnr;
		struct cache *c;

		assert(level < MAX_LEVEL - 1);	/* has room to add another level */
		new_rootnr = do_alloc(r, right);
		if (!new_rootnr) return -ENOSPC;
		c = init_node(new_rootnr, right->u.node.header.type, level + 1);
		if (!c) return -errno;
		p->nodes[level + 1] = c;
		ret = insert_key_ptr(r, c, 0, key_for(left, 0), left->write_blocknr);
		p->slots[level + 1] = 0;	/* path on the left node */
		if (ret) return ret;
		r->node = c;				/* new root node */
		r->blocknr = new_rootnr;
	}
	ret = insert_key_ptr(r, p->nodes[level + 1], p->slots[level + 1],
						key_for(left, nritems/2), rightnr);
	if (ret) return ret;
	if (slot >= nritems/2) {		/* change path to the right-hand node */
		p->slots[level + 1]++;		/* in parent node */
		p->nodes[level] = right;	/* and split level */
		p->slots[level] = slot - nritems/2;
	}
	memmove(&right->u.node.u.key_ptrs[0],
			&left->u.node.u.key_ptrs[nritems/2],
			nrmoving * sizeof(struct key_ptr));
	right->u.node.header.nritems = nrmoving;
	memset(&left->u.node.u.key_ptrs[nritems/2], 0,
			nrmoving * sizeof(struct key_ptr));
	left->u.node.header.nritems = nritems - nrmoving;
	return SUCCESS;
}

/* makes the index node larger than the lower bounds */
PRIVATE int fix_index_node(struct root *r, struct path *p, int level) {
	assert(!is_root_level(level, p)); 	/* root has no lower bounds */
	/* todo: move more key_ptrs from sibling to this node,
	 * merging siblings if necessary
	 */
	return SUCCESS;
}

/* splits the leaf node if necessary to free up ins_len bytes in it */
PRIVATE int ensure_leaf_space(struct root *r, struct path *p, int ins_len) {
	/* todo: may need a 3-way split if the slot is in the middle and
	 * the items on both sides take up too much space.  Might that require
	 * an extra proactive split of the parent node, if it had only 1 free slot?
	 */
	return SUCCESS;
}

/* searches a tree for a key, or the slot where it should be inserted.
 * (This function's signature is modeled after the one in Btrfs.)
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
	int ret;
	blocknr_t blocknr = r->blocknr;

	memset(p, 0, sizeof(*p));	/* to have NULLs after the root node */
	while (TRUE) {
		int i, level, comparison;
		struct header *hdr;
		struct cache *node = get_block(blocknr);
		if (!node) return -errno;
		hdr = &node->u.node.header;
		assert(hdr->header_magic == HEADER_MAGIC);
		level = hdr->level;
		p->nodes[level] = node;
		for (i = 0; i < hdr->nritems; i++) {
			comparison = compare_keys(key_for(node, i), key);
			if (comparison > 0) break;	/* one slot past the key */
		}
		p->slots[level] = i ? i - 1 : 0;
		if (level == 0) {		/* leaf node */
			if (ins_len > 0) {
				ret = ensure_leaf_space(r, p, ins_len);
				if (ret) return ret;
			}
			assert(hdr->nritems);	/* an empty leaf would not be preserved */
			comparison = compare_keys(key_for(node, p->slots[level]), key);
			return comparison ? KEY_NOT_FOUND : KEY_FOUND;
		} else {				/* index node */
			if (!hdr->nritems) {
				/* special case:  empty root node during mkfs */
				struct cache *leaf;	/* init an empty leaf on the path */
				blocknr_t leafnr;
				assert(level == 1);
				assert(!p->nodes[2]);
				assert(ins_len > 0);
				p->slots[level] = 0;
				leafnr = do_alloc(r, node);
				if (!leafnr) return -ENOSPC;
				leaf = init_node(leafnr, LEAF_TYPE_FOR(hdr->type), 0);
				if (!leaf) return -errno;
				ret = insert_key_ptr(r, node, p->slots[level], key, leafnr);
				if (ret) return ret;
				p->nodes[0] = leaf;
				p->slots[0] = 0;
				return KEY_NOT_FOUND;
			}
			blocknr = ptr_for(node, p->slots[level]);
			if (ins_len > 0) {		/* inserting */
				if (i == 0) {		/* make leftmost key less */
					ret = update_key_ptr(r, node, p->slots[level],
										key, blocknr);
					if (ret) return ret;
				}
				if (hdr->nritems >= UPPER_BOUNDS(r->fs_info->lower_bounds)) {
					ret = split_index_node(r, p, level);
					if (ret) return ret;
				}
			}
			if (ins_len < 0) {		/* deleting */
				if (!is_root_level(level, p)
				&& hdr->nritems <= r->fs_info->lower_bounds) {
					ret = fix_index_node(r, p, level);
					if (ret) return ret;
				}
			}
		}
	}
}

/* vim: set ts=4 sw=4: */
