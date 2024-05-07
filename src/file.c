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
	struct buffer_head *bh;
	// 获取i_block数组
	unsigned int *i_block = inode_info->i_block;

	int ret = 0, bno;
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

	// 与当前文件占的块数比较
	// 超过是否能创建
	if(iblock >= i_blocks && !create){
		printk("iblock >= i_blocks and create = 0");
		ret = 0;
		goto end;
	}
	
	// 可以创建
	// 查找

	// 查找一级索引块
	if(iblock < XCRAFT_N_DIRECT){
		// 是否需要分配
		if(iblock >= i_blocks){
			// 需要分配
			if(!create){
				bno = get_free_blocks(sb_info, 1);
				if(!bno){
					// 分配失败
					ret = -ENOSPC;
					goto end;
				}
			}
			// 分配成功需要更新inode->i_blocks
			inode->i_blocks+=1;
			mark_inode_dirty(inode);
			i_block[iblock] = bno;
		}
		// 获取iblock对应的物理块块号
		bno = i_block[iblock];
	}
	
	// 查找二级索引块
	// 0 -1
	// N_direct N_direct + 2*bno_per - 1
	// N_direct + 2*bno_per - 1 N_direct + 2*bno_per + bno_per*bno_per - 1









end:
	return ret;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static void XCraft_readahead(struct readahead_control *rac)
{

}
#else
static int XCraft_readpage(struct file *file, struct page *page)
{

	return 0;
}
#endif

static int XCraft_writepage(struct page *page,
			  struct writeback_control *wbc)
{

	return 0;
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

	return 0;
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


