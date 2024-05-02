#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include "../include/XCraft.h"
#include "../include/inode.h"

// slab cache for XCraft_inode_info
static struct kmem_cache *XCraft_inode_cache;

// init our slab cache
static int XCraft_init_inode_cache(void)
{
    XCraft_inode_cache = kmem_cache_create("XCraft_inode_cache",
                                           sizeof(struct XCraft_inode_info),
                                           0, 0, NULL);
    if (XCraft_inode_cache == NULL)
        return -ENOMEM;
    return 0;
}

// destroy our slab cache
static int XCraft_destroy_inode_cache(void)
{
    kmem_cache_destroy(XCraft_inode_cache);
    return 0;
}

// sops
// alloc_inode
static struct inode *XCraft_alloc_inode(struct super_block *sb)
{
    struct XCraft_inode_info *xi;
    xi = kmem_cache_alloc(XCraft_inode_cache, GFP_KERNEL);
    return &(xi->vfs_inode);
}

// destroy_inode
static void XCraft_destroy_inode(struct inode *inode)
{
    struct XCraft_inode_info *xi = XCRAFT_I(inode);
    kmem_cache_free(XCraft_inode_cache, xi);
}

// write_inode

int XCraft_write_inode(struct inode *inode, struct writeback_control *wbc)
{
    struct XCraft_inode_info *xi = XCRAFT_I(inode);
    struct XCraft_inode *disk_inode;
    struct super_block *sb = inode->i_sb;
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    struct XCraft_superblock *disk_sb = sb_info->s_super;
    struct buffer_head *bh;
    uint32_t ino = inode->i_ino;
    // block_group num
    uint32_t inode_group = inode_get_block_group(sb, ino);
    // get ino in group
    uint32_t inode_shift_in_group = inode_get_block_group_shift(sb, ino);
    

    // caculate ino is beyond
    uint32_t s_inodes_count = le32toh(disk_sb->s_inodes_count);
    if (ino >= s_inodes_count)
        return 0;

    // get inode_size
    uint32_t inode_size = le16toh(disk_sb->s_inode_size);
    // get blocks_per_group
    uint32_t blocks_per_group = le32toh(disk_sb->s_blocks_per_group);

    // get inode_block
    uint32_t inode_block_begin = (1 + XCRAFT_DESC_LIMIT_blo) + inode_group * blocks_per_group + 2;
    uint32_t inode_block = inode_block_begin + inode_shift_in_group / XCRAFT_INODES_PER_BLOCK;
    uint32_t inode_shift_in_block = inode_shift_in_group % XCRAFT_INODES_PER_BLOCK;

    bh = sb_bread(sb, inode_block);
    if(!bh)
        return -EIO;
    disk_inode = (struct XCraft_inode *)bh->b_data;
    disk_inode += inode_shift_in_block;
    // 字节序转换存疑?
    disk_inode->i_mode = htole16(inode->i_mode);
    disk_inode->i_uid = htole16(i_uid_read(inode));
    disk_inode->i_gid = htole16(i_gid_read(inode));
    disk_inode->i_size_lo = htole32(inode->i_size);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 ctime = inode_get_ctime(inode);
    disk_inode->i_ctime = htole32(ctime.tv_sec);
#else
    disk_inode->i_ctime = htole32(inode->i_ctime.tv_sec);
#endif

    disk_inode->i_atime = htole32(inode->i_atime.tv_sec);
    disk_inode->i_mtime = htole32(inode->i_mtime.tv_sec);
    disk_inode->i_dtime = htole32(xi->i_dtime);
    disk_inode->i_blocks_lo = htole32(inode->i_blocks);
    disk_inode->i_links_count = htole16(inode->i_nlink);
    disk_inode->i_flags = htole32(xi->i_flags);
    for(int i=0;i<XCRAFT_N_BLOCK;i++)
        disk_inode->i_block[i] = htole16(xi->i_block[i]);
    strncpy(disk_inode->i_data, xi->i_data,sizeof(xi->i_data));
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    return 0;
}


// put_super
static void XCraft_put_super(struct super_block *sb){
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    if(sb_info){
        kfree(sb_info->s_sbh);
        kfree(sb_info->s_super);
        struct buffer_head **group_desc = sb_info->s_group_desc;
        for(int i=0;i < XCraft_DESC_LIMIT_blo; i++)
            brelse(group_desc[i]);
        kfree(group_desc);
        kfree(sb_info);
    }
}

// sync_fs
static int XCraft_sync_fs(struct super_block *sb, int wait){
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    struct XCraft_superblock *ssb=sb_info->s_super;

    
    struct  XCraft_superblock *disk_sb;

    disk_sb=(struct XCraft_superblock*)sb_info->s_sbh->b_data;

    disk_sb->s_inodes_count = ssb->s_inodes_count;
    disk_sb->s_blocks_count = ssb->s_blocks_count;
    disk_sb->s_free_blocks_count = ssb->s_free_blocks_count;
    disk_sb->s_free_inodes_count = ssb->s_free_inodes_count;
    disk_sb->s_blocks_per_group = ssb->s_blocks_per_group;
    disk_sb->s_inodes_per_group = ssb->s_inodes_per_group;
    disk_sb->s_groups_count = ssb->s_groups_count;
    disk_sb->s_last_group_blocks = ssb->s_last_group_blocks;

    // 回写super block
    mark_buffer_dirty(sb_info->s_sbh);
    if (wait)
        sync_dirty_buffer(sb_info->s_sbh);

    // 回写块组描述符
    struct buffer_head *bh1 = NULL,*bh2 = NULL;
    for(int i=0;i<sb_info->s_gdb_count;i++){
        bh1 = sb_bread(sb, i+1);
        if(!bh1)
            return -EIO;
        struct XCraft_group_desc *disk_gdb = (struct XCraft_group_desc *)bh1->b_data;
        memcpy((char *)disk_gdb, (char *)(sb_info->s_group_desc[i]->b_data), XCRAFT_BLOCK_SIZE);
        mark_buffer_dirty(bh1);
        if (wait)
            sync_dirty_buffer(bh1);
        brelse(bh1);
    }
    return 0;
}

// statfs
static int XCraft_statfs(struct dentry *dentry, struct kstatfs *buf){
    struct super_block *sb = dentry->d_sb;
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    buf->f_type = XCRAFT_MAGIC;
    buf->f_bsize = XCRAFT_BLOCK_SIZE;
    buf->f_blocks = le32toh(sb_info->s_super->s_blocks_count);
    buf->f_bfree = le32toh(sb_info->s_super->s_free_blocks_count);
    stat->f_bavail = le32toh(sb_info->s_super->s_free_blocks_count);
    stat->f_files = le32toh(sb_info->s_super->s_inodes_count - sb_info->s_super->s_free_inodes_count);
    stat->f_ffree = le32toh(sb_info->s_super->s_free_inodes_count);
    stat->f_namelen = XCRAFT_NAME_LEN;
}

struct super_operations XCraft_sops
{
    .alloc_inode = XCraft_alloc_inode,
    .destroy_inode = XCraft_destroy_inode,
    .write_inode = XCraft_write_inode,
    .put_super = XCraft_put_super,
    .sync_fs = XCraft_sync_fs,
    .statfs = XCraft_statfs,
}

// fill super for mount
static int
XCraft_fill_super(struct super_block *sb, void *data, int silent){
    struct buffer_head *bh = NULL, *bh1 = NULL;
    struct XCraft_superblock_info *sb_info = NULL;
    struct XCraft_superblock *disk_sb = kzalloc(sizeof(struct XCraft_superblock), GFP_KERNEL);
    struct inode* root_inode = NULL;
    int ret = 0;

    // init sb
    sb->s_magic = XCRAFT_MAGIC;
    sb_set_blocksize(sb, XCRAFT_BLOCK_SIZE);

    sb->s_op = &XCraft_sops;

    // read sb from disk
    bh = sb_bread(sb, 0);
    if(!bh)
        return -EIO;
    struct XCraft_superblock *disk_sb_tmp = (struct XCraft_superblock *)bh->b_data;
    memcpy((char *)disk_sb, (char *)disk_sb_tmp, sizeof(struct XCraft_superblock));

    // check magic
    if(disk_sb->s_magic != sb->s_magic){
        pr_err("Wrong magic number\n")
        ret = -EINVAL;
        goto out_brelse;
    }

    // init superblock info
    sb_info = kzalloc(sizeof(struct XCraft_superblock_info), GFP_KERNEL);
    if(!sb_info){
        ret = -ENOMEM;
        goto out_sb_info;
    }
    sb_info->s_blocks_per_group = le32toh(disk_sb->s_blocks_per_group);
    sb_info->s_inodes_per_group = le32toh(disk_sb->s_inodes_per_group);
    sb_info->s_desc_per_block = XCRAFT_GROUP_DESCS_PER_BLOCK;
    sb_info->s_groups_count = le32toh(disk_sb->s_groups_count);
    sb_info->s_sbh = bh;
    sb_info->s_super = disk_sb;
    sb_info->s_La_init_group=0;//初始化第一个块组
    sb->s_fs_info = sb_info;
    sb_info->s_last_group_inodes = le32toh(disk_sb->s_inodes_count)-sb_info->s_inodes_per_group*(sb_info->s_groups_count-1);
    sb_info->s_last_group_blocks = le32toh(disk_sb->s_last_group_blocks);
    // sb_info->s_group_desc = NULL;

    // get s_gdb_count and s_group_desc
    unsigned long gdb_count = 0;
    gdb_count= ceil_div(sb_info->s_groups_count, sb_info->s_desc_per_block);
    struct buffer_head** group_desc = kzalloc(sizeof(struct buffer_head*) * XCRAFT_DESC_LIMIT_blo, GFP_KERNEL);
    
    sb_info->s_ibmap_info = kzalloc(sizeof(struct XCraft_ibmap_info *) * sb_info->s_groups_count, GFP_KERNEL);
    sb_info->s_ibmap_info[0]=kzalloc(sizeof(struct XCraft_ibmap_info),GFP_KERNEL);
    sb_info->s_ibmap_info[0]->ifree_bitmap=kzalloc(1,GFP_KERNEL);
    if(sb_info->s_groups_count>1)
        sb_info->s_ibmap_info[0]->bfree_bitmap=kzalloc(1,GFP_KERNEL);
    else
        sb_info->s_ibmap_info[0]->bfree_bitmap=kzalloc(XCRAFT_BFREE_PER_GROUP_BLO(sb_info->s_last_group_blocks),GFP_KERNEL);
    if(!sb_info->s_ibmap_info[0]->ifree_bitmap||!sb_info->s_ibmap_info[0]->bfree_bitmap){
        ret = -ENOMEM;
        goto out_free_group_desc;
    }
    for(int i=0;i<XCRAFT_DESC_LIMIT_blo;i++){
        bh1 = sb_bread(sb, i+1);
        group_desc[i] = bh1;
        if(!bh1){
            ret = -EIO;
            goto out_free_group_desc;
        }
    }
    sb_info->s_gdb_count = gdb_count;//组描述符占多少个块
    sb_info->s_group_desc = group_desc;

    bh=sb_bread(sb,1+XCRAFT_DESC_LIMIT_blo);

    // init root inode
    root_inode = XCraft_iget(sb, 0);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto out_free_group_desc;
    }
#if MNT_IDMAP_REQUIRED()
    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, root_inode->i_mode);
#elif USER_NS_REQUIRED()
    inode_init_owner(&init_user_ns, root_inode, NULL, root_inode->i_mode);
#else
    inode_init_owner(root_inode, NULL, root_inode->i_mode);
#endif
    sb->s_root = d_make_root(root_inode);
    if(!sb->s_root) {
        ret = -ENOMEM;
        goto iput;
    }

    return 0;

iput:
    iput(root_inode);
out_free_group_desc:
    for(int i=0;i < XCRAFT_DESC_LIMIT_blo; i++)
        brelse(group_desc[i]);
    kfree(group_desc);
out_sb_info:
    kfree(sb_info);
out_brelse:
    brelse(bh);

    return ret;
}
