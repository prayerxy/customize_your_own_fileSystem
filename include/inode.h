#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include"XCraft.h"
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
static int inode_get_block_group(struct XCraft_superblock_info *sb_info, uint32_t ino)
{
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32toh(disk_sb->s_inodes_count)-sb_info->s_last_group_inodes;
    if(ino>=tmp)
        return sb_info->s_groups_count-1;
    else
        return ino/sb_info->s_inodes_per_group;
    
}

static int inode_get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t ino)
{
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32toh(disk_sb->s_inodes_count)-sb_info->s_last_group_inodes;
    if(ino>=tmp)
        return ino-tmp;
    else
        return ino%sb_info->s_inodes_per_group;
}
static int get_block_group(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32_to_cpu(disk_sb->s_blocks_count)-sb_info->s_last_group_blocks;
    if(block>=tmp)
        return sb_info->s_groups_count-1;
    else
        return block/sb_info->s_blocks_per_group;
}
static int get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32_to_cpu(disk_sb->s_blocks_count)-sb_info->s_last_group_blocks;
    if(block>=tmp)
        return block-tmp;
    else
        return block%sb_info->s_blocks_per_group;
}