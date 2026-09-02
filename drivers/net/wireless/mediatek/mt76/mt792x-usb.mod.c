#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(mt792xu_rr, "_gpl", "");
KSYMTAB_FUNC(mt792xu_wr, "_gpl", "");
KSYMTAB_FUNC(mt792xu_rmw, "_gpl", "");
KSYMTAB_FUNC(mt792xu_copy, "_gpl", "");
KSYMTAB_FUNC(mt792xu_mcu_power_on, "_gpl", "");
KSYMTAB_FUNC(mt792xu_dma_init, "_gpl", "");
KSYMTAB_FUNC(mt792xu_wfsys_reset, "_gpl", "");
KSYMTAB_FUNC(mt792xu_init_reset, "_gpl", "");
KSYMTAB_FUNC(mt792xu_stop, "_gpl", "");
KSYMTAB_FUNC(mt792xu_disconnect, "_gpl", "");

SYMBOL_CRC(mt792xu_rr, 0x26b181f9, "_gpl");
SYMBOL_CRC(mt792xu_wr, 0x27391757, "_gpl");
SYMBOL_CRC(mt792xu_rmw, 0x32f49d71, "_gpl");
SYMBOL_CRC(mt792xu_copy, 0x565cdb6c, "_gpl");
SYMBOL_CRC(mt792xu_mcu_power_on, 0x84c8ee9a, "_gpl");
SYMBOL_CRC(mt792xu_dma_init, 0x736ffe38, "_gpl");
SYMBOL_CRC(mt792xu_wfsys_reset, 0xe82c35e8, "_gpl");
SYMBOL_CRC(mt792xu_init_reset, 0xe5967499, "_gpl");
SYMBOL_CRC(mt792xu_stop, 0x24cd63a9, "_gpl");
SYMBOL_CRC(mt792xu_disconnect, 0xbf14cd6a, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8ebb18a9, "mt792x_stop" },
	{ 0xc095c22b, "__mt76u_vendor_request" },
	{ 0x897980ed, "___mt76u_wr" },
	{ 0x4829a47e, "memcpy" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe2964344, "__wake_up" },
	{ 0x6ee045ad, "mt76u_resume_rx" },
	{ 0xa39347ed, "skb_queue_purge_reason" },
	{ 0xca06dd3e, "usb_put_dev" },
	{ 0xfe6e6c71, "mt76_free_device" },
	{ 0x544c96ab, "mt76u_queues_deinit" },
	{ 0xbb260be5, "_dev_err" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x03c309a7, "mt76_unregister_device" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xd9759264, "mt76u_stop_tx" },
	{ 0xdf3858ef, "mt76u_vendor_request" },
	{ 0x6497453a, "____mt76_poll_msec" },
	{ 0x03c12dfe, "cancel_work_sync" },
	{ 0x379c02a0, "mt76u_stop_rx" },
	{ 0x3fe5fa43, "___mt76u_rr" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x21a6fb96, "__mt76_poll" },
	{ 0xf9a482f9, "msleep" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt792x-lib,mt76-usb,mt76");


MODULE_INFO(srcversion, "E056239C65050376D961731");
