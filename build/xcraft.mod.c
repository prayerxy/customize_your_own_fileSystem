#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
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
	{ 0x260074d8, "inode_init_owner" },
	{ 0xc64a6dfe, "iget_locked" },
	{ 0x4ad0f073, "d_instantiate" },
	{ 0xb0e602eb, "memmove" },
	{ 0xfaa044ee, "unregister_filesystem" },
	{ 0x9d6a54fe, "mark_buffer_dirty" },
	{ 0x9ffe51c6, "d_make_root" },
	{ 0x373c8e3c, "sb_set_blocksize" },
	{ 0x8a6faa67, "current_time" },
	{ 0x69acdf38, "memcpy" },
	{ 0xf48fc612, "ihold" },
	{ 0x37a0cba, "kfree" },
	{ 0xc3a170f5, "generic_file_fsync" },
	{ 0x7888a93a, "iput" },
	{ 0x35d08948, "generic_file_write_iter" },
	{ 0xf2e8153d, "register_filesystem" },
	{ 0x2b4eaa3, "kmem_cache_create" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x182c1601, "kill_block_super" },
	{ 0x5c263ac4, "unlock_new_inode" },
	{ 0x122c3a7e, "_printk" },
	{ 0x1ab4c994, "generic_write_end" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0xaadc978b, "make_kuid" },
	{ 0xd9efee55, "block_write_full_page" },
	{ 0x4e779bb7, "kmem_cache_alloc" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x615911d7, "__bitmap_set" },
	{ 0x9eafafd5, "inode_init_once" },
	{ 0xd4fa9c3b, "__brelse" },
	{ 0xc6441797, "generic_file_read_iter" },
	{ 0x2d1e5850, "kmem_cache_create_usercopy" },
	{ 0x80b52740, "truncate_pagecache" },
	{ 0x9c8ae11, "sync_dirty_buffer" },
	{ 0xfd950597, "set_nlink" },
	{ 0x5b8379e7, "kmem_cache_free" },
	{ 0x5a921311, "strncmp" },
	{ 0x97dca8a9, "from_kgid" },
	{ 0x9166fada, "strncpy" },
	{ 0x53a1e8d9, "_find_next_bit" },
	{ 0x5eec64b7, "__bread_gfp" },
	{ 0xf5f6249c, "from_kuid" },
	{ 0xfb578fc5, "memset" },
	{ 0x31549b2a, "__x86_indirect_thunk_r10" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x43115cf4, "make_kgid" },
	{ 0x922f45a6, "__bitmap_clear" },
	{ 0xd73f5e87, "d_add" },
	{ 0x2606df8f, "mount_bdev" },
	{ 0x5821b905, "mpage_readahead" },
	{ 0xe8145d58, "init_user_ns" },
	{ 0xc74c0028, "inc_nlink" },
	{ 0xe39ec0fa, "block_write_begin" },
	{ 0x850e6a88, "kmalloc_trace" },
	{ 0x754d539c, "strlen" },
	{ 0x2ae403f3, "__mark_inode_dirty" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x82d2665c, "generic_file_llseek" },
	{ 0xf24c1337, "drop_nlink" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xad6d045f, "kmalloc_caches" },
	{ 0x22113abd, "iget_failed" },
	{ 0xf1931756, "kmem_cache_destroy" },
	{ 0x453e7dc, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0C3A5ABC9D68E6210D4245A");
