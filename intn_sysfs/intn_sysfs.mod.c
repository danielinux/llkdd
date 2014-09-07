#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x2c54098b, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x8db480a3, __VMLINUX_SYMBOL_STR(platform_device_unregister) },
	{ 0x551f8de0, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0x48d32bb6, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x655bf44a, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x64538c71, __VMLINUX_SYMBOL_STR(platform_device_register_full) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x922d6e9b, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xb9996cb7, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

