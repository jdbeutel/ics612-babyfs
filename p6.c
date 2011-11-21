/* ICS612 proj6 jbeutel 2011-11-16 */

/* project 6 file for UHM ICS 612, Spring 2007*/
/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>
#include <time.h>	/* time() */
#include <string.h>	/* memset() */
#include "p6.h"
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
	struct index_node *index;
	struct leaf_node *leaf;
	struct inode_metadata *imd;
	int devsize, i;

	devsize = dev_open();
	if (devsize < 0) return;

	if (read_block(0, super_block)) return;
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

	caches[1] = init_block(1);
	index = (struct index_node *) caches[1]->contents;
	index->header.header_magic = HEADER_MAGIC;
	index->header.blocknr = 1;
	index->header.type = TYPE_EXT_IDX;
	index->header.nritems = 5;
	index->header.level = 0;	/* root node */

	/* todo: bootstrap root node and then add these via basic tree ops */
	index->key_ptrs[0].key.objectid = 0;
	index->key_ptrs[0].key.type = TYPE_SUPERBLOCK;
	index->key_ptrs[0].key.offset = 1;
	index->key_ptrs[0].blocknr = 2;

	index->key_ptrs[1].key.objectid = 1;
	index->key_ptrs[1].key.type = TYPE_EXT_IDX;
	index->key_ptrs[1].key.offset = 1;
	index->key_ptrs[1].blocknr = 2;

	index->key_ptrs[2].key.objectid = 2;
	index->key_ptrs[2].key.type = TYPE_EXT_LEAF;
	index->key_ptrs[2].key.offset = 1;
	index->key_ptrs[2].blocknr = 2;

	index->key_ptrs[3].key.objectid = 3;
	index->key_ptrs[3].key.type = TYPE_FS_IDX;
	index->key_ptrs[3].key.offset = 1;
	index->key_ptrs[3].blocknr = 2;

	index->key_ptrs[4].key.objectid = 4;
	index->key_ptrs[4].key.type = TYPE_FS_LEAF;
	index->key_ptrs[4].key.offset = 1;
	index->key_ptrs[4].blocknr = 2;

	caches[2] = init_block(2);
	leaf = (struct leaf_node *) caches[2]->contents;
	leaf->header.header_magic = HEADER_MAGIC;
	leaf->header.blocknr = 2;
	leaf->header.type = TYPE_EXT_LEAF;
	leaf->header.nritems = 5;
	leaf->header.level = 1;

	leaf->items[0].key.objectid = 0;
	leaf->items[0].key.type = TYPE_SUPERBLOCK;
	leaf->items[0].key.offset = 1;
	leaf->items[0].offset = BLOCKSIZE;	/* not in block */
	leaf->items[0].size = 0;

	leaf->items[1].key.objectid = 1;
	leaf->items[1].key.type = TYPE_EXT_IDX;
	leaf->items[1].key.offset = 1;
	leaf->items[1].offset = BLOCKSIZE;	/* not in block */
	leaf->items[1].size = 0;

	leaf->items[2].key.objectid = 2;
	leaf->items[2].key.type = TYPE_EXT_LEAF;
	leaf->items[2].key.offset = 1;
	leaf->items[2].offset = BLOCKSIZE;	/* not in block */
	leaf->items[2].size = 0;

	leaf->items[3].key.objectid = 3;
	leaf->items[3].key.type = TYPE_FS_IDX;
	leaf->items[3].key.offset = 1;
	leaf->items[3].offset = BLOCKSIZE;	/* not in block */
	leaf->items[3].size = 0;

	leaf->items[4].key.objectid = 4;
	leaf->items[4].key.type = TYPE_FS_LEAF;
	leaf->items[4].key.offset = 1;
	leaf->items[4].offset = BLOCKSIZE;	/* not in block */
	leaf->items[4].size = 0;

	caches[3] = init_block(3);
	index = (struct index_node *) caches[3]->contents;
	index->header.header_magic = HEADER_MAGIC;
	index->header.blocknr = 3;
	index->header.type = TYPE_FS_IDX;
	index->header.nritems = 1;
	index->header.level = 0;	/* root node */

	index->key_ptrs[0].key.objectid = ROOT_DIR_INODE;	/* root dir */
	index->key_ptrs[0].key.type = TYPE_INODE;
	index->key_ptrs[0].key.offset = INODE_KEY_OFFSET;
	index->key_ptrs[0].blocknr = 4;

	caches[4] = init_block(4);
	leaf = (struct leaf_node *) caches[4]->contents;
	leaf->header.header_magic = HEADER_MAGIC;
	leaf->header.blocknr = 4;
	leaf->header.type = TYPE_FS_LEAF;
	leaf->header.nritems = 1;
	leaf->header.level = 1;

	leaf->items[0].key.objectid = ROOT_DIR_INODE;
	leaf->items[0].key.type = TYPE_INODE;
	leaf->items[0].key.offset = INODE_KEY_OFFSET;
	leaf->items[0].size = sizeof(*imd);
	leaf->items[0].offset = BLOCKSIZE - leaf->items[0].size;
	imd = (struct inode_metadata *) (((char *)leaf) + leaf->items[0].offset);
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
