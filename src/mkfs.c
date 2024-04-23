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
static struct XCraft_superblock_info *write_superblock(int fd, struct stat *fstats);

//mkfs write group_desc to block
static int XCraft_write_group_desc(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write inode_bitmap to block
static int XCraft_write_inode_bitmap(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write block_bitmap to block
static int XCraft_write_block_bitmap(int fd, struct XCraft_superblock_info *sb_info);

//mkfs write inode_store
static int XCraft_write_inode_store(int fd, struct XCraft_superblock_info *sb_info);


int main(int argc, char **argv){



    


    return 0;
}



