#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>

#include "../include/bitmap.h"
#include "../include/XCraft.h"

// 普通文件中由逻辑iblock将指定的物理块和bh_result相关联，如果物理块不存在，create字段为1则创建一个
static int XCraft_file_get_block(struct inode *inode,
                                   sector_t iblock,
                                   struct buffer_head *bh_result,
                                   int create)
{
	






}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static void XCraft_readahead(struct readahead_control *rac)
{

}
#else
static int XCraft_readpage(struct file *file, struct page *page)
{

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


end:
	return ret;
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


