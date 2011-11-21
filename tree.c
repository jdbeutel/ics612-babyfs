/* ICS612 proj6 jbeutel 2011-11-20 */

#include "babyfs.h"

struct cache *init_node(blocknr_t blocknr, uint16_t type, uint16_t level) {
	struct cache *c;
	struct node *node;

	c = init_block(blocknr);
	node = (struct node *) c->contents;
	node->header.header_magic = HEADER_MAGIC;
	node->header.blocknr = blocknr;
	node->header.type = type;
	node->header.nritems = 0;
	node->header.level = level;
	return c;
}
