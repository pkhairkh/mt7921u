#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(mt792x_tx, "_gpl", "");
KSYMTAB_FUNC(mt792x_stop, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_link_bss_remove, "_gpl", "");
KSYMTAB_FUNC(mt792x_remove_interface, "_gpl", "");
KSYMTAB_FUNC(mt792x_conf_tx, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_stats, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_tsf, "_gpl", "");
KSYMTAB_FUNC(mt792x_set_tsf, "_gpl", "");
KSYMTAB_FUNC(mt792x_tx_worker, "_gpl", "");
KSYMTAB_FUNC(mt792x_roc_timer, "_gpl", "");
KSYMTAB_FUNC(mt792x_csa_timer, "_gpl", "");
KSYMTAB_FUNC(mt792x_flush, "_gpl", "");
KSYMTAB_FUNC(__mt792x_assign_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt792x_assign_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt792x_unassign_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt792x_set_wakeup, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_et_strings, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_et_sset_count, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_et_stats, "_gpl", "");
KSYMTAB_FUNC(mt792x_sta_statistics, "_gpl", "");
KSYMTAB_FUNC(mt792x_set_coverage_class, "_gpl", "");
KSYMTAB_FUNC(mt792x_init_wiphy, "_gpl", "");
KSYMTAB_FUNC(mt792x_get_mac80211_ops, "_gpl", "");
KSYMTAB_FUNC(mt792x_init_wcid, "_gpl", "");
KSYMTAB_FUNC(mt792x_mcu_drv_pmctrl, "_gpl", "");
KSYMTAB_FUNC(mt792x_mcu_fw_pmctrl, "_gpl", "");
KSYMTAB_FUNC(__mt792xe_mcu_drv_pmctrl, "_gpl", "");
KSYMTAB_FUNC(mt792xe_mcu_drv_pmctrl, "_gpl", "");
KSYMTAB_FUNC(mt792xe_mcu_fw_pmctrl, "_gpl", "");
KSYMTAB_FUNC(mt792x_load_firmware, "_gpl", "");
KSYMTAB_FUNC(mt792x_config_mac_addr_list, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_work, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_set_timeing, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_update_mib_stats, "_gpl", "");
KSYMTAB_FUNC(mt792x_rx_get_wcid, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_assoc_rssi, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_reset_counters, "_gpl", "");
KSYMTAB_FUNC(mt792x_update_channel, "_gpl", "");
KSYMTAB_FUNC(mt792x_reset, "_gpl", "");
KSYMTAB_FUNC(mt792x_mac_init_band, "_gpl", "");
KSYMTAB_FUNC(mt792x_pm_wake_work, "_gpl", "");
KSYMTAB_FUNC(mt792x_pm_power_save_work, "_gpl", "");
KSYMTAB_DATA(__tracepoint_lp_event, "_gpl", "");
KSYMTAB_FUNC(__traceiter_lp_event, "_gpl", "");
KSYMTAB_DATA(__SCK__tp_func_lp_event, "_gpl", "");
KSYMTAB_FUNC(mt792x_tx_stats_show, "_gpl", "");
KSYMTAB_FUNC(mt792x_queues_acq, "_gpl", "");
KSYMTAB_FUNC(mt792x_queues_read, "_gpl", "");
KSYMTAB_FUNC(mt792x_pm_stats, "_gpl", "");
KSYMTAB_FUNC(mt792x_pm_idle_timeout_set, "_gpl", "");
KSYMTAB_FUNC(mt792x_pm_idle_timeout_get, "_gpl", "");
KSYMTAB_FUNC(mt792x_irq_handler, "_gpl", "");
KSYMTAB_FUNC(mt792x_irq_tasklet, "_gpl", "");
KSYMTAB_FUNC(mt792x_rx_poll_complete, "_gpl", "");
KSYMTAB_FUNC(mt792x_dma_enable, "_gpl", "");
KSYMTAB_FUNC(mt792x_wpdma_reset, "_gpl", "");
KSYMTAB_FUNC(mt792x_wpdma_reinit_cond, "_gpl", "");
KSYMTAB_FUNC(mt792x_dma_disable, "_gpl", "");
KSYMTAB_FUNC(mt792x_dma_cleanup, "_gpl", "");
KSYMTAB_FUNC(mt792x_poll_tx, "_gpl", "");
KSYMTAB_FUNC(mt792x_poll_rx, "_gpl", "");
KSYMTAB_FUNC(mt792x_wfsys_reset, "_gpl", "");

SYMBOL_CRC(mt792x_tx, 0x055440de, "_gpl");
SYMBOL_CRC(mt792x_stop, 0x8ebb18a9, "_gpl");
SYMBOL_CRC(mt792x_mac_link_bss_remove, 0xc7152fdf, "_gpl");
SYMBOL_CRC(mt792x_remove_interface, 0xd71b2787, "_gpl");
SYMBOL_CRC(mt792x_conf_tx, 0x71075ca3, "_gpl");
SYMBOL_CRC(mt792x_get_stats, 0x3b090827, "_gpl");
SYMBOL_CRC(mt792x_get_tsf, 0x35fd4757, "_gpl");
SYMBOL_CRC(mt792x_set_tsf, 0xdf6d3d3d, "_gpl");
SYMBOL_CRC(mt792x_tx_worker, 0x67c05775, "_gpl");
SYMBOL_CRC(mt792x_roc_timer, 0x9c06d4d5, "_gpl");
SYMBOL_CRC(mt792x_csa_timer, 0x95d6c637, "_gpl");
SYMBOL_CRC(mt792x_flush, 0xb665e965, "_gpl");
SYMBOL_CRC(__mt792x_assign_vif_chanctx, 0x3696aaa9, "_gpl");
SYMBOL_CRC(mt792x_assign_vif_chanctx, 0x8557b189, "_gpl");
SYMBOL_CRC(mt792x_unassign_vif_chanctx, 0xde450faf, "_gpl");
SYMBOL_CRC(mt792x_set_wakeup, 0x51a3af57, "_gpl");
SYMBOL_CRC(mt792x_get_et_strings, 0xb7ce82de, "_gpl");
SYMBOL_CRC(mt792x_get_et_sset_count, 0x5e4652a9, "_gpl");
SYMBOL_CRC(mt792x_get_et_stats, 0xabbd2068, "_gpl");
SYMBOL_CRC(mt792x_sta_statistics, 0xc0faffc2, "_gpl");
SYMBOL_CRC(mt792x_set_coverage_class, 0x8285663b, "_gpl");
SYMBOL_CRC(mt792x_init_wiphy, 0x852e7f09, "_gpl");
SYMBOL_CRC(mt792x_get_mac80211_ops, 0x3728dcd6, "_gpl");
SYMBOL_CRC(mt792x_init_wcid, 0xe2e8112a, "_gpl");
SYMBOL_CRC(mt792x_mcu_drv_pmctrl, 0xeecd1174, "_gpl");
SYMBOL_CRC(mt792x_mcu_fw_pmctrl, 0x4a677d54, "_gpl");
SYMBOL_CRC(__mt792xe_mcu_drv_pmctrl, 0x0778468a, "_gpl");
SYMBOL_CRC(mt792xe_mcu_drv_pmctrl, 0x5b2b1b4b, "_gpl");
SYMBOL_CRC(mt792xe_mcu_fw_pmctrl, 0x7ca273e9, "_gpl");
SYMBOL_CRC(mt792x_load_firmware, 0x20ab24c4, "_gpl");
SYMBOL_CRC(mt792x_config_mac_addr_list, 0x6c9e3827, "_gpl");
SYMBOL_CRC(mt792x_mac_work, 0xe731fec5, "_gpl");
SYMBOL_CRC(mt792x_mac_set_timeing, 0xd94c4699, "_gpl");
SYMBOL_CRC(mt792x_mac_update_mib_stats, 0x2091f439, "_gpl");
SYMBOL_CRC(mt792x_rx_get_wcid, 0xbbfab63c, "_gpl");
SYMBOL_CRC(mt792x_mac_assoc_rssi, 0xf083bc18, "_gpl");
SYMBOL_CRC(mt792x_mac_reset_counters, 0x994c8962, "_gpl");
SYMBOL_CRC(mt792x_update_channel, 0x4eea5aac, "_gpl");
SYMBOL_CRC(mt792x_reset, 0xd62f08ef, "_gpl");
SYMBOL_CRC(mt792x_mac_init_band, 0xab4759dd, "_gpl");
SYMBOL_CRC(mt792x_pm_wake_work, 0x0df73a34, "_gpl");
SYMBOL_CRC(mt792x_pm_power_save_work, 0xe4ccb149, "_gpl");
SYMBOL_CRC(__tracepoint_lp_event, 0x793efb94, "_gpl");
SYMBOL_CRC(__traceiter_lp_event, 0x780d0b45, "_gpl");
SYMBOL_CRC(__SCK__tp_func_lp_event, 0xb9749f8d, "_gpl");
SYMBOL_CRC(mt792x_tx_stats_show, 0x1d02445d, "_gpl");
SYMBOL_CRC(mt792x_queues_acq, 0x3fd57ff7, "_gpl");
SYMBOL_CRC(mt792x_queues_read, 0x48ba9358, "_gpl");
SYMBOL_CRC(mt792x_pm_stats, 0xb73d04c5, "_gpl");
SYMBOL_CRC(mt792x_pm_idle_timeout_set, 0x5a7b5c38, "_gpl");
SYMBOL_CRC(mt792x_pm_idle_timeout_get, 0xc3a0fb62, "_gpl");
SYMBOL_CRC(mt792x_irq_handler, 0xbd22e98c, "_gpl");
SYMBOL_CRC(mt792x_irq_tasklet, 0x8e471e29, "_gpl");
SYMBOL_CRC(mt792x_rx_poll_complete, 0x3efa1d9c, "_gpl");
SYMBOL_CRC(mt792x_dma_enable, 0x9533e0eb, "_gpl");
SYMBOL_CRC(mt792x_wpdma_reset, 0xe2d0fcc1, "_gpl");
SYMBOL_CRC(mt792x_wpdma_reinit_cond, 0x01907c7a, "_gpl");
SYMBOL_CRC(mt792x_dma_disable, 0x275c989c, "_gpl");
SYMBOL_CRC(mt792x_dma_cleanup, 0x64373fbc, "_gpl");
SYMBOL_CRC(mt792x_poll_tx, 0x8f91d0a7, "_gpl");
SYMBOL_CRC(mt792x_poll_rx, 0x57af97b1, "_gpl");
SYMBOL_CRC(mt792x_wfsys_reset, 0xf253f316, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9ec33265, "mt76_connac_mcu_restart" },
	{ 0xb7ee5b62, "mt76_dma_rx_poll" },
	{ 0xc6d09aa9, "release_firmware" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xc1a9d628, "trace_raw_output_prep" },
	{ 0xc678b384, "__trace_trigger_soft_disabled" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x61aefcc4, "mt76_connac_mcu_uni_add_dev" },
	{ 0x9b9d415f, "request_firmware" },
	{ 0xf3f1225e, "trace_event_printf" },
	{ 0x315a15cd, "ieee80211_iterate_stations_atomic" },
	{ 0x44a86b7a, "mt76_connac_pm_wake" },
	{ 0x7405cced, "__traceiter_dev_irq" },
	{ 0x0ea3ee0e, "trace_event_raw_init" },
	{ 0x4829a47e, "memcpy" },
	{ 0x6dbbd66c, "mt76_has_tx_pending" },
	{ 0x1333d082, "ieee80211_emulate_change_chanctx" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x8441b125, "bpf_trace_run2" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe2964344, "__wake_up" },
	{ 0x501a1ae6, "mt76_dma_cleanup" },
	{ 0x39b76a42, "mt76_connac_pm_queue_skb" },
	{ 0x7dc3d53e, "mt76_tx_status_check" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0xaa6381ab, "mt76_connac_free_pending_tx_skbs" },
	{ 0x339e8461, "mt76_connac_power_save_sched" },
	{ 0x8f8269e0, "trace_event_buffer_commit" },
	{ 0x3bf47833, "napi_complete_done" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0x59b494b8, "mt76_tx" },
	{ 0xd83dea23, "__napi_schedule" },
	{ 0x28e32ce8, "mt76_connac_mcu_set_mac_enable" },
	{ 0xe40b66ef, "mt76_wcid_alloc" },
	{ 0x476b165a, "sized_strscpy" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x56fee637, "ieee80211_queue_delayed_work" },
	{ 0xda404eab, "devm_kmemdup" },
	{ 0xb6e16cdb, "perf_trace_buf_alloc" },
	{ 0x0d9609a4, "perf_trace_run_bpf_submit" },
	{ 0xbb260be5, "_dev_err" },
	{ 0xf70e4a4d, "preempt_schedule_notrace" },
	{ 0x609e2224, "mt76_connac2_load_ram" },
	{ 0xdc3fcbc9, "__sw_hweight8" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x2781f7e4, "seq_putc" },
	{ 0x8e0afadb, "trace_event_reg" },
	{ 0x364c23ad, "mutex_is_locked" },
	{ 0x0b9ee70f, "mt76_update_survey" },
	{ 0x9d2ab8ac, "__tasklet_schedule" },
	{ 0x3f336bf1, "ieee80211_wake_queues" },
	{ 0x921b07b1, "__cpu_online_mask" },
	{ 0x3c3fce39, "__local_bh_enable_ip" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0x908ca40c, "mt76_connac_wowlan_support" },
	{ 0x13bb5a4d, "mt76_set_irq_mask" },
	{ 0xc9272f42, "mt76_connac_pm_dequeue_skbs" },
	{ 0x3dad9978, "cancel_delayed_work" },
	{ 0xdcb764ad, "memset" },
	{ 0x9d55c01b, "_dev_warn" },
	{ 0xeeee3281, "ieee80211_emulate_add_chanctx" },
	{ 0x240e422b, "ieee80211_emulate_switch_vif_chanctx" },
	{ 0xa4d7d80e, "mt76_ethtool_page_pool_stats" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x70577545, "__tracepoint_dev_irq" },
	{ 0xd111de91, "trace_event_buffer_reserve" },
	{ 0xb6138c76, "ieee80211_iterate_active_interfaces_atomic" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x95ec24ac, "mt76_txq_schedule_all" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x3bc40c6b, "mt76_connac2_load_patch" },
	{ 0x190dd4c1, "seq_write" },
	{ 0x6497453a, "____mt76_poll_msec" },
	{ 0x03c12dfe, "cancel_work_sync" },
	{ 0xab1af175, "seq_printf" },
	{ 0x0c3690fc, "_raw_spin_lock_bh" },
	{ 0xe01400b7, "mt76_wcid_cleanup" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x5589c25e, "ieee80211_queue_work" },
	{ 0x99e4627d, "napi_schedule_prep" },
	{ 0x7a1929c1, "trace_handle_return" },
	{ 0xda81578b, "ieee80211_emulate_remove_chanctx" },
	{ 0xab9d88b8, "mt76_ethtool_worker" },
	{ 0xf9a482f9, "msleep" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt76-connac-lib,mt76,mac80211");


MODULE_INFO(srcversion, "7CC1BA1134276D5E12C34C3");
