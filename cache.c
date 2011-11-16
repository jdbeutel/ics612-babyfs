/* ICS612 proj6 jbeutel 2011-11-15 */
/* device block caching and shadowing, inspired by Minix */

#include "p6.h"
#include "babyfs.h"

struct block {
	blocknr_t read_blocknr;
	blocknr_t write_blocknr;
	int users;		/* how many currently using */
	struct block	*lru_next,	/* next-least-recently-used */
			*lru_prev,	/* more-recently-used */
			*read_hash,	/* hash chain of read block */
			*write_hash;	/* hash chain of write block */
	unsigned int	read:1,		/* was read from device */
			allocated:1,	/* write block has been allocated */
			data:1,		/* contains file data */
			dirty:1;	/* needs flushing */
	block contents;
}

struct block blocks[100];

struct block *get_block(blocknr_t blocknr) {
}

/* ensure a block for writing, allocating in extent tree if necessary */
int shadow_block(struct block *b) {	
}

int put_block(struct block *b) {
}

int flush_all() {
}
