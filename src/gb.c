#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "../include/XCraft.h"


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

static inline int inode_get_block_group(struct XCraft_superblock_info *sb_info, uint32_t ino)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_inodes_count)-(sb_info->s_last_group_inodes);
    if(ino>=tmp)
        return sb_info->s_groups_count-1;
    else
        return ino/sb_info->s_inodes_per_group;
    
}

static inline int inode_get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t ino)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_inodes_count)-(sb_info->s_last_group_inodes);
    if(ino>=tmp)
        return ino-tmp;
    else
        return ino%sb_info->s_inodes_per_group;
}
static inline int get_block_group(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_blocks_count)-(sb_info->s_last_group_blocks);
    if(block>=tmp)
        return sb_info->s_groups_count-1;
    else
        return block/sb_info->s_blocks_per_group;
}
static inline int get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_blocks_count)-(sb_info->s_last_group_blocks);
    if(block>=tmp)
        return block-tmp;
    else
        return block%sb_info->s_blocks_per_group;
}