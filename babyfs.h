/* ICS612 proj6 jbeutel 2011-11-15 */

#include <stdint.h>
#include <time.h>

typedef uint32_t blocknr_t;
typedef uint16_t block_offset_t;
typedef uint16_t block_size_t;

struct babyfs_superblock {	/* first block of device */
	uint32_t magic_number;		/* "baby" */
	blocknr_t extent_tree_blocknr;	/* root */
	blocknr_t fs_tree_blocknr;	/* root */
	uint32_t total_blocks;	/* device size */
}

struct babyfs_header {		/* at start of all nodes in tree */
	blocknr_t blocknr;
	uint8_t flags;
	uint8_t nritems;	/* populated key or item slots */
	uint8_t level;		/* number of nodes to get to root */
}

struct babyfs_key {		/* in index and leaf nodes */
	uint32_t objectid;	/* inode or blocknr */
	uint8_t type;
	uint32_t offset;
}

#define BABYFS_TYPE_INODE	10
#define BABYFS_TYPE_DIR_ENT	11
#define BABYFS_TYPE_FILE_EXTENT	12

#define BABYFS_TYPE_SUPERBLOCK	20
#define BABYFS_TYPE_EXT_IDX	21
#define BABYFS_TYPE_EXT_LEAF	22
#define BABYFS_TYPE_FS_IDX	23
#define BABYFS_TYPE_FS_LEAF	24
#define BABYFS_TYPE_FILE_DATA	25

struct babyfs_key_ptr {		/* in index nodes */
	struct babyfs_key key;
	blocknr_t blocknr;
}

struct babyfs_item {		/* in leaf node */
	struct babyfs_key key;
	block_offset_t offset;	/* of metadata, in bytes from start of block */
	block_size_t size;	/* of metadata, in bytes */
}

struct babyfs_inode_metadata {
	uint8_t inode_type;
	time_t ctime;
	time_t mtime;
}

struct babyfs_dir_ent_metadata {
	uint32_t inode;
	char name[1]; /* variable-sized, \0-terminated, up to 200 chars */
}

struct babyfs_file_extent_metadata {
	blocknr_t blocknr;
	uint32_t size;	/* in bytes */
}
