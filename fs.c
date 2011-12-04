/* ICS612 proj6 jbeutel 2011-11-20 */

#include <stdio.h>
#include "babyfs.h"

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
