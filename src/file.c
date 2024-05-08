#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>

#include "../include/bitmap.h"
#include "../include/XCraft.h"

// 普通文件中由逻辑iblock将指定的物理块和bh_result相关联，如果物理块不存在，create字段为1则创建一个
// iblock是索引 从0开始
static int XCraft_file_get_block(struct inode *inode,
                                   sector_t iblock,
                                   struct buffer_head *bh_result,
                                   int create)
{
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_inode_info *inode_info = XCRAFT_I(inode);
	struct buffer_head *bh, *bh2, *bh3;
	// 获取i_block数组
	unsigned int *i_block = inode_info->i_block;

	// bno存储最后我们找到的块号 tmp是中间块号存储变量
	int ret, bno, bno2;
	// 文件最大大小
	unsigned int max_file_blocks;
	// 一个块能容纳多少个块号
	unsigned int bno_num_per_block = XCRAFT_BLOCK_SIZE / sizeof(__le32);

	max_file_blocks = XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block + XCRAFT_N_DOUBLE_INDIRECT * bno_num_per_block * bno_num_per_block;

	if(iblock >= max_file_blocks)
		return -EFBIG;

	// 当前自身文件的内容大小
	unsigned int i_size = inode->i_size;
	uint32_t i_blocks = ceil_div(i_size, XCRAFT_BLOCK_SIZE);
	ret = 0;
	// 与当前文件占的块数比较
	// 超过是否能创建
	if(iblock >= i_blocks && !create){
		printk("iblock >= i_blocks and create = 0");
		ret = 0;
		goto end;
	}
	
	// 可以创建
	// 查找

	// 查找一级间接索引块的时候使用
	unsigned int indirect_index, indirect_offset, indirect_bno;

	// 查找二级间接索引块的时候使用
	unsigned int double_indirect_index, double_indirect_one_offset, double_indirect_two_offset, double_indirect_bno;
	// 索引块读取使用 一级和两级
	__le32 *bno_block, *bno2_block;

	// 查找一级索引块
	if(iblock < XCRAFT_N_DIRECT){
		// 是否需要分配
		if(iblock >= i_blocks){
			// 进入到此条件语句，create一定是1
			bno = get_free_blocks(sb_info, 1);
			if(!bno){
				// 分配失败
				ret = -ENOSPC;
				goto end;
			}
			// 将分配的块进行初始化
			bh = sb_bread(sb, bno);
			if(!bh){
				ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 分配成功需要更新inode
			i_block[iblock] = bno;
		}
		// 获取iblock对应的物理块块号
		bno = i_block[iblock];
		mark_inode_dirty(inode);
	}
	else if(iblock >= XCRAFT_N_DIRECT && iblock < XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block){
	    // 此时iblock对应的块在一级间接索引块中
		indirect_index = (iblock - XCRAFT_N_DIRECT) / bno_num_per_block;
		indirect_offset = (iblock - XCRAFT_N_DIRECT) % bno_num_per_block;
		indirect_bno = i_block[XCRAFT_N_DIRECT + indirect_index];
		if(!indirect_bno){
			// 表示此时其还没有被初始化，必定iblock >= i_blocks && !create
			indirect_bno = get_free_blocks(sb_info, 1);
			if(!indirect_bno){
			    ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh = sb_bread(sb, indirect_bno);
			if(!bh){
			    ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 更新inode信息
			i_block[XCRAFT_N_DIRECT + indirect_index] = indirect_bno;
		}
		bh = sb_bread(sb, indirect_bno);
		if(!bh){
		    ret = -EIO;
			goto end;
		}
		// 依据indirect_offset获取对应的块号
		bno_block = (__le32 *)bh->b_data;
		bno = le32_to_cpu(bno_block[indirect_offset]);
		if(!bno){
			// 此时此bno对应的块没有被分配
			bno = get_free_blocks(sb_info, 1);
			if(!bno){
				ret = -ENOSPC;
				goto end;
			}

			// 对新分配的块进行初始化
			bh2 = sb_bread(sb, bno);
			if(!bh2){
			    ret = -EIO;
				goto end;
			}
			memset(bh2->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh2);
			brelse(bh2);
			bno_block[indirect_offset] = cpu_to_le32(bno); 
			mark_buffer_dirty(bh);
			brelse(bh);
		}
		mark_inode_dirty(inode);
	}
	else{
		double_indirect_index = (iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) / (bno_num_per_block * bno_num_per_block);
		double_indirect_one_offset = ((iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block))/ bno_num_per_block;
		double_indirect_two_offset = ((iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block))% bno_num_per_block;
		double_indirect_bno = i_block[XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT + double_indirect_index];
		if(!double_indirect_bno){
		    double_indirect_bno = get_free_blocks(sb_info, 1);
			if(!double_indirect_bno){
				ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh = sb_bread(sb, double_indirect_bno);
			if(!bh){
			    ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 更新inode信息
			i_block[XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT + double_indirect_index] = double_indirect_bno;
		}
		bh = sb_bread(sb, double_indirect_bno);
		if(!bh){
		    ret = -EIO;
			goto end;
		}
		// 依据double_indirect_one_offset获取对应的块号
		bno2_block = (__le32 *)bh->b_data;
		bno2 = le32_to_cpu(bno2_block[double_indirect_one_offset]);
		if(!bno2){
			// 此时此bno对应的块没有分配
			bno2 = get_free_blocks(sb_info, 1);
			if(!bno2){
				ret = -ENOSPC;
				goto end;
			}

			// 对新分配的块进行初始化
			bh2 = sb_bread(sb, bno2);
			if(!bh2){
			    ret = -EIO;
				goto end;
			}
			memset(bh2->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh2);
			brelse(bh2);
			bno2_block[double_indirect_one_offset] = cpu_to_le32(bno2);
			mark_buffer_dirty(bh);
			brelse(bh);
		}
		bh = sb_bread(sb, bno2);
		if(!bh){
		    ret = -EIO;
			goto end;
		}

		bno_block = (__le32 *)bh->b_data;
		bno = le32_to_cpu(bno_block[double_indirect_two_offset]);
		if(!bno){
		    // 此时此bno对应的块没有分配
			bno = get_free_blocks(sb_info, 1);
			if(!bno){
				ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh3 = sb_bread(sb, bno);
			if(!bh3){
			    ret = -EIO;
				goto end;
			}
			memset(bh3->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh3);
			brelse(bh3);
			bno_block[double_indirect_two_offset] = cpu_to_le32(bno);
			mark_buffer_dirty(bh);
			brelse(bh);
		}
		mark_inode_dirty(inode);
	}
	
	map_bh(bh_result, sb, bno);

end:
	return ret;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static void XCraft_readahead(struct readahead_control *rac)
{
	mpage_readahead(rac, XCraft_file_get_block);
}
#else
static int XCraft_readpage(struct file *file, struct page *page)
{

	return mpage_readpage(page, XCraft_file_get_block);
}
#endif

static int XCraft_writepage(struct page *page,
			  struct writeback_control *wbc)
{

	return block_write_full_page(page, XCraft_file_get_block, wbc);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static int XCraft_write_begin(struct file *file, struct address_space *mapping,
				loff_t pos, unsigned len,
				struct page **pagep, void **fsdata)
#else
static int XCraft_write_begin(struct file *file, struct address_space *mapping,
			    loff_t pos, unsigned len, unsigned flags,
			    struct page **pagep, void **fsdata)
#endif
{
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(file->f_inode->i_sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	int err;
	uint32_t nr_allocs = 0;

	// 文件最大大小
	unsigned int max_file_blocks;
	// 一个块能容纳多少个块号
	unsigned int bno_num_per_block = XCRAFT_BLOCK_SIZE / sizeof(__le32);
	// 计算文件最大大小
	max_file_blocks = XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block + XCRAFT_N_DOUBLE_INDIRECT * bno_num_per_block * bno_num_per_block;
	/* 检查写操作能否完成，用写之后的文件偏移位置和文件的最大大小作比较*/
	
	if(pos + len > max_file_blocks * XCRAFT_BLOCK_SIZE)
		return -ENOSPC;
	nr_allocs = max(pos + len, file->f_inode->i_size) / XCRAFT_BLOCK_SIZE;
	if(nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;
	if(nr_allocs > le32_to_cpu(disk_sb->s_free_blocks_count))
		return -ENOSPC;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
    err = block_write_begin(mapping, pos, len, pagep, XCraft_file_get_block);
#else
    err = block_write_begin(mapping, pos, len, flags, pagep,
                            XCraft_file_get_block);
#endif
	/* if this failed, reclaim newly allocated blocks */
    if (err < 0)
        pr_err("newly allocated blocks reclaim not implemented yet\n");
	return err;
}


static int XCraft_write_end(struct file *file,
			  struct address_space *mapping,
			  loff_t pos, unsigned len, unsigned copied,
			  struct page *page, void *fsdata)
{

	
	return 0;
}



// 定位文件指针
loff_t XCraft_llseek(struct file *file, loff_t offset, int whence);

// 读操作
static ssize_t XCraft_file_read_iter(struct kiocb *iocb, struct iov_iter *to);


// 写操作
static ssize_t XCraft_file_write_iter(struct kiocb *iocb, struct iov_iter *from);

// 同步文件内容
int XCraft_sync_file(struct file *file, loff_t start, loff_t end, int datasync);

// address_space_operations
const struct address_space_operations XCraft_aops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
    .readahead = XCraft_readahead,
#else
    .readpage = XCraft_readpage,
#endif
	.writepage		= XCraft_writepage,
	.write_begin		= XCraft_write_begin,
	.write_end		= XCraft_write_end,
	
};


const struct file_operations XCraft_file_operations = {
	.llseek		= XCraft_llseek,
    .owner = THIS_MODULE,
	.read_iter	= XCraft_file_read_iter,
	.write_iter	= XCraft_file_write_iter,
	.fsync		= XCraft_sync_file,
};


