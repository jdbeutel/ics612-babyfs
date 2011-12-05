/* ICS612 proj6 jbeutel 2011-11-16 */

/* project 6 file for UHM ICS 612, Spring 2007*/
/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>
#include <assert.h>	/* assert() */
#include <errno.h>	/* ENOTSUP */
#include <string.h>	/* strchr() */
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
	struct fs_info fs_info;
	char name[MAX_FILE_NAME_LENGTH + 1];
	char *delim;
	int32_t inode = ROOT_DIR_INODE, new_inode;
	struct inode_metadata imd;
	int ret;

	ret = read_superblock(&fs_info);
	if (ret) return ret;
	if (*path++ != FILE_SEPARATOR) {	/* not an absolute path */
		return -ENOTSUP;	/* no support for current working directory */
	}
	ret = get_inode_metadata(&fs_info, inode, &imd);
	if (ret) return ret;
	if (imd.inode_type != INODE_DIR) {
		return -EUCLEAN;	/* root dir isn't a dir */
	}
	delim = strchr(path, FILE_SEPARATOR);
	while (delim) {
		if (delim - path > MAX_FILE_NAME_LENGTH) {
			return -ENAMETOOLONG;
		}
		memmove(name, path, delim - path);
		name[delim - path] = '\0';
		inode = get_dir_ent_inode(&fs_info, inode, name);
		if (!inode) return -errno;
		ret = get_inode_metadata(&fs_info, inode, &imd);
		if (ret) return ret;
		if (imd.inode_type != INODE_DIR) {
			return -ENOTDIR;	/* subdir isn't a dir */
		}
		path = delim + 1;
		delim = strchr(path, FILE_SEPARATOR);
	}
	if (strlen(path) > MAX_FILE_NAME_LENGTH) {
		return -ENAMETOOLONG;
	}
	if (!strlen(path)) {
		return -EINVAL;		/* no dir name */
	}
	if (get_dir_ent_inode(&fs_info, inode, path)) {
		return -EEXIST;
	}
	new_inode = find_free_inode(&fs_info);
	if (!new_inode) return -errno;
	ret = insert_dir_ent(&fs_info, inode, path, new_inode);
	if (ret) return ret;
	ret = insert_inode(&fs_info, new_inode, INODE_DIR);
	if (ret) return ret;
	/* todo: discard all caches on every error return? */
	ret = flush_all();
	if (ret) return ret;
	ret = write_superblock(&fs_info);	/* write superblock last */
	return ret;
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
	struct cache *cache;
	int devsize;
	blocknr_t b;

	cache = get_block(SUPERBLOCK_NR);
	if (!cache)	return;
	if (cache->u.superblock.super_magic == SUPER_MAGIC) {
		fprintf(stderr, "device already has this file system\n");
		return;
	}
	put_block(cache);

	devsize = dev_open();
	if (devsize < 5) {
		fprintf(stderr, "device too small (%d blocks)\n", devsize);
		return;
	}
	fs_info.total_blocks = devsize;
	fs_info.lower_bounds = MIN_LOWER_BOUNDS;	/* min for testing tree ops */
	fs_info.alloc_block = mkfs_alloc_block;	/* for bootstrapping extent tree */

	/* bootstrap root node of extent tree */
	cache = init_node(1, TYPE_EXT_IDX, 1);
	fs_info.extent_root.blocknr = cache->write_blocknr;
	fs_info.extent_root.fs_info = &fs_info;

	/* add extents via basic tree ops */
	insert_extent(&fs_info.extent_root, 0, TYPE_SUPERBLOCK, 1);
	insert_extent(&fs_info.extent_root, 1, TYPE_EXT_IDX, 1);
	insert_extent(&fs_info.extent_root, 2, TYPE_EXT_LEAF, 1);

	/* extents bootstrapped now, so allocate normally for FS tree */
	fs_info.alloc_block = normal_alloc_block;
	b = alloc_block(&fs_info, cache, TYPE_FS_IDX);	/* block for FS tree root */
	assert(b == 3);
	put_block(cache);	/* extent tree root node */

	/* FS tree root node */
	cache = init_node(b, TYPE_FS_IDX, 1);
	fs_info.fs_root.blocknr = cache->write_blocknr;
	fs_info.fs_root.fs_info = &fs_info;	/* circular */
	put_block(cache);

	insert_inode(&fs_info, ROOT_DIR_INODE, INODE_DIR);

	flush_all();
	write_superblock(&fs_info);	/* write superblock last */
}

/* vim: set ts=4 sw=4 tags=tags: */
