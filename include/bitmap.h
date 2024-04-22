#include <linux/bitmap.h>
#include <linux/buffer_head.h>
#include "XCraft.h"
// get first free inode
int get_free_inode(struct XCraft_superblock_info sbi);

// get first free block
int get_free_block(struct XCraft_superblock_info sbi);


// get len free inodes
int get_len_free_inodes(struct XCraft_superblock_info sbi);


// get len free blocks
int get_len_free_blocks(struct XCraft_superblock_info sbi);