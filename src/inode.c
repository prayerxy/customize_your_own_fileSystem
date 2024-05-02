#include "../include/inode.h"
#include "../include/bitmap.h"
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//  additional
//  获取块组描述符
//	
static struct XCraft_group_desc *XCraft_get_group_desc(struct XCraft_super_block_info *sb_info, uint32_t block_group)
{
	struct buffer_head **s_group_desc = sb_info->s_group_desc;
	if (block_group >= sb_info->s_groups_count)
	{
		printk("block_group >= s_groups_count!\n");
		return NULL;
	}
	uint32_t group_desc = block_group / XCRAFT_GROUP_DESCS_PER_BLOCK;
	uint32_t offset = block_group % XCRAFT_GROUP_DESCS_PER_BLOCK;
	struct XCraft_group_desc *desc = (struct XCraft_group_desc *)s_group_desc[group_desc]->b_data;

	return desc + offset;
}

// 由ino获取指定的inode
static struct inode *XCraft_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode = NULL;
	struct XCraft_inode *disk_inode = NULL;
	struct XCraft_inode_info *xi = NULL;
	struct XCraft_super_block_info *sb_info = XCRAFT_SB(sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	struct buffer_head *bh = NULL;

	uint32_t block_group = inode_get_block_group(sb_info, ino);
	uint32_t inode_index_in_group = inode_get_block_group_shift(sb_info, ino);

	int ret;
	// 如果ino超过了范围
	if (ino >= le32toh(disk_sb->s_inodes_count))
		return ERR_PTR(-EINVAL);

	// 获取块组描述符
	struct XCraft_group_desc *desc = XCraft_get_group_desc(sb_info, block_group);
	if (!desc){
		printk("desc is NULL!\n");
		return ERR_PTR(-EINVAL);
	}

	if(!XCraft_BG_GROUP_IS_INIT(desc->bg_flags)){
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
	uint32_t inode_table = le32toh(desc->bg_inode_table);
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
	inode->i_mode = le16toh(disk_inode->i_mode);

	i_uid_write(inode, le16toh(disk_inode->i_uid));
    i_gid_write(inode, le16toh(disk_inode->i_gid));
    inode->i_size = le32toh(disk_inode->i_size_lo);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    inode_set_ctime(inode, (time64_t) le32toh(disk_inode->i_ctime), 0);
#else
    inode->i_ctime.tv_sec = (time64_t) le32toh(disk_inode->i_ctime);
    inode->i_ctime.tv_nsec = 0;
#endif

    inode->i_atime.tv_sec = (time64_t) le32toh(disk_inode->i_atime);
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_sec = (time64_t) le32toh(disk_inode->i_mtime);
    inode->i_mtime.tv_nsec = 0;
    inode->i_blocks = le32toh(disk_inode->i_blocks_lo);
    set_nlink(inode, le32toh(disk_inode->i_links_count));


	// XCraft_inode_info 字段赋值
	xi->i_dtime = le32toh(disk_inode->i_dtime);
	xi->i_block_group = block_group;
	xi->i_flags = le32toh(disk_inode->i_flags);
	
	if(S_ISDIR(inode->i_mode)){
		for(int i=0;i<XCRAFT_N_BLOCK;i++)
			xi->i_block[i] = le32toh(disk_inode->i_block[i]);
		inode->i_fop = &XCraft_dir_operations;
	}else if(S_ISREG(inode->i_mode)){
		for(int i=0;i<XCRAFT_N_BLOCK;i++)
			xi->i_block[i] = le32toh(disk_inode->i_block[i]);
		inode->i_fop = &XCraft_file_operations;
		inode->i_mapping->a_ops = &XCraft_aops
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
struct inode *XCraft_new_inode(handle_t *handle, struct inode *dir,
							   const struct qstr *qstr, int mode){					
	struct inode *inode = NULL;
	struct XCraft_inode_info *xi = NULL;
	struct super_block *sb = dir->i_sb;
	struct XCraft_super_block_info *sb_info = XCRAFT_SB(sb);
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
	if(le32toh(disk_sb->s_free_inodes_count) == 0 || le32toh(disk_sb->s_free_blocks_count) == 0)
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
	bno = get_free_block(sb_info);
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
	XCraft_put_inode(sb_info, ino);

	return ERR_PTR(ret);
}

// hash tree添加目录项
static int XCraft_dx_add_entry(handle_t *handle, const struct qstr *qstr,
							   struct inode *dir, struct inode *inode);

// 提交目录项
// 0表示添加目录项成功
static int XCraft_add_entry(handle_t *handle, struct dentry *dentry,
							struct inode *inode);


// 哈希搜索目录项
// dir是目录inode, qstr中存储了文件名，res_dir是我们最终要返回的对应此文件名的目录项
// buffer_head为找到此目录项所在块的buffer_head
// err是存储的返回的错误信息
static struct buffer_head * XCraft_dx_find_entry(struct inode *dir,
			struct qstr *entry, struct XCraft_dir_entry **res_dir,
			int *err);



// 搜索目录项，观察是否找到文件名对应的目录项
// dir是目录inode, qstr中存储了文件名，res_dir是我们最终要返回的对应此文件名的目录项
// buffer_head为找到此目录项所在块的buffer_head
static struct buffer_head *XCraft_find_entry(struct inode *dir,
					struct qstr *entry,
					struct XCraft_dir_entry **res_dir);





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
	inode = XCraft_new_inode(handle, dir, &dentry->d_name, mode);
	if(IS_ERR(inode)){
		ret = PTR_ERR(inode);
		goto end;
	}

	// 获取inode_info
	struct XCraft_inode_info *inode_info = XCRAFT_I(inode);

	// 目录inode我们要开始添加目录项
	// ret 为0表示添加成功
	ret = XCraft_add_entry(NULL, dentry, inode);
	if(ret)
		goto end_inode;
	
	return 0;

end_inode:
	// 释放inode中的i_block[0]
	XCraft_put_block(sb_info, inode_info->i_block[0]);
	// 释放inode
	XCraft_put_inode(sb_info, inode->i_ino);
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
	struct buffer_head *bh = NULL;
	int ret;
	
	// 检查文件名长度
	if(dentry->d_name.len > XCRAFT_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	// 获取目录项
	bh = XCraft_find_entry(dir, &dentry->d_name, &de);

	// 由inode号获取inode
	uint32_t ino = le32toh(de->inode);
	// static struct inode *XCraft_iget(struct super_block *sb, unsigned long ino)
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
	


}

// unlink
static int XCraft_unlink(struct inode *dir, struct dentry *dentry);

// symlink
#if XCraft_iop_version_judge()
static int XCraft_symlink(struct user_namespace *mnt_userns, struct inode *dir,
						  struct dentry *dentry, const char *symname)
#else
static int XCraft_symlink(struct inode *dir,
						  struct dentry *dentry, const char *symname)
#endif

// mkdir
#if XCraft_iop_version_judge()
	static int XCraft_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
							struct dentry *dentry, umode_t mode)
#else
	static int XCraft_mkdir(struct inode *dir,
							struct dentry *dentry, umode_t mode)
#endif

	// rmdir
	static int XCraft_rmdir(struct inode *dir, struct dentry *dentry);

// rename
#if XCraft_iop_version_judge()
static int XCraft_rename(struct user_namespace *mnt_userns,
						 struct inode *old_dir, struct dentry *old_dentry,
						 struct inode *new_dir, struct dentry *new_dentry,
						 unsigned int flags)
#else
static int XCraft_rename(struct inode *old_dir, struct dentry *old_dentry,
						 struct inode *new_dir, struct dentry *new_dentry,
						 unsigned int flags)
#endif

// get_link
static const char *XCraft_get_link(struct dentry *dentry, struct inode *inode,
									   struct delayed_call *callback);

const struct inode_operations XCraft_inode_operations = {
	.create = XCraft_create,
	.lookup = XCraft_lookup,
	.link = XCraft_link,
	.unlink = XCraft_unlink,
	.symlink = XCraft_symlink,
	.mkdir = XCraft_mkdir,
	.rmdir = XCraft_rmdir,
	.rename = XCraft_rename,
};

const struct inode_operations XCraft_symlink_inode_operations = {
	.get_link = XCraft_get_link,
};
