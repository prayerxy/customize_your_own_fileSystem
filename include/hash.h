#ifndef _XCRAFT_HASH_H
#define _XCRAFT_HASH_H
#include "XCraft.h"
int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo);

// 在bh对应的磁盘块中插入目录项
// de记录最后的目录项位置, bh为插入该目录项所在的磁盘块缓冲区
// de 一般传入null，不是null就表明已经是找到了目录项
static int add_dirent_to_buf(struct dentry *dentry,
			     struct inode *inode, struct XCraft_dir_entry *de,
			     struct buffer_head * bh);



static int XCraft_make_hash_tree(const struct qstr *qstr,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh);

static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo);






#endif