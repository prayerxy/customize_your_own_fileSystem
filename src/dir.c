#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "XCraft.h"
#include "hash.h"
// eno是除.和..之外第几个目录项开始遍历搜索 eno从0开始
// i_block = xi->i_block[0]
static int XCraft_dx_readdir(struct inode *inode, struct dir_context *ctx, int eno, unsigned int i_block){
	pr_debug("begin dx_readdir!\n");
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
	struct super_block *sb = inode->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct dx_entry *entries = NULL, *entries2 = NULL, *entries3 = NULL;
	unsigned indirect_levels = 0, count = 0, count2 = 0, count3 = 0;
	// cur_level 为当前所处的level
	int retval, cur_level;
	// dx_root
	struct dx_root *root;
	// dx_node
	struct dx_node *node;

	struct buffer_head *bh, *bh2;

	bh = sb_bread(sb, i_block);
	if(!bh){
		retval = -EIO;
		return retval;
	}

	root = (struct dx_root *)bh->b_data;
	indirect_levels = root->info.indirect_levels;
	entries = root->entries;
	
	// 存储遍历信息
	struct dx_frame frames[indirect_levels + 1], *frame;
	memset(frames, 0, sizeof(frames));

	int i;

	retval = 0;
	
	// 用于遍历dx_root和dx_node计数使用
	unsigned tmp, tmp2;
	

	// 获取dx_root层的entries count
	count = dx_get_count(entries);
	frame = frames;
	frames[0].bh = bh;

	// tmp,tmp2和当前级数赋值
	tmp = tmp2 = 0;
	cur_level = 0;
	// 物理块号存储
	unsigned int bno, bno2, bno_tmp;
	struct buffer_head *bh_tmp;
	int eno_count = 0;

	// 遍历目录项时使用
	struct XCraft_dir_entry* de;
	unsigned short reclen;
	char *top;
	while(tmp<count){
		// 从entries获取block
		bno = le32_to_cpu(entries->block);
		bh = sb_bread(sb, bno);
		if(!bh){
			retval = -EIO;
			return retval;
		}
		frames[0].entries = root->entries;
		frames[0].at = entries;

		bh2 = bh;
		bno2 = bno;
		if(cur_level<indirect_levels){
again:
			cur_level+=1;
			node = (struct dx_node *)bh2->b_data;
			entries2 = node->entries;
			count2 = dx_get_count(entries2);
back:
			while(tmp2<count2){
				bno_tmp = le32_to_cpu(entries2->block);
				bh_tmp = sb_bread(sb, bno_tmp);
				if(!bh_tmp){
					retval = -EIO;
					goto out_bh;
				}
				if(cur_level < indirect_levels){
					frame = frame + 1;
					frame->bh = bh2;
					node = (struct dx_node *)bh2->b_data;
					frame->entries = node->entries;
					frame->at = entries2;
					// 重置tmp2
					tmp2 = 0;
					bh2 = bh_tmp;
					bno2 = bno_tmp;
					goto again;
				}

				// 否则此时在最后一级dx_entry
				// bh_tmp直接对应磁盘块
				reclen = sizeof(struct XCraft_dir_entry);
				de = (struct XCraft_dir_entry *)bh_tmp->b_data;
				top = bh_tmp->b_data + sb->s_blocksize - sizeof(struct XCraft_dir_entry);
				while((char *)de<=top){
					if(!de->inode)
						break;
					if(eno_count>=eno){
						if(de->inode && !dir_emit(ctx, de->name, XCRAFT_NAME_LEN, le32_to_cpu(de->inode), DT_UNKNOWN))
							break;
						ctx->pos++;
					}
					eno_count++;
					reclen = le16_to_cpu(de->rec_len);
					de = (struct XCraft_dir_entry *)((char *)de + reclen);
				}
				tmp2+=1;
				entries2 += 1;
			}
			cur_level-=1;
			// 我们向前方进行回溯
			while(cur_level>0){
				// 保证我们下面遍历的都是dx_node类型
				bh2 = frame->bh;
				tmp2 = (frame->at) - (frame->entries);
				entries2 = frame->at;
				node = (struct dx_node *)bh2->b_data;
				count2 = dx_get_count(node->entries);
				if(tmp2<count2-1){
					tmp2++;
					entries2 = entries2 + 1;
					frame = frame - 1;
					break;
				}
				frame = frame - 1;
				cur_level-=1;
			}
			// cur_level为0说明此时我们去遍历dx_root
			if(!cur_level)
				goto root_again;
			// 否则此时cur_level不为0, 说明在此前跳出, 继续遍历即可
			goto back;
		}

		// 此时dx_root层已经是最后一层，直接搜索bh对应的磁盘块
		reclen = sizeof(struct XCraft_dir_entry);
		de = (struct XCraft_dir_entry *)bh->b_data;
		top = bh->b_data + sb->s_blocksize - reclen;
		while((char *)de <= top){
		    if(!de->inode)
				break;
			if(eno_count>=eno){
			    if(de->inode && !dir_emit(ctx, de->name, XCRAFT_NAME_LEN, le32_to_cpu(de->inode),DT_UNKNOWN))
					break;
				ctx->pos++;
			}
			eno_count++;
			reclen = le16_to_cpu(de->rec_len);
			de = (struct XCraft_dir_entry *)((char *)de + reclen);
		}
root_again:
		entries = entries + 1;
		tmp++;
		tmp2 = 0;
	}
	
	dx_release(frames);
	pr_debug("over dx_readdir!\n");
	return retval;
out_bh:
	if(bh)
		brelse(bh);
end:
	return retval;
}


//ls
static int XCraft_readdir(struct file *dir, struct dir_context *ctx){
	struct inode *inode = file_inode(dir);
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
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
	

	eno = ctx->pos - 2;
	// 遍历目录项 分哈希树和不是哈希树两种情况
	i_block = le32_to_cpu(xi->i_block[0]);

	if(XCraft_INODE_ISHASH_TREE(xi->i_flags)){
		// 哈希树情况，遍历hash树查找目录项
		ret = XCraft_dx_readdir(inode, ctx, eno, i_block);
		pr_debug("XCraft_dx_readdir ret: %d\n",ret);
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
		reclen = sizeof(struct XCraft_dir_entry);
		top = bh->b_data + sb->s_blocksize - reclen;
		while((char *)de <= top && count<XCRAFT_dentry_LIMIT){
			if(!de->inode)
				// 搜索完了
				break;
			// 此时进行提交
			if(de->inode && !dir_emit(ctx, de->name, XCRAFT_NAME_LEN, le32_to_cpu(de->inode),
                              DT_UNKNOWN))
				break;
			ctx->pos++;
			reclen = le16_to_cpu(de->rec_len);
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