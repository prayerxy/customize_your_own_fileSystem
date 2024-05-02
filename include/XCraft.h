// htree Ref:https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-2
//  Ref：https://github.com/sysprog21/simplefs/tree/master
// block group + dir hash
#ifndef XCRAFT_H
#define XCRAFT_H
#include <linux/types.h>
#include <linux/version.h>
#include <stdint.h>
#include <stdint.h>
#define XCRAFT_MAGIC 0x58435241 /* "XCRA" */
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
#define XCraft_BG_ISINIT(flag) (flag&XCraft_BG_INODE_INIT)||(flag&XCraft_BG_BLOCK_INIT)
// inode flags
#define XCraft_INODE_HASH_TREE 0x0001
#define XCraft_INODE_HASH_TREE_IS(flag) flag & XCraft_INODE_HASH_TREE

/* todo
XCraft partition layout
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  descriptor   |  sb->nr_group_descs blocks
 * +---------------+
 * | ifree bitmap  |  sb->nr_ifree_blocks blocks
 * +---------------+
 * | bfree bitmap  |  sb->nr_bfree_blocks blocks
 * +---------------+
 * |    data       |
 * |      blocks   |  rest of the blocks
 * +---------------+
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
#define XCRAFT_inodes_str_blocks_PER (XCRAFT_INODES_PER_GROUP)/XCRAFT_INODES_PER_BLOCK

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
#define XCRAFT_inodes_str_blocks_last(sb) (le32_to_cpu((sb)->s_inodes_count) - (le32_to_cpu((sb)->s_groups_count) - 1) * le32_to_cpu((sb)->s_inodes_per_group)) / XCRAFT_INODES_PER_BLOCK

struct XCraft_group_desc{
    __le32 bg_block_bitmap; /* block bitmap block 物理块号*/
    __le32 bg_inode_bitmap; /* inode bitmap block 物理块号*/
    __le32 bg_inode_table; /* 物理块号*/
    __le16 bg_free_blocks_count; /* number of free blocks */
    __le16 bg_free_inodes_count; /* number of free inodes */
    __le16 bg_used_dirs_count; /* number of directories */
    __le16 bg_flags;/*EXT4_BG_flags(INODE_UNINIT,etc) 表示该块组的信息 初始化or未初始化*/
};
#define XCRAFT_GROUP_DESC_SIZE sizeof(struct XCraft_group_desc)
#define XCRAFT_GROUP_DESCS_PER_BLOCK (XCRAFT_BLOCK_SIZE / XCRAFT_GROUP_DESC_SIZE)


//最开始建树的时候，10个以上分类成哈希树
#define XCRAFT_dentry_LIMIT 10
struct XCraft_dir_entry{
     //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
    //le32_to_cpu
    __le32 inode; /* inode number */
    __le16 rec_len; /* record length */
    __u8 file_type; /* file type */
    char name[XCRAFT_NAME_LEN]; /* file name */
};

struct dx_entry{
    __le32 hash; /* hash value */
    __le32 block; /* block + ?逻辑块号 存疑*/
};

//占据一个块
struct dx_root{
    //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
   __u8 hash_version; /* hash version */
   __u8 indirect_levels; /* 0 if no dx_node else 1 */
   __le16 limit;//最大目录项数 header + entries
   __le16 count;//目录项数  header + entries
   struct dx_entry entries[0]; /* entries 一个块后面全部是dx_entry */
};

struct dx_node
{
    //对于 u8 类型的数据，字节序转换是不必要的。
    //其他数据要转换 le16_to_cpu
    __le16 limit; /* limit */
    __le16 count; /* count */
    struct dx_entry entries[0]; /* entries */
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

struct XCraft_superblock_info{
    unsigned long s_blocks_per_group; /* number of blocks per group */
    unsigned long s_inodes_per_group; /* number of inodes per group */
    unsigned long s_last_group_inodes;//最后一个组的inode数
    unsigned long s_last_group_blocks;//最后一个组的块数
    unsigned long s_gdb_count; /* number of group descriptor blocks */
    unsigned long s_desc_per_block; /* number of group descriptors per block */
    xcraft_group_t s_groups_count; /* number of groups */
    struct buffer_head *s_sbh; /* superblock buffer_head */
    struct XCraft_superblock* s_super; /* superblock */
    struct buffer_head**s_group_desc;//指向组描述符数组的块的指针
};

struct XCraft_hash_info
{
    __u32 hash;//存储函数计算出来的hash值
    __u32 hash_version;//hash版本
    __u32 *seed;//Hash种子数组
};

struct XCraft_frame
{
    struct buffer_head *bh;//dx_entry所在磁盘块
    struct dx_entry *entries;//同一级的dx_entry
    struct dx_entry *at;//最终的dx_entry
};


#define XCRAFT_SB(sb) ((struct XCraft_superblock_info *)((sb)->s_fs_info))
#define XCRAFT_I(inode) (container_of(inode, struct XCraft_inode_info, vfs_inode))

#endif /* __KERNEL__ */
#endif // XCRAFT_H