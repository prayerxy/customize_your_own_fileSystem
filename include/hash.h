#ifndef XCRAFT_HASH_H
#define XCRAFT_HASH_H

#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/dcache.h>	/* For d_splice_alias */
#include "XCraft.h"
#include "bitmap.h"
// hash value to caculate and hash tree to build

int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo);


// hash tree to build
static int XCraft_make_hash_tree(const struct qstr *qstr,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh);

// do_split
static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo);

#endif