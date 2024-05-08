#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/XCraft.h"
// eno是除.和..之外第几个目录项开始遍历搜索 eno从0开始
// i_block = xi->i_block[0]
static int XCraft_dx_readdir(struct inode *inode, struct dir_context *ctx, int eno, unsigned int i_block){
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);



}




//ls
static int XCraft_readdir(struct file *file, struct dir_context *ctx){
	struct inode *inode = file_inode(dir);
	struct XCraft_inode_info *xi = XCraft_i(inode);
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh;
	struct XCraft_dir_entry *de;
	char *top;
	unsigned short reclen;
	int ret;
	// 第几个目录项除了.和.. 0开始
	int eno;

	unsigned int i_block;

	// 检查是否是目录
	if(!S_ISDIR(inode->i_mode))
		return -ENOTDIR;
	
	/* Commit . and .. to ctx */
	if(!dir_emit_dots(dir, ctx))
		return 0;
	
	// ctx->pos确定是第几个目录项
	// ctx->pos从0开始

	eno = ctx->pos - 2;

	// 遍历目录项 分哈希树和不是哈希树两种情况
	i_block = xi->i_block[0];

	if(XCraft_INODE_ISHASH_TREE(xi->i_flags)){
		// 哈希树情况，遍历hash树查找目录项



	}
	else{
		int count = 0;
		bh = sb_bread(sb, i_block);
		if(!bh){
			ret = -EIO;
			goto end;
		}
		de = (struct XCraft_dir_entry *)bh->b_data;
		de+=eno;
		top = bh->b_data + sb->s_blocksize - reclen;
		while((char *)de <= top && count<XCRAFT_dentry_LIMIT){
			if(!de->inode)
				// 搜索完了
				break;
			// 此时进行提交
			if(de->inode && !dir_emit(ctx, de->name, XCRAFT_NAME_LEN, de->inode,
                              DT_UNKNOWN))
				break;
			ctx->pos++;
			reclen = le16_to_cpu(de->reclen);
			de = (struct XCraft_dir_entry *)((char *)de + reclen);
			count++;
		}
		brelse(bh);
	}

end:	
	return ret;
}


const struct file_operations XCraft_dir_operations = {
	.iterate_shared	= XCraft_readdir,
};