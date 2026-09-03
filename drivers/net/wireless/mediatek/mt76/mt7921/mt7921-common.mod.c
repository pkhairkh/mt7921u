#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(mt7921_rx_check, "_gpl", "");
KSYMTAB_FUNC(mt7921_queue_rx_skb, "_gpl", "");
KSYMTAB_FUNC(mt7921_usb_sdio_tx_prepare_skb, "_gpl", "");
KSYMTAB_FUNC(mt7921_usb_sdio_tx_complete_skb, "_gpl", "");
KSYMTAB_FUNC(mt7921_usb_sdio_tx_status_data, "_gpl", "");
KSYMTAB_FUNC(mt7921_mcu_parse_response, "_gpl", "");
KSYMTAB_FUNC(mt7921_run_firmware, "_gpl", "");
KSYMTAB_FUNC(mt7921_mcu_radio_led_ctrl, "_gpl", "");
KSYMTAB_FUNC(mt7921_mcu_set_eeprom, "_gpl", "");
KSYMTAB_FUNC(__mt7921_start, "_gpl", "");
KSYMTAB_FUNC(mt7921_roc_abort_sync, "_gpl", "");
KSYMTAB_FUNC(mt7921_set_channel, "_gpl", "");
KSYMTAB_FUNC(mt7921_mac_sta_add, "_gpl", "");
KSYMTAB_FUNC(mt7921_mac_sta_event, "_gpl", "");
KSYMTAB_FUNC(mt7921_mac_sta_remove, "_gpl", "");
KSYMTAB_DATA(mt7921_ops, "_gpl", "");
KSYMTAB_FUNC(mt7921_regd_update, "_gpl", "");
KSYMTAB_FUNC(mt7921_mac_init, "_gpl", "");
KSYMTAB_FUNC(mt7921_register_device, "_gpl", "");
KSYMTAB_FUNC(mt7921_twt_teardown_flow, "_gpl", "");
KSYMTAB_FUNC(mt7921_csi_cleanup, "_gpl", "");
KSYMTAB_FUNC(mt7921_test_trigger_debugfs_init, "_gpl", "");
KSYMTAB_FUNC(mt7921_test_trigger_debugfs_remove, "_gpl", "");
KSYMTAB_FUNC(mt7921_acs_cleanup, "_gpl", "");

SYMBOL_CRC(mt7921_rx_check, 0x74ca23e3, "_gpl");
SYMBOL_CRC(mt7921_queue_rx_skb, 0x9c031d81, "_gpl");
SYMBOL_CRC(mt7921_usb_sdio_tx_prepare_skb, 0x499cb481, "_gpl");
SYMBOL_CRC(mt7921_usb_sdio_tx_complete_skb, 0x8093fe45, "_gpl");
SYMBOL_CRC(mt7921_usb_sdio_tx_status_data, 0x2c75e8f4, "_gpl");
SYMBOL_CRC(mt7921_mcu_parse_response, 0xa594430f, "_gpl");
SYMBOL_CRC(mt7921_run_firmware, 0x18fa4011, "_gpl");
SYMBOL_CRC(mt7921_mcu_radio_led_ctrl, 0x432cacae, "_gpl");
SYMBOL_CRC(mt7921_mcu_set_eeprom, 0xb54f5ee5, "_gpl");
SYMBOL_CRC(__mt7921_start, 0xaa221a96, "_gpl");
SYMBOL_CRC(mt7921_roc_abort_sync, 0x7f256a69, "_gpl");
SYMBOL_CRC(mt7921_set_channel, 0xcd0f0050, "_gpl");
SYMBOL_CRC(mt7921_mac_sta_add, 0xa3fd1836, "_gpl");
SYMBOL_CRC(mt7921_mac_sta_event, 0x6a201a27, "_gpl");
SYMBOL_CRC(mt7921_mac_sta_remove, 0xc0524be4, "_gpl");
SYMBOL_CRC(mt7921_ops, 0x12258fbf, "_gpl");
SYMBOL_CRC(mt7921_regd_update, 0x8198cd37, "_gpl");
SYMBOL_CRC(mt7921_mac_init, 0xf456a9e2, "_gpl");
SYMBOL_CRC(mt7921_register_device, 0xa26241d0, "_gpl");
SYMBOL_CRC(mt7921_twt_teardown_flow, 0xa167d57d, "_gpl");
SYMBOL_CRC(mt7921_csi_cleanup, 0x2ad5b1a0, "_gpl");
SYMBOL_CRC(mt7921_test_trigger_debugfs_init, 0x0a2b0792, "_gpl");
SYMBOL_CRC(mt7921_test_trigger_debugfs_remove, 0x49acfab8, "_gpl");
SYMBOL_CRC(mt7921_acs_cleanup, 0x6fe3c689, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x58c5163f, "mt76_connac_mcu_update_gtk_rekey" },
	{ 0x47a9eeff, "mt76_connac_mcu_set_rate_txpower" },
	{ 0x34eda94b, "mt76_update_channel" },
	{ 0x8d1b6861, "ieee80211_sta_register_airtime" },
	{ 0xe2e8112a, "mt792x_init_wcid" },
	{ 0xbf77c0f8, "mt76_init_sar_power" },
	{ 0x0a17e1c9, "ieee80211_connection_loss" },
	{ 0xc6d09aa9, "release_firmware" },
	{ 0x21fcf695, "simple_attr_open" },
	{ 0x46123926, "ieee80211_iterate_interfaces" },
	{ 0x8ebb18a9, "mt792x_stop" },
	{ 0x40af4b18, "debugfs_attr_write" },
	{ 0xfdd61080, "mt76_connac_mcu_sta_update_hdr_trans" },
	{ 0xf2343e5d, "mt76_connac_mcu_sched_scan_enable" },
	{ 0xb077983c, "ieee80211_chswitch_done" },
	{ 0x34446683, "skb_put" },
	{ 0x44d9a1b2, "skb_tstamp_tx" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xb8342c83, "__kmalloc_noprof" },
	{ 0x3ceb3c58, "consume_skb" },
	{ 0x5a9f1d63, "memmove" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x89e7ee01, "mt76_find_channel_node" },
	{ 0x4e888c85, "mt76_connac_mcu_uni_set_chctx" },
	{ 0x67c05775, "mt792x_tx_worker" },
	{ 0xb665e965, "mt792x_flush" },
	{ 0xbbfab63c, "mt792x_rx_get_wcid" },
	{ 0x6ab88d9e, "cfg80211_cac_event" },
	{ 0x490c61d5, "skb_dequeue" },
	{ 0x42128a8c, "mt76_connac_gen_ppe_thresh" },
	{ 0x1771d4ce, "mt76_connac_mcu_set_rts_thresh" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xa9b8a123, "mt76_connac_mcu_set_channel_domain" },
	{ 0x61aefcc4, "mt76_connac_mcu_uni_add_dev" },
	{ 0xa9c4d9a6, "mt76_release_buffered_frames" },
	{ 0x9b9d415f, "request_firmware" },
	{ 0x86478d17, "mt76_get_txpower" },
	{ 0xca646b2e, "mt76_tx_status_skb_add" },
	{ 0x44a86b7a, "mt76_connac_pm_wake" },
	{ 0x15f57bbe, "mt76_connac2_mac_fill_rx_rate" },
	{ 0x0df73a34, "mt792x_pm_wake_work" },
	{ 0xfb1e8334, "mt76_connac_mcu_set_deep_sleep" },
	{ 0x823462d4, "mt76_wake_tx_queue" },
	{ 0x4829a47e, "memcpy" },
	{ 0x11285ed5, "mt76_connac_mcu_chip_config" },
	{ 0x220a9892, "mt76_get_antenna" },
	{ 0x037a0cba, "kfree" },
	{ 0x8285663b, "mt792x_set_coverage_class" },
	{ 0xaeb082ad, "_raw_read_unlock_bh" },
	{ 0x2c05b8db, "__mt76_tx_complete_skb" },
	{ 0x8dee722d, "_raw_read_lock_bh" },
	{ 0x28f68296, "seq_lseek" },
	{ 0xc50331e7, "mt76_set_stream_caps" },
	{ 0xf083bc18, "mt792x_mac_assoc_rssi" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0x4d17e172, "ieee80211_get_hdrlen_from_skb" },
	{ 0xe2964344, "__wake_up" },
	{ 0xe731fec5, "mt792x_mac_work" },
	{ 0xffab3329, "mt76_connac_mcu_update_arp_filter" },
	{ 0xed8057fb, "mt76_connac2_tx_check_aggr" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x4fc109ee, "mt76_mcu_rx_event" },
	{ 0x780d0b45, "__traceiter_lp_event" },
	{ 0xb5b3c7b7, "pskb_expand_head" },
	{ 0xd7f84c02, "mt76_wcid_init" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0xaa6381ab, "mt76_connac_free_pending_tx_skbs" },
	{ 0xc0faffc2, "mt792x_sta_statistics" },
	{ 0xa39347ed, "skb_queue_purge_reason" },
	{ 0x339e8461, "mt76_connac_power_save_sched" },
	{ 0x71075ca3, "mt792x_conf_tx" },
	{ 0x6b939a61, "cfg80211_reg_check_beaconing" },
	{ 0xb29f6a64, "___ratelimit" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb8585380, "debugfs_create_devm_seqfile" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0x5e4652a9, "mt792x_get_et_sset_count" },
	{ 0x4e8fb31e, "mt76_wcid_add_poll" },
	{ 0x8d0d7c66, "__cfg80211_alloc_reply_skb" },
	{ 0x28e32ce8, "mt76_connac_mcu_set_mac_enable" },
	{ 0xdf6d3d3d, "mt792x_set_tsf" },
	{ 0xe40b66ef, "mt76_wcid_alloc" },
	{ 0x51a3af57, "mt792x_set_wakeup" },
	{ 0x25e9d0a2, "mt76_wcid_key_setup" },
	{ 0x09a5b50c, "_dev_info" },
	{ 0xaef1cd20, "skb_queue_tail" },
	{ 0xbb624a5a, "ieee80211_stop_tx_ba_cb_irqsafe" },
	{ 0xdb7bce53, "mt76_rx" },
	{ 0x20ab24c4, "mt792x_load_firmware" },
	{ 0x5c557e2c, "mt76_connac_mcu_sched_scan_req" },
	{ 0xabbd2068, "mt792x_get_et_stats" },
	{ 0x56fa17f5, "mt76_connac2_txwi_free" },
	{ 0x793efb94, "__tracepoint_lp_event" },
	{ 0xd85b6d54, "mt76_connac_mcu_uni_add_bss" },
	{ 0xdbcbd166, "mt76_connac2_mac_add_txs_skb" },
	{ 0x7665a95b, "idr_remove" },
	{ 0x7dae9ffa, "wiphy_to_ieee80211_hw" },
	{ 0x07b73ffe, "of_get_child_by_name" },
	{ 0x1d6d44ab, "debugfs_attr_read" },
	{ 0x2c39e9ed, "mt76_connac_mcu_hw_scan" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x56fee637, "ieee80211_queue_delayed_work" },
	{ 0xda404eab, "devm_kmemdup" },
	{ 0xbb260be5, "_dev_err" },
	{ 0xf70e4a4d, "preempt_schedule_notrace" },
	{ 0x2de421d7, "skb_pull" },
	{ 0x69dc5be7, "mt76_register_device" },
	{ 0x3b090827, "mt792x_get_stats" },
	{ 0xd7d13ca0, "mt76_mcu_skb_send_and_get_msg" },
	{ 0x64fb973b, "simple_attr_release" },
	{ 0xdc3fcbc9, "__sw_hweight8" },
	{ 0xd94c4699, "mt792x_mac_set_timeing" },
	{ 0x24d273d1, "add_timer" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0xf061c050, "sk_skb_reason_drop" },
	{ 0xb6c4379f, "system_percpu_wq" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x257e2682, "mt76_find_power_limits_node" },
	{ 0xf1969a8e, "__usecs_to_jiffies" },
	{ 0x542d4434, "ieee80211_remain_on_channel_expired" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x285e9723, "skb_push" },
	{ 0x2781f7e4, "seq_putc" },
	{ 0x6ece03a9, "nla_put" },
	{ 0x3f336bf1, "ieee80211_wake_queues" },
	{ 0x6f9dde3c, "mt76_mcu_send_and_get_msg" },
	{ 0x5ee79636, "ieee80211_beacon_get_template" },
	{ 0xb73d04c5, "mt792x_pm_stats" },
	{ 0x921b07b1, "__cpu_online_mask" },
	{ 0x3c2470aa, "mt76_eeprom_override" },
	{ 0xfb27b7c6, "dev_coredumpv" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xd95f067b, "__mt76_mcu_msg_alloc" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0x95d6c637, "mt792x_csa_timer" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xa0c06b37, "mt76_get_survey" },
	{ 0x6bedf402, "ieee80211_freq_khz_to_channel" },
	{ 0x195b70f6, "devm_hwmon_device_register_with_groups" },
	{ 0xdcb764ad, "memset" },
	{ 0x9d55c01b, "_dev_warn" },
	{ 0x242eb1f7, "cfg80211_vendor_cmd_reply" },
	{ 0xcc295656, "cfg80211_chandef_usable" },
	{ 0x0d1d07d0, "mt76_token_release" },
	{ 0x35fd4757, "mt792x_get_tsf" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x0f9d4878, "__pskb_pull_tail" },
	{ 0x5a7b5c38, "mt792x_pm_idle_timeout_set" },
	{ 0x055440de, "mt792x_tx" },
	{ 0xb7ce82de, "mt792x_get_et_strings" },
	{ 0x28e8a217, "mt76_sta_state" },
	{ 0xe7d9c63c, "ieee80211_ready_on_channel" },
	{ 0xd2d233d7, "ieee80211_stop_queues" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x692c5be0, "devm_kasprintf" },
	{ 0xd71b2787, "mt792x_remove_interface" },
	{ 0xb87ea521, "seq_read" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x3ccbba63, "wiphy_rfkill_set_hw_state_reason" },
	{ 0x7dac98f7, "ieee80211_vif_to_wdev" },
	{ 0x17f568e9, "mt76_rates" },
	{ 0x48ba9358, "mt792x_queues_read" },
	{ 0x3b6a5467, "mt76_connac_mcu_cancel_hw_scan" },
	{ 0x19d52ec4, "wiphy_rfkill_start_polling" },
	{ 0x3fd57ff7, "mt792x_queues_acq" },
	{ 0x634f9c63, "ieee80211_sched_scan_results" },
	{ 0x34e4c5d2, "ieee80211_scan_completed" },
	{ 0xb6138c76, "ieee80211_iterate_active_interfaces_atomic" },
	{ 0x6c9e3827, "mt792x_config_mac_addr_list" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x0243621f, "mt76_insert_ccmp_hdr" },
	{ 0x3b85aae8, "mt76_connac_mcu_sta_cmd" },
	{ 0x522f8bd7, "param_ops_bool" },
	{ 0xb5ccb1f2, "mt76_connac2_reverse_frag0_hdr_trans" },
	{ 0x1d02445d, "mt792x_tx_stats_show" },
	{ 0xd97849ea, "mt76_rx_aggr_start" },
	{ 0x190dd4c1, "seq_write" },
	{ 0xb43f9365, "ktime_get" },
	{ 0xa6e36705, "vzalloc_noprof" },
	{ 0x03c12dfe, "cancel_work_sync" },
	{ 0xab1af175, "seq_printf" },
	{ 0xc3a0fb62, "mt792x_pm_idle_timeout_get" },
	{ 0x9c06d4d5, "mt792x_roc_timer" },
	{ 0x7b495e01, "mt76_connac_mcu_coredump_event" },
	{ 0x5584448a, "ieee80211_channel_to_freq_khz" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x0c3690fc, "_raw_spin_lock_bh" },
	{ 0xab4759dd, "mt792x_mac_init_band" },
	{ 0xe9ed88ff, "debugfs_create_file_full" },
	{ 0xd4bf773a, "mt76_skb_adjust_pad" },
	{ 0xa4fa4c79, "ieee80211_cqm_rssi_notify" },
	{ 0x852e7f09, "mt792x_init_wiphy" },
	{ 0x534020f0, "ieee80211_radar_detected" },
	{ 0x842a215a, "mt76_connac_sta_state_dp" },
	{ 0x217aad35, "mt76_connac_mcu_set_suspend_iter" },
	{ 0x09eae07a, "single_release" },
	{ 0xf9ddb5d9, "timer_init_key" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x60467150, "mt76_queues_read" },
	{ 0x66c1c31b, "mt76_sta_pre_rcu_remove" },
	{ 0x521878e4, "mt76_register_debugfs_fops" },
	{ 0xd62f08ef, "mt792x_reset" },
	{ 0x327a9822, "mt76_rx_signal" },
	{ 0xe4027318, "mt76_connac2_mac_write_txwi" },
	{ 0xbe7d03d0, "__mt76_set_tx_blocked" },
	{ 0x498177c2, "mt76_connac_mcu_sta_ba" },
	{ 0xde5539a3, "mt76_connac2_mac_decode_he_radiotap" },
	{ 0x5589c25e, "ieee80211_queue_work" },
	{ 0x994c8962, "mt792x_mac_reset_counters" },
	{ 0x44ca04e3, "napi_consume_skb" },
	{ 0x497f6506, "__nla_parse" },
	{ 0x21a6fb96, "__mt76_poll" },
	{ 0x489cbaa2, "param_ops_int" },
	{ 0x142db8a5, "mt76_rx_aggr_stop" },
	{ 0x10421460, "single_open" },
	{ 0xe57fd77e, "mt76_connac_mcu_add_key" },
	{ 0xde450faf, "mt792x_unassign_vif_chanctx" },
	{ 0xf1aeee87, "debugfs_create_dir" },
	{ 0x6efbdb8f, "ieee80211_disconnect" },
	{ 0xe4ccb149, "mt792x_pm_power_save_work" },
	{ 0x8557b189, "mt792x_assign_vif_chanctx" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt76-connac-lib,mt76,mac80211,mt792x-lib,cfg80211");


MODULE_INFO(srcversion, "E0F3EB292808561043CC822");
