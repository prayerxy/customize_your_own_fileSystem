#ifndef XCRAFT_GB_H
#define XCRAFT_GB_H

#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "XCraft.h"


static inline struct XCraft_group_desc* get_group_desc(struct XCraft_superblock_info *sbi,xcraft_group_t group, struct buffer_head *bh)
{
    struct XCraft_group_desc* desc = NULL;
    uint32_t block = group / sbi->s_desc_per_block;
    uint32_t offset = group % sbi->s_desc_per_block;
    bh = sbi->s_group_desc[block];
    desc=(struct XCraft_group_desc *)(bh)->b_data;
    desc += offset;
    return desc;
}
static inline struct XCraft_group_desc* get_group_desc2(struct XCraft_superblock_info *sbi,xcraft_group_t group)
{
    struct XCraft_group_desc* desc = NULL;
    uint32_t block = group / sbi->s_desc_per_block;
    uint32_t offset = group % sbi->s_desc_per_block;
    desc=(struct XCraft_group_desc *)(sbi->s_group_desc[block])->b_data;
    desc += offset;
    return desc;
}
static inline int group_free_inodes_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc)
{
    return le16_to_cpu(desc->bg_free_inodes_count);
}

static inline int group_free_blocks_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc)
{
    return le16_to_cpu(desc->bg_free_blocks_count);
}

#endif