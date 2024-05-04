#ifndef _XCRAFT_HASH_H
#define _XCRAFT_HASH_H
#include "XCraft.h"
// hash value to caculate 
// name:文件名 len:文件名长度  hinfo:hash值(储存hash值的结构体)
int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo){

    return 0;
}

static inline uint32_t dx_node_limit(void)
{
	unsigned int limit = (XCRAFT_BLOCK_SIZE - sizeof(__le16))  / sizeof(struct dx_entry);
	return limit;
}

static inline void dx_set_block(struct dx_entry *entry, uint32_t value)
{
	entry->block = cpu_to_le32(value);
}

static inline uint32_t dx_get_block(struct dx_entry *entry)
{
	return le32_to_cpu(entry->block);
}

static inline void dx_set_hash(struct dx_entry *entry, uint32_t value)
{
	entry->hash = cpu_to_le32(value);
}

static inline uint32_t dx_get_hash(struct dx_entry *entry)
{
	return le32_to_cpu(entry->hash);
}

static inline uint32_t dx_root_limit(void)
{
	unsigned int limit = (XCRAFT_BLOCK_SIZE - sizeof(struct dx_root_info)) / sizeof(struct dx_entry);
	return limit;
}

static inline void dx_set_count(struct dx_entry *entries, unsigned value)
{
	((struct dx_countlimit *) entries)->count = cpu_to_le16(value);
}
static inline void dx_set_limit(struct dx_entry *entries, unsigned value)
{
	((struct dx_countlimit *) entries)->limit = cpu_to_le16(value);
}
static inline unsigned dx_get_count(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->count);
}

static inline unsigned dx_get_limit(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->limit);
}

int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo);

// 在bh对应的磁盘块中插入目录项
// de记录最后的目录项位置, bh为插入该目录项所在的磁盘块缓冲区
// de 一般传入null，不是null就表明已经是找到了目录项
static int add_dirent_to_buf(struct dentry *dentry,
			     struct inode *inode, struct XCraft_dir_entry *de,
			     struct buffer_head * bh);



static int XCraft_make_hash_tree(struct dentry *dentry,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh);

static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo);






#endif