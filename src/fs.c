#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include "../include/XCraft.h"

// mount
static struct dentry *XCraft_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data);


//file_system_type define
static struct file_system_type XCraft_fs_type = {
    .owner		= THIS_MODULE,
	.name		= "XCraft",
	.mount		= XCraft_mount,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init XCraft_init(void);


static void __exit XCraft_exit(void);



module_init(XCraft_init);
module_exit(XCraft_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pray4u");
MODULE_DESCRIPTION("XCraft by pray4u");