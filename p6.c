/* ICS612 proj6 jbeutel 2011-11-16 */

/* project 6 file for UHM ICS 612, Spring 2007*/
/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>
#include <time.h>	/* time() */
#include <string.h>	/* memset() */
#include "babyfs.h"

/* open an exisiting file for reading or writing */
int my_open (const char * path)
{
  printf ("my_open (%s) not implemented\n", path);
  return -1;
}

/* open a new file for writing only */
int my_creat (const char * path)
{
  printf ("my_creat (%s) not implemented\n", path);
  return -1;
}

/* sequentially read from a file */
int my_read (int fd, void * buf, int count)
{
  printf ("my_read (%d, %x, %d) not implemented\n", fd, buf, count);
  return -1;
}

/* sequentially write to a file */
int my_write (int fd, const void * buf, int count)
{
  printf ("my_write (%d, %x, %d) not implemented\n", fd, buf, count);
  return -1;
}

int my_close (int fd)
{
  printf ("my_close (%d) not implemented\n", fd);
  return -1;
}

int my_remove (const char * path)
{
  printf ("my_remove (%s) not implemented\n", path);
  return -1;
}

int my_rename (const char * old, const char * new)
{
  printf ("my_remove (%s, %s) not implemented\n", old, new);
  return -1;
}

/* only works if all but the last component of the path already exists */
int my_mkdir (const char * path)
{
  printf ("my_mkdir (%s) not implemented\n", path);
  return -1;
}

int my_rmdir (const char * path)
{
  printf ("my_rmdir (%s) not implemented\n", path);
  return -1;
}

/* checks to see if the device already has a file system on it,
 * and if not, creates one. */
void my_mkfs ()
{
	block super_block;
	struct cache *caches[5];
	struct superblock *sb;
	struct node *node;
	struct inode_metadata *imd;
	int devsize, i;

	devsize = dev_open();
	if (devsize < 0) return;

	if (read_block(SUPERBLOCK_NR, super_block)) return;
	sb = (struct superblock *) super_block;
	if (sb->super_magic == SUPER_MAGIC) {
		fprintf(stderr, "device already has this file system\n");
		return;
	}
	if (devsize < 5) {
		fprintf(stderr, "device too small (%d blocks)\n", devsize);
		return;
	}
	memset(super_block, 0, sizeof(super_block));
	sb->super_magic = SUPER_MAGIC;
	sb->version = BABYFS_VERSION;
	sb->extent_tree_blocknr = 1;
	sb->fs_tree_blocknr = 3;
	sb->total_blocks = devsize;
	sb->lower_bounds = MIN_LOWER_BOUNDS;	/* min for testing tree ops */

	caches[1] = init_node(1, TYPE_EXT_IDX, 0);	/* root node */
	node = &caches[1]->u.node;
	node->header.nritems = 5;

	/* todo: bootstrap root node and then add these via basic tree ops */
	node->u.key_ptrs[0].key.objectid = 0;
	node->u.key_ptrs[0].key.type = TYPE_SUPERBLOCK;
	node->u.key_ptrs[0].key.offset = 1;
	node->u.key_ptrs[0].blocknr = 2;

	node->u.key_ptrs[1].key.objectid = 1;
	node->u.key_ptrs[1].key.type = TYPE_EXT_IDX;
	node->u.key_ptrs[1].key.offset = 1;
	node->u.key_ptrs[1].blocknr = 2;

	node->u.key_ptrs[2].key.objectid = 2;
	node->u.key_ptrs[2].key.type = TYPE_EXT_LEAF;
	node->u.key_ptrs[2].key.offset = 1;
	node->u.key_ptrs[2].blocknr = 2;

	node->u.key_ptrs[3].key.objectid = 3;
	node->u.key_ptrs[3].key.type = TYPE_FS_IDX;
	node->u.key_ptrs[3].key.offset = 1;
	node->u.key_ptrs[3].blocknr = 2;

	node->u.key_ptrs[4].key.objectid = 4;
	node->u.key_ptrs[4].key.type = TYPE_FS_LEAF;
	node->u.key_ptrs[4].key.offset = 1;
	node->u.key_ptrs[4].blocknr = 2;

	caches[2] = init_node(2, TYPE_EXT_LEAF, 1);
	node = &caches[2]->u.node;
	node->header.nritems = 5;

	node->u.items[0].key.objectid = 0;
	node->u.items[0].key.type = TYPE_SUPERBLOCK;
	node->u.items[0].key.offset = 1;
	node->u.items[0].offset = BLOCKSIZE;	/* not in block */
	node->u.items[0].size = 0;

	node->u.items[1].key.objectid = 1;
	node->u.items[1].key.type = TYPE_EXT_IDX;
	node->u.items[1].key.offset = 1;
	node->u.items[1].offset = BLOCKSIZE;	/* not in block */
	node->u.items[1].size = 0;

	node->u.items[2].key.objectid = 2;
	node->u.items[2].key.type = TYPE_EXT_LEAF;
	node->u.items[2].key.offset = 1;
	node->u.items[2].offset = BLOCKSIZE;	/* not in block */
	node->u.items[2].size = 0;

	node->u.items[3].key.objectid = 3;
	node->u.items[3].key.type = TYPE_FS_IDX;
	node->u.items[3].key.offset = 1;
	node->u.items[3].offset = BLOCKSIZE;	/* not in block */
	node->u.items[3].size = 0;

	node->u.items[4].key.objectid = 4;
	node->u.items[4].key.type = TYPE_FS_LEAF;
	node->u.items[4].key.offset = 1;
	node->u.items[4].offset = BLOCKSIZE;	/* not in block */
	node->u.items[4].size = 0;

	caches[3] = init_node(3, TYPE_FS_IDX, 0);	/* root node */
	node = &caches[3]->u.node;
	node->header.nritems = 1;

	node->u.key_ptrs[0].key.objectid = ROOT_DIR_INODE;	/* root dir */
	node->u.key_ptrs[0].key.type = TYPE_INODE;
	node->u.key_ptrs[0].key.offset = INODE_KEY_OFFSET;
	node->u.key_ptrs[0].blocknr = 4;

	caches[4] = init_node(4, TYPE_FS_LEAF, 1);
	node = &caches[4]->u.node;
	node->header.nritems = 1;

	node->u.items[0].key.objectid = ROOT_DIR_INODE;
	node->u.items[0].key.type = TYPE_INODE;
	node->u.items[0].key.offset = INODE_KEY_OFFSET;
	node->u.items[0].size = sizeof(*imd);
	node->u.items[0].offset = BLOCKSIZE - node->u.items[0].size;
	imd = (struct inode_metadata *) (((char *)node) + node->u.items[0].offset);
	imd->inode_type = INODE_DIR;
	imd->ctime = time(NULL);
	imd->mtime = time(NULL);

	for (i = 1; i <= 4; i++) {
		put_block(caches[i]);
	}
	flush_all();
	/* write superblock last */
	write_block(0, super_block);
}
