#include "../include/inode.h"
#include "../include/bitmap.h"
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// additional
// 获取块组描述符
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

	uint32_t block_group = inode_get_block_group(sb, ino);
	uint32_t inode_index_in_group = inode_get_block_group_shift(sb, ino);

	int ret;
	// 如果ino超过了范围
	if (ino >= le32_to_cpu(disk_sb->s_inodes_count))
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
struct inode *XCraft_new_inode(handle_t *handle, struct inode *dir,
							   const struct qstr *qstr, int mode){					
	struct inode *inode = NULL;
	struct XCraft_inode *disk_inode = NULL;
	struct XCraft_inode_info *xi = NULL;
	struct XCraft_super_block_info *sb_info = XCRAFT_SB(dir->i_sb);
	struct XCraft_superblock *disk_sb = sb_info->s_super;
	struct buffer_head *bh = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    struct timespec64 cur_time;
#endif

	// 检查文件的类型是否正确

}

// hash tree添加目录项
static int XCraft_dx_add_entry(handle_t *handle, const struct qstr *qstr,
							   struct inode *dir, struct inode *inode);

// 提交目录项
static int XCraft_add_entry(handle_t *handle, struct dentry *dentry,
							struct inode *inode);

// create
//  create file or dir
//  in this dir
#if XCraft_iop_version_judge()
static int XCraft_create(struct user_namespace *mnt_userns, struct inode *dir,
						 struct dentry *dentry, umode_t mode, bool excl)
#else
static int XCraft_create(struct inode *dir,
						 struct dentry *dentry, umode_t mode, bool excl)
#endif

	// lookup
	static struct dentry *XCraft_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);

// link
static int XCraft_link(struct dentry *old_dentry,
					   struct inode *dir, struct dentry *dentry);

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
