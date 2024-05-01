#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "XCraft.h"
//寻找现有可用块组中是否有空闲的inode
//如果没有，需要新建一个块组 更改块组描述符为Init状态
// get first free inode

int get_free_inode(struct XCraft_superblock_info sbi);

// get first free block
int get_free_block(struct XCraft_superblock_info sbi);


// get len free inodes
int get_len_free_inodes(struct XCraft_superblock_info sbi);


// get len free blocks
int get_len_free_blocks(struct XCraft_superblock_info sbi);