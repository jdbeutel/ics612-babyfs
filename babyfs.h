/* ICS612 proj6 jbeutel 2011-11-15 */

#include <stdint.h>
#include <time.h>

typedef uint32_t blocknr_t;	/* need 21 bits for 2G in 1K blocks */
typedef uint16_t item_offset_t;	/* need 10 bits for bytes within a block */
typedef uint16_t item_size_t;	/* need 10 bits for bytes within a block */

/* nodes and keys in FS tree */
#define TYPE_INODE		10
#define TYPE_DIR_ENT		11
#define TYPE_FILE_EXTENT	12

/* nodes and keys in extent tree */
#define TYPE_SUPERBLOCK		20
#define TYPE_EXT_IDX		21
#define TYPE_EXT_LEAF		22
#define TYPE_FS_IDX		23
#define TYPE_FS_LEAF		24
#define TYPE_FILE_DATA		25

struct header {			/* starts all nodes in a tree */
	blocknr_t blocknr;	/* for testing and simple consistency check */
	uint8_t type;		/* for testing, redundant with key.type */
	uint8_t nritems;	/* populated key or item slots */
	uint8_t level;		/* number of nodes to get to root */
};

struct key {		/* in index and leaf nodes */
	uint32_t objectid;	/* inode or blocknr */
	uint8_t type;
	uint32_t offset;
};

struct key_ptr {		/* in index nodes */
	struct key key;
	blocknr_t blocknr;
};

#define NODE_PAYLOAD_BYTES	(BLOCKSIZE-sizeof(struct header))
#define MAX_KEY_PTRS	(NODE_PAYLOAD_BYTES/sizeof(struct key_ptr))
struct index_node {
	struct header header;
	struct key_ptr key_ptrs[MAX_KEY_PTRS];
};

struct item {		/* in leaf node */
	struct key key;
	item_offset_t offset;	/* of metadata, in bytes from start of block */
	item_size_t size;	/* of metadata, in bytes */
};

#define MAX_ITEMS	(NODE_PAYLOAD_BYTES/sizeof(struct item))
struct leaf_node {
	struct header header;
	struct item items[MAX_ITEMS];
};

#define INODE_DIR	1
#define INODE_FILE	2
struct inode_metadata {
	uint8_t inode_type;
	time_t ctime;
	time_t mtime;
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

#define MIN_LOWER_BOUNDS	2
#define MAX_LOWER_BOUNDS	(MAX_KEY_PTRS/3)
#define MAGIC_NUMBER	0xBABF
#define BABYFS_VERSION	0

struct superblock {	/* first block of device */
	uint32_t magic_number;
	uint8_t version;
	blocknr_t extent_tree_blocknr;	/* root */
	blocknr_t fs_tree_blocknr;	/* root */
	int total_blocks;	/* device size */
	uint8_t lower_bounds;	/* b for balancing inner nodes b..3b */
};
