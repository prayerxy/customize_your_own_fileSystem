#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x367fcc51, "module_layout" },
	{ 0xe9c617de, "kmem_cache_destroy" },
	{ 0x99080f3a, "iget_failed" },
	{ 0x58f94a7a, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xeace6c34, "drop_nlink" },
	{ 0xa8b0c538, "generic_file_llseek" },
	{ 0xe16bc941, "__mark_inode_dirty" },
	{ 0x754d539c, "strlen" },
	{ 0x973efad9, "block_write_begin" },
	{ 0xd45d4e25, "inc_nlink" },
	{ 0x6fbcc169, "init_user_ns" },
	{ 0xa23ea52a, "mount_bdev" },
	{ 0x959ca796, "d_add" },
	{ 0x922f45a6, "__bitmap_clear" },
	{ 0x81fcfa48, "pv_ops" },
	{ 0xa668d609, "make_kgid" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xfb578fc5, "memset" },
	{ 0xa9b6b395, "from_kuid" },
	{ 0xb8f7c5a8, "mpage_readpage" },
	{ 0x494779d6, "__bread_gfp" },
	{ 0xa50a3da7, "_find_next_bit" },
	{ 0x1a79c8e9, "__x86_indirect_thunk_r13" },
	{ 0x9166fada, "strncpy" },
	{ 0xd3169e96, "from_kgid" },
	{ 0x5a921311, "strncmp" },
	{ 0x1caeeb0e, "kmem_cache_free" },
	{ 0x63710b1e, "set_nlink" },
	{ 0x92c5134e, "sync_dirty_buffer" },
	{ 0xc428cd5b, "truncate_pagecache" },
	{ 0x39122d19, "kmem_cache_create_usercopy" },
	{ 0x29a7e6d, "generic_file_read_iter" },
	{ 0x89068065, "__brelse" },
	{ 0x5e964888, "inode_init_once" },
	{ 0x615911d7, "__bitmap_set" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x180265d5, "kmem_cache_alloc" },
	{ 0x4e5856b0, "block_write_full_page" },
	{ 0x48734f74, "make_kuid" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xcbbeff2c, "generic_write_end" },
	{ 0x92997ed8, "_printk" },
	{ 0x91b9dcac, "unlock_new_inode" },
	{ 0x91211fac, "kill_block_super" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbb0ae81, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xf9032ed2, "kmem_cache_create" },
	{ 0xb393e61d, "register_filesystem" },
	{ 0x4b4b21da, "generic_file_write_iter" },
	{ 0x876a6c3b, "iput" },
	{ 0x2f286057, "generic_file_fsync" },
	{ 0x37a0cba, "kfree" },
	{ 0xc7321fd0, "ihold" },
	{ 0x69acdf38, "memcpy" },
	{ 0x11838a95, "current_time" },
	{ 0xa2624e0d, "sb_set_blocksize" },
	{ 0x204f0613, "d_make_root" },
	{ 0xa5417e2c, "mark_buffer_dirty" },
	{ 0x8810754a, "_find_first_bit" },
	{ 0x974b722, "unregister_filesystem" },
	{ 0xb0e602eb, "memmove" },
	{ 0x8d4824fd, "d_instantiate" },
	{ 0xe80dc82f, "iget_locked" },
	{ 0xbe686326, "inode_init_owner" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CAB6BD3F4F7C4F497D73DCC");
