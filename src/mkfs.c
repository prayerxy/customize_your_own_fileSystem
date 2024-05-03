#include <fcntl.h>
#include <stdint.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <unistd.h>
#include <errno.h>
#include "../include/XCraft.h"

struct superblock_padding{
    struct XCraft_superblock xcraft_sb;
    char padding[4060];//4096-36
};

#define XCRAFT_inodes_str_blocks_last(sb) (le32toh((sb)->s_inodes_count) - (le32toh((sb)->s_groups_count) - 1) * le32toh((sb)->s_inodes_per_group)) / XCRAFT_INODES_PER_BLOCK


//mkfs write super_block
// 初始化超级块 写到磁盘上 fd是image文件的文件描述符 fstats是文件的stat信息
static struct superblock_padding *write_superblock(int fd, struct stat *fstats){
    struct superblock_padding* sb=malloc(sizeof(struct superblock_padding));
    if(!sb){
        perror("malloc superblock_padding failed");
        return NULL;
    }
    uint32_t s_blocks_count = fstats->st_size /XCRAFT_BLOCK_SIZE;//向下取整
    uint32_t s_inodes_per_group = XCRAFT_INODES_PER_GROUP;//32768/4
    uint32_t s_inodes_count;
    //注意最后一个块组没有那么多个块
    uint32_t s_blocks_per_group=XCRAFT_BLOCKS_PER_GROUP;//32768
    //注意这里的块组数量可能不是整数
    uint32_t s_groups_count = ceil_div(s_blocks_count, s_blocks_per_group);
    uint32_t s_last_group_blocks = s_blocks_count % s_blocks_per_group;
    if(s_blocks_count % s_blocks_per_group){
        uint32_t last_group_blocks = s_blocks_count % s_blocks_per_group;
        if(last_group_blocks <=1024){
            //如果最后一个块组的块数小于1024个，并入前一个块组
            s_groups_count--;
            if(s_groups_count == 0){
                perror("s_blocks_count is too small");
                free(sb);
                return NULL;
            }
            s_last_group_blocks+=s_blocks_per_group;
        }
       
    }
    else   
        s_last_group_blocks=s_blocks_per_group;//整除
    
    uint32_t last_inode_count=s_last_group_blocks/XCRAFT_INODE_RATIO;
    s_inodes_count = (s_groups_count-1)*s_inodes_per_group + last_inode_count;
    uint32_t mod=last_inode_count%XCRAFT_INODES_PER_BLOCK;
    if(mod){
        s_inodes_count += XCRAFT_INODES_PER_BLOCK-mod;
    }
    /*
    最后一个块组的inode_store不一定是XCRAFT_inodes_str_blocks_PER，
    而是(s_inodes_count-(s_groups_count-1)*s_inodes_per_group)/XCRAFT_INODES_PER_BLOCK
    */
   

    //注意根inode 索引占用一个数据块
    uint32_t bfree_blo;
    if(s_groups_count==1){
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(s_last_group_blocks);
    }
    else bfree_blo=1;
    uint32_t s_free_blocks_count = s_blocks_count-1-XCRAFT_DESC_LIMIT_blo-1-bfree_blo-XCRAFT_inodes_str_blocks_PER-1;
    uint32_t s_free_inodes_count = s_inodes_count-1;
    memset(sb, 0, sizeof(struct superblock_padding));
    sb->xcraft_sb=(struct XCraft_superblock){
        .s_inodes_count = htole32(s_inodes_count),
        .s_blocks_count = htole32(s_blocks_count),
        .s_free_blocks_count = htole32(s_free_blocks_count),
        .s_free_inodes_count = htole32(s_free_inodes_count),
        .s_blocks_per_group = htole32(s_blocks_per_group),
        .s_inodes_per_group = htole32(s_inodes_per_group),
        .s_groups_count = htole32(s_groups_count),
        .s_last_group_blocks = htole32(s_last_group_blocks),
        .s_magic=htole16(XCRAFT_MAGIC),
        .s_inode_size=htole16(XCRAFT_INODE_SIZE),
    };

    int ret=write(fd, sb, sizeof(struct superblock_padding));
    if(ret != sizeof(struct superblock_padding)){
        perror("write superblock failed");
        free(sb);
        return NULL;
    }
    printf(
       "Superblock info:\n" 
       "\ts_inodes_count: %u\n"
       "\ts_blocks_count: %u\n"
        "\ts_free_blocks_count: %u\n"
        "\ts_free_inodes_count: %u\n"
        "\ts_blocks_per_group: %u\n"
        "\ts_inodes_per_group: %u\n"
        "\ts_groups_count: %u\n"
        "\ts_last_group_blocks: %u\n"
        "\ts_magic=%#x(RE)\n"
        "\ts_inode_size: %u\n",
        sb->xcraft_sb.s_inodes_count,sb->xcraft_sb.s_blocks_count,sb->xcraft_sb.s_free_blocks_count,
        sb->xcraft_sb.s_free_inodes_count,sb->xcraft_sb.s_blocks_per_group,sb->xcraft_sb.s_inodes_per_group,
        sb->xcraft_sb.s_groups_count,sb->xcraft_sb.s_last_group_blocks,sb->xcraft_sb.s_magic,sb->xcraft_sb.s_inode_size
    );
    return sb;
    
}



//mkfs write group_desc to block
// 初始化组描述符 写到磁盘上
// group_desc可能占用多个块 在main函数算出需要多少块
static int XCraft_write_group_desc(int fd, struct superblock_padding *sb){
    uint32_t group_desc_size = sb->xcraft_sb.s_groups_count * sizeof(struct XCraft_group_desc);
    //计算desc需要多少块
    uint32_t group_desc_blocks = ceil_div(group_desc_size, XCRAFT_BLOCK_SIZE);
    //限制最大块数量
    if(group_desc_blocks > XCRAFT_DESC_LIMIT_blo){
        perror("group_desc_blocks is too large");
        return -1;
    }
    struct XCraft_group_desc *group_desc = malloc(group_desc_blocks * XCRAFT_BLOCK_SIZE);
    if(!group_desc){
        perror("malloc group_desc failed");
        return -1;
    }
    //清0
    memset(group_desc, 0, group_desc_blocks * XCRAFT_BLOCK_SIZE);
    //初始化第一个块组的组描述符
    group_desc[0].bg_inode_bitmap = htole32(1+XCRAFT_DESC_LIMIT_blo);
    group_desc[0].bg_block_bitmap = htole32(2+XCRAFT_DESC_LIMIT_blo);
    uint16_t t1=le32toh(sb->xcraft_sb.s_inodes_per_group);
    uint16_t t2=le32toh(sb->xcraft_sb.s_blocks_per_group);
    uint32_t bfree_blo;
    if(sb->xcraft_sb.s_groups_count==1){
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(le32toh(sb->xcraft_sb.s_last_group_blocks));
        t2=sb->xcraft_sb.s_blocks_count;
        t1=sb->xcraft_sb.s_inodes_count;
    }
    else bfree_blo=1;
    group_desc[0].bg_inode_table = htole32(2+bfree_blo+XCRAFT_DESC_LIMIT_blo);//inode_bitmap与block_bitmap只占一个块
    group_desc[0].bg_nr_inodes = htole16(t1);
    group_desc[0].bg_nr_blocks = htole16(t2);
    //root inode索引一个数据块
    group_desc[0].bg_free_blocks_count = sb->xcraft_sb.s_free_blocks_count;
    group_desc[0].bg_free_inodes_count = htole16(XCRAFT_INODES_PER_GROUP -1);
    group_desc[0].bg_used_dirs_count = htole16(1);//根目录
    group_desc[0].bg_flags = htole16(XCraft_BG_INODE_INIT| XCraft_BG_BLOCK_INIT);//初始化
    printf(
        " Initialize first bl_group group_desc success\n"
        " wrote %u group_desc blocks\n"
        "\t group_desc size=%ld B\n"
        "\t group[0] bg_nr_inodes=%u\n"
        "\t group[0] bg_nr_blocks=%u\n"
        "\t group[0] inode_table=%u\n"
        "\t group[0] inode count=%u\n",
        group_desc_blocks,sizeof(struct XCraft_group_desc),group_desc[0].bg_nr_inodes,group_desc[0].bg_nr_blocks,group_desc[0].bg_inode_table,group_desc[0].bg_nr_inodes);
    for(uint32_t i=1; i<sb->xcraft_sb.s_groups_count; i++){
        group_desc[i].bg_inode_bitmap = 0;
        group_desc[i].bg_block_bitmap = 0;
        group_desc[i].bg_inode_table = 0;
        group_desc[i].bg_free_blocks_count = 0;
        group_desc[i].bg_free_inodes_count = 0;
        group_desc[i].bg_used_dirs_count = 0;
        group_desc[i].bg_flags = 0;//bl与inode未初始化
        if(i == sb->xcraft_sb.s_groups_count-1){
            //最后一个块组的块数不一定是s_blocks_per_group
            //最后一个块组的inode_store不一定是XCRAFT_inodes_str_blocks_PER
            group_desc[i].bg_nr_blocks = sb->xcraft_sb.s_last_group_blocks;
            uint16_t tmp=XCRAFT_inodes_str_blocks_last(&(sb->xcraft_sb))*XCRAFT_INODES_PER_BLOCK;
            group_desc[i].bg_nr_inodes = htole16(tmp);
            printf("\t last group inode count=%u\n",tmp);
        }
        else{
            group_desc[i].bg_nr_blocks = sb->xcraft_sb.s_blocks_per_group;
            group_desc[i].bg_nr_inodes = sb->xcraft_sb.s_inodes_per_group;
            printf("\t group[%u] inode count=%u\n",i,sb->xcraft_sb.s_inodes_per_group);
        }
    }
    int ret=write(fd, group_desc, group_desc_blocks * XCRAFT_BLOCK_SIZE);
    if(ret != group_desc_blocks * XCRAFT_BLOCK_SIZE){
        perror("write group_desc failed");
        free(group_desc);
        return -1;
    }
    free(group_desc);
    return 0;

}

//mkfs write inode_bitmap to block
// 初始化inode_bitmap 写到磁盘上 只写第一个块组的inode_bitmap
static int XCraft_write_inode_bitmap(int fd, struct superblock_padding *sb){
    char *inode_bitmap = malloc(XCRAFT_BLOCK_SIZE);
    if(!inode_bitmap){
        perror("malloc inode_bitmap failed");
        return -1;
    }
    uint64_t *ifree=(uint64_t *)inode_bitmap;
    memset(ifree, 0xff, XCRAFT_BLOCK_SIZE);
    //根inode索引一个数据块，同时 avoid using inode 0
    //第一个块组的inode_bitmap只占据一个块
    ifree[0] = htole64(0xfffffffffffffffc);
    int ret=write(fd, ifree, XCRAFT_BLOCK_SIZE);
    if(ret!=XCRAFT_BLOCK_SIZE){
        perror("write inode_bitmap failed");
        free(inode_bitmap);
        return -1;
    }
    free(inode_bitmap);
    return 0;
}

//mkfs write block_bitmap to block
// 初始化block_bitmap 写到磁盘上 只写第一个块组的block_bitmap
static int XCraft_write_block_bitmap(int fd, struct superblock_padding *sb){
    uint32_t bfree_blo;
    if(sb->xcraft_sb.s_groups_count==1){
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(le32toh(sb->xcraft_sb.s_last_group_blocks));
    }
    else bfree_blo=1;
    char *block_bitmap = malloc(XCRAFT_BLOCK_SIZE*bfree_blo);
    if(!block_bitmap){
        perror("malloc block_bitmap failed");
        return -1;
    }
    //超级块+组描述符+inode_bitmap+block_bitmap+inode_store+根目录inode索引的数据块
    uint32_t nr_used=1+XCRAFT_DESC_LIMIT_blo+1+bfree_blo+XCRAFT_inodes_str_blocks_PER+1;
    uint64_t*bfree=(uint64_t *)block_bitmap;
    memset(bfree, 0xff, XCRAFT_BLOCK_SIZE*bfree_blo);
    uint32_t i=0;
    while(nr_used){
        uint64_t line = 0xffffffffffffffff;
        //从低位开始清0 直至清除nr_used个位
        for (uint64_t mask = 0x1; mask; mask <<= 1) {
            line &= ~mask;
            nr_used--;
            if (!nr_used)
                break;
        }
        bfree[i] = htole64(line);
        i++;
    }
    int ret=write(fd, bfree, XCRAFT_BLOCK_SIZE*bfree_blo);
    if(ret!=XCRAFT_BLOCK_SIZE*bfree_blo){
        perror("write block_bitmap failed");
        free(block_bitmap);
        return -1;
    }
    free(block_bitmap);
    printf(
        " Initialize first bl_group block_bitmap success\n"
        "wrote %u block_bitmap blocks\n"
        "\t block size=%ld B\n",
        bfree_blo,sizeof(uint64_t)*8);

    
    return 0;

}

//mkfs write inode_store
// 初始化inode_store 写到磁盘上 只写第一个块组的inode_store 注意要初始化root inode
static int XCraft_write_inode_store(int fd, struct superblock_padding *sb){
    char*inode_store = malloc(XCRAFT_BLOCK_SIZE);
    if(!inode_store){
        perror("malloc inode_store failed");
        return -1;
    }
    memset(inode_store, 0, XCRAFT_BLOCK_SIZE);
    //初始化root inode
    struct XCraft_inode *root_inode = (struct XCraft_inode *)inode_store;
     uint32_t bfree_blo;
    if(sb->xcraft_sb.s_groups_count==1){
        bfree_blo=XCRAFT_BFREE_PER_GROUP_BLO(le32toh(sb->xcraft_sb.s_last_group_blocks));
    }
    else bfree_blo=1;
    uint32_t first_data_blo=1+XCRAFT_DESC_LIMIT_blo+1+bfree_blo+XCRAFT_inodes_str_blocks_PER;
    root_inode+=1;
    root_inode->i_mode=htole16(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR |
                            S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
    root_inode->i_uid=0;
    root_inode->i_size_lo=htole32(XCRAFT_BLOCK_SIZE);
    root_inode->i_ctime=root_inode->i_atime=root_inode->i_mtime=root_inode->i_dtime=htole32(0);
    root_inode->i_gid=0;
    //..与.目录项
    root_inode->i_links_count=htole16(2);
    root_inode->i_blocks_lo=htole32(1);
    root_inode->i_flags=0;
    root_inode->i_block[0]=htole32(first_data_blo);

    int ret=write(fd, inode_store, XCRAFT_BLOCK_SIZE);
    memset(inode_store, 0, XCRAFT_BLOCK_SIZE);
    uint32_t i=1;
    uint32_t inode_str_blo=sb->xcraft_sb.s_groups_count==1?XCRAFT_inodes_str_blocks_last(&(sb->xcraft_sb)):XCRAFT_inodes_str_blocks_PER;
    for(i=1;i<inode_str_blo;i++){
        ret=write(fd, inode_store, XCRAFT_BLOCK_SIZE);
        if(ret!=XCRAFT_BLOCK_SIZE){
            perror("write inode_store failed");
            free(inode_store);
            return -1;
        }
    }
    printf(
        " Initialize first bl_group inode_store success\n"
        "wrote %u inode_store blocks\n"
        "\t inode size=%ld B\n",
        i,sizeof(struct XCraft_inode));
    free(inode_store);
    return 0;
    

}

static int XCraft_delayed_initialize(int fd, struct superblock_padding *sb){
    //其他块组在第一个块组无空闲空间时才初始化
    return 0;
}

int main(int argc, char **argv){

//1.打开argc[1]文件 img文件 获取文件描述符fd
//2.写超级块
//3.写组描述符
//4.写第一个块组inode_bitmap、block_bitmap、inode_store
    if(argc != 2){
        fprintf(stderr, "Usage: %s <image file>\n", argv[0]);
        return 1;
    }
    int fd=open(argv[1], O_RDWR);
    if(fd < 0){
        perror("open image file failed");
        return EXIT_FAILURE;
    }
    //获取文件信息
    struct stat fstats;
    int ret=fstat(fd, &fstats);
    if(ret){
        perror("fstat image file failed");
        ret=EXIT_FAILURE;
        goto fclose;
    }
    //得到设备块大小
    if((fstats.st_mode & S_IFMT) == S_IFBLK){
        unsigned long block_size;
        ret=ioctl(fd, BLKSSZGET, &block_size);
        if(ret!=0){
            perror("ioctl BLKSSZGET failed");
            ret=EXIT_FAILURE;
            goto fclose;
        }
       fstats.st_size = block_size;
    }
    //检查Image文件大小是否合法 至少128M
    long int min_size=XCRAFT_BLOCKS_PER_GROUP*XCRAFT_BLOCK_SIZE;
    if(fstats.st_size < min_size){
        fprintf(stderr, "Image file size is too small\n");
        ret=EXIT_FAILURE;
        goto fclose;
    }
    //写超级块
    struct superblock_padding *sb=write_superblock(fd, &fstats);
    if(!sb){
        perror("write_superblock() failed");
        ret= EXIT_FAILURE;
        goto fclose;
    }
    //写group_desc
    ret=XCraft_write_group_desc(fd, sb);
    if(ret){
        perror("write_group_desc() failed");
        ret= EXIT_FAILURE;
        goto free_sb;
    }
    //写一个块组的inode_bitmap
    if(XCraft_write_inode_bitmap(fd, sb)){
        perror("write_inode_bitmap() failed");
        ret= EXIT_FAILURE;
        goto free_sb;
    }
    //写第一个块组的block_bitmap
    if(XCraft_write_block_bitmap(fd, sb)){
        perror("write_block_bitmap() failed");
        ret=EXIT_FAILURE;
        goto free_sb;
    }
    //写第一个块组的inode_store
    if(XCraft_write_inode_store(fd, sb)){
        perror("write_inode_store() failed");
        ret= EXIT_FAILURE;
        goto free_sb;
    }
    if(XCraft_delayed_initialize(fd, sb)){
        perror("delayed_initialize() failed");
        ret= EXIT_FAILURE;
        goto free_sb;
    }
free_sb:
    free(sb);
fclose:
    close(fd);

    return 0;
}