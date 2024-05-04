
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/XCraft.h"
#include "../include/bitmap.h"
#include "../include/hash.h"
// #include <endian.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// additional
// 获取块组描述符

static const struct inode_operations XCraft_inode_operations;
static const struct inode_operations XCraft_symlink_inode_operations;

// 由ino获取指定的inode
struct inode *XCraft_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode = NULL;
	struct XCraft_inode *disk_inode = NULL;
	struct XCraft_inode_info *xi = NULL;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	struct buffer_head *bh = NULL;

	uint32_t block_group = inode_get_block_group(sb_info, ino);
	uint32_t inode_index_in_group = inode_get_block_group_shift(sb_info, ino);
	int i;
	int ret;
	// 如果ino超过了范围
	if (ino >= le32_to_cpu(disk_sb->s_inodes_count))
		return ERR_PTR(-EINVAL);

	// 获取块组描述符
	struct XCraft_group_desc *desc = get_group_desc2(sb_info, block_group);
	if (!desc){
		printk("desc is NULL!\n");
		return ERR_PTR(-EINVAL);
	}

	if(!XCraft_BG_ISINIT(desc->bg_flags)){
		printk("desc->bg_flags is not init!\n");
		return ERR_PTR(-EINVAL);
	}

	// 从内存中分配inode，并赋值i_ino字段
	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	// 如果inode在内存中，则直接返回
	if (!(inode->i_state & I_NEW))
		return inode;

	xi = XCRAFT_I(inode);

	// 开始确定此块组中inode所在起始的物理块号
	uint32_t inode_table = le32_to_cpu(desc->bg_inode_table);
	// inode所在的物理块号和在此块中的偏移量
	uint32_t inode_block = inode_table + (inode_index_in_group / XCRAFT_INODES_PER_BLOCK);
	uint32_t inode_offset = inode_index_in_group % XCRAFT_INODES_PER_BLOCK;
	
	bh = sb_bread(sb, inode_block);
	if (!bh){
		ret = -EIO;
		goto failed;
	}

	disk_inode = (struct XCraft_inode *)bh->b_data + inode_offset;

	// 对inode各个字段进行赋值
	inode->i_ino = ino;
	inode->i_sb = sb;
	inode->i_op = &XCraft_inode_operations;
	inode->i_mode = le16_to_cpu(disk_inode->i_mode);

	i_uid_write(inode, le16_to_cpu(disk_inode->i_uid));
    i_gid_write(inode, le16_to_cpu(disk_inode->i_gid));
    inode->i_size = le32_to_cpu(disk_inode->i_size_lo);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    inode_set_ctime(inode, (time64_t) le32_to_cpu(disk_inode->i_ctime), 0);
#else
    inode->i_ctime.tv_sec = (time64_t) le32_to_cpu(disk_inode->i_ctime);
    inode->i_ctime.tv_nsec = 0;
#endif

    inode->i_atime.tv_sec = (time64_t) le32_to_cpu(disk_inode->i_atime);
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_sec = (time64_t) le32_to_cpu(disk_inode->i_mtime);
    inode->i_mtime.tv_nsec = 0;
    inode->i_blocks = le32_to_cpu(disk_inode->i_blocks_lo);
    set_nlink(inode, le32_to_cpu(disk_inode->i_links_count));


	// XCraft_inode_info 字段赋值
	xi->i_dtime = le32_to_cpu(disk_inode->i_dtime);
	xi->i_block_group = block_group;
	xi->i_flags = le32_to_cpu(disk_inode->i_flags);
	
	if(S_ISDIR(inode->i_mode)){
		for(i=0;i<XCRAFT_N_BLOCK;i++)
			xi->i_block[i] = le32_to_cpu(disk_inode->i_block[i]);
		inode->i_fop = &XCraft_dir_operations;
	}else if(S_ISREG(inode->i_mode)){
		for(i=0;i<XCRAFT_N_BLOCK;i++)
			xi->i_block[i] = le32_to_cpu(disk_inode->i_block[i]);
		inode->i_fop = &XCraft_file_operations;
		inode->i_mapping->a_ops = &XCraft_aops;
	}else if(S_ISLNK(inode->i_mode)){
	    strncpy(xi->i_data, disk_inode->i_data,sizeof(disk_inode->i_data));
		inode->i_link = xi->i_data;
		inode->i_op = &XCraft_symlink_inode_operations;
	}

	brelse(bh);

	unlock_new_inode(inode);
	return inode;

failed:
	brelse(bh);
	iget_failed(inode);
	return ERR_PTR(ret);
}

// qstr = &dentry->d_name
// 注意inode_init_owner的版本号的变化
// qstr在这里没有使用，因为还没插入目录项
struct inode *XCraft_new_inode(struct inode *dir, struct qstr*qstr, int mode){					
	struct inode *inode = NULL;
	struct XCraft_inode_info *xi = NULL;
	struct super_block *sb = dir->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	struct buffer_head *bh = NULL;
	// 获取的inode的ino号，以及其需要的i_block[0]
	uint32_t ino, bno;
	// 返回结果
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 cur_time;
#endif

	// 检查文件的类型是否正确
	if (!S_ISDIR(mode) && !S_ISREG(mode) && !S_ISLNK(mode)) {
        pr_err(
            "File type not supported (only directory, regular file and symlink "
            "supported)\n");
        return ERR_PTR(-EINVAL);
    }

	// 检查inode是否还有空余的
	if(le32_to_cpu(disk_sb->s_free_inodes_count) == 0 || le32_to_cpu(disk_sb->s_free_blocks_count) == 0)
		return ERR_PTR(-ENOSPC);

	// 分配inode
	// 返回0表示没有找到，因为根目录inode已经分配了
	// 获取时要修改超级块以及块组描述符信息

	// 要修改此获取函数
	ino = get_free_inode(sb_info);

	if(!ino)
		return ERR_PTR(-ENOSPC);
	// 获取ino对应的inode
	inode = XCraft_iget(sb,ino);
	if(IS_ERR(inode)){
		ret = PTR_ERR(inode);
		goto failed_ino;
	}

	// 首先将i_flags字段置0
	xi = XCRAFT_I(inode);
	xi->i_flags = 0;


	if (S_ISLNK(mode)) {
#if MNT_IDMAP_REQUIRED()
        inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
#elif USER_NS_REQUIRED()
        inode_init_owner(&init_user_ns, inode, dir, mode);
#else
        inode_init_owner(inode, dir, mode);
#endif
        set_nlink(inode, 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
        cur_time = current_time(inode);
        inode->i_atime = inode->i_mtime = cur_time;
        inode_set_ctime_to_ts(inode, cur_time);
#else
        inode->i_ctime = inode->i_atime = inode->i_mtime = current_time(inode);
#endif
        inode->i_op = &XCraft_symlink_inode_operations;
        return inode;
    }


	// 获取一个空闲的块号
	// 获取时要修改超级块和块组描述符信息

	// 要修改此获取函数
	bno = get_free_blocks(sb_info, 1);
	if(!bno){
		ret = -ENOSPC;
		goto failed_inode;
	}

	#if MNT_IDMAP_REQUIRED()
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
#elif USER_NS_REQUIRED()
    inode_init_owner(&init_user_ns, inode, dir, mode);
#else
    inode_init_owner(inode, dir, mode);
#endif
	inode->i_blocks = 1;

	// 统一将目录和文件的i_block[0]置成bno，其他字段置为0
	memset(xi->i_block,0,sizeof(xi->i_block));
	if(S_ISDIR(mode)){
		// 目录只初始化其i_block[0]
		xi->i_block[0] = bno;
		// 字段赋值存疑
		inode->i_size = XCRAFT_BLOCK_SIZE;
		inode->i_fop = &XCraft_dir_operations;
		set_nlink(inode, 2);  // . and ..

	}else if(S_ISREG(mode)){
		
		// 只最开始分配一个块
		xi->i_block[0] = bno;
		// 记录此时文件的大小，后续依据此来确定是否需要重新添加i_block
		inode->i_size = 0;
		inode->i_fop = &XCraft_file_operations;
		inode->i_mapping->a_ops = &XCraft_aops;
		set_nlink(inode, 1);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    cur_time = current_time(inode);
    inode->i_atime = inode->i_mtime = cur_time;
    inode_set_ctime_to_ts(inode, cur_time);
#else
    inode->i_ctime = inode->i_atime = inode->i_mtime = current_time(inode);
#endif

    return inode;

failed_inode:
	iput(inode);

failed_ino:
	put_inode(sb_info, ino);

	return ERR_PTR(ret);
}


// frame = dx_probe(&dentry->d_name, dir, &hinfo, frames, &err);
// entry是目录项名，dir是目录inode，hinfo传入哈希信息，frame_in存储上一级信息，err为错误信息
static struct dx_frame *
dx_probe(struct qstr *entry, struct inode *dir,
	 struct XCraft_hash_info *hinfo, struct dx_frame *frame_in, int *err)
{	
	struct dx_entry *at, *entries, *p, *q, *m;
	struct dx_root *root;
	struct dx_node *node;
	struct buffer_head *bh;
	struct dx_frame *frame = frame_in;
	struct super_block *sb = dir->i_sb;
	__u32 hash;
	unsigned int i_block;
	unsigned indirect;
	uint16_t count, limit;
	// 确定循环的时候当时位于第几级，用于获取limit和count字段
	int level;

	frame->bh = NULL;

	// 获取i_block[0]对应的磁盘块
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	i_block = dir_info->i_block[0];
	bh = sb_bread(sb, i_block);
	if(!bh){
		*err = -EIO;
		goto fail;
	}

	root = (struct dx_root *) bh->b_data;
	hinfo->hash_version = root->info.hash_version;

	hinfo->seed = XCRAFT_SB(sb)->s_hash_seed;
	if(entry)
		XCraft_dirhash(entry->name, entry->len, hinfo);
	hash = hinfo->hash;

	// 由indirect_levels字段可以判断我们的哈希树有几级
	// 我们需要通过其与我们自己定义的哈希树层级进行比较
	// indirect字段如果大于1不合理，
	indirect = root->info.indirect_levels;
	if(indirect >= XCRAFT_HTREE_LEVEL){
		brelse(bh);
		*err = ERR_BAD_DX_DIR;
		goto fail;
	}

	entries = (struct dx_entry *) (((char *)&root->info) + sizeof(root->info));

	// 比较limit字段
	if(dx_get_limit(entries) != dx_root_limit()){
		brelse(bh);
		*err = ERR_BAD_DX_DIR;
		goto fail;
	}

	level = 0;
	// 开始寻找目录项
	while(1){
		// count 和 limit 字段获取
		count = dx_get_count(entries);
		limit = dx_get_limit(entries);

		if(!count || count > limit){
			brelse(bh);
			*err = ERR_BAD_DX_DIR;
			goto fail2;
		}
		
		// 二分查找目录项根据哈希值
		p = entries + 1;
		q = entries + count - 1;
		while(p <= q){
			m = p + (q - p) / 2;
			if(hash < le32_to_cpu(m->hash)){
				q = m - 1;
			}else
				p = m + 1;
		}
		// 确定位置
		at = p - 1;
		// 填充此一级frame字段
		frame->bh = bh;
		frame->entries = entries;
		frame->at = at;

		// indirect为0，说明此时并没有dx_node
		// indirct为0时到达极限
		if(++level > indirect)
			return frame;
		bh = sb_bread(sb, dx_get_block(at));
		if(!bh){
			*err = -EIO;
			goto fail2;
		}

		at = entries = ((struct dx_node *) bh->b_data)->entries;

		// 判断limit字段是否一致
		if(dx_get_limit(entries) != dx_node_limit()){
			brelse(bh);
			*err = ERR_BAD_DX_DIR;
			goto fail2;
		}

		// 开始获取下一级的
		frame++;
		frame->bh = NULL;
		level++;
	}

fail2:
	while(frame >= frame_in){
		brelse(frame->bh);
		frame--;
	}

fail:
	return NULL;
}

// hash tree添加目录项
static int XCraft_dx_add_entry(struct dentry *dentry, struct inode *inode){
	struct dx_frame frames[XCRAFT_HTREE_LEVEL], *frame;
	struct dx_entry *entries, *at;
	struct XCraft_hash_info hinfo;
	struct buffer_head *bh, *bh2;
	struct inode *dir = dentry->d_parent->d_inode;
	struct super_block * sb = dir->i_sb;
	struct XCraft_dir_entry *de;

	struct dx_root *root;
	// 后续需要分裂的dx_entry等所在的块用到
	struct dx_node *node2;
	struct dx_entry *entries2;

	int err = 0;
	// 哈希树有几级 之后与宏定义的hash tree层数比较
	int levels;

	unsigned int icount, icount1, icount2;
	uint32_t hash2;
	uint32_t newblock;

	// restart字段是如果中间我们当前的级数需要增加，
	// 当我们级数增加后，我们需要重新开始执行dx_entry的分裂过程
	int restart;

again:
	restart = 0;
	frame = dx_probe(&dentry->d_name, dir, &hinfo, frames, &err);
	// 获取失败
	if(IS_ERR(frame))
		return PTR_ERR(frame);
	
	// 获取最底层对应的块
	entries = frame->entries;
	at = frame->at;

	// 获取下一级对应的磁盘块
	bh = sb_bread(sb, dx_get_block(at));
	if(IS_ERR(bh)){
		err = PRT_ERR(bh);
		bh = NULL;
		goto cleanup;
	}

	// 尝试插入目录项
	err = add_dirent_to_buf(dentry, inode, NULL, bh);
	if(err != -ENOSPC)
		goto cleanup;


	// 初始化为err
	// 此时肯定需要分裂
	err = 0;

	// need split index or not
	if(dx_get_count(entries) == dx_get_limit(entries)){
		// 初始化是否需要加级
		int add_level = 1;
		levels = frame - frames + 1;
		// 需要在前面加入dx_entry

		// 寻找最早需要分裂的层数
		while(frame > frames){
			// 检查其上一层的是否需要分裂
			if(dx_get_count((frame - 1)->entries) < dx_get_limit((frame - 1)->entries)){
				// 此时已经找到位置了
				add_level = 0;
				break;
			}
			frame--;
			at = frame->at;
			entries = frame->entries;
		}

		// 判断现在的级数是否到达了上限
		if(add_level && levels == XCRAFT_HTREE_LEVEL){
			// 此时级数已经满了，不能加级
			printk("XCraft_dx_add_entry: too many levels\n");
			err = -ENOSPC;
			goto cleanup;
		}

		icount = dx_get_count(entries);
		
		// 分配分裂需要的新块
		bh2 = XCraft_append(dir, &newblock);
		if(IS_ERR(bh2)){
			err = PTR_ERR(bh2);
			goto cleanup;
		}
		node2 = (struct dx_node *)(bh2->b_data);
		entries2 = node2->entries;
		node2->fake = 0;

		// 两种情况:
		// 需要加级数 不需要加级数
		// 不需要加级数的直接在前一层插入dx_entry
		
		if(!add_level){
			icount1 = icount/2, icount2 = icount - icount1;
			// 分裂位置的hash
			hash2 = dx_get_hash(entries + icount1);
			memcpy((char *) entries2, (char *)(entries + icount1), sizeof(struct dx_entry) * icount2);
			dx_set_count(entries, icount1);
			dx_set_count(entries2, icount2);
			dx_set_limit(entries2, dx_node_limit());

			// 确定哪个block会添加我们的目录项
			// 用来更新我们的frame
			if(at - entries >= icount1){
				frame->at = entries2 + (at - entries) - icount1;
				frame->entries = entries = entries2;
				swap(frame->bh, bh2);
			}

			dx_insert_block((frame - 1), hash2, newblock);
			// 此时我们需要mark_buffer_dirty，分裂的两个块，添加dx_entry的块
			mark_buffer_dirty(frame->bh);
			mark_buffer_dirty((frame - 1)->bh);
			mark_buffer_dirty(bh2);
			brelse(bh2);
		}
		else{
			// 需要添加级数
			memcpy((char *) entries2, (char *) entries, icount * sizeof(struct dx_entry));
			
			dx_set_limit(entries2, dx_node_limit());

			// 对dx_root进行重新修改赋值
			dx_set_count(entries, 1);
			dx_set_block(entries, newblock);
			
			root = (struct dx_root *) frames[0].bh->b_data;;
			// 级数增加
			root->info.indirect_levels += 1;
			// mark_buffer_dirty
			// 修改了dx_root，新分配的newblock
			mark_buffer_dirty(frame->bh);
			mark_buffer_dirty(bh2);
			brelse(bh2);
			restart = 1;
			goto cleanup;
		}
	}

	// dx_entry的分裂已经完成
	de = do_split(dir, &bh, frame, &hinfo);
	if(IS_ERR(de))
		err = PTR_ERR(de);

cleanup:
	brelse(bh);
	dx_release(frames);
	if(restart && err == 0)
		goto again;
	return err;
}

// 提交目录项
// 0表示添加目录项成功
static int XCraft_add_entry(struct dentry *dentry, struct inode *inode){
	struct inode *dir = dentry->d_parent->d_inode;
	struct XCraft_inode_info *xi = XCRAFT_I(inode);
	struct buffer_head *bh = NULL;
	struct super_block * sb;
	unsigned int blocksize;
	unsigned int i_block;
	int retval;

	// super block
	// 获取超级块
	sb = dir->i_sb;
	// 块大小
	blocksize = sb->s_blocksize;

	if(!dentry->d_name.len)
		return -EINVAL;
	
	// 判断时现在是否为哈希树
	if(XCraft_INODE_ISHASH_TREE(xi->i_flags)){
		retval = XCraft_dx_add_entry(dentry, inode);
		// retval为0表示添加目录项成功
		if(!retval)
			return retval;
		// 添加不成功
		printk("hash tree add entry failed\n");
		goto end;
	}

	// 遍历第一个块，此块已经被我们分配过
	i_block = xi->i_block[0];
	bh = sb_bread(sb, i_block);
	if(!bh){
	    retval = -EIO;
		goto end;
	}

	retval = add_dirent_to_buf(dentry, inode, NULL, bh);
	// 插入成功
	
	if(retval == -EEXIST)
		printk("same name of dentry has been existed\n");
	else if(retval == -ENOSPC)
		// 开始建立哈希树
		retval = XCraft_make_hash_tree(dentry, dir, inode, bh);
	
put_bh:
	brelse(bh);
end:
	return retval;
}


// 哈希搜索目录项
// dir是目录inode, qstr中存储了文件名，res_dir是我们最终要返回的对应此文件名的目录项
// buffer_head为找到此目录项所在块的buffer_head
// err是存储的返回的错误信息
static struct buffer_head * XCraft_dx_find_entry(struct inode *dir,
			struct qstr *entry, struct XCraft_dir_entry **res_dir)
{
	struct super_block *sb = dir->i_sb;
	struct XCraft_hash_info hinfo;

	struct dx_frame frames[XCRAFT_HTREE_LEVEL], *frame;
	struct buffer_head *bh, *ret;

	int err;
	uint32_t block;

	// 文件名长度
	int namelen;
	namelen = entry->len;

	// 文件名
	char *name = entry->name;
	if(namelen > XCRAFT_NAME_LEN){
		*res_dir = NULL;
		ret = NULL;
		goto out;
	}

	// 在磁盘块中寻找会用到
	struct XCraft_dir_entry *de;
	char *top;
	unsigned short reclen;

	// 首先由hash值获取frame
	frame = dx_probe(entry, dir, &hinfo, frames, &err);
	if(IS_ERR(frame)){
		*res_dir = NULL;
		ret = NULL;
		goto out;
	}
	

	block = dx_get_block(frame->at);
	bh = sb_bread(sb, block);
	if(!bh){
		*res_dir = NULL;
		ret = NULL;
		goto out_frames;	
	}

	// 开始在block对应的磁盘块中进行搜索
	de = (struct XCraft_dir_entry *) bh->b_data;
	top = bh->b_data + sb->s_blocksize;
	while((char *)de < top){
		// 比较文件名是否匹配
		if(strncmp(de->name, name, namelen) == 0){
			*res_dir = de;
			ret = bh;
			goto out_frames;
		}

		reclen = le16_to_cpu(de->rec_len);
		de = (struct XCraft_dir_entry *) ((char *)de + reclen);
	}

	// 没有发现，将*res_dir置为NULL
	*res_dir = NULL;
out_frames:
	dx_release(frames);
out:
	return ret;
}



// 搜索目录项，观察是否找到文件名对应的目录项
// dir是目录inode, qstr中存储了文件名，res_dir是我们最终要返回的对应此文件名的目录项
// buffer_head为找到此目录项所在块的buffer_head
// 找到目录项后我们会将res_dir进行设置，没有找到就设置为NULL
// 返回的buffer_head同理
static struct buffer_head *XCraft_find_entry(struct inode *dir,
					struct qstr *entry,
					struct XCraft_dir_entry **res_dir)
{
	int err;
	struct buffer_head *bh, *ret;
	struct super_block *sb = dir->i_sb;
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	int namelen;
	char *name = entry->name;
	unsigned int i_block;

	// 如果没有哈希树，此d_entry为我们的第一个块中的第一个目录项
	struct XCraft_dir_entry *de;
	char *top;

	// 如果没有哈希树，用于遍历目录项使用
	int count = 0;
	unsigned short reclen;
	// 检查文件名长度
	namelen = entry->len;
	if(namelen > XCRAFT_NAME_LEN)
		return NULL;
	
	// 获取i_block[0]
	i_block = dir_info->i_block[0];
	if(XCraft_INODE_ISHASH_TREE(dir_info->i_flags)){
		// 此时已经是哈希树
		ret = XCraft_dx_find_entry(dir, entry, res_dir);
		if (!IS_ERR(ret) || PTR_ERR(ret) != ERR_BAD_DX_DIR)
			// 此时表示已经找到
			goto cleanup_and_exit;
		// 哈希树没有找到此目录项
		ret = NULL;
	}

	// 遍历i_block
	bh = sb_bread(sb, i_block);
	if(!bh){
		ret = NULL;
		*res_dir = NULL;
		goto cleanup_and_exit;
	}

	// 在此块中搜索目录项
	de = (struct XCraft_dir_entry *) bh->b_data;
	// 块底部极限位置
	top = bh->b_data + sb->s_blocksize;


	while((char *)de < top && count<=XCRAFT_dentry_LIMIT){
		// 检查名字是否匹配
		if(strncmp(de->name, name, namelen) == 0){
			// 已经找到
			*res_dir = de;
			ret = bh;
			goto cleanup_and_exit;
		}

		reclen = le16_to_cpu(de->rec_len);
		de = (struct XCraft_dir_entry *) ((char *)de + reclen);
		count++;
	}

	// 没有发现, 将*res_dir置为NULL
	*res_dir = NULL;

cleanup_and_exit:
	return ret;
}



// 	create
//  create file or dir
//  in this dir
#if MNT_IDMAP_REQUIRED()
static int XCraft_create(struct mnt_idmap *id,
                           struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode,
                           bool excl)
#elif USER_NS_REQUIRED()
static int XCraft_create(struct user_namespace *ns,
                           struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode,
                           bool excl)
#else
static int XCraft_create(struct inode *dir,
                           struct dentry *dentry,
                           umode_t mode,
                           bool excl)
#endif
{
	struct super_block *sb = dir->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct inode *inode;
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 cur_time;
#endif

	// check filename length
	if(strlen(dentry->d_name.name) > XCRAFT_NAME_LEN)
		return -ENAMETOOLONG;

	// 获取inode
	inode = XCraft_new_inode(dir, &dentry->d_name, mode);
	if(IS_ERR(inode)){
		ret = PTR_ERR(inode);
		goto end;
	}

	// 获取inode_info
	struct XCraft_inode_info *inode_info = XCRAFT_I(inode);

	// 目录inode我们要开始添加目录项
	// ret 为0表示添加成功
	ret = XCraft_add_entry(dentry, inode);
	if(ret)
		goto end_inode;
	
	mark_inode_dirty(inode);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    cur_time = current_time(dir);
    dir->i_mtime = dir->i_atime = cur_time;
    inode_set_ctime_to_ts(dir, cur_time);
#else
    dir->i_mtime = dir->i_atime = dir->i_ctime = current_time(dir);
#endif

	if(S_ISDIR(mode))
		inc_nlink(dir);
	mark_inode_dirty(dir);

	// setup dentry
	d_instantiate(dentry, inode);

	return 0;

end_inode:
	// 释放inode中的i_block[0]
	put_blocks(sb_info, inode_info->i_block[0],1);
	// 释放inode
	put_inode(sb_info, inode->i_ino);
	iput(inode);
end:
	return ret;

}

// lookup
static struct dentry *XCraft_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags){
	struct super_block *sb = dir->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	struct XCraft_inode_info *inode_info = NULL;
	struct inode *inode = NULL;
	struct buffer_head *bh = NULL;
	struct XCraft_disk_inode *disk_inode = NULL;
	struct XCraft_dir_entry *de;
	int ret;
	
	// 检查文件名长度
	if(dentry->d_name.len > XCRAFT_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	// 获取目录项
	bh = XCraft_find_entry(dir, &dentry->d_name, &de);
	if(!bh)
		return -EIO;
	// 由inode号获取inode
	uint32_t ino = le32_to_cpu(de->inode);
	inode = XCraft_iget(sb, ino);
	if(IS_ERR(inode)){
		ret = PTR_ERR(inode);
			goto end;
	}

	//  更新dir中的相关时间信息
	dir->i_atime = current_time(dir);
	mark_inode_dirty(dir);

	// fill the dentry with the inode
	d_add(dentry, inode);
end:
	brelse(bh);
	return NULL;
}

// link
static int XCraft_link(struct dentry *old_dentry,
					   struct inode *dir, struct dentry *dentry){
	// 获取old_dentry对应的inode
	struct inode *inode = old_dentry->d_inode;
	int ret;

	// 对获取的inode进行时间修改
	inode->i_ctime = current_time(inode);
	// 硬链接数 + 1
	inode_inc_link_count(inode);
	ihold(inode);

	// 添加目录项
	ret = XCraft_add_entry(dentry, inode);
	// 表示插入成功
	if(!ret){
		mark_inode_dirty(inode);
		d_instantiate(dentry, inode);
	}
	else{
		drop_nlink(inode);
		iput(inode);
	}

	return ret;
}

// unlink
static int XCraft_unlink(struct inode *dir, struct dentry *dentry){
	struct super_block *sb = dir->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	// 获取dentry对应的inode
	struct inode *inode = d_inode(dentry);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 cur_time;
#endif

	uint32_t ino = inode->i_ino;





	return 0;
}

#if MNT_IDMAP_REQUIRED()
static int XCraft_symlink(struct mnt_idmap *id,
                            struct inode *dir,
                            struct dentry *dentry,
                            const char *symname)
#elif USER_NS_REQUIRED()
static int XCraft_symlink(struct user_namespace *ns,
                            struct inode *dir,
                            struct dentry *dentry,
                            const char *symname)
#else
static int XCraft_symlink(struct inode *dir,
                            struct dentry *dentry,
                            const char *symname)
#endif
{
	unsigned int l = strlen(symname) + 1;
	struct inode *inode = XCraft_new_inode(dir, &dentry->d_name, S_IFLNK | S_IRWXUGO);
	struct XCraft_inode_info *ci = XCRAFT_I(inode);
	struct XCraft_inode_info *ci_dir = XCRAFT_I(dir);
	int ret = 0;

	// 检查inode
	if (IS_ERR(inode)) {
        ret = PTR_ERR(inode);
        goto end;
    }

	// Check if symlink content is not too long
	if(l>sizeof(ci->i_data))
		return -ENAMETOOLONG;

	// 添加目录项在dir中
	// 添加目录项
	ret = XCraft_add_entry(dentry, inode);
	// 表示插入成功
	if(!ret){
		inode->i_link = (char *) ci->i_data;
		memcpy(inode->i_link, symname, l);
		inode->i_size = l - 1;
		mark_inode_dirty(inode);
		d_instantiate(dentry, inode);
		return 0;
	}

end:

	return ret;
}
// mkdir
#if MNT_IDMAP_REQUIRED()
static int XCraft_mkdir(struct mnt_idmap *id,
                          struct inode *dir,
                          struct dentry *dentry,
                          umode_t mode)
{

	return XCraft_create(id, dir, dentry, mode | S_IFDIR, 0);
}

#elif USER_NS_REQUIRED()
static int XCraft_mkdir(struct user_namespace *ns,
                          struct inode *dir,
                          struct dentry *dentry,
                          umode_t mode)
{

    return XCraft_create(ns, dir, dentry, mode | S_IFDIR, 0);
}
#else
static int XCraft_mkdir(struct inode *dir,
                          struct dentry *dentry,
                          umode_t mode)
{


    return XCraft_create(dir, dentry, mode | S_IFDIR, 0);
}
#endif

// rmdir
static int XCraft_rmdir(struct inode *dir, struct dentry *dentry){
	struct super_block *sb = dir->i_sb;



	return 0;
}

// rename
// 
#if MNT_IDMAP_REQUIRED()
static int XCraft_rename(struct mnt_idmap *id,
                           struct inode *old_dir,
                           struct dentry *old_dentry,
                           struct inode *new_dir,
                           struct dentry *new_dentry,
                           unsigned int flags)
#elif USER_NS_REQUIRED()
static int XCraft_rename(struct user_namespace *ns,
                           struct inode *old_dir,
                           struct dentry *old_dentry,
                           struct inode *new_dir,
                           struct dentry *new_dentry,
                           unsigned int flags)
#else
static int XCraft_rename(struct inode *old_dir,
                           struct dentry *old_dentry,
                           struct inode *new_dir,
                           struct dentry *new_dentry,
                           unsigned int flags)
#endif
{
	struct super_block *sb = old_dir->i_sb;
	struct XCraft_inode_info *new_dir_info = XCRAFT_I(new_dir);
	// old_dentry中关联的inode和new_dentry中关联的inode
	struct inode * old_inode, * new_inode;

	// 由old_dentry找到的目录项结构
	struct XCraft_dir_entry *old_de, *new_de;
	// 后面使用find_entry作为结果返回
	struct buffer_head *old_bh, *new_bh;

	old_bh = new_bh = NULL;
	old_de = new_de = NULL;

	// 最终的返回字段
	int retval;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 cur_time;
#endif

	// 不支持的字段我们会失败
	if (flags & (RENAME_EXCHANGE | RENAME_WHITEOUT))
        return -EINVAL;

	// 检查新文件名的长度
	if(strlen(new_dentry->d_name.name) > XCRAFT_NAME_LEN)
		return -ENAMETOOLONG;
	
	old_bh = XCraft_find_entry(old_dir,&old_dentry->d_name, &old_de);

	old_inode = old_dentry->d_inode;
	retval = -ENOENT;

	// find_entry没找到，或者找到的inode的ino和old_inode的ino不一样
	
	


	


	return 0;
}
// get_link
static const char *XCraft_get_link(struct dentry *dentry, struct inode *inode,
									   struct delayed_call *callback)
{

	return inode->i_link;
}

static const struct inode_operations XCraft_inode_operations = {
	.create = XCraft_create,
	.lookup = XCraft_lookup,
	.link = XCraft_link,
	.unlink = XCraft_unlink,
	.symlink = XCraft_symlink,
	.mkdir = XCraft_mkdir,
	.rmdir = XCraft_rmdir,
	.rename = XCraft_rename,
};

static const struct inode_operations XCraft_symlink_inode_operations = {
	.get_link = XCraft_get_link,
};
