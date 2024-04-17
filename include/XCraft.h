// htree Ref:https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-2
// block group + dir hash
#ifndef XCRAFT_H
#define XCRAFT_H
#include <linux/types.h>
#include <linux/magic.h>
#define XCRAFT_MAGIC 0x58435241 /* "XCRA" */
#define XCRAFT_BLOCK_SIZE (1 << 12) /* 4 KiB */
#define XCRAFT_N_BLOCK 15   //12个直接索引块 2个1级间接索引块 1个2级间接索引块
typedef xcraft_group_t unsigned int
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
    __le32 i_blocks_lo; /* number of blocks */
    __le32 i_flags; /* file flags B+树等 */
    __le32 i_block[XCRAFT_N_BLOCK]; /* pointers to blocks */
    char i_data[32]; /* store symlink content */
};


struct XCraft_superblock{
    __le32 s_inodes_count; /* number of inodes */
    __le32 s_blocks_count; /* number of blocks */
    __le32 s_free_blocks_count; /* number of free blocks */
    __le32 s_free_inodes_count; /* number of free inodes */
    __le32 s_blocks_per_group; /* number of blocks per group */
    __le32 s_inodes_per_group; /* number of inodes per group */
    __le16 s_magic; /* magic number */
    __le16 s_inode_size; /* inode size */
    __le16 s_desc_size; /* group descriptor size */
};

struct XCraft_superblock_info{
    __le32 s_inodes_count; /* number of inodes */
    __le32 s_blocks_count; /* number of blocks */
    __le32 s_free_blocks_count; /* number of free blocks */
    __le32 s_free_inodes_count; /* number of free inodes */
    __le32 s_blocks_per_group; /* number of blocks per group */
    __le32 s_inodes_per_group; /* number of inodes per group */
    __le16 s_magic; /* magic number */
    __le16 s_inode_size; /* inode size */
    __le16 s_desc_size; /* group descriptor size */
    struct XCraft_superblock s_super;
    
    struct buffer_head*__rcu *s_group_desc;
    
};


#ifdef __KERNEL__
struct XCraft_inode_info{
    char i_data[32]; /* store symlink content */
    xcraft_group_t i_block_group;
    unsigned long i_flags;//标志位 区别哈希树和普通文件等
    __le32 i_block[XCRAFT_N_BLOCK];//指向数据块的指针
    __le32 i_dtime;
    struct inode vfs_inode;
};


#endif /* __KERNEL__ */
#endif // XCRAFT_H