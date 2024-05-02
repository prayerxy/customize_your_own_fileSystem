#include"XCraft.h"

#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
static int inode_get_block_group(struct super_block *sb, uint32_t ino)
{
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32_to_cpu(disk_sb->s_inodes_count)-sb_info->s_last_group_inodes;
    if(ino>=tmp)
        return sb_info->s_groups_count-1;
    else
        return ino/sb_info->s_inodes_per_group;
    
}

static int inode_get_block_group_shift(struct super_block *sb, uint32_t ino)
{
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    uint32_t tmp=le32_to_cpu(disk_sb->s_inodes_count)-sb_info->s_last_group_inodes;
    if(ino>=tmp)
        return ino-tmp;
    else
        return ino%sb_info->s_inodes_per_group;
}