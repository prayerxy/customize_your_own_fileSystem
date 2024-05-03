// htree Ref:https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-2
//  Ref：https://github.com/sysprog21/simplefs/tree/master
// block group + dir hash
#ifndef XCRAFT_H
#define XCRAFT_H
#include <linux/types.h>
#include <linux/version.h>

#define XCRAFT_MAGIC 0x1234 /* "XCRA" */
#define XCRAFT_BLOCK_SIZE (1 << 12) /* 4 KiB */
#define XCRAFT_N_BLOCK 15   //2个直接索引块 2个1级间接索引块 1个2级间接索引块
#define XCRAFT_NAME_LEN 255
#define XCRAFT_INODE_RATIO 4
#define XCRAFT_DESC_LIMIT_blo 2
#define XCRAFT_BLOCKS_PER_GROUP 32768
// 版本号判断 inode_operations因版本号而变化
#define USER_NS_REQUIRED() LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
#define MNT_IDMAP_REQUIRED() LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)

// desc flags
#define XCraft_BG_INODE_INIT	0x0001 /* Inode table/bitmap not in use */
#define XCraft_BG_BLOCK_INIT	0x0002 /* Block bitmap not in use */
#define Xcraft_BG_INODE_ZEROED	0x0004 /* On-disk itable initialized to zero */
#define XCraft_BG_ISINIT(flag) (flag&XCraft_BG_INODE_INIT)&&(flag&XCraft_BG_BLOCK_INIT)
// inode flags
#define XCraft_INODE_HASH_TREE 0x0001
#define XCraft_INODE_ISHASH_TREE(flag) flag & XCraft_INODE_HASH_TREE

//ifree_per_group_blo 
#define XCRAFT_IFREE_PER_GROUP_BLO 1
//bfree_per_group_blo
#define XCRAFT_BFREE_PER_GROUP_BLO(blocks) ceil_div(blocks,8*XCRAFT_BLOCK_SIZE)
/* todo
XCraft partition layout
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  descriptor   |  sb->s_gdb_count blocks
 * +---------------+
 * | ifree bitmap  |  bg_0(32768 blocks) 4096*8
 * +---------------+
 * | bfree bitmap  |  XCRAFT_BFREE_PER_GROUP_BLO blocks
 * +---------------+
 * |  inode_str_   |  
 * |        blocks |  bg_nr_inodes/XCRAFT_INODES_PER_BLOCK   blocks
 * +---------------+
 * |  data blocks  |  bg_nr_blocks-1-XCRAFT_BFREE_PER_GROUP_BLO-inode_str_blocks    blocks
 * +---------------+
 * |  ...other bg  |  ...
 */

static inline uint32_t ceil_div(uint32_t a, uint32_t b){
    uint32_t c = a / b;
    if(a % b){
        c++;
    }
    return c;
}
typedef unsigned int xcraft_group_t; 
struct XCraft_inode{
    __le16 i_mode; /* file mode */
    __le16 i_uid; /* owner UID */
    __le32 i_size_lo; /* file size in bytes */
    __le32 i_atime; /* access time */
    __le32 i_ctime; /* creation time */
    __le32 i_mtime; /* modification time */
    __le32 i_dtime; /* deletion time */
    __le16 i_gid; /* owner GID */
    __le16 i_links_count; /* hard links count */
    __le32 i_blocks_lo; /* number of blocks 如果是目录inode 哈希树块数量；如果是文件inode 文件占的块大小*/
    __le32 i_flags; /* file flags B+树等 */
    __le32 i_block[XCRAFT_N_BLOCK]; /* pointers to blocks */
    char i_data[32]; /* store symlink content */
};

#define XCRAFT_INODE_SIZE sizeof(struct XCraft_inode)
#define XCRAFT_INODES_PER_BLOCK (XCRAFT_BLOCK_SIZE / XCRAFT_INODE_SIZE)

#define XCRAFT_INODES_PER_GROUP 8192

 /*
    最后一个块组的inode_store不一定是XCRAFT_inodes_str_blocks_PER，
    而是(s_inodes_count-(s_groups_count-1)*s_inodes_per_group)/XCRAFT_INODES_PER_BLOCK
*/
#define XCRAFT_inodes_str_blocks_PER XCRAFT_INODES_PER_GROUP/XCRAFT_INODES_PER_BLOCK
struct XCraft_superblock{
    __le32 s_inodes_count; /* number of inodes */
    __le32 s_blocks_count; /* number of blocks */
    __le32 s_free_blocks_count; /* number of free blocks */
    __le32 s_free_inodes_count; /* number of free inodes */
    __le32 s_blocks_per_group; /* number of blocks per group */
    __le32 s_inodes_per_group; /* number of inodes per group */
    __le32 s_groups_count; /* number of groups */
    __le32 s_last_group_blocks;//最后一个组的块数
    __le16 s_magic; /* magic number */
    __le16 s_inode_size; /* inode size */
};

//注意这里一定是整除 因为在write_super加上Mod
//mkfs用户态用到

struct XCraft_group_desc{
    __le32 bg_block_bitmap; /* block bitmap block 物理块号*/
    __le32 bg_inode_bitmap; /* inode bitmap block 物理块号*/
    __le32 bg_inode_table; /* 物理块号*/
    __le16 bg_nr_inodes; /* number of inodes in this group */
    __le16 bg_nr_blocks; /* number of blocks in this group */
    __le16 bg_free_blocks_count; /* number of free blocks */
    __le16 bg_free_inodes_count; /* number of free inodes */
    __le16 bg_used_dirs_count; /* number of directories */
    __le16 bg_flags;/*EXT4_BG_flags(INODE_UNINIT,etc) 表示该块组的信息 初始化or未初始化*/

};
#define XCRAFT_GROUP_DESC_SIZE sizeof(struct XCraft_group_desc)
#define XCRAFT_GROUP_DESCS_PER_BLOCK (XCRAFT_BLOCK_SIZE / XCRAFT_GROUP_DESC_SIZE)


//最开始建树的时候，10个以上分类成哈希树
#define XCRAFT_dentry_LIMIT 10

// hash树的层数
#define XCRAFT_HTREE_LEVEL 3

// 判断indirect_levels字段是否超出范围
#define ERR_BAD_DX_DIR	-75000

// 定义file type类型
#define XCRAFT_FT_LINK 0
#define XCRAFT_FT_REG_FILE 1
#define XCRAFT_FT_DIR 2

struct XCraft_dir_entry{
     //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
    //le32_to_cpu
    __le32 inode; /* inode number */
    __le16 rec_len; /* record length */
    __u8 name_len; /* name length */
    __u8 file_type; /* file type */
    char name[XCRAFT_NAME_LEN]; /* file name */
};

struct dx_entry{
    __le32 hash; /* hash value */
    __le32 block; /* block + ?逻辑块号 存疑*/
};

//占据一个块
#define XCRAFT_HTREE_VERSION 6
#define XCRAFT_HTREE_VERSION 6
struct dx_root{
    //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
    struct dx_root_info{
   __u8 hash_version; /* hash version */
   __u8 indirect_levels; /* 0 if no dx_node else 1 */
   __le16 limit;//最大目录项数 header + entries
   __le16 count;//目录项数  header + entries
    }info;
    struct dx_entry entries[]; /* entries 一个块后面全部是dx_entry */
};

struct dx_node
{
    //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
    __le16 limit; /* limit */
    __le16 count; /* count */
    struct dx_entry entries[]; /* entries */
};


#ifdef __KERNEL__
struct XCraft_inode_info{
    //ino 逻辑Inode号接在inode->i_no中
    char i_data[32]; /* store symlink content */
    __u32 i_dtime;
    xcraft_group_t i_block_group;//所在组号
    unsigned long i_flags;//标志位 区别哈希树和普通文件等
    unsigned int i_block[XCRAFT_N_BLOCK];//指向数据块的指针
    struct inode vfs_inode;
};
struct XCraft_ibmap_info{
    unsigned long*ifree_bitmap;//inode bitmap
    unsigned long*bfree_bitmap;//block bitmap
};
struct XCraft_superblock_info{
    unsigned long s_blocks_per_group; /* number of blocks per group */
    unsigned long s_inodes_per_group; /* number of inodes per group */
    unsigned long s_last_group_inodes;//最后一个组的inode数
    unsigned long s_last_group_blocks;//最后一个组的块数
    unsigned long s_gdb_count; /* number of group descriptor blocks */
    unsigned long s_desc_per_block; /* number of group descriptors per block */
    __u32 s_La_init_group;//最后一个初始化的块组
    xcraft_group_t s_groups_count; /* number of groups */
    __u32 s_hash_seed[4]; /* hash seed */
    struct buffer_head *s_sbh; /* superblock buffer_head */
    struct XCraft_superblock* s_super; /* superblock */
    struct buffer_head**s_group_desc;//指向组描述符数组的块的指针
    struct XCraft_ibmap_info **s_ibmap_info;//指向bitmap的指针
    
};

struct XCraft_hash_info
{
    __u32 hash; //存储函数计算出来的hash值
    __u32 hash_version; //hash版本
    __u32 *seed; //Hash种子数组
};

struct dx_frame
{
    struct buffer_head *bh;//dx_entry所在磁盘块
    struct dx_entry *entries;//同一级的dx_entry
    struct dx_entry *at;//最终的dx_entry
};

struct dx_map_entry
{
	u32 hash;
	u16 offs;
	u16 size;
};

struct inode *XCraft_iget(struct super_block *sb, unsigned long ino);
int XCraft_destroy_inode_cache(void);
int XCraft_init_inode_cache(void);

/* file functions */
extern const struct file_operations XCraft_file_operations;
extern const struct file_operations XCraft_dir_operations;
extern const struct address_space_operations XCraft_aops;


// static inline struct XCraft_group_desc* get_group_desc(struct XCraft_superblock_info *sbi,xcraft_group_t group, struct buffer_head *bh);
// static inline struct XCraft_group_desc* get_group_desc2(struct XCraft_superblock_info *sbi,xcraft_group_t group);
// static inline int group_free_inodes_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc);
// static inline int group_free_blocks_count(struct XCraft_superblock_info *sbi,struct XCraft_group_desc* desc);
// static inline int inode_get_block_group(struct XCraft_superblock_info *sb_info, uint32_t ino);
// static inline int inode_get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t ino);
// static inline int get_block_group(struct XCraft_superblock_info *sb_info, uint32_t block);
// static inline int get_block_group_shift(struct XCraft_superblock_info *sb_info, uint32_t block);



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

#define XCRAFT_SB(sb) ((struct XCraft_superblock_info *)((sb)->s_fs_info))
#define XCRAFT_I(inode) (container_of(inode, struct XCraft_inode_info, vfs_inode))

#endif /* __KERNEL__ */
#endif // XCRAFT_H