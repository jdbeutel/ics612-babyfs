/* ICS612 proj6 jbeutel 2011-11-15 */
/* device block caching and shadowing */

#include "p6.h"
#include "babyfs.h"

struct cache {
	blocknr_t read_blocknr;		/* if was_read, where from */
	blocknr_t write_blocknr;	/* if allocated for writing */
	int users;			/* how many currently using */
	unsigned int	was_read:1,	/* was read from device */
			will_write:1;	/* allocated, needs flushing */
	block contents;
	struct cache	*less_recently_used,
			*more_recently_used;
};

extern struct cache *get_block(blocknr_t blocknr);
extern int shadow_block_to(struct cache *c, blocknr_t write_blocknr);
extern struct cache *init_block(blocknr_t write_blocknr);
extern int put_block(struct cache *c);
extern int flush_all();
