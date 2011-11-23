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
	struct root *er = &fs_info.extent_root;
	struct root *fr = &fs_info.fs_root;
	struct cache *caches[5];
	struct cache *sb;
	int devsize;

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

	/* bootstrap root node of extent tree */
	er->node = init_node(1, TYPE_EXT_IDX, 1);
	er->blocknr = er->node->write_blocknr;
	er->fs_info = &fs_info;

	/* add extends via basic tree ops */
	insert_extent(&fs_info, 0, TYPE_SUPERBLOCK, 1);
	insert_extent(&fs_info, 1, TYPE_EXT_IDX, 1);
	insert_extent(&fs_info, 2, TYPE_EXT_LEAF, 1);
	insert_extent(&fs_info, 3, TYPE_FS_IDX, 1);
	insert_extent(&fs_info, 4, TYPE_FS_LEAF, 1);

	/* FS tree root node */
	fr->node = init_node(3, TYPE_FS_IDX, 1);
	fr->blocknr = fr->node->write_blocknr;
	fr->fs_info = &fs_info;

	insert_inode(&fs_info, ROOT_DIR_INODE, INODE_DIR);

	put_block(er->node);
	put_block(fr->node);
	flush_all();
	write_superblock(fs_info);	/* write superblock last */
}

/* vim: set ts=4 sw=4 tags=tags: */
