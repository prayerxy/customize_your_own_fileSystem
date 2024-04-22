#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/bitmap.h"
#include "../include/XCraft.h"
#include 
// additional
<<<<<<< HEAD
// ç”±inoè·å–æŒ‡å®šçš„inode
=======
// ÓÉino»ñÈ¡¶ÔÓ¦µÄinode
>>>>>>> d4cc19427a6d0e53fd85b16221ad3f38cb5d3f89
struct inode *XCraft_iget(struct super_block *sb, unsigned long ino);

// qstr = &dentry->d_name
struct inode *XCraft_new_inode(handle_t *handle, struct inode * dir,
			     const struct qstr *qstr, int mode);

<<<<<<< HEAD
// hash treeæ·»åŠ ç›®å½•é¡¹
=======
// hash treeÌí¼ÓÄ¿Â¼Ïî
>>>>>>> d4cc19427a6d0e53fd85b16221ad3f38cb5d3f89
static int XCraft_dx_add_entry(handle_t *handle, const struct qstr *qstr,
			     struct inode *dir, struct inode *inode);


<<<<<<< HEAD
// ï¿½ï¿½dentry->d_parentï¿½ï¿½È¡ï¿½ï¿½Ä¿Â¼inodeï¿½ï¿½È»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½æ´´ï¿½ï¿½ï¿½Ä¼ï¿½ï¿½ï¿½ï¿½ï¿½Ä¿Â¼ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½
=======
// ÓÉdentry->d_parent»ñÈ¡µ½Ä¿Â¼inode£¬È»ºóÔÚËüµÄÏÂÃæ´´½¨ÎÄ¼şÌí¼ÓÄ¿Â¼ÏîÊ±µ÷ÓÃ
>>>>>>> d4cc19427a6d0e53fd85b16221ad3f38cb5d3f89
static int XCraft_add_entry(handle_t *handle, struct dentry *dentry,
			  struct inode *inode);


//create 
static int XCraft_create(struct user_namespace *mnt_userns, struct inode *dir,
		       struct dentry *dentry, umode_t mode, bool excl);

//lookup
static struct dentry *XCraft_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);

//link
static int XCraft_link(struct dentry *old_dentry,
		     struct inode *dir, struct dentry *dentry);

//unlink
static int XCraft_unlink(struct inode *dir, struct dentry *dentry);

//symlink
static int XCraft_symlink(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, const char *symname);

//mkdir
static int XCraft_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode);

//rmdir
static int XCraft_rmdir(struct inode *dir, struct dentry *dentry);

//rename
static int XCraft_rename(struct user_namespace *mnt_userns,
			struct inode *old_dir, struct dentry *old_dentry,
			struct inode *new_dir, struct dentry *new_dentry,
			unsigned int flags);

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
