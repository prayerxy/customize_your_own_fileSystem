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
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9a7ac26e, "module_layout" },
	{ 0x9a32323, "kmem_cache_destroy" },
	{ 0xded69698, "iget_failed" },
	{ 0xe1c8a74e, "__mark_inode_dirty" },
	{ 0x754d539c, "strlen" },
	{ 0x89cff89c, "init_user_ns" },
	{ 0x3d5bbd7d, "d_add" },
	{ 0xe304a920, "make_kgid" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x520e5e0c, "from_kuid" },
	{ 0x756237ba, "__bread_gfp" },
	{ 0x9166fada, "strncpy" },
	{ 0x37b1a33c, "from_kgid" },
	{ 0xba067e9b, "kmem_cache_free" },
	{ 0x743f8ddb, "set_nlink" },
	{ 0xc6c98a6c, "sync_dirty_buffer" },
	{ 0xbc55c576, "__brelse" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x8a7a5fd7, "kmem_cache_alloc" },
	{ 0x8b79bedb, "make_kuid" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0xb7ebb730, "unlock_new_inode" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x67e2a02c, "kmem_cache_create" },
	{ 0xea092dab, "iput" },
	{ 0x37a0cba, "kfree" },
	{ 0x74639170, "current_time" },
	{ 0x6611d05c, "mark_buffer_dirty" },
	{ 0x8810754a, "_find_first_bit" },
	{ 0x63927f50, "iget_locked" },
	{ 0x7bec93a7, "inode_init_owner" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "11E6E2FEDADFCEFEA2FF7BA");
