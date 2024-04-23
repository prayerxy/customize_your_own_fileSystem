#define pr_diary(xcraft) KBUILD_MODNAME ": " xcraft

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include "XCraft.h"

// slab cache for XCraft_inode_info
static struct kmem_cache *XCraft_inode_cache;

// init our slab cache
static int XCraft_init_inode_cache(void);

// destroy our slab cache
static int XCraft_destroy_inode_cache(void);

// sops
// alloc_inode
static struct inode *XCraft_alloc_inode(struct super_block *sb);

// free_inode
static void XCraft_free_inode(struct inode *inode);

// write_inode
int XCraft_write_inode(struct inode *inode, struct writeback_control *wbc);

// drop_inode
int XCraft_drop_inode(struct inode *inode);

// put_super
static void XCraft_put_super(struct super_block *sb);

// sync_fs
static int XCraft_sync_fs(struct super_block *sb, int wait);

// statfs
static int XCraft_statfs(struct dentry *dentry, struct kstatfs *buf);


struct super_operations XCraft_sops
{
    .alloc_inode = XCraft_alloc_inode,
    .free_inode = XCraft_free_inode,
    .write_inode = XCraft_write_inode,
    .drop_inode = XCraft_drop_inode,
    .put_super = XCraft_put_super,
    .sync_fs = XCraft_sync_fs,
    .statfs = XCraft_statfs,
}

// fill super for mount
static int XCraft_fill_super(struct super_block *sb, void *data, int silent);
