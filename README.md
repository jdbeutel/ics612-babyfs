ICS612 project 6 design: a file system
======================================
J. David Beutel  2011-11-13

I'll make a file system, named Babyfs, with a design similar to [Btrfs][1],
but simplified and limited to the requirements of [project 6][5].


OVERVIEW
--------

Babyfs will have two [Rodeh b-trees][4]: one for the file system,
and one for the allocation of extents of blocks on the device.
The FS tree contains metadata:  inodes, directories, and pointers to
extents of file data.  The extent tree allocates blocks on
the device for holding file data and metadata (the trees).

The superblock has a pointer to the root block of each tree.
To guarantee integrity in case of a loss of power or communication,
all modifications to data or trees are shadowed:  no blocks on the
device are overwritten; instead, changes are written to free blocks,
shadowing the changed paths of the trees and parts of the file.  Finally,
the superblock is overwritten, to point to the root blocks of the modified
versions of the trees.


B-TREE
------

The balanced trees described by [Rodeh][4] are b-plus trees,
where the metadata is kept in the leaf nodes, and the inner
nodes contain a balanced index of keys.  Rodeh makes them
"top-down" b-trees, suitable for shadowing, by eliminating
the leaf chaining convention, and by proactively splitting or fixing
index nodes that contain too many or few keys when inserting
or deleting, respectively, along the path.  The key count boundary
is b..3b where b >= 2, e.g., 2..6.  

Each node in a tree is one block (1024 bytes), and starts with
`header` (16 bytes).  Index nodes can contain up to 63 `key_ptr`
structures (16 bytes), so the boundary is 20..60, although the
tree operations can be tested with the minimal boundary, 2..6.
The keys are kept sorted in the index, and the pointer is the
block number of the node containing that key (or the next larger
key that exists in the tree).  The lower bounds doesn't apply
to a root node, which is never fixed.

The maximum depth of the index nodes is 5, based on the lower bounds
(20^5 = 3,200,000), providing a key for each block (maximum 2^21 = 2,097,152).
(I'm not sure if the upper bounds is sufficent for this, but if so,
then the maximum depth will be 4, 60^4 = 12,960,000.)  This provides enough
keys for the extent tree in the degenerate case of all tree nodes and
1-block files, and more than enough for the file system tree to hold
up to 10,000 files and directories.

Leaf nodes contain 1..63 `item` structures (16 bytes)
stacked after the `header`, paired with variable sized metadata
for each item, stacked from the end of the block inward (growing
toward the item structures).  An item contains its key
and the size and offset of its metadata within the block (in bytes,
if any).


DATA
----

The keys consist of an objectid, type code, and offset.  They are
sorted in that order, so keys with the same objectid are together in
the index.  The metadata associated with a key, if any, depends
on the type of the key.

#### Key in FS tree:

	objectid: inode number
	type:
		INODE - offset: 0
		DIR_ENT - offset: hash of the name of this directory entry
		FILE_EXTENT - offset: starting offset into the file (in blocks)

#### Key in extent tree:

	objectid: block number
	type: SUPERBLOCK, EXT_IDX, EXT_LEAF, FS_IDX, FS_LEAF, FILE_DATA
	offset: number of blocks in extent (1 for a tree node, or N for a file)

### Logical example:

#### FS tree:

	objectid: 0
		type: INODE, offset: 0 -> inode_type: DIR, ctime: 2011-11-10
		type: DIR_ENT, offset: 987654 -> name: "foo", inode: 1
	objectid: 1
		type: INODE -> inode_type: FILE, ctime: 2011-11-11
		type: FILE_EXTENT, offset: 0 -> blocknr: 5, size: 42500

#### extent tree:

	objectid: 0, type: SUPERBLOCK,	offset: 1
	objectid: 1, type: EXT_IDX,		offset: 1
	objectid: 2, type: EXT_LEAF,	offset: 1
	objectid: 3, type: FS_IDX,		offset: 1
	objectid: 4, type: FS_LEAF,		offset: 1
	objectid: 5, type: FILE_DATA,	offset: 42


DATA STRUCTURES
---------------

(these are out of date; see the actual code)

	struct superblock {
		u32 magic_number;		/* 0xaaa1babf */
		u32 extent_tree_blocknr;
		u32 fs_tree_blocknr;
		u32 total_blocks;
	}

	struct header {
		u32 blocknr;
		u8 flags;
		u8 nritems;
		u8 level;
	}

	struct key {
		u32 objectid;
		u8 type;
		u32 offset;
	}

	struct key_ptr {
		struct key key;
		u32 blocknr;
	}

	struct item {
		struct key key;
		u16 offset;
		u16 size;
	}

	struct inode_metadata {
		u8 inode_type;
		time_t ctime;
		time_t mtime;
	}

	struct dir_ent_metadata {
		u32 inode;
		char[201] name;		/* todo: dynamic size? */
	}

	struct file_extent_metadata {
		u32 blocknr;
		u32 size;	/* in bytes */
	}


NOTES
-----

When the FS tree or a file allocate blocks in the extent tree,
it may trigger a cascade, if the extent tree needs to then allocate
more blocks for itself.  I will try to avoid a Catch-22 here by
buffering all the changed blocks in memory, letting them change
freely throughout the operation, and only writing them out at the
end of the operation, like a little transaction.  (This could get tricky.)

Diagrams [here][2] and [here][6] look like they would be useful, but they
were lost in a kernel.org hacking incident and have not been recovered yet.


SCOPE
-----

Babyfs will have the following similarities to Btrfs:

* uses [Rodeh b-trees][4] for file system and extent allocation,
* with Btrfs-like keys, items, leaves, and block headers,
* and shadowing updates to guarantee integrity.

However, as for simplifications and limitations with respect to Btrfs,
Babyfs will not support the following:

* subvolumes, snapshots, clones, copy-on-write,
* reference counts, back-references, defragmentation, file system resizing,
* transactions, generation numbers, superblock mirrors,
* checksums on data or metadata (no checksum tree),
* journaling (no log tree), buffering (beyond one operation),
* multiple devices (no chunk or device trees), RAID,
* block sizes other than 1024 bytes,
* in-line data extents,
* SSD wear leveling,
* encryption, compression,
* seeks,
* locks, concurrent access (multi-thread/process),
* special (device) files,
* device size over 2GB,
* file size over 200MB,
* file protection (ownership, group, and access permissions),
* file access control lists and extended attributes,
* file access times (but will support create and modify times),
* fstat (so no way to see create and modify times),
* links (both hard and soft),
* current directories (and parent directory back-links),
* over 10 files open at the same time,
* machine-independent disk format (little- versus big-endian).


[1]: http://en.wikipedia.org/wiki/Btrfs
[2]: https://btrfs.wiki.kernel.org/articles/t/r/e/Trees.html
[3]: https://btrfs.wiki.kernel.org/articles/c/o/d/Code_documentation.html
[4]: http://www.cs.tau.ac.il/~ohadrode/papers/btree_TOS.pdf
    "O. Rodeh.  2008.  'B-trees, shadowing,  and clones'  New York: ACM Transactions on Storage, volume 3, issue 4, February 2008.  Retrieved November 12, 2011"
[5]: http://www2.hawaii.edu/~esb/2011fall.ics612/project6.html
[6]: https://btrfs.wiki.kernel.org/articles/d/a/t/Data_Structures_3b4e.html

<!-- vim: set ts=4: -->
