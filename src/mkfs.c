#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <unistd.h>
#include <errno.h>
#include "XCraft.h"

//mkfs write super_block
// 初始化超级块 写到磁盘上 fd是image文件的文件描述符 fstats是文件的stat信息
static struct XCraft_superblock_info *write_superblock(int fd, struct stat *fstats);



//mkfs write group_desc to block
// 初始化组描述符 写到磁盘上
// group_desc可能占用多个块 在main函数算出需要多少块
static int XCraft_write_group_desc(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write inode_bitmap to block
// 初始化inode_bitmap 写到磁盘上 只写第一个块组的inode_bitmap
static int XCraft_write_inode_bitmap(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write block_bitmap to block
// 初始化block_bitmap 写到磁盘上 只写第一个块组的block_bitmap
static int XCraft_write_block_bitmap(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write inode_store
// 初始化inode_store 写到磁盘上 只写第一个块组的inode_store 注意要初始化root inode
static int XCraft_write_inode_store(int fd, struct XCraft_superblock_info *sb_info);


int main(int argc, char **argv){

//1.打开argc[1]文件 img文件 获取文件描述符fd
//2.写超级块
//3.写组描述符
//4.写第一个块组inode_bitmap、block_bitmap、inode_store

    


    return 0;
}



