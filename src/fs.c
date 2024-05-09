#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
// #include <linux/init.h>
// #include <linux/types.h>
#include "../include/XCraft.h"

// mount
static struct dentry *XCraft_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	struct dentry *dentry =
        mount_bdev(fs_type, flags, dev_name, data, XCraft_fill_super);
	
	if(IS_ERR(dentry))
		pr_err("'%s' mount failure\n", dev_name);
	else
		pr_info("'%s' mount success\n", dev_name);
	return dentry;
}

// unmount
void XCraft_kill_sb(struct super_block *sb){
	kill_block_super(sb);
	printk("unmounted disk\n");
}

//file_system_type define
static struct file_system_type XCraft_fs_type = {
    .owner		= THIS_MODULE,
	.name		= "XCraft",
	.mount		= XCraft_mount,
	.kill_sb	= XCraft_kill_sb,
	.fs_flags	= FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init XCraft_init(void){

	// 初始化分配inode的仓库
	int ret = XCraft_init_inode_cache();
	if(ret){
		pr_err("inode cache creation failed\n");
		goto end;
	}

	ret = register_filesystem(&XCraft_fs_type);
	if(ret){
		pr_err("register_filesystem() failed\n");
		goto end;
	}

	pr_info("module loaded\n");
end:
	return ret;
}


static void __exit XCraft_exit(void){
	int ret = unregister_filesystem(&XCraft_fs_type);
	if(ret)
		pr_err("unregister_filesystem() failed\n");

	XCraft_destroy_inode_cache();
	pr_info("module unloaded\n");
}



module_init(XCraft_init);
module_exit(XCraft_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pray4u");
MODULE_DESCRIPTION("XCraft by pray4u");