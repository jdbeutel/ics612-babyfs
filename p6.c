/* ICS612 proj6 jbeutel 2011-11-16 */

/* project 6 file for UHM ICS 612, Spring 2007*/
/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>

/* open an exisiting file for reading or writing */
int my_open (char * path)
{
  printf ("my_open (%s) not implemented\n", path);
  return -1;
}

/* open a new file for writing only */
int my_creat (char * path)
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
int my_write (int fd, void * buf, int count)
{
  printf ("my_write (%d, %x, %d) not implemented\n", fd, buf, count);
  return -1;
}

int my_close (int fd)
{
  printf ("my_close (%d) not implemented\n", fd);
  return -1;
}

int my_remove (char * path)
{
  printf ("my_remove (%s) not implemented\n", path);
  return -1;
}

int my_rename (char * old, char * new)
{
  printf ("my_remove (%s, %s) not implemented\n", old, new);
  return -1;
}

/* only works if all but the last component of the path already exists */
int my_mkdir (char * path)
{
  printf ("my_mkdir (%s) not implemented\n", path);
  return -1;
}

int my_rmdir (char * path)
{
  printf ("my_rmdir (%s) not implemented\n", path);
  return -1;
}

/* check to see if the device already has a file system on it,
 * and if not, create one. */
void my_mkfs ()
{
	block super_check, blocks[5];
	struct superblock *super;
	struct index_node *index;
	struct leaf_node *leaf;
	int devsize, i;

	devsize = dev_open();
	if (devsize < 0) return;

	if (read_block(0, super_check)) return;
	super = super_check;
	if (super->magic_number == MAGIC_NUMBER) {
		fprintf(stderr, "device already has this file system\n");
		return;
	}
	if (devsize < 5) {
		fprintf(stderr, "device too small (%d blocks)\n", devsize);
		return;
	}
	super = blocks[0];
	super->magic_number = MAGIC_NUMBER;
	super->version = BABYFS_VERSION;
	super->extent_tree_blocknr = 1;
	super->fs_tree_blocknr = 3;
	super->total_blocks = devsize;
	super->lower_bounds = MIN_LOWER_BOUNDS;	/* min for testing tree ops */

	index = blocks[1];
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

	leaf = blocks[2];
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

	index = blocks[3];
	index->header.blocknr = 3;
	index->header.type = TYPE_FS_IDX;
	index->header.nritems = 1;
	index->header.level = 0;	/* root node */

	index->key_ptrs[0].key.objectid = ROOT_DIR_INODE;	/* root dir */
	index->key_ptrs[0].key.type = TYPE_INODE;
	index->key_ptrs[0].key.offset = INODE_KEY_OFFSET;
	index->key_ptrs[0].blocknr = 4;

	leaf = blocks[4];
	leaf->header.blocknr = 4;
	leaf->header.type = TYPE_FS_LEAF;
	leaf->header.nritems = 1;
	leaf->header.level = 1;

	leaf->items[0].key.objectid = ROOT_DIR_INODE;
	leaf->items[0].key.type = TYPE_INODE;
	leaf->items[0].key.offset = INODE_KEY_OFFSET;
	leaf->items[0].offset = BLOCKSIZE;	/* not in block */
	leaf->items[0].size = 0;

	for (i = 1; i < sizeof(blocks); i++) {
		if (write_block(i, block[i])) return;
	}
	/* write superblock last */
	write_block(0, block[0]);
}
