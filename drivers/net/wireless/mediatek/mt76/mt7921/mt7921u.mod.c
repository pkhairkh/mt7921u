#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x580233d2, "__mt76u_init" },
	{ 0x6fe3c689, "mt7921_acs_cleanup" },
	{ 0x3ceb3c58, "consume_skb" },
	{ 0x565cdb6c, "mt792xu_copy" },
	{ 0x24cd63a9, "mt792xu_stop" },
	{ 0x5d670410, "usb_register_driver" },
	{ 0x44a86b7a, "mt76_connac_pm_wake" },
	{ 0x2ad5b1a0, "mt7921_csi_cleanup" },
	{ 0xb54f5ee5, "mt7921_mcu_set_eeprom" },
	{ 0x8093fe45, "mt7921_usb_sdio_tx_complete_skb" },
	{ 0x32f49d71, "mt792xu_rmw" },
	{ 0xe2964344, "__wake_up" },
	{ 0x6ee045ad, "mt76u_resume_rx" },
	{ 0x6f077807, "kthread_park" },
	{ 0xf456a9e2, "mt7921_mac_init" },
	{ 0x361f7230, "mt76_alloc_device" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0xa39347ed, "skb_queue_purge_reason" },
	{ 0x339e8461, "mt76_connac_power_save_sched" },
	{ 0x84c8ee9a, "mt792xu_mcu_power_on" },
	{ 0xa26241d0, "mt7921_register_device" },
	{ 0xaa221a96, "__mt7921_start" },
	{ 0x18fa4011, "mt7921_run_firmware" },
	{ 0xca06dd3e, "usb_put_dev" },
	{ 0xc8882ec2, "usb_bulk_msg" },
	{ 0x1b9ba5f0, "usb_reset_device" },
	{ 0x2c75e8f4, "mt7921_usb_sdio_tx_status_data" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x36bfc956, "usb_get_dev" },
	{ 0x363f4316, "mt76u_alloc_mcu_queue" },
	{ 0xfe6e6c71, "mt76_free_device" },
	{ 0xa1f219c1, "kthread_unpark" },
	{ 0x12258fbf, "mt7921_ops" },
	{ 0x544c96ab, "mt76u_queues_deinit" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x285e9723, "skb_push" },
	{ 0xe82c35e8, "mt792xu_wfsys_reset" },
	{ 0xbf14cd6a, "mt792xu_disconnect" },
	{ 0x27391757, "mt792xu_wr" },
	{ 0x409e887c, "usb_deregister" },
	{ 0xdcb764ad, "memset" },
	{ 0xc0524be4, "mt7921_mac_sta_remove" },
	{ 0x499cb481, "mt7921_usb_sdio_tx_prepare_skb" },
	{ 0x4b3f0348, "mt76u_read_copy" },
	{ 0xa167d57d, "mt7921_twt_teardown_flow" },
	{ 0x88855e97, "mt76_connac2_mcu_fill_message" },
	{ 0xa594430f, "mt7921_mcu_parse_response" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x95ec24ac, "mt76_txq_schedule_all" },
	{ 0x9641f71f, "mt76u_alloc_queues" },
	{ 0xd9759264, "mt76u_stop_tx" },
	{ 0xe5967499, "mt792xu_init_reset" },
	{ 0x736ffe38, "mt792xu_dma_init" },
	{ 0xa3fd1836, "mt7921_mac_sta_add" },
	{ 0x4eea5aac, "mt792x_update_channel" },
	{ 0x3728dcd6, "mt792x_get_mac80211_ops" },
	{ 0x9c031d81, "mt7921_queue_rx_skb" },
	{ 0x379c02a0, "mt76u_stop_rx" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x1ae6727b, "mt76_connac_mcu_set_hif_suspend" },
	{ 0xd62f08ef, "mt792x_reset" },
	{ 0x6a201a27, "mt7921_mac_sta_event" },
	{ 0x74ca23e3, "mt7921_rx_check" },
	{ 0x26b181f9, "mt792xu_rr" },
	{ 0xcd0f0050, "mt7921_set_channel" },
	{ 0xf9a482f9, "msleep" },
	{ 0x2f2c95c4, "flush_work" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt76-usb,mt7921-common,mt792x-usb,mt76-connac-lib,mt76,mt792x-lib");

MODULE_ALIAS("usb:v0E8Dp7961d*dc*dsc*dp*icFFiscFFipFFin*");
MODULE_ALIAS("usb:v3574p6211d*dc*dsc*dp*icFFiscFFipFFin*");
MODULE_ALIAS("usb:v0846p9060d*dc*dsc*dp*icFFiscFFipFFin*");
MODULE_ALIAS("usb:v0846p9065d*dc*dsc*dp*icFFiscFFipFFin*");
MODULE_ALIAS("usb:v35BCp0107d*dc*dsc*dp*icFFiscFFipFFin*");

MODULE_INFO(srcversion, "EFE770C012237CBF52842D2");
