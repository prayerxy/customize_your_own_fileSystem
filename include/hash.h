#ifndef _XCRAFT_HASH_H
#define _XCRAFT_HASH_H
#include "XCraft.h"
int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo);

static int XCraft_make_hash_tree(const struct qstr *qstr,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh);

static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo);


#endif