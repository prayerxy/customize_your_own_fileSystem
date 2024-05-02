
#ifndef XCRAFT_BITMAP_H
#define XCRAFT_BITMAP_H

#include <stdlib.h>
#include "XCraft.h"
#include "gb.h"


//寻找现有可用块组中是否有空闲的inode
//如果没有，需要新建一个块组 更改块组描述符为Init状态
// get first free inode
//返回desc  offset是desc在此块中的偏移量
static inline struct XCraft_group_desc* new_gb_desc(struct XCraft_superblock_info* sbi,buffer_head *group_desc_bh){
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
    if(s_La_init_group>=s_groups_count){
        printk("No more group descriptor block\n");
        return NULL;
    }
    if(s_La_init_group+1<s_desc_per_block){
        if(!XCraft_BG_ISINIT(le16toh(desc[s_La_init_group].bg_flags))){
            //初始化这个块组
            //同时初始化inode bitmap block bitmap inode table
            uint32_t uninit=s_La_init_group+1;
            uint32_t bg_inode_bitmap=le32toh(desc[s_La_init_group].bg_inode_bitmap);
            bg_inode_bitmap=bg_inode_bitmap+le32toh(desc[s_La_init_group].bg_nr_blocks);//inode bitmap的位置
            uint32_t bg_block_bitmap=bg_inode_bitmap+XCRAFT_IFREE_PER_GROUP_BLO;//block bitmap的位置
            uint32_t bg_inode_table=bg_block_bitmap+XCRAFT_BFREE_PER_GROUP_BLO(le16toh(desc[uninit].bg_nr_blocks));//inode table的位置
            desc[uninit].bg_block_bitmap=htole32(bg_block_bitmap);
            desc[uninit].bg_inode_bitmap=htole32(bg_inode_bitmap);
            desc[uninit].bg_inode_table=htole32(bg_inode_table);
            
            assert(desc[uninit].bg_nr_inodes!=0);
            uint32_t inode_str_blos=le16toh(desc[uninit].bg_nr_inodes)/XCRAFT_INODES_PER_BLOCK;
            desc[uninit].bg_free_blocks_count=htole32(le16toh(desc[uninit].bg_nr_blocks)-XCRAFT_IFREE_PER_GROUP_BLO-XCRAFT_BFREE_PER_GROUP_BLO(le16toh(desc[uninit].bg_nr_blocks))-inode_str_blos);
            desc[uninit].bg_free_inodes_count=htole32(desc[uninit].bg_nr_inodes);
            desc[uninit].bg_flags=htole16(XCraft_BG_INODE_INIT|XCraft_BG_BLOCK_INIT);
            flag=1;
            ret=s_La_init_group;
            goto out;
        }
    }
    else{
        s_La_init_group%=s_desc_per_block;//在下一个块中的块组描述符偏移量
        if(s_gdb_count==1){
            printk("No more group descriptor block\n");
            return NULL;
        }
        blo_t++;
        bh=sbi->s_group_desc[blo_t];
        if(!bh)
            return NULL;
        struct XCraft_group_desc desc2=(struct XCraft_group_desc *)bh->b_data;
        if(!XCraft_BG_ISINIT(le16toh(desc2[i].bg_flags))){
            //初始化这个块组
            //同时初始化inode bitmap block bitmap inode table
            s_La_init_group++;//我们要初始化的位置
            struct XCraft_group_desc prev_desc;
            prev_desc=(s_La_init_group-1)<0?desc[s_desc_per_block-1]:desc2[s_La_init_group-1];

            uint32_t bg_inode_bitmap=le32toh(prev_desc.bg_inode_bitmap);
            bg_inode_bitmap=bg_inode_bitmap+le32toh(prev_desc.bg_nr_blocks);//inode bitmap的位置
            
            uint32_t bg_block_bitmap=bg_inode_bitmap+XCRAFT_IFREE_PER_GROUP_BLO;//block bitmap的位置
            uint32_t bg_inode_table=bg_block_bitmap+XCRAFT_BFREE_PER_GROUP_BLO(le16toh(desc2[s_La_init_group].bg_nr_blocks));//inode table的位置
            desc2[s_La_init_group].bg_block_bitmap=htole32(bg_block_bitmap);
            desc2[s_La_init_group].bg_inode_bitmap=htole32(bg_inode_bitmap);
            desc2[s_La_init_group].bg_inode_table=htole32(bg_inode_table);
            
            assert(desc2[s_La_init_group].bg_nr_inodes!=0);
            uint32_t inode_str_blos=le16toh(desc2[s_La_init_group].bg_nr_inodes)/XCRAFT_INODES_PER_BLOCK;
            desc2[s_La_init_group].bg_free_blocks_count=htole32(le16toh(desc2[s_La_init_group].bg_nr_blocks)-XCRAFT_IFREE_PER_GROUP_BLO-XCRAFT_BFREE_PER_GROUP_BLO(le16toh(desc2[s_La_init_group].bg_nr_blocks))-inode_str_blos);
            desc2[s_La_init_group].bg_free_inodes_count=htole32(desc2[s_La_init_group].bg_nr_inodes);
            desc2[s_La_init_group].bg_flags=htole16(XCraft_BG_INODE_INIT|XCraft_BG_BLOCK_INIT);

            desc=desc2;//为了在out中更新super的free_blocks_count
            flag=1;
            ret=s_La_init_group;
            goto out;
            
        }

    }

out:
    if(flag){
        //更新super的free_blocks_count
        uint32_t free_blocks_count=le32toh(sbi->s_super->s_free_blocks_count);
        free_blocks_count-=le16toh(desc[ret].bg_nr_blocks)-le16toh(desc[ret].bg_free_blocks_count);
        sbi->s_super->s_free_blocks_count=htole32(free_blocks_count);
        sbi->s_La_init_group++;//最后一个初始化的块组位置
        struct buffer_head *bh2 = sbi->s_super->s_sbh;
        struct XCraft_superblock *tmp = (struct XCraft_superblock *)bh2->b_data;
        memcpy((char *)tmp, (char *)sbi->s_super, sizeof(struct XCraft_superblock));

        group_desc_bh=bh;
        mark_buffer_dirty(bh2);
        mark_buffer_dirty(bh);
        return desc+ret;
    }
    else return NULL;
}


//寻找现有可用块组中是否有空闲的inode
//如果没有，需要新建一个块组 更改块组描述符为Init状态
// get first free inode

static inline uint32_t get_free_inode(struct XCraft_superblock_info *sbi){
    xcraft_group_t group = sbi->s_La_init_group;
    struct buffer_head *bh = NULL;
    
    struct XCraft_group_desc *desc = get_group_desc(sbi, group, bh);
    if(group_free_inodes_count(sbi, desc) == 0){
        //先遍历所有块组
        for(int i = 0; i < sbi->s_groups_count; i++){
            desc = get_group_desc(sbi, i, bh);
            if(group_free_inodes_count(sbi, desc) > 0){
                group = i;
                break;
            }
        }
        if(group_free_inodes_count(sbi, desc) == 0){
            desc = new_gb_desc(sbi,bh);
            if(!desc)//没有更多的块组描述符
                return 0;
        }
    }
    //找到了空闲的inode及块组描述符desc
    unsigned long*ifree_bitmap=kzalloc(XCRAFT_IFREE_PER_GROUP_BLO,GFP_KERNEL);

  
    
}

// get first free block
static inline uint32_t get_free_block(struct XCraft_superblock_info *sbi);


// get len free inodes
static inline int get_len_free_inodes(struct XCraft_superblock_info *sbi, int len);


// get len free blocks
static inline int get_len_free_blocks(struct XCraft_superblock_info *sbi, int len);

// 将一个inode置为不再使用，即空闲
static inline void XCraft_put_inode(struct XCraft_superblock_info *sbi, uint32_t ino);

// 将连续len个block置为不再使用，即空闲
// uint32_t block为起始块号
static inline void XCraft_put_block(struct XCraft_superblock_info *sbi, uint32_t block, int len);




#endif