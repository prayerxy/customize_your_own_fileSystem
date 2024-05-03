#ifndef XCRAFT_BITMAP_H
#define XCRAFT_BITMAP_H

#include "XCraft.h"

static inline int write_group_inode_bitmap(struct XCraft_superblock_info *sbi, xcraft_group_t group){
    // struct XCraft_ibmap_info *ibmap_info = sbi->s_ibmap_info[group];
    // //全部可用
    // memset(ibmap_info->ifree_bitmap, 0xff, XCRAFT_BLOCK_SIZE);
    //一开始已经初始化为全1
    return 0;
}
static inline int write_group_block_bitmap(struct XCraft_superblock_info *sbi, xcraft_group_t group){
    struct XCraft_ibmap_info *ibmap_info = sbi->s_ibmap_info[group];
    uint64_t*bfree=(uint64_t *)ibmap_info->bfree_bitmap;
    struct buffer_head *bh = NULL;
    struct XCraft_group_desc *desc=get_group_desc(sbi,group,bh);
    uint32_t bfree_block=XCRAFT_BFREE_PER_GROUP_BLO(le16_to_cpu(desc->bg_nr_blocks));

    //被位图及inodes占用的块数 剩余的是data_blocks
    uint32_t nr_used= desc->bg_nr_blocks-desc->bg_free_blocks_count;
    uint32_t i;
    memset(bfree, 0xff, XCRAFT_BLOCK_SIZE*bfree_block);
    i = 0;
    while(nr_used){
        uint64_t line = 0xffffffffffffffff;
        //从低位开始清0 直至清除nr_used个位
        uint64_t mask;
        for (mask = 0x1; mask; mask <<= 1) {
            line &= ~mask;
            nr_used--;
            if (!nr_used)
                break;
        }
        bfree[i] = cpu_to_le64(line);
        i++;
    }
    //磁盘上没有更改bitmap，但是内存中的bitmap已经改变，在之后同步再写入磁盘
    //注意可能bitmap有多个块
    return 0;
}
//寻找现有可用块组中是否有空闲的inode
//如果没有，需要新建一个块组 更改块组描述符为Init状态
// get first free inode
//返回desc  offset是desc在此块中的偏移量
static inline struct XCraft_group_desc* new_gb_desc(struct XCraft_superblock_info* sbi,struct buffer_head *group_desc_bh){
    int blo_t=0;
    struct buffer_head *bh=sbi->s_group_desc[blo_t];
    struct XCraft_group_desc *desc=NULL;
    uint32_t s_La_init_group=sbi->s_La_init_group;
    uint32_t s_groups_count=sbi->s_groups_count;
    uint32_t s_desc_per_block=sbi->s_desc_per_block;
    uint32_t s_gdb_count=sbi->s_gdb_count;//gdb占用的块数
    desc=(struct XCraft_group_desc *)bh->b_data;
    int flag=0;
    int ret=0;
    if(s_La_init_group+1>=s_groups_count){
        printk("No more group descriptor block\n");
        return NULL;
    }
    //说明前一个块组是常规块组
    if(s_La_init_group+1<s_desc_per_block){
        if(!XCraft_BG_ISINIT(le16_to_cpu(desc[s_La_init_group].bg_flags))){
            //初始化这个块组
            //同时初始化inode bitmap block bitmap inode table
            uint32_t uninit=s_La_init_group+1;
            uint32_t bg_inode_bitmap=le32_to_cpu(desc[s_La_init_group].bg_inode_bitmap);
            bg_inode_bitmap=bg_inode_bitmap+le32_to_cpu(desc[s_La_init_group].bg_nr_blocks);//inode bitmap的位置
            uint32_t bg_block_bitmap=bg_inode_bitmap+XCRAFT_IFREE_PER_GROUP_BLO;//block bitmap的位置
            uint32_t bg_inode_table=bg_block_bitmap+XCRAFT_BFREE_PER_GROUP_BLO(le16_to_cpu(desc[uninit].bg_nr_blocks));//inode table的位置
            desc[uninit].bg_block_bitmap=cpu_to_le32(bg_block_bitmap);
            desc[uninit].bg_inode_bitmap=cpu_to_le32(bg_inode_bitmap);
            desc[uninit].bg_inode_table=cpu_to_le32(bg_inode_table);
            
            // assert(desc[uninit].bg_nr_inodes!=0);
            uint32_t inode_str_blos=le16_to_cpu(desc[uninit].bg_nr_inodes)/XCRAFT_INODES_PER_BLOCK;
            desc[uninit].bg_free_blocks_count=cpu_to_le32(le16_to_cpu(desc[uninit].bg_nr_blocks)-XCRAFT_IFREE_PER_GROUP_BLO-XCRAFT_BFREE_PER_GROUP_BLO(le16_to_cpu(desc[uninit].bg_nr_blocks))-inode_str_blos);
            desc[uninit].bg_free_inodes_count=cpu_to_le32(desc[uninit].bg_nr_inodes);
            desc[uninit].bg_flags=cpu_to_le16(XCraft_BG_INODE_INIT|XCraft_BG_BLOCK_INIT);
            flag=1;
            ret=s_La_init_group;
            goto out;
        }
    }
    else{
        uint32_t next_init_group =s_La_init_group%s_desc_per_block;//在下一个块中的块组描述符偏移量
        next_init_group++;
        if(next_init_group+1>s_desc_per_block){
            printk("No more group descriptor block\n");
            return NULL;
        }
        blo_t++;
        bh=sbi->s_group_desc[blo_t];
        if(!bh)
            return NULL;
        struct XCraft_group_desc* desc2=(struct XCraft_group_desc *)bh->b_data;
        if(!XCraft_BG_ISINIT(le16_to_cpu(desc2[next_init_group].bg_flags))){
            //初始化这个块组
            //同时初始化inode bitmap block bitmap inode table

            struct XCraft_group_desc prev_desc;
            prev_desc=(next_init_group-1)<0?desc[s_desc_per_block-1]:desc2[next_init_group-1];

            uint32_t bg_inode_bitmap=le32_to_cpu(prev_desc.bg_inode_bitmap);
            bg_inode_bitmap=bg_inode_bitmap+le32_to_cpu(prev_desc.bg_nr_blocks);//inode bitmap的位置
            
            uint32_t bg_block_bitmap=bg_inode_bitmap+XCRAFT_IFREE_PER_GROUP_BLO;//block bitmap的位置
            uint32_t bg_inode_table=bg_block_bitmap+XCRAFT_BFREE_PER_GROUP_BLO(le16_to_cpu(desc2[next_init_group].bg_nr_blocks));//inode table的位置
            desc2[next_init_group].bg_block_bitmap=cpu_to_le32(bg_block_bitmap);
            desc2[next_init_group].bg_inode_bitmap=cpu_to_le32(bg_inode_bitmap);
            desc2[next_init_group].bg_inode_table=cpu_to_le32(bg_inode_table);
            
            //assert(desc2[next_init_group].bg_nr_inodes!=0);
            uint32_t inode_str_blos=le16_to_cpu(desc2[next_init_group].bg_nr_inodes)/XCRAFT_INODES_PER_BLOCK;
            desc2[next_init_group].bg_free_blocks_count=cpu_to_le32(le16_to_cpu(desc2[next_init_group].bg_nr_blocks)-XCRAFT_IFREE_PER_GROUP_BLO-XCRAFT_BFREE_PER_GROUP_BLO(le16_to_cpu(desc2[next_init_group].bg_nr_blocks))-inode_str_blos);
            desc2[next_init_group].bg_free_inodes_count=cpu_to_le32(desc2[next_init_group].bg_nr_inodes);
            desc2[next_init_group].bg_flags=cpu_to_le16(XCraft_BG_INODE_INIT|XCraft_BG_BLOCK_INIT);

            desc=desc2;//为了在out中更新super的free_blocks_count
            flag=1;
            ret=next_init_group;
            goto out;
            
        }

    }

out:
    if(flag){
        //更新内存的bitmap
        s_La_init_group++;
        write_group_inode_bitmap(sbi,s_La_init_group);
        write_group_block_bitmap(sbi,s_La_init_group);

        //更新super的free_blocks_count
        uint32_t free_blocks_count=le32_to_cpu(sbi->s_super->s_free_blocks_count);
        free_blocks_count-=le16_to_cpu(desc[ret].bg_nr_blocks)-le16_to_cpu(desc[ret].bg_free_blocks_count);
        sbi->s_super->s_free_blocks_count=cpu_to_le32(free_blocks_count);
        sbi->s_La_init_group=s_La_init_group;//最后一个初始化的块组位置

        struct buffer_head *bh2 = sbi->s_sbh;
        struct XCraft_superblock *tmp = (struct XCraft_superblock *)bh2->b_data;
        memcpy((char *)tmp, (char *)sbi->s_super, sizeof(struct XCraft_superblock));

        group_desc_bh=bh;
        mark_buffer_dirty(bh2);
        mark_buffer_dirty(bh);
        return desc+ret;
    }
    else return NULL;
}

//inode_begin是此块组的第一个inode号
static inline uint32_t get_first_free_bits(unsigned long *freemap,
                                           unsigned long size,
                                           uint32_t len,uint32_t begin)
{

    uint32_t bit, prev = 0, count = 0;
    for_each_set_bit (bit, freemap, size) {
        if (prev != bit - 1)
            count = 0;
        prev = bit;
        if (++count == len) {
            bitmap_clear(freemap, bit - len + 1, len);
            return (bit - len + 1)+begin;
        }
    }
    return 0;
}

//寻找现有可用块组中是否有空闲的inode
//如果没有，需要新建一个块组 更改块组描述符为Init状态
// get first free inode

/* Return an unused inode number and mark it used.
 * Return 0 if no free inode was found.
 */
static inline uint32_t get_free_inode(struct XCraft_superblock_info *sbi){
    xcraft_group_t group = sbi->s_La_init_group;
    struct buffer_head *bh = NULL;
    int i;
    
    struct XCraft_group_desc *desc = get_group_desc(sbi, group, bh);
    if(group_free_inodes_count(sbi, desc) == 0){
        //先遍历所有块组
        for(i = 0; i < sbi->s_La_init_group; i++){
            desc = get_group_desc(sbi, i, bh);
            if(group_free_inodes_count(sbi, desc) > 0){
                group = i;
                break;
            }
        }
        if(group_free_inodes_count(sbi, desc) == 0){
            desc = new_gb_desc(sbi,bh);
            group = sbi->s_La_init_group;
            if(!desc)//没有更多的块组描述符
                return 0;
        }
    }
    //找到了空闲的inode及块组描述符desc
    // unsigned long*ifree_bitmap=kzalloc(XCRAFT_IFREE_PER_GROUP_BLO,GFP_KERNEL);
    uint32_t inode_begin=0;
    if(group==0)
        inode_begin=0;
    else//说明不是第一个块组
        inode_begin=XCRAFT_INODES_PER_GROUP*(group);
    uint32_t ret=get_first_free_bits(sbi->s_ibmap_info[group]->ifree_bitmap,desc->bg_nr_inodes,1,inode_begin);
    if(ret){
        desc->bg_free_inodes_count=cpu_to_le16(le16_to_cpu(desc->bg_free_inodes_count)-1);
        uint32_t free_inodes_count=le32_to_cpu(sbi->s_super->s_free_inodes_count);
        free_inodes_count--;
        sbi->s_super->s_free_inodes_count=cpu_to_le32(free_inodes_count);
        return ret;
    }
    return 0; 
}


/* Return 'len' unused block(s) number and mark it used.
 * Return 0 if no enough free block(s) were found.
 */
// get len free blocks
static inline int get_free_blocks(struct XCraft_superblock_info *sbi, int len){
    xcraft_group_t group = sbi->s_La_init_group;
    struct buffer_head *bh = NULL;
    struct XCraft_group_desc *desc = get_group_desc(sbi, group, bh);
    int i;
    if(group_free_blocks_count(sbi, desc) < len){
        //先遍历所有块组
        for(i = 0; i < sbi->s_La_init_group; i++){
            desc = get_group_desc(sbi, i, bh);
            if(group_free_blocks_count(sbi, desc) >= len){
                group = i;
                break;
            }
        }
        if(group_free_blocks_count(sbi, desc) < len){
            desc = new_gb_desc(sbi,bh);
            group = sbi->s_La_init_group;
            if(!desc)//没有更多的块组描述符
                return 0;
        }
    }
    //找到了空闲的block及块组描述符desc
    uint32_t blocks_begin=0;
    if(group==0)
        blocks_begin=0;
    else//说明不是第一个块组
        blocks_begin=XCRAFT_BLOCKS_PER_GROUP*(group);
    uint32_t ret=get_first_free_bits(sbi->s_ibmap_info[group]->bfree_bitmap,desc->bg_nr_blocks,len,blocks_begin);
    if(ret){
        //不连续Len会报错 待解决todo
        desc->bg_free_blocks_count=cpu_to_le16(le16_to_cpu(desc->bg_free_blocks_count)-len);
        uint32_t free_blocks_count=le32_to_cpu(sbi->s_super->s_free_blocks_count);
        free_blocks_count-=len;
        sbi->s_super->s_free_blocks_count=cpu_to_le32(free_blocks_count);
        return ret;
    }
    return 0; 
}


/* Mark the 'len' bit(s) from i-th bit in freemap as free (i.e. 1) */
static inline int put_free_bits(unsigned long *freemap,
                                unsigned long size,
                                uint32_t i,
                                uint32_t len)
{
    /* i is greater than freemap size */
    if (i + len - 1 > size)
        return -1;

    bitmap_set(freemap, i, len);

    return 0;
}

// 将一个inode置为不再使用，即空闲
static inline void put_inode(struct XCraft_superblock_info *sbi, uint32_t ino){

    xcraft_group_t group = inode_get_block_group(sbi, ino);
    uint32_t offset = inode_get_block_group_shift(sbi, ino);
    struct buffer_head *bh = NULL;
    struct XCraft_group_desc *desc = get_group_desc(sbi, group, bh);
    if(put_free_bits(sbi->s_ibmap_info[group]->ifree_bitmap, desc->bg_nr_inodes, offset, 1))
        return;
    
    desc->bg_free_inodes_count=cpu_to_le16(le16_to_cpu(desc->bg_free_inodes_count)+1);
    uint32_t free_inodes_count=le32_to_cpu(sbi->s_super->s_free_inodes_count);
    free_inodes_count++;
    sbi->s_super->s_free_inodes_count=cpu_to_le32(free_inodes_count);

}

// 将连续len个block置为不再使用，即空闲
// uint32_t block为起始块号
static inline void put_blocks(struct XCraft_superblock_info *sbi, uint32_t block, int len){
    xcraft_group_t group = get_block_group(sbi, block);
    uint32_t offset = get_block_group_shift(sbi, block);
    struct buffer_head *bh = NULL;
    struct XCraft_group_desc *desc = get_group_desc(sbi, group, bh);
    if(put_free_bits(sbi->s_ibmap_info[group]->bfree_bitmap, desc->bg_nr_blocks, offset, len))
        return;
    
    desc->bg_free_blocks_count=cpu_to_le16(le16_to_cpu(desc->bg_free_blocks_count)+len);
    uint32_t free_blocks_count=le32_to_cpu(sbi->s_super->s_free_blocks_count);
    free_blocks_count+=len;
    sbi->s_super->s_free_blocks_count=cpu_to_le32(free_blocks_count);
}




#endif /* XCRAFT_BITMAP_H*/