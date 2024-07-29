#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include "XCraft.h"


// slab cache for XCraft_inode_info
static struct kmem_cache *XCraft_inode_cache;

// init our slab cache
int XCraft_init_inode_cache(void)
{
    XCraft_inode_cache = kmem_cache_create("XCraft_inode_cache",
                                           sizeof(struct XCraft_inode_info),
                                           0, 0, NULL);
    XCraft_inode_cache = kmem_cache_create_usercopy(
        "XCraft_inode_cache", sizeof(struct XCraft_inode_info), 0, 0, 0,
        sizeof(struct XCraft_inode_info), NULL);
    if (XCraft_inode_cache == NULL)
        return -ENOMEM;
    return 0;
}

// destroy our slab cache
int XCraft_destroy_inode_cache(void)
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
    if(!xi)
        return NULL;
    inode_init_once(&xi->vfs_inode);
    return &xi->vfs_inode;
}

// destroy_inode
void XCraft_destroy_inode(struct inode *inode)
{
    struct XCraft_inode_info *xi = XCRAFT_I(inode);
    kmem_cache_free(XCraft_inode_cache, xi);
}



struct XCraft_group_desc* get_group_desc(struct XCraft_superblock_info *sbi, xcraft_group_t group, struct buffer_head *bh)
{
    struct XCraft_group_desc* desc = NULL;
    uint32_t block = group / sbi->s_desc_per_block;
    uint32_t offset = group % sbi->s_desc_per_block;
    struct buffer_head **s_group_desc;
    s_group_desc = sbi->s_group_desc;
    bh = s_group_desc[block];
    desc=(struct XCraft_group_desc *)(bh->b_data);
    desc += offset;
    return desc;
}
struct XCraft_group_desc* get_group_desc2(struct XCraft_superblock_info *sbi,xcraft_group_t group)
{
    struct XCraft_group_desc* desc = NULL;
    uint32_t block = group / sbi->s_desc_per_block;
    uint32_t offset = group % sbi->s_desc_per_block;
    desc=(struct XCraft_group_desc *)(sbi->s_group_desc[block])->b_data;
    desc += offset;
    return desc;
}
int group_free_inodes_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc)
{
    return le16_to_cpu(desc->bg_free_inodes_count);
}

int group_free_blocks_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc)
{
    return le16_to_cpu(desc->bg_free_blocks_count);
}

int inode_get_block_group(struct XCraft_superblock_info *sb_info, uint32_t ino)
{   
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_inodes_count)-(sb_info->s_last_group_inodes);
    if(ino>=tmp)
        return sb_info->s_groups_count-1;
    else
        return ino/(sb_info->s_inodes_per_group);
    
}

int inode_get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t ino)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_inodes_count)-(sb_info->s_last_group_inodes);
    if(ino>=tmp)
        return ino-tmp;
    else
        return ino%(sb_info->s_inodes_per_group);
}
int get_block_group(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_blocks_count)-(sb_info->s_last_group_blocks);
    if(block>=tmp)
        return sb_info->s_groups_count-1;
    else
        return block/(sb_info->s_blocks_per_group);
}
int get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t block)
{
    struct XCraft_superblock *disk_sb=sb_info->s_super;
    uint32_t tmp;
    tmp = le32_to_cpu(disk_sb->s_blocks_count)-(sb_info->s_last_group_blocks);
    if(block>=tmp)
        return block-tmp;
    else
        return block%(sb_info->s_blocks_per_group);
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
    uint32_t inode_group = inode_get_block_group(sb_info, ino);
    // get ino in group
    uint32_t inode_shift_in_group = inode_get_block_group_shift(sb_info, ino);
    

    // caculate ino is beyond
    uint32_t s_inodes_count = le32_to_cpu(disk_sb->s_inodes_count);

    uint16_t inode_size;
    uint32_t blocks_per_group;
    uint32_t inode_block_begin, inode_block, inode_shift_in_block;

    struct XCraft_group_desc* desc = NULL;
    int i=0;
    if (ino >= s_inodes_count)
        return 0;

    // get inode_size
    inode_size = le16_to_cpu(disk_sb->s_inode_size);
    // get blocks_per_group
    blocks_per_group = le32_to_cpu(disk_sb->s_blocks_per_group);

    // 获取inode_block_begin
    desc = get_group_desc2(sb_info, inode_group);
    inode_block_begin = le32_to_cpu(desc->bg_inode_table);
    inode_block = inode_block_begin + inode_shift_in_group / XCRAFT_INODES_PER_BLOCK;
    inode_shift_in_block = inode_shift_in_group % XCRAFT_INODES_PER_BLOCK;
    
    bh = sb_bread(sb, inode_block);
    if(!bh)
        return -EIO;
    disk_inode = (struct XCraft_inode *)bh->b_data;
    disk_inode += inode_shift_in_block;
    disk_inode->i_mode = cpu_to_le16(inode->i_mode);
    disk_inode->i_uid = cpu_to_le16(i_uid_read(inode));
    disk_inode->i_gid = cpu_to_le16(i_gid_read(inode));
    disk_inode->i_size_lo = cpu_to_le32(inode->i_size);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 ctime = inode_get_ctime(inode);
    disk_inode->i_ctime = cpu_to_le32(ctime.tv_sec);
#else
    disk_inode->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
#endif

    disk_inode->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
    disk_inode->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
    disk_inode->i_nr_files = cpu_to_le32(xi->i_nr_files);
    disk_inode->i_blocks_lo = cpu_to_le32(inode->i_blocks);
    disk_inode->i_links_count = cpu_to_le16(inode->i_nlink);
    disk_inode->i_flags = cpu_to_le32(xi->i_flags);
    
    for(i=0;i<XCRAFT_N_BLOCK;i++)
        disk_inode->i_block[i] = xi->i_block[i];
    
    strncpy(disk_inode->i_data, xi->i_data,sizeof(xi->i_data));
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    return 0;
}


// put_super
static void XCraft_put_super(struct super_block *sb){
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    struct buffer_head **group_desc;
    int i;
    if(sb_info){
        brelse(sb_info->s_sbh);
        kfree(sb_info->s_super);
        group_desc = sb_info->s_group_desc;
        for(i=0; i < XCRAFT_DESC_LIMIT_blo; i++)
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
    struct buffer_head *bh1;
    struct buffer_head *bh2;
    struct buffer_head *bh3;
    struct XCraft_group_desc *disk_gdb ;
    int i;

    uint32_t bfree_blo;

    // 获取位图开始位置
    uint32_t bg_inode_bitmap;
    uint32_t bg_block_bitmap;

    disk_sb=(struct XCraft_superblock*)sb_info->s_sbh->b_data;

    disk_sb->s_inodes_count = ssb->s_inodes_count;
    disk_sb->s_blocks_count = ssb->s_blocks_count;
    disk_sb->s_free_blocks_count = ssb->s_free_blocks_count;
    disk_sb->s_free_inodes_count = ssb->s_free_inodes_count;
    disk_sb->s_blocks_per_group = ssb->s_blocks_per_group;
    disk_sb->s_inodes_per_group = ssb->s_inodes_per_group;
    disk_sb->s_groups_count = ssb->s_groups_count;
    disk_sb->s_last_group_blocks = ssb->s_last_group_blocks;
    disk_sb->s_magic = ssb->s_magic;
    disk_sb->s_inode_size = ssb->s_inode_size;

    // 回写super block
    mark_buffer_dirty(sb_info->s_sbh);
    if (wait)
        sync_dirty_buffer(sb_info->s_sbh);

    // 回写块组描述符
    
    //s_gdb_count是组描述符占多少个块
    for(i=0;i<sb_info->s_gdb_count;i++){
        mark_buffer_dirty(sb_info->s_group_desc[i]);
        if (wait)
            sync_dirty_buffer(sb_info->s_group_desc[i]);
    }


    // 回写inode位图和bitmap位图
    for(i=0;i<sb_info->s_groups_count;i++){
        disk_gdb = get_group_desc2(sb_info,i);
        bg_inode_bitmap= le32_to_cpu(disk_gdb->bg_inode_bitmap);
        bg_block_bitmap = le32_to_cpu(disk_gdb->bg_block_bitmap);
        // inode位图只有一块
        bh1 = sb_bread(sb, bg_inode_bitmap);
        if(!bh1)
            return -EIO;
        memcpy(bh1->b_data, (void *)(sb_info->s_ibmap_info[i]->ifree_bitmap), XCRAFT_BLOCK_SIZE);
        mark_buffer_dirty(bh1);
        if (wait)
            sync_dirty_buffer(bh1);
        brelse(bh1);

        // block位图可能有多块
        // 先只写一个块
        bh2 = sb_bread(sb, bg_block_bitmap);
        if(!bh2)
            return -EIO;

        memcpy(bh2->b_data, (void *)(sb_info->s_ibmap_info[i]->bfree_bitmap), XCRAFT_BLOCK_SIZE);
        mark_buffer_dirty(bh2);
        if (wait)
            sync_dirty_buffer(bh2);
        brelse(bh2);
        
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(le32_to_cpu(disk_gdb->bg_nr_blocks));
        if(bfree_blo>1){
            bh3 = sb_bread(sb, bg_block_bitmap+1);
            if(!bh3)
                return -EIO;
            memcpy(bh3->b_data, (void *)(sb_info->s_ibmap_info[i]->bfree_bitmap) + XCRAFT_BLOCK_SIZE, XCRAFT_BLOCK_SIZE);
            mark_buffer_dirty(bh3);
            if (wait)
                sync_dirty_buffer(bh3);
            brelse(bh3);
        }

    }
    return 0;
}

// statfs
static int XCraft_statfs(struct dentry *dentry, struct kstatfs *buf){
    struct super_block *sb = dentry->d_sb;
    struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
    buf->f_type = XCRAFT_MAGIC;
    buf->f_bsize = XCRAFT_BLOCK_SIZE;
    buf->f_blocks = le32_to_cpu(sb_info->s_super->s_blocks_count);
    buf->f_bfree = le32_to_cpu(sb_info->s_super->s_free_blocks_count);
    buf->f_bavail = le32_to_cpu(sb_info->s_super->s_free_blocks_count);
    buf->f_files = le32_to_cpu(sb_info->s_super->s_inodes_count - sb_info->s_super->s_free_inodes_count);
    buf->f_ffree = le32_to_cpu(sb_info->s_super->s_free_inodes_count);
    buf->f_namelen = XCRAFT_NAME_LEN;
    return 0;
}

struct super_operations XCraft_sops = 
{
    .alloc_inode = XCraft_alloc_inode,
    .destroy_inode = XCraft_destroy_inode,
    .write_inode = XCraft_write_inode,
    .put_super = XCraft_put_super,
    .sync_fs = XCraft_sync_fs,
    .statfs = XCraft_statfs,
};

// fill super for mount
int XCraft_fill_super(struct super_block *sb, void *data, int silent){
    struct buffer_head *bh = NULL;
    struct XCraft_superblock_info *sb_info = NULL;
    struct XCraft_superblock *disk_sb = kzalloc(sizeof(struct XCraft_superblock), GFP_KERNEL);
    struct inode* root_inode = NULL;
    struct XCraft_inode_info *root_inode_info = NULL;
    int ret;
    int i;
    struct XCraft_superblock *disk_sb_tmp;
    unsigned long gdb_count = 0;

    // 获取块组描述符
    struct buffer_head** group_desc;

    uint32_t bfree_blo;

    unsigned int root_iblock;
    ret = 0;

    // init sb
    sb->s_magic = XCRAFT_MAGIC;
    sb_set_blocksize(sb, XCRAFT_BLOCK_SIZE);
    sb->s_maxbytes = XCraft_get_max_filesize();
    sb->s_op = &XCraft_sops;

    // read sb from disk
    bh = sb_bread(sb, 0);
    if(!bh)
        return -EIO;
    disk_sb_tmp = (struct XCraft_superblock *)bh->b_data;
    // memcpy((char *)disk_sb, (char *)disk_sb_tmp, sizeof(struct XCraft_superblock));
    disk_sb->s_inodes_count = disk_sb_tmp->s_inodes_count;
    disk_sb->s_blocks_count = disk_sb_tmp->s_blocks_count;
    disk_sb->s_free_blocks_count = disk_sb_tmp->s_free_blocks_count;
    disk_sb->s_free_inodes_count = disk_sb_tmp->s_free_inodes_count;
    disk_sb->s_blocks_per_group = disk_sb_tmp->s_blocks_per_group;
    disk_sb->s_inodes_per_group = disk_sb_tmp->s_inodes_per_group;
    disk_sb->s_groups_count = disk_sb_tmp->s_groups_count;
    disk_sb->s_last_group_blocks = disk_sb_tmp->s_last_group_blocks;
    disk_sb->s_magic = disk_sb_tmp->s_magic;
    disk_sb->s_inode_size = disk_sb_tmp->s_inode_size;

    // check magic
    if(disk_sb->s_magic != sb->s_magic){
        pr_err("Wrong magic number\n");
        ret = -EINVAL;
        goto out_brelse;
    }
    brelse(bh);

    // init superblock info
    sb_info = kzalloc(sizeof(struct XCraft_superblock_info), GFP_KERNEL);
    if(!sb_info){
        ret = -ENOMEM;
        goto out_brelse;
    }
    sb_info->s_blocks_per_group = le32_to_cpu(disk_sb->s_blocks_per_group);
    sb_info->s_inodes_per_group = le32_to_cpu(disk_sb->s_inodes_per_group);
    sb_info->s_desc_per_block = XCRAFT_GROUP_DESCS_PER_BLOCK;
    sb_info->s_groups_count = le32_to_cpu(disk_sb->s_groups_count);
    sb_info->s_sbh = sb_bread(sb, 0);
    sb_info->s_super = disk_sb;
    sb_info->s_La_init_group=0; //初始化第一个块组
    sb->s_fs_info = sb_info;
    sb_info->s_last_group_inodes = le32_to_cpu(disk_sb->s_inodes_count)-sb_info->s_inodes_per_group*(sb_info->s_groups_count-1);
    sb_info->s_last_group_blocks = le32_to_cpu(disk_sb->s_last_group_blocks);
    // sb_info->s_group_desc = NULL;
    

    // hash seed
    sb_info->s_hash_seed[0] = 0x2c55f0c2;
    sb_info->s_hash_seed[1] = 0x249b59fa;
    sb_info->s_hash_seed[2] = 0xef80f217;
    sb_info->s_hash_seed[3] = 0xb8fb094d;

    // get s_gdb_count and s_group_desc
    
    gdb_count= ceil_div(sb_info->s_groups_count, sb_info->s_desc_per_block);
    group_desc = kzalloc(sizeof(struct buffer_head*) * XCRAFT_DESC_LIMIT_blo, GFP_KERNEL);
    
    sb_info->s_ibmap_info = kzalloc(sizeof(struct XCraft_ibmap_info *) * sb_info->s_groups_count, GFP_KERNEL);
    sb_info->s_ibmap_info[0]=kzalloc(sizeof(struct XCraft_ibmap_info),GFP_KERNEL);
    sb_info->s_ibmap_info[0]->ifree_bitmap=kzalloc(XCRAFT_BLOCK_SIZE,GFP_KERNEL);


    if(sb_info->s_groups_count==1){
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(sb_info->s_last_group_blocks);
    }
    else bfree_blo=1;

  
    sb_info->s_ibmap_info[0]->bfree_bitmap=kzalloc(XCRAFT_BLOCK_SIZE*bfree_blo,GFP_KERNEL);

    if(!sb_info->s_ibmap_info[0]->ifree_bitmap||!sb_info->s_ibmap_info[0]->bfree_bitmap){
        ret = -ENOMEM;
        goto out_free_group_desc;
    }
    
    for(i=0;i<XCRAFT_DESC_LIMIT_blo;i++){
        group_desc[i] = sb_bread(sb, i+1);
        
        
        if(!group_desc[i]){
            ret = -EIO;
            goto out_free_group_desc;
        }
        
    }
    sb_info->s_gdb_count = gdb_count;//组描述符占多少个块
    sb_info->s_group_desc = group_desc;

    
    // 读第0个块组的位图
    bh=sb_bread(sb,1 + XCRAFT_DESC_LIMIT_blo);
    if(!bh){
        ret = -EIO;
        goto out_free_group_desc;
    }
    memcpy((void *)sb_info->s_ibmap_info[0]->ifree_bitmap, bh->b_data,
            XCRAFT_BLOCK_SIZE);
    brelse(bh);

    bh=sb_bread(sb,2 + XCRAFT_DESC_LIMIT_blo);
    if(!bh){
        ret = -EIO;
        goto out_free_group_desc;
    }
    memcpy((void *)sb_info->s_ibmap_info[0]->bfree_bitmap, bh->b_data,XCRAFT_BLOCK_SIZE);
    brelse(bh);

    if(bfree_blo>1){
        bh=sb_bread(sb,3 + XCRAFT_DESC_LIMIT_blo);
        if(!bh){
            ret = -EIO;
            goto out_free_group_desc;
        }
        memcpy((void *)sb_info->s_ibmap_info[0]->bfree_bitmap+XCRAFT_BLOCK_SIZE, bh->b_data,XCRAFT_BLOCK_SIZE);
        brelse(bh);
    }
    
    
    // 对其他块组的位图进行初始化 申请空间
    for(i=1;i<sb_info->s_groups_count;i++){
        sb_info->s_ibmap_info[i]=kzalloc(sizeof(struct XCraft_ibmap_info),GFP_KERNEL);
        sb_info->s_ibmap_info[i]->ifree_bitmap=kzalloc(XCRAFT_BLOCK_SIZE,GFP_KERNEL);
        if(i!=sb_info->s_groups_count-1){
            sb_info->s_ibmap_info[i]->bfree_bitmap=kzalloc(XCRAFT_BLOCK_SIZE,GFP_KERNEL);
            memset(sb_info->s_ibmap_info[i]->bfree_bitmap,0xff,XCRAFT_BLOCK_SIZE);
        }
        else{
            sb_info->s_ibmap_info[i]->bfree_bitmap=kzalloc(XCRAFT_BFREE_PER_GROUP_BLO(sb_info->s_last_group_blocks)*XCRAFT_BLOCK_SIZE,GFP_KERNEL);
            memset(sb_info->s_ibmap_info[i]->bfree_bitmap,0xff,XCRAFT_BFREE_PER_GROUP_BLO(sb_info->s_last_group_blocks)*XCRAFT_BLOCK_SIZE);
        }
        memset(sb_info->s_ibmap_info[i]->ifree_bitmap,0xff,XCRAFT_BLOCK_SIZE);
        if(!sb_info->s_ibmap_info[i]->ifree_bitmap||!sb_info->s_ibmap_info[i]->bfree_bitmap){
            ret = -ENOMEM;
            goto out_free_group_desc;
        }
    }

    // init root inode
    root_inode = XCraft_iget(sb, 1);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto out_free_group_desc;
    }

    root_inode_info = XCRAFT_I(root_inode);
    root_iblock = le32_to_cpu(root_inode_info->i_block[0]);
    bh = sb_bread(sb, root_iblock);
    if(!bh){
        ret = -EIO;
        goto iput;
    }
    memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
    mark_buffer_dirty(bh);
    brelse(bh);
    

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
    for(i=0;i < XCRAFT_DESC_LIMIT_blo; i++)
        brelse(group_desc[i]);
    kfree(group_desc);
    kfree(sb_info);
out_brelse:
    brelse(bh);

    return ret;
}
