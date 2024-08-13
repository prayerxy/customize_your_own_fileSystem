#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>

#include "XCraft.h"
#include "bitmap.h"
#include "hash.h"
#include "extents.h"

// 进行删除操作，map中映射要删除的逻辑块
int XCraft_delete_ext(struct inode *inode, struct XCraft_map_blocks *map)
{
	struct XCraft_ext_path *path = NULL;
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sbi = XCRAFT_SB(sb);
	struct XCraft_extent *ex;
	struct XCraft_extent_header *eh;
	// 映射的物理块号 深度
	int pblk = 0, depth;
	// 最终成功映射的物理块长度
	int allocated = 0;
	// ret为1表示成功发现并删除，为0代表没有找到
	// ret 为其他值代表出现错误
	int ret = 0;
	int i = 0;
	// 调整extent时用到
	int len = 0;

	// 标志位
	int flag = 0;

	path = XCraft_find_extent(inode, map->m_lblk, NULL);
	// path是否获取失败
	if (IS_ERR(path))
	{
		ret = PTR_ERR(path);
		path = NULL;
		goto out;
	}

	// 获取深度
	depth = get_ext_tree_depth(inode);
	// 叶子节点层的extent
	ex = path[depth].p_ext;
	eh = path[depth].p_hdr;
	// 记录叶子节点层中
	unsigned int ee_block, ee_start, ee_len;
	if (ex)
	{
		ee_block = le32_to_cpu(ex->ee_block);
		ee_start = le32_to_cpu(ex->ee_start);
		ee_len = le32_to_cpu(ex->ee_len);
		// 是否是在ex的范围里面
		if (map->m_lblk >= ee_block && map->m_lblk < ee_block + ee_len)
		{
			ret = 1;
			pblk = map->m_lblk - ee_block + ee_start;
			allocated = ee_len - (map->m_lblk - ee_block);
			map->m_pblk = pblk;
			map->m_len = allocated;
			goto delete_ext;
		}
		else
			goto out;
	}
	else
		goto out;

delete_ext:
	// 先释放物理块
	put_blocks(sbi, pblk, map->m_len);

	if (map->m_lblk == ee_block)
	{
		if (ex == XCRAFT_FIRST_EXTENT(eh))
			flag = 1;
		// 此时这个extent可以删掉
		memset(ex, 0, sizeof(struct XCraft_extent));
		// 然后调整extent
		len = XCRAFT_LAST_EXTENT(eh) - ex;
		if (len > 0)
		{
			memmove(ex, ex + 1, len * sizeof(struct XCraft_extent));
		}
		eh->eh_entries = cpu_to_le16(le16_to_cpu(eh->eh_entries) - 1);
		// 如果ex是叶子节点的第一个extent需要向上更新索引
		if (flag)
		{
			if (eh->eh_entries != 0){
				ret = XCraft_correct_indexes(inode, path);
				mark_inode_dirty(inode);
				if(ret)
					goto out;
			}
		}
	}
	else
	{
		// 此时这个extent更新信息
		ex->ee_len = cpu_to_le32(le32_to_cpu(ex->ee_len) - map->m_len);
	}

	// 更新信息
	if (path[depth].p_bh)
		mark_buffer_dirty(path[depth].p_bh);
out:
	XCraft_ext_drop_refs(path);
	if (path)
		kfree(path);
	return ret;
}

u64 XCraft_ext_maxfileblocks(struct inode *inode){
	u64 res;
	// int i;
	// int ext_max_in_blk;
	// struct XCraft_inode_info *inode_info = XCRAFT_I(inode);
	// ext_max_in_blk = XCraft_ext_space_leaf(inode);
	// res = 1;
	// for (i=0;i<XCRAFT_MAX_EXTENT_DEPTH;i++)
	// 	res *= ext_max_in_blk;
	// res *= XCraft_ext_space_root_idx(inode_info);
	res = (1UL<<15);
	return res;

}

// 扩展树逻辑块号映射到物理块号
static int XCraft_ext_file_get_block(struct inode *inode,
									 sector_t iblock,
									 struct buffer_head *bh_result,
									 int create)
{
	pr_debug("beign ext_file_get_block\n");
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_inode_info *inode_info = XCRAFT_I(inode);
	// 创建映射map
	struct XCraft_map_blocks map;
	// u64 maxfileblocks = XCraft_ext_maxfileblocks(inode);
	map.m_pblk = 0;
	map.m_lblk = iblock;
	// 映射一个extent的长度
	map.m_len = XCRAFT_EXT_BLOCKS_NUM;
	int ret;
	unsigned int i_size;
	uint32_t i_blocks;
	i_size = inode->i_size;
	i_blocks = ceil_div(i_size, XCRAFT_BLOCK_SIZE);

	ret = 0;
	// if (iblock >= maxfileblocks){
	// 	printk("yes\n");
	// 	return -EFBIG;
	// }
	
	if (iblock >= i_blocks && !create)
	{
		ret = 0;
		goto end;
	}

	ret = XCraft_ext_map_blocks(inode, &map, create);
	pr_debug("map.m_pblk: %d\n", map.m_pblk);
	if (ret > 0)
	{
		map_bh(bh_result, sb, map.m_pblk);
		ret = 0;
	}
	
end:
	return ret;
}

// 索引块方法获取文件最大大小
unsigned int XCraft_get_max_filesize(void)
{
	unsigned int max_file_blocks;
	// 一个块能容纳多少个块号
	unsigned int bno_num_per_block = XCRAFT_BLOCK_SIZE / sizeof(__le32);
	max_file_blocks = XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block + XCRAFT_N_DOUBLE_INDIRECT * bno_num_per_block * bno_num_per_block;
	// 返回最大文件大小
	return max_file_blocks * XCRAFT_BLOCK_SIZE;
}

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
	__le32 *i_block = inode_info->i_block;

	// bno存储最后我们找到的块号 tmp是中间块号存储变量
	int ret, bno, bno2;
	// 文件最大大小
	unsigned int max_file_blocks;
	// 一个块能容纳多少个块号
	unsigned int bno_num_per_block = XCRAFT_BLOCK_SIZE / sizeof(__le32);

	max_file_blocks = XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block + XCRAFT_N_DOUBLE_INDIRECT * bno_num_per_block * bno_num_per_block;

	if (iblock >= max_file_blocks){
		printk("yes2\n");
		return -EFBIG;
	}

	// 当前自身文件的内容大小
	unsigned int i_size = inode->i_size;
	uint32_t i_blocks = ceil_div(i_size, XCRAFT_BLOCK_SIZE);
	ret = 0;
	// 与当前文件占的块数比较
	// 超过是否能创建
	if (iblock >= i_blocks && !create)
	{
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
	if (iblock < XCRAFT_N_DIRECT)
	{
		// 是否需要分配
		if (iblock >= i_blocks)
		{
			// 进入到此条件语句，create一定是1
			bno = get_free_blocks(sb_info, 1);
			if (!bno)
			{
				// 分配失败
				ret = -ENOSPC;
				goto end;
			}
			// 将分配的块进行初始化
			bh = sb_bread(sb, bno);
			if (!bh)
			{
				ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 分配成功需要更新inode
			i_block[iblock] = cpu_to_le32(bno);
		}
		// 获取iblock对应的物理块块号
		bno = le32_to_cpu(i_block[iblock]);
		mark_inode_dirty(inode);
	}
	else if (iblock >= XCRAFT_N_DIRECT && iblock < XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block)
	{
		// 此时iblock对应的块在一级间接索引块中
		indirect_index = (iblock - XCRAFT_N_DIRECT) / bno_num_per_block;
		indirect_offset = (iblock - XCRAFT_N_DIRECT) % bno_num_per_block;
		indirect_bno = le32_to_cpu(i_block[XCRAFT_N_DIRECT + indirect_index]);
		if (!indirect_bno)
		{
			// 表示此时其还没有被初始化，必定iblock >= i_blocks && !create
			indirect_bno = get_free_blocks(sb_info, 1);
			if (!indirect_bno)
			{
				ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh = sb_bread(sb, indirect_bno);
			if (!bh)
			{
				ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 更新inode信息
			i_block[XCRAFT_N_DIRECT + indirect_index] = cpu_to_le32(indirect_bno);
		}
		bh = sb_bread(sb, indirect_bno);
		if (!bh)
		{
			ret = -EIO;
			goto end;
		}
		// 依据indirect_offset获取对应的块号
		bno_block = (__le32 *)bh->b_data;
		bno = le32_to_cpu(bno_block[indirect_offset]);
		if (!bno)
		{
			// 此时此bno对应的块没有被分配
			bno = get_free_blocks(sb_info, 1);
			if (!bno)
			{
				ret = -ENOSPC;
				goto end;
			}

			// 对新分配的块进行初始化
			bh2 = sb_bread(sb, bno);
			if (!bh2)
			{
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
	else
	{
		double_indirect_index = (iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) / (bno_num_per_block * bno_num_per_block);
		double_indirect_one_offset = ((iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block)) / bno_num_per_block;
		double_indirect_two_offset = ((iblock - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block)) % bno_num_per_block;
		double_indirect_bno = i_block[XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT + double_indirect_index];
		if (!double_indirect_bno)
		{
			double_indirect_bno = get_free_blocks(sb_info, 1);
			if (!double_indirect_bno)
			{
				ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh = sb_bread(sb, double_indirect_bno);
			if (!bh)
			{
				ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			mark_buffer_dirty(bh);
			brelse(bh);
			// 更新inode信息
			i_block[XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT + double_indirect_index] = cpu_to_le32(double_indirect_bno);
		}
		bh = sb_bread(sb, double_indirect_bno);
		if (!bh)
		{
			ret = -EIO;
			goto end;
		}
		// 依据double_indirect_one_offset获取对应的块号
		bno2_block = (__le32 *)bh->b_data;
		bno2 = le32_to_cpu(bno2_block[double_indirect_one_offset]);
		if (!bno2)
		{
			// 此时此bno对应的块没有分配
			bno2 = get_free_blocks(sb_info, 1);
			if (!bno2)
			{
				ret = -ENOSPC;
				goto end;
			}

			// 对新分配的块进行初始化
			bh2 = sb_bread(sb, bno2);
			if (!bh2)
			{
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
		if (!bh)
		{
			ret = -EIO;
			goto end;
		}

		bno_block = (__le32 *)bh->b_data;
		bno = le32_to_cpu(bno_block[double_indirect_two_offset]);
		if (!bno)
		{
			// 此时此bno对应的块没有分配
			bno = get_free_blocks(sb_info, 1);
			if (!bno)
			{
				ret = -ENOSPC;
				goto end;
			}
			// 对新分配的块进行初始化
			bh3 = sb_bread(sb, bno);
			if (!bh3)
			{
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

// 扩展树方式
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static void XCraft_ext_readahead(struct readahead_control *rac)
{
	mpage_readahead(rac, XCraft_ext_file_get_block);
}
#else
static int XCraft_ext_readpage(struct file *file, struct page *page)
{
	//检查是否有读取权限inode索引的文件
	return mpage_readpage(page, XCraft_ext_file_get_block);
}
#endif

// 索引块方式
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

// 扩展树方式
static int XCraft_ext_writepage(struct page *page,
								struct writeback_control *wbc)
{

	return block_write_full_page(page, XCraft_ext_file_get_block, wbc);
}

// 索引块方式
static int XCraft_writepage(struct page *page,
							struct writeback_control *wbc)
{

	return block_write_full_page(page, XCraft_file_get_block, wbc);
}

// 扩展树方式
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
static int XCraft_ext_write_begin(struct file *file, struct address_space *mapping,
								  loff_t pos, unsigned len,
								  struct page **pagep, void **fsdata)
#else
static int XCraft_ext_write_begin(struct file *file, struct address_space *mapping,
								  loff_t pos, unsigned len, unsigned flags,
								  struct page **pagep, void **fsdata)
#endif
{
	pr_debug("ext_write_begin is doing!\n");
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(file->f_inode->i_sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	int err;
	uint32_t nr_allocs = 0;
	// u64 maxfilesize = XCraft_ext_maxfileblocks(file->f_inode) * XCRAFT_BLOCK_SIZE;
	// if (pos + len > maxfilesize)
	// 	return -ENOSPC;
	nr_allocs = max(pos + len, file->f_inode->i_size) / XCRAFT_BLOCK_SIZE;
	if (nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;

	if (nr_allocs > le32_to_cpu(disk_sb->s_free_blocks_count))
		return -ENOSPC;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
	err = block_write_begin(mapping, pos, len, pagep, XCraft_ext_file_get_block);
#else
	err = block_write_begin(mapping, pos, len, flags, pagep,
							XCraft_ext_file_get_block);
#endif
	/* if this failed, reclaim newly allocated blocks */
	if (err < 0)
		pr_err("newly allocated blocks reclaim not implemented yet\n");
	pr_debug("ext_write_begin is done! err is %d\n", err);
	return err;
}

// 索引块方式
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

	if (pos + len > max_file_blocks * XCRAFT_BLOCK_SIZE)
		return -ENOSPC;
	nr_allocs = max(pos + len, file->f_inode->i_size) / XCRAFT_BLOCK_SIZE;
	if (nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;
	if (nr_allocs > le32_to_cpu(disk_sb->s_free_blocks_count))
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

// 扩展树方式
static int XCraft_ext_write_end(struct file *file,
							struct address_space *mapping,
							loff_t pos, unsigned len, unsigned copied,
							struct page *page, void *fsdata)
{
	pr_debug("ext_write_end is doing\n");
	struct inode *inode = file->f_inode;
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct buffer_head *bh;

	// 删除标志位
	int exit_flag = 0;

	struct XCraft_map_blocks map;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
	struct timespec64 cur_time;
#endif
	uint32_t nr_blocks_old;

	/* Complete the write() */
	int ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
	if (ret < len)
	{
		pr_err("wrote less than requested.");
		return ret;
	}

	nr_blocks_old = inode->i_blocks;

	/* Update inode metadata */
	inode->i_blocks = inode->i_size / XCRAFT_BLOCK_SIZE + 2;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	cur_time = current_time(inode);
	inode->i_mtime = cur_time;
	inode_set_ctime_to_ts(inode, cur_time);
#else
	inode->i_mtime = inode->i_ctime = current_time(inode);
#endif

	mark_inode_dirty(inode);

	uint32_t first_block;
	// 释放掉多余的块
	if (nr_blocks_old > inode->i_blocks)
	{
		truncate_pagecache(inode, inode->i_size);
		first_block = inode->i_blocks - 1;
		// 开始释放
		while (1)
		{

			map.m_lblk = first_block;
			map.m_pblk = 0;
			// 如果后面搜到连续的，则返回更多
			map.m_len = 1;
			// 最后删除可能会对叶子节点进行调整，所以要返回路径来调整叶子节点
			// 调用函数
			exit_flag = XCraft_delete_ext(inode, &map);
			if (!exit_flag)
				break;
			else if (exit_flag == 1)
			{
				// 更新first_block
				first_block += map.m_len;
			}
			else
				goto end;
		}
	}
	mark_inode_dirty(inode);
	pr_debug("ext_write_end is done, ret is %d\n", ret);
end:
	return ret;
}

// 索引块方式
static int XCraft_write_end(struct file *file,
							struct address_space *mapping,
							loff_t pos, unsigned len, unsigned copied,
							struct page *page, void *fsdata)
{
	struct inode *inode = file->f_inode;
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	__le32 *i_block = xi->i_block;
	struct buffer_head *bh;
	// 后续遍历时会需要bno和bno2来存储物理块号
	int bno, bno2;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
	struct timespec64 cur_time;
#endif
	uint32_t nr_blocks_old;

	/* Complete the write() */
	int ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
	if (ret < len)
	{
		pr_err("wrote less than requested.");
		return ret;
	}

	nr_blocks_old = inode->i_blocks;

	/* Update inode metadata */
	inode->i_blocks = inode->i_size / XCRAFT_BLOCK_SIZE + 2;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	cur_time = current_time(inode);
	inode->i_mtime = cur_time;
	inode_set_ctime_to_ts(inode, cur_time);
#else
	inode->i_mtime = inode->i_ctime = current_time(inode);
#endif

	mark_inode_dirty(inode);

	// 开始遍历的逻辑块号
	uint32_t first_block;
	// inode是文件时最多能有多少数据块
	unsigned int max_file_blocks;
	// 一个块能容纳多少个块号
	unsigned int bno_num_per_block = XCRAFT_BLOCK_SIZE / sizeof(__le32);

	// 一级间接索引块中会使用
	unsigned int indirect_index, indirect_offset, indirect_bno;

	// 二级间接索引块的时候使用
	unsigned int double_indirect_index, double_indirect_one_offset, double_indirect_two_offset, double_indirect_bno;

	// 索引块读取会使用 一级和两级
	__le32 *bno_block, *bno2_block;

	if (nr_blocks_old > inode->i_blocks)
	{
		// 释放掉多余的块我们要

		/*Free unused blocks from page cache */
		truncate_pagecache(inode, inode->i_size);
		// remove unused blocks
		first_block = inode->i_blocks - 1;
		max_file_blocks = XCraft_get_max_filesize() / XCRAFT_BLOCK_SIZE;

		// 逻辑块号遍历
		while (first_block < max_file_blocks)
		{
			// 获取的对应的物理块号存在bno中
			if (first_block < XCRAFT_N_DIRECT)
			{
				bno = le32_to_cpu(i_block[first_block]);
				if (!bno)
					break;
				// 重置
				i_block[first_block] = 0;
				mark_inode_dirty(inode);
			}
			else if (first_block >= XCRAFT_N_DIRECT && first_block < XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT * bno_num_per_block)
			{
				indirect_index = (first_block - XCRAFT_N_DIRECT) / bno_num_per_block;
				indirect_offset = (first_block - XCRAFT_N_DIRECT) % bno_num_per_block;
				indirect_bno = le32_to_cpu(i_block[XCRAFT_N_DIRECT + indirect_index]);
				if (!indirect_bno)
					break;
				bh = sb_bread(sb, indirect_bno);
				// 读取一级间接索引块
				if (!bh)
				{
					ret = -EIO;
					goto end;
				}

				// 依据indirect_offset获取对应的块号
				bno_block = (__le32 *)bh->b_data;
				bno = le32_to_cpu(bno_block[indirect_offset]);
				if (!bno)
					break;
				bno_block[indirect_offset] = bno;
				mark_buffer_dirty(bh);
				brelse(bh);
			}
			else
			{
				double_indirect_index = (first_block - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) / (bno_num_per_block * bno_num_per_block);
				double_indirect_one_offset = ((first_block - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block)) / bno_num_per_block;
				double_indirect_two_offset = ((first_block - XCRAFT_N_DIRECT - XCRAFT_N_INDIRECT * bno_num_per_block) % (bno_num_per_block * bno_num_per_block)) % bno_num_per_block;
				double_indirect_bno = le32_to_cpu(i_block[XCRAFT_N_DIRECT + XCRAFT_N_INDIRECT + double_indirect_index]);
				if (!double_indirect_bno)
					break;
				bh = sb_bread(sb, double_indirect_bno);
				if (!bh)
				{
					ret = -EIO;
					goto end;
				}
				bno2_block = (__le32 *)bh->b_data;
				bno2 = le32_to_cpu(bno2_block[double_indirect_one_offset]);
				if (!bno2)
					break;
				bh = sb_bread(sb, bno2);
				if (!bh)
				{
					ret = -EIO;
					goto end;
				}
				bno_block = (__le32 *)bh->b_data;
				bno = le32_to_cpu(bno_block[double_indirect_two_offset]);
				if (!bno)
					break;
				// 重置
				bno_block[double_indirect_two_offset] = 0;
				mark_buffer_dirty(bh);
				brelse(bh);
			}

			// 释放对应的物理块
			bh = sb_bread(sb, bno);
			if (!bh)
			{
				ret = -EIO;
				goto end;
			}
			memset(bh->b_data, 0, XCRAFT_BLOCK_SIZE);
			put_blocks(sb_info, bno, 1);
			mark_buffer_dirty(bh);
			brelse(bh);
			first_block += 1;
		}
	}
end:
	return ret;
}

// 定位文件指针
loff_t XCraft_llseek(struct file *file, loff_t offset, int whence)
{
	return generic_file_llseek(file, offset, whence);
}

// 读操作
static ssize_t XCraft_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	//获取file指针
	struct file *file = iocb->ki_filp;
	struct inode*inode=file->f_inode;

	int ret=XCraft_permission(inode, MAY_READ);
	if(ret){
		printk("xcraft: XCraft_permission error in read\n");
		return ret;
	}
	return generic_file_read_iter(iocb, to);
}

// 写操作
static ssize_t XCraft_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	//获取file指针
	struct file *file = iocb->ki_filp;
	struct inode*inode=file->f_inode;

	int ret=XCraft_permission(inode, MAY_WRITE);
	if(ret){
		printk("xcraft: XCraft_permission error in write\n");
		return ret;
	}
	return generic_file_write_iter(iocb, from);
}

// 同步文件内容
int XCraft_sync_file(struct file *file, loff_t start, loff_t end, int datasync)
{
	return generic_file_fsync(file, start, end, datasync);
}

// address_space_operations
const struct address_space_operations XCraft_aops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
	.readahead = XCraft_ext_readahead,
#else
	.readpage = XCraft_ext_readpage,
#endif
	.writepage = XCraft_ext_writepage,
	.write_begin = XCraft_ext_write_begin,
	.write_end = XCraft_ext_write_end,

};

const struct file_operations XCraft_file_operations = {
	.llseek = XCraft_llseek,
	.owner = THIS_MODULE,
	.read_iter = XCraft_file_read_iter,
	.write_iter = XCraft_file_write_iter,
	.fsync = XCraft_sync_file,
};
