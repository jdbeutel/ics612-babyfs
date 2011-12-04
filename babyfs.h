/* ICS612 proj6 jbeutel 2011-11-15 */

#include <stdint.h>	/* uint32_t, etc */
#include <time.h>	/* time_t */
#include "p6.h"

/* ***************************************************
 * The following types are in RAM and on the device;
 * this project does not minimize their space.
 */

typedef uint32_t blocknr_t;	/* need 21 bits for 2G in 1K blocks */
typedef uint16_t item_offset_t;	/* need 10 bits for bytes within a block */
typedef uint16_t item_size_t;	/* need 10 bits for bytes within a block */

/* FS tree node and key types */
#define TYPE_INODE		0xf0
#define TYPE_DIR_ENT		0xf1
#define TYPE_FILE_EXTENT	0xf2

/* extent tree node and key types */
#define TYPE_SUPERBLOCK		0xe0
#define TYPE_EXT_IDX		0xe1
#define TYPE_EXT_LEAF		0xe2
#define TYPE_FS_IDX		0xe3
#define TYPE_FS_LEAF		0xe4
#define TYPE_FILE_DATA		0xe5

#define LEAF_TYPE(x)	((x) == TYPE_EXT_LEAF || (x) == TYPE_FS_LEAF)
#define INDEX_TYPE(x)	((x) == TYPE_EXT_IDX || (x) == TYPE_FS_IDX)
#define LEAF_TYPE_FOR(x)((x) == TYPE_EXT_IDX ? TYPE_EXT_LEAF : TYPE_FS_LEAF)

#define HEADER_MAGIC	0xbacababf
/* 8 bits would be enough for the type, but the compiler allocs 32 bits
 * in struct key anyway, and this lines up nicely with od -X, so I won't
 * worry about minimal format on disk for this project.
 */
struct header {			/* starts all nodes in a tree */
	uint32_t header_magic;	/* makes easy to spot in hex dumps */
	uint16_t type;		/* for testing, redundant with key.type */
	uint16_t level;		/* number of nodes down to a leaf */
	blocknr_t blocknr;	/* for testing and simple consistency check */
	uint8_t nritems;	/* populated key or item slots */
	uint8_t filler1;	/* round up to 16 bytes for neat hex dumps */
	uint16_t filler2;	/* round up to 16 bytes for neat hex dumps */
};

struct key {		/* in index and leaf nodes */
	uint32_t objectid;	/* inode or blocknr */
	uint32_t type;
	uint32_t offset;
};

struct key_ptr {		/* in index nodes */
	struct key key;
	blocknr_t blocknr;
};

struct item {		/* in leaf nodes */
	struct key key;
	item_offset_t offset;	/* of metadata, in bytes from start of block */
	item_size_t size;	/* of metadata, in bytes */
};

#define NODE_PAYLOAD_BYTES	(BLOCKSIZE-sizeof(struct header))
#define MAX_KEY_PTRS	(NODE_PAYLOAD_BYTES/sizeof(struct key_ptr))
#define MAX_ITEMS	(NODE_PAYLOAD_BYTES/sizeof(struct item))
struct node {
	struct header header;
	union u {
		struct key_ptr	key_ptrs[MAX_KEY_PTRS];	/* in index nodes */
		struct item	items[MAX_ITEMS];	/* in leaf nodes */
	} u;
};

#define INODE_DIR	0xd1
#define INODE_FILE	0xd2
struct inode_metadata {
	uint32_t inode_type;
	uint32_t filler1;	/* round up to 32 bytes for neat hex dumps */
	time_t ctime;		/* 64 bits on my x86_64 */
	uint64_t filler2;	/* lines up times in hex dump for comparison */
	time_t mtime;		/* 64 bits on my x86_64 */
};
#define ROOT_DIR_INODE		0
#define INODE_KEY_OFFSET	0

struct dir_ent_metadata {
	uint32_t inode;
	char name[1]; /* variable-sized, \0-terminated, up to 200 chars */
};

struct file_extent_metadata {
	blocknr_t blocknr;
	uint32_t size;	/* in bytes */
};

#define MIN_LOWER_BOUNDS	2	/* for testing tree ops */
#define MAX_LOWER_BOUNDS	(MAX_KEY_PTRS/3)
#define UPPER_BOUNDS(x)		((x)*3)
#define SUPER_MAGIC		0xaaa1babf
#define BABYFS_VERSION		0
#define SUPERBLOCK_NR		0

struct superblock {	/* first block of device */
	uint32_t super_magic;	/* magic number protects from my_mkfs() */
	uint8_t version;	/* 0 */
	blocknr_t extent_tree_blocknr;	/* root */
	blocknr_t fs_tree_blocknr;	/* root */
	int total_blocks;	/* device size */
	uint8_t lower_bounds;	/* b for balancing inner nodes b..3b */
};


/* ***************************************************
 * The following types are in RAM only.
 */

#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define FAILURE -1
#define KEY_FOUND 0
#define KEY_NOT_FOUND 1
#define PRIVATE static
#define PUBLIC
#define MAX_LEVEL 6

struct cache {
	unsigned int	was_read:1,	/* was read from or written to device */
			will_write:1;	/* was allocated, needs flushing */
	blocknr_t read_blocknr;		/* if was_read, where from/to */
	blocknr_t write_blocknr;	/* if allocated for writing */
	unsigned int users;		/* number currently using (recursive) */
	struct cache	*less_recently_used,
			*more_recently_used;
	union u2 {
		block contents;
		struct node node;
		struct superblock superblock;
	} u;
};

struct fs_info;
struct root {
	blocknr_t blocknr;	/* even after cache is put back */
	struct fs_info *fs_info;
};
struct fs_info {
	struct root extent_root;
	struct root fs_root;
	int total_blocks;	/* device size */
	uint8_t lower_bounds;	/* b for balancing inner nodes b..3b */
	blocknr_t (*alloc_block)(struct root *extent_root, blocknr_t nearby,
				uint16_t type);
};

/* the path from a root to a leaf.  The leaf is level 0. */
struct path {
	struct cache *nodes[MAX_LEVEL];
	int slots[MAX_LEVEL];
};

/* cache.c */
extern struct cache *get_block(blocknr_t blocknr);
extern void shadow_block_to(struct cache *c, blocknr_t write_blocknr);
extern struct cache *init_block(blocknr_t write_blocknr);
extern void put_block(struct cache *c);
extern int flush_all();
extern int write_superblock(struct fs_info fs_info);

/* tree.c */
extern struct cache *init_node(blocknr_t blocknr, uint16_t type,
				uint16_t level);
extern blocknr_t do_alloc(struct fs_info *fs_info, struct cache *nearby,
			uint16_t type);
extern blocknr_t mkfs_alloc_block(struct root *extent_root, blocknr_t nearby,
				uint16_t type);
extern blocknr_t normal_alloc_block(struct root *extent_root, blocknr_t nearby,
				uint16_t type);
extern int insert_extent(struct root *extent_root, uint32_t blocknr,
			uint16_t type, uint32_t block_count);
extern int insert_inode(struct fs_info *fsi, uint32_t inode,
			uint16_t inode_type);
extern void free_path(struct path *p);
