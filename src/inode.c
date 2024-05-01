#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/bitmap.h"
#include "../include/XCraft.h"
#include "../include/hash.h"
// additional
// 由ino获取指定的inode
struct inode *XCraft_iget(struct super_block *sb, unsigned long ino);

// qstr = &dentry->d_name
//注意inode_init_owner的版本号的变化
struct inode *XCraft_new_inode(handle_t *handle, struct inode * dir,
			     const struct qstr *qstr, int mode);

// hash tree添加目录项
static int XCraft_dx_add_entry(handle_t *handle, const struct qstr *qstr,
			     struct inode *dir, struct inode *inode);


//提交目录项
static int XCraft_add_entry(handle_t *handle, struct dentry *dentry,
			  struct inode *inode);


//create
// create file or dir
// in this dir
#if XCraft_iop_version_judge()
static int XCraft_create(struct user_namespace *mnt_userns, struct inode *dir,
		       struct dentry *dentry, umode_t mode, bool excl)
#else
static int XCraft_create(struct inode *dir,
		       struct dentry *dentry, umode_t mode, bool excl)
#endif


//lookup
static struct dentry *XCraft_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);

//link
static int XCraft_link(struct dentry *old_dentry,
		     struct inode *dir, struct dentry *dentry);

//unlink
static int XCraft_unlink(struct inode *dir, struct dentry *dentry);

//symlink
#if XCraft_iop_version_judge()
static int XCraft_symlink(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, const char *symname)
#else
static int XCraft_symlink(struct inode *dir,
			struct dentry *dentry, const char *symname)
#endif

//mkdir
#if XCraft_iop_version_judge()
static int XCraft_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode)
#else
static int XCraft_mkdir(struct inode *dir,
		      struct dentry *dentry, umode_t mode)
#endif

//rmdir
static int XCraft_rmdir(struct inode *dir, struct dentry *dentry);

//rename
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
	.create		= XCraft_create,
	.lookup		= XCraft_lookup,
	.link		= XCraft_link,
	.unlink		= XCraft_unlink,
	.symlink	= XCraft_symlink,
	.mkdir		= XCraft_mkdir,
	.rmdir		= XCraft_rmdir,
	.rename		= XCraft_rename,
};

const struct inode_operations XCraft_symlink_inode_operations = {
	.get_link	= XCraft_get_link,
};


