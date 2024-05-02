#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include <stdlib.h>
#include "XCraft.h"


static inline struct XCraft_group_desc* get_group_desc(struct XCraft_superblock_info *sbi,xcraft_group_t group, struct buffer_head **bh)
{
    struct XCraft_group_desc* desc = NULL;
    uint32_t block = group / sbi->s_desc_per_block;
    uint32_t offset = group % sbi->s_desc_per_block;
    *bh = sbi->s_group_desc[block];
    desc=(struct XCraft_group_desc *)(*bh)->b_data;
    desc += offset;
    return desc;
}

static inline int group_free_inodes_count(struct XCraft_superblock_info *sbi,XCraft_group_desc* desc)
{
    return le16toh(desc->bg_free_inodes_count);
}

static inline int group_free_blocks_count(struct XCraft_superblock_info *sbi,XCraft_group_desc* desc)
{
    return le16toh(desc->bg_free_blocks_count);
}