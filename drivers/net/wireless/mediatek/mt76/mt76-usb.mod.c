#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(__mt76u_vendor_request, "_gpl", "");
KSYMTAB_FUNC(mt76u_vendor_request, "_gpl", "");
KSYMTAB_FUNC(___mt76u_rr, "_gpl", "");
KSYMTAB_FUNC(___mt76u_wr, "_gpl", "");
KSYMTAB_FUNC(mt76u_read_copy, "_gpl", "");
KSYMTAB_FUNC(mt76u_single_wr, "_gpl", "");
KSYMTAB_FUNC(mt76u_alloc_mcu_queue, "_gpl", "");
KSYMTAB_FUNC(mt76u_stop_rx, "_gpl", "");
KSYMTAB_FUNC(mt76u_resume_rx, "_gpl", "");
KSYMTAB_FUNC(mt76u_stop_tx, "_gpl", "");
KSYMTAB_FUNC(mt76u_queues_deinit, "_gpl", "");
KSYMTAB_FUNC(mt76u_alloc_queues, "_gpl", "");
KSYMTAB_FUNC(__mt76u_init, "_gpl", "");
KSYMTAB_FUNC(mt76u_init, "_gpl", "");

SYMBOL_CRC(__mt76u_vendor_request, 0xc095c22b, "_gpl");
SYMBOL_CRC(mt76u_vendor_request, 0xdf3858ef, "_gpl");
SYMBOL_CRC(___mt76u_rr, 0x3fe5fa43, "_gpl");
SYMBOL_CRC(___mt76u_wr, 0x897980ed, "_gpl");
SYMBOL_CRC(mt76u_read_copy, 0x4b3f0348, "_gpl");
SYMBOL_CRC(mt76u_single_wr, 0xaa472b72, "_gpl");
SYMBOL_CRC(mt76u_alloc_mcu_queue, 0x363f4316, "_gpl");
SYMBOL_CRC(mt76u_stop_rx, 0x379c02a0, "_gpl");
SYMBOL_CRC(mt76u_resume_rx, 0x6ee045ad, "_gpl");
SYMBOL_CRC(mt76u_stop_tx, 0xd9759264, "_gpl");
SYMBOL_CRC(mt76u_queues_deinit, 0x544c96ab, "_gpl");
SYMBOL_CRC(mt76u_alloc_queues, 0x9641f71f, "_gpl");
SYMBOL_CRC(__mt76u_init, 0x580233d2, "_gpl");
SYMBOL_CRC(mt76u_init, 0x447ee41c, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x01b11052, "usb_free_urb" },
	{ 0x14d6dbbb, "devm_kmalloc" },
	{ 0x34446683, "skb_put" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x3c54fbfc, "page_pool_alloc_frag" },
	{ 0xb8342c83, "__kmalloc_noprof" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xc1a9d628, "trace_raw_output_prep" },
	{ 0xc678b384, "__trace_trigger_soft_disabled" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xf3f1225e, "trace_event_printf" },
	{ 0x0ea3ee0e, "trace_event_raw_init" },
	{ 0x4829a47e, "memcpy" },
	{ 0x6dbbd66c, "mt76_has_tx_pending" },
	{ 0x7b557766, "page_pool_destroy" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x8441b125, "bpf_trace_run2" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xe2964344, "__wake_up" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x6f077807, "kthread_park" },
	{ 0x50c0b77d, "sched_set_fifo_low" },
	{ 0x7dc3d53e, "mt76_tx_status_check" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0x8f8269e0, "trace_event_buffer_commit" },
	{ 0xb29f6a64, "___ratelimit" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xaef5d983, "__alloc_skb" },
	{ 0x4e280f98, "usb_submit_urb" },
	{ 0x476b165a, "sized_strscpy" },
	{ 0xa1f219c1, "kthread_unpark" },
	{ 0x9fb453a5, "build_skb" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xb6e16cdb, "perf_trace_buf_alloc" },
	{ 0x0d9609a4, "perf_trace_run_bpf_submit" },
	{ 0xbb260be5, "_dev_err" },
	{ 0xf70e4a4d, "preempt_schedule_notrace" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xf25c315b, "usb_control_msg" },
	{ 0x640678b5, "usb_poison_urb" },
	{ 0x8e0afadb, "trace_event_reg" },
	{ 0xd4c026e0, "usb_unpoison_urb" },
	{ 0xc6634315, "mt76_ac_to_hwq" },
	{ 0x9844c6f5, "page_pool_put_unrefed_netmem" },
	{ 0x921b07b1, "__cpu_online_mask" },
	{ 0x3c3fce39, "__local_bh_enable_ip" },
	{ 0x56fc4379, "kthread_stop" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xdcb764ad, "memset" },
	{ 0xb446fd58, "mt76_queue_tx_complete" },
	{ 0xce5708e3, "kthread_create_on_node" },
	{ 0x7447a4bc, "bpf_trace_run3" },
	{ 0x531351fb, "mt76_rx_poll_complete" },
	{ 0xd111de91, "trace_event_buffer_reserve" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x522f8bd7, "param_ops_bool" },
	{ 0x8b07079c, "usb_kill_urb" },
	{ 0x03c12dfe, "cancel_work_sync" },
	{ 0x56470118, "__warn_printk" },
	{ 0x1ec57b4f, "__mt76_worker_fn" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x46b44ce5, "usb_init_urb" },
	{ 0x09d48773, "skb_add_rx_frag_netmem" },
	{ 0x489cbaa2, "param_ops_int" },
	{ 0x0d44deb3, "mt76_create_page_pool" },
	{ 0x7a1929c1, "trace_handle_return" },
	{ 0xa58399d7, "skb_to_sgvec" },
	{ 0x2cf0c910, "sg_init_table" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt76");


MODULE_INFO(srcversion, "80E56A6693B320D494D4EBD");
