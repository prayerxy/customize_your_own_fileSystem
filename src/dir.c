#define pr_fmt(fmt) "XCraft: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../include/XCraft.h"

//ls
static int XCraft_readdir(struct file *file, struct dir_context *ctx);




const struct file_operations XCraft_dir_operations = {
	.iterate_shared	= XCraft_readdir,
};