/* ICS612 proj6 jbeutel 2011-11-20 */

#include <stdio.h>
#include <errno.h>	/* errno */
#include <string.h>	/* memset() */
#include <assert.h>	/* assert() */
#include "babyfs.h"

PUBLIC struct cache *init_node( blocknr_t blocknr,
								uint16_t type, uint16_t level) {
	printf("debug: init_node %d %x %d\n", blocknr, type, level);
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
	printf("debug: mkfs_alloc_block %d\n", nearby + 1);
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

PRIVATE int metadata_size_for(struct path *p) {
	struct node *leaf = &p->nodes[0]->u.node;
	int slot = p->slots[0];
	return leaf->u.items[slot].size;
}

PRIVATE void *metadata_for(struct path *p) {
	struct node *leaf = &p->nodes[0]->u.node;
	int slot = p->slots[0];
	
	if (!metadata_size_for(p)) {
		return NULL;	/* no metadata */
	}
	return ((void *)leaf) + leaf->u.items[slot].offset;
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

/* shadows the node at the given level of the given path (up to the root),
 * if not already shadowed, to allow for writing.
 */ 
PRIVATE int ensure_will_write(struct root *r, struct path *p, int level) {
	int ret;
	blocknr_t shadow;
	struct cache *node = p->nodes[level];

	if (!node->will_write) {	/* need to shadow */
		assert(node->was_read);		/* must have come from somewhere */
		shadow = do_alloc(r, node);
		if (!shadow) return -ENOSPC;
		shadow_block_to(node, shadow);
		if (!is_root_level(level, p)) {	/* need to update ptr in parent node */
			struct cache *parent = p->nodes[level + 1];
			int slot = p->slots[level + 1];
			struct key_ptr *kp = &parent->u.node.u.key_ptrs[slot];
			assert(parent->will_write);	/* was ensured on tree descent */
			assert(ptr_for(parent, slot) == node->read_blocknr);
			kp->blocknr = shadow;
		}
	}
	return SUCCESS;
}

PRIVATE void update_index_key(struct root *r, struct path *p, int level,
							struct key *key) {
	struct cache *node = p->nodes[level];
	struct key_ptr *kp = &node->u.node.u.key_ptrs[p->slots[level]];

	assert(node->will_write);	/* was ensured on tree descent */
	kp->key.objectid = key->objectid;
	kp->key.type = key->type;
	kp->key.offset = key->offset;
}

/* inserts key pointing to blocknr at the given level of the path */
PRIVATE void insert_key_ptr(struct root *r, struct path *p, int level,
							struct key *key, blocknr_t b) {
	struct cache *node = p->nodes[level];
	int slot = p->slots[level];
	struct key_ptr *kp = &node->u.node.u.key_ptrs[slot];
	int ub = UPPER_BOUNDS(r->fs_info->lower_bounds);
	struct header *hdr = &node->u.node.header;
	int move_count = hdr->nritems - slot;
	assert(move_count >= 0);
	assert(hdr->nritems < ub);	/* proactive splits guarantee room to insert */

	assert(node->will_write);	/* was ensured on tree descent */
	if (move_count) {	/* memmove() looks like it needs this guard */
		memmove(kp + 1, kp, move_count * sizeof(struct key_ptr));
	}
	kp->blocknr = b;
	hdr->nritems++;
	update_index_key(r, p, level, key);
}

PRIVATE int insert_item_in_leaf(struct root *r, struct path *p,
							struct key *key, int ins_len) {
	int slot = p->slots[0];
	struct cache *leaf = p->nodes[0];
	struct header *hdr = &leaf->u.node.header;
	struct item *ip = &leaf->u.node.u.items[slot];
	int last_offset = BLOCKSIZE;
	int item_bytes = (void *) &leaf->u.node.u.items[hdr->nritems]
								- (void *) &leaf->u.node;
	int free;
	int move_count = hdr->nritems - slot;
	int ins_metadata = ins_len - sizeof(struct item);
	int ret;

	if (hdr->nritems) {
		last_offset = leaf->u.node.u.items[hdr->nritems - 1].offset;
	}
	free = last_offset - item_bytes;
	assert(ins_len <= free);
	assert(move_count >= 0);
	assert(leaf->will_write);	/* was ensured on tree descent */
	if (move_count) {	/* memmove() looks like it needs this guard */
		int move_metadata = (ip->offset + ip->size) - last_offset;
		if (move_metadata && ins_metadata) {
			/* have metadata and it needs to travel to the left */
			memmove(((void *)&leaf->u.node) + last_offset - ins_metadata,
					((void *)&leaf->u.node) + last_offset, move_metadata);
		}
		memmove(((void *)ip) + sizeof(struct item),
				ip, move_count * sizeof(struct item));
		ip->offset = last_offset - ins_metadata + move_metadata;
	} else {
		ip->offset = last_offset - ins_metadata;
	}
	ip->size = ins_metadata;
	ip->key.objectid = key->objectid;
	ip->key.type = key->type;
	ip->key.offset = key->offset;
	hdr->nritems++;
	return SUCCESS;
}

/* splits the index node at the given level of the given path */
PRIVATE int split_index_node(struct root *r, struct path *p, int level) {
	int slot = p->slots[level];
	struct cache *left = p->nodes[level];
	int nritems = left->u.node.header.nritems;
	int nrstaying = nritems/2;			/* smaller half stays on the left */
	int nrmoving = nritems - nrstaying;	/* larger half moves to the right */
	struct cache *right;
	blocknr_t rightnr;

	assert(left->will_write);	/* was ensured on tree descent */
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
		p->slots[level + 1] = 0;	/* path on the left node */
		insert_key_ptr(r, p, level + 1,
						key_for(left, 0), left->write_blocknr);
		r->blocknr = new_rootnr;
	}
	memmove(&right->u.node.u.key_ptrs[0],	/* move larger half to right node */
			&left->u.node.u.key_ptrs[nrstaying],
			nrmoving * sizeof(struct key_ptr));
	right->u.node.header.nritems = nrmoving;
	memset(&left->u.node.u.key_ptrs[nrstaying], 0,	/* clear moved in left */
			nrmoving * sizeof(struct key_ptr));
	left->u.node.header.nritems = nrstaying;
	p->slots[level + 1]++;		/* temporarily, for inserting in parent node */
	insert_key_ptr(r, p, level + 1, key_for(right, 0), rightnr);
	if (slot >= nrstaying) {		/* need to change path to the right */
		p->nodes[level] = right;
		p->slots[level] = slot - nrstaying;
		put_block(left);			/* free left since it's now off the path */
	} else {
		p->slots[level + 1]--;		/* path back to left node in parent node */
		put_block(right);			/* free right since it's not on the path */
	}
	return SUCCESS;
}

/* makes the index node larger than the lower bounds */
PRIVATE int fix_index_node(struct root *r, struct path *p, int level) {
	assert(!is_root_level(level, p)); 	/* root has no lower bounds */
	/* todo: move more key_ptrs from sibling to this node,
	 * merging sibling if necessary
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
 * p - path result found/prepared for modification (shadowed at least)
 * ins_len - number of bytes needed for the item and its metadata in leaf
 *	(if inserting), or negative if deleting.  When inserting,
 *	index nodes on the path and the leaf node are proactively split
 *	if at the upper bounds, or to provide the required space in the leaf.
 *	When deleting, index nodes (below root) at the lower bounds
 *	on the path are proactively fixed.  When 0, the nodes on the path
 *	are not modified or shadowed.
 * returns 0 if the key is found, 1 if not, or a negative errno.
 */
PUBLIC int search_slot(struct root *r, struct key *key, struct path *p,
						int ins_len) {
	int ret;
	blocknr_t blocknr = r->blocknr;		/* start at root blocknr */
	memset(p, 0, sizeof(*p));		/* make NULL after the root level */

	/* traverse from root to leaf */
	while (TRUE) {				
		int i, j, least_key, level, comparison;
		struct header *hdr;
		struct cache *node = get_block(blocknr);
		if (!node) return -errno;
		hdr = &node->u.node.header;
		assert(hdr->header_magic == HEADER_MAGIC);
		level = hdr->level;
		p->nodes[level] = node;

		/* check for special case: empty root node during mkfs */
		if (!hdr->nritems) {
			struct cache *leaf;
			blocknr_t leafnr;
			assert(level == 1);
			assert(!p->nodes[2]);
			assert(ins_len > 0);

			p->slots[level] = 0;
			/* init first leaf and add to path, but leave empty */
			leafnr = do_alloc(r, node);
			if (!leafnr) return -ENOSPC;
			leaf = init_node(leafnr, LEAF_TYPE_FOR(hdr->type), 0);
			if (!leaf) return -errno;
			/* a new node gets a new ptr to it */
			insert_key_ptr(r, p, level, key, leafnr);
			p->nodes[0] = leaf;
			p->slots[0] = 0;
			/* the caller will insert the first item in the new leaf */
			return KEY_NOT_FOUND;
		}

		/* go one slot past the search key */
		for (i = 0; i < hdr->nritems; i++) {
			comparison = compare_keys(key_for(node, i), key);
			if (comparison > 0) break;	/* one slot past the key */
		}
		if (i) {		/* the slot to the left is equal or less */
			p->slots[level] = i - 1;
			least_key = FALSE;
		} else { 	/* the key is less than everything else in the tree */
			least_key = TRUE;
			p->slots[level] = 0;	/* it would be inserted here */
			j = level;
			while (TRUE) {
				assert(p->slots[j] == 0);
				if (is_root_level(j++, p))	break;
			}
		}

		/* if going to modify, shadow now (on tree descent) */
		if (ins_len) {
			ret = ensure_will_write(r, p, level);
			if (ret) return ret;
		}

		/* leaf node */
		if (level == 0) {
			if (ins_len > 0) {
				ret = ensure_leaf_space(r, p, ins_len);
				if (ret) return ret;
			}
			assert(hdr->nritems);	/* an empty leaf would not be preserved */
			/* so there is an item to compare with */
			comparison = compare_keys(key_for(node, p->slots[0]), key);
			if (comparison < 0) {
				p->slots[0]++;	/* would insert at next slot in leaf */
			}
			return comparison ? KEY_NOT_FOUND : KEY_FOUND;

		/* index node */
		} else {
			if (ins_len > 0) {		/* inserting */
				if (least_key) {	/* will make leftmost key less */
					update_index_key(r, p, level, key);
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
			blocknr = ptr_for(node, p->slots[level]);
		}
	}
}

/* inserts an empty item into the tree at the given key.
 * (This function's signature is modeled after the one in Btrfs.)
 * r - the root of the tree to insert into
 * key - the key to insert
 * p - path result prepared
 * ins_len - number of bytes needed for the item and its metadata in leaf.
 * returns 0 if inserted, or a negative errno.
 */
PUBLIC int insert_empty_item(struct root *r, struct key *key, struct path *p,
						int ins_len) {
	int level;
	int ret;
	ret = search_slot(r, key, p, ins_len);
	if (ret == KEY_FOUND) return -EEXIST;
	if (ret != KEY_NOT_FOUND) return ret;
	return insert_item_in_leaf(r, p, key, ins_len);
}

/* puts the nodes on the path back to the cache.
 * It's up to the owner of the path to call this.
 */
PUBLIC void free_path(struct path *p) {
	int level = 0;
	do {
		put_block(p->nodes[level]);
	} while(!is_root_level(level++, p));
}

PUBLIC int insert_extent(struct fs_info *fsi, uint32_t blocknr, uint16_t type,
						uint32_t block_count) {
	struct path p;
	struct key key;
	int ret;

	key.objectid = blocknr;
	key.type = type;
	key.offset = block_count;
	ret = insert_empty_item(&fsi->extent_root, &key, &p, sizeof(struct item));
	if (ret) return ret;
	/* no metadata for project 6 extents */
	free_path(&p);
	return SUCCESS;
}

PUBLIC int insert_inode(struct fs_info *fsi, uint32_t inode,
						uint16_t inode_type) {
	struct path p;
	struct key key;
	struct inode_metadata *imd; 
	int ins_len = sizeof(struct item) + sizeof(*imd); 
	int ret;

	key.objectid = inode;
	key.type = TYPE_INODE;
	key.offset = INODE_KEY_OFFSET;
	ret = insert_empty_item(&fsi->fs_root, &key, &p, ins_len);
	if (ret) return ret;

	imd = (struct inode_metadata *) metadata_for(&p);
	imd->inode_type = inode_type;
	imd->ctime = time(NULL);
	imd->mtime = time(NULL);

	free_path(&p);
	return SUCCESS;
}

/* vim: set ts=4 sw=4 tags=tags: */
