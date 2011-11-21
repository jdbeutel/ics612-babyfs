/* ICS612 proj6 jbeutel 2011-11-20 */

#include "babyfs.h"

struct cache *init_node(blocknr_t blocknr, uint16_t type, uint16_t level) {
	struct cache *c;

	c = init_block(blocknr);
	c->u.node.header.header_magic = HEADER_MAGIC;
	c->u.node.header.blocknr = blocknr;
	c->u.node.header.type = type;
	c->u.node.header.nritems = 0;
	c->u.node.header.level = level;
	return c;
}
