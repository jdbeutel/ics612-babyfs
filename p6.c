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
	struct fs_info fs_info;
	struct cache *caches[5];
	struct cache *sb;
	struct node *node;
	struct inode_metadata *imd;
	int devsize, i;

	sb = get_block(SUPERBLOCK_NR);
	if (!sb)	return;
	if (sb->u.superblock.super_magic == SUPER_MAGIC) {
		fprintf(stderr, "device already has this file system\n");
		return;
	}
	put_block(sb);

	devsize = dev_open();
	if (devsize < 5) {
		fprintf(stderr, "device too small (%d blocks)\n", devsize);
		return;
	}
	fs_info.total_blocks = devsize;
	fs_info.lower_bounds = MIN_LOWER_BOUNDS;	/* min for testing tree ops */
	fs_info.alloc_block = mkfs_alloc_block;	/* for bootstrapping extent tree */

	caches[1] = init_node(1, TYPE_EXT_IDX, 1);	/* root node */
	fs_info.extent_root.node = caches[1];
	fs_info.extent_root.blocknr = caches[1]->write_blocknr;
	fs_info.extent_root.fs_info = &fs_info;

	/* todo: bootstrap root node and then add these via basic tree ops */
	insert_extent(&fs_info.extent_root, 0, TYPE_SUPERBLOCK, 1);
	insert_extent(&fs_info.extent_root, 1, TYPE_EXT_IDX, 1);
	insert_extent(&fs_info.extent_root, 2, TYPE_EXT_LEAF, 1);
	insert_extent(&fs_info.extent_root, 3, TYPE_FS_IDX, 1);
	insert_extent(&fs_info.extent_root, 4, TYPE_FS_LEAF, 1);

	caches[3] = init_node(3, TYPE_FS_IDX, 1);	/* root node */
	fs_info.fs_root.node = caches[3];
	fs_info.fs_root.blocknr = caches[3]->write_blocknr;
	fs_info.fs_root.fs_info = &fs_info;
	node = &caches[3]->u.node;
	node->header.nritems = 1;

	node->u.key_ptrs[0].key.objectid = ROOT_DIR_INODE;	/* root dir */
	node->u.key_ptrs[0].key.type = TYPE_INODE;
	node->u.key_ptrs[0].key.offset = INODE_KEY_OFFSET;
	node->u.key_ptrs[0].blocknr = 4;

	caches[4] = init_node(4, TYPE_FS_LEAF, 0);
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

	put_block(caches[1]);
	put_block(caches[3]);
	put_block(caches[4]);
	flush_all();
	write_superblock(fs_info);	/* write superblock last */
}

/* vim: set ts=4 sw=4 tags=tags: */
