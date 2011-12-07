/* ICS612 proj6 jbeutel 2011-11-20 */

#include <errno.h>	/* ENOENT */
#include <assert.h>	/* assert() */
#include <string.h>	/* memmove() */
#include "babyfs.h"

PRIVATE void set_inode_key(uint32_t inode, struct key *key) {
	key->objectid = inode;
	key->type = TYPE_INODE;
	key->offset = INODE_KEY_OFFSET;
}

PUBLIC int insert_inode(struct fs_info *fsi, uint32_t inode,
						uint16_t inode_type) {
	struct path p;
	struct key key;
	struct inode_metadata *imd; 
	int ins_len = sizeof(struct item) + sizeof(*imd); 
	int ret;

	set_inode_key(inode, &key);
	ret = insert_empty_item(&fsi->fs_root, &key, &p, ins_len);
	if (ret) return ret;

	imd = (struct inode_metadata *) metadata_for(&p);
	imd->inode_type = inode_type;
	imd->ctime = get_time();
	imd->mtime = get_time();

	free_path(&p);
	return SUCCESS;
}

PUBLIC int get_inode_metadata(struct fs_info *fsi, uint32_t inode,
								struct inode_metadata *imd) {
	struct path p;
	struct key key;
	int ret;

	set_inode_key(inode, &key);
	ret = search_slot(&fsi->fs_root, &key, &p, 0);
	if (ret == KEY_NOT_FOUND) ret = -ENOENT;
	if (ret == KEY_FOUND) {
		memmove(imd, metadata_for(&p), sizeof(*imd));
	}
	if (ret == KEY_FOUND || ret == KEY_NOT_FOUND) {
		free_path(&p);
	}
	assert(KEY_FOUND == SUCCESS);
	return ret;
}

PRIVATE uint32_t hash(char *name) {
	uint32_t hash = 0;
	int len = strlen(name);
	int i;
	for (i = 0; i < len; i++) {
		hash = 31 * hash + name[i];
	}
	return hash;
}

PRIVATE void set_dir_ent_key(uint32_t dir_inode, char *name, struct key *key) {
	key->objectid = dir_inode;
	key->type = TYPE_DIR_ENT;
	key->offset = hash(name);
}

/* gets the inode of a directory entry, or 0 if errno.
 * (inode 0 is the root dir, which is not an entry in any dir)
 */
PUBLIC uint32_t get_dir_ent_inode(struct fs_info *fsi, uint32_t dir_inode,
									char *name) {
	struct path p;
	struct key key;
	struct dir_ent_metadata *demd;
	int ret;

	set_dir_ent_key(dir_inode, name, &key);
	ret = search_slot(&fsi->fs_root, &key, &p, 0);
	if (ret == KEY_NOT_FOUND) {
		free_path(&p);	/* only for search_slot(), not step_to_next_slot() */
	}
	while (TRUE) {
		if (ret == KEY_NOT_FOUND) {
			errno = ENOENT;
		}
		if (ret < 0) {
			errno = -ret;
		}
		if (ret != KEY_FOUND) {
			return 0;	/* failure, with errno */
		}
		demd = (struct dir_ent_metadata *) metadata_for(&p);
		if (!strcmp(name, demd->name)) {	/* name matches */
			ret = demd->inode;
			free_path(&p);
			return ret;
		}
		ret = step_to_next_slot(&p);
		if (ret == KEY_FOUND && compare_keys(&key, item_key(&p))) {
			errno = ENOENT;
			free_path(&p);
			return 0;	/* failure, with errno */
		}
	}
}

/* gets the lowest free inode, or 0 if errno.
 * (inode 0 is the root dir, which is never free)
 * not concerned with optimizations at this point in the project.
 */
PUBLIC uint32_t find_free_inode(struct fs_info *fsi) {
	struct key first;
	struct key *key;
	struct path p;
	uint32_t prev_inode;
	int ret;

	set_inode_key(0, &first);
	ret = search_slot(&fsi->fs_root, &first, &p, 0);
	if (ret < 0) {
		errno = -ret;
		return 0;
	}
	key = key_for(p.nodes[0], p.slots[0]);
	while (TRUE) {
		prev_inode = key->objectid;
		ret = step_to_next_slot(&p);
		if (ret < 0) {
			errno = -ret;
			return 0;
		}
		if (ret == KEY_NOT_FOUND) {	/* no more items, and no path to free */
			return prev_inode + 1;
		}
		assert(ret == SUCCESS);
		key = key_for(p.nodes[0], p.slots[0]);
		if (key->objectid - prev_inode > 1) {
			free_path(&p);
			return prev_inode + 1;
		}
	}
}

/* inserts a directory entry.
 * dir_inode - inode of the directory in which to insert
 * name - of the inserted file or directory
 * ent_inode - inode of the inserted file or directory
 * returns 0 for success, or negative errno.
 */
PUBLIC int insert_dir_ent(struct fs_info *fsi, uint32_t dir_inode,
								char *name, uint32_t ent_inode) {
	struct path p;
	struct key key;
	struct dir_ent_metadata *demd;
	int len = strlen(name), ret;

	if (len > MAX_FILE_NAME_LENGTH) {
		return -ENAMETOOLONG;
	}
	set_dir_ent_key(dir_inode, name, &key);
	ret = insert_empty_item_allowing_duplicates(&fsi->fs_root, &key, &p,
							sizeof(struct item) + sizeof(*demd) + len);
	if (ret) return ret;
	demd = (struct dir_ent_metadata *) metadata_for(&p);
	demd->inode = ent_inode;
	memmove(&demd->name, name, len);
	free_path(&p);
	return 0;	/* failure, with errno */
}

/* vim: set ts=4 sw=4 tags=tags: */
