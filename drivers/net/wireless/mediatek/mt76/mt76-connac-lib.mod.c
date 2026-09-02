#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(mt76_connac_mcu_start_firmware, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_patch_sem_ctrl, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_start_patch, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_init_download, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_channel_domain, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_mac_enable, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_vif_ps, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_rts_thresh, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_beacon_loss_iter, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_add_nested_tlv, "_gpl", "");
KSYMTAB_FUNC(__mt76_connac_mcu_alloc_sta_req, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_alloc_wtbl_req, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_bss_omac_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_basic_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_uapsd, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_hdr_trans_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_update_hdr_trans, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_update_hdr_trans, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_generic_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_he_tlv_v2, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_phy_mode_v2, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_smps_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_ht_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_cmd, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_wtbl_ba_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_uni_add_dev, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_ba_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_wed_update, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sta_ba, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_phy_mode, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_phy_mode_ext, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_he_phy_cap, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_eht_phy_cap, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_uni_set_chctx, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_uni_add_bss, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_build_rnr_scan_param, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_hw_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_cancel_hw_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sched_scan_req, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_sched_scan_enable, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_chip_config, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_deep_sleep, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_sta_state_dp, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_coredump_event, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_get_ch_power, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_rate_txpower, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_update_arp_filter, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_p2p_oppps, "_gpl", "");
KSYMTAB_DATA(mt76_connac_wowlan_support, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_update_gtk_rekey, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_gtk_rekey, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_suspend_mode, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_wow_ctrl, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_hif_suspend, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_suspend_iter, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_reg_rr, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_reg_wr, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_add_key, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_bss_ext_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_bss_basic_tlv, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_set_pm, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_restart, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_del_wtbl_all, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_mcu_rdd_cmd, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_load_ram, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_load_patch, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mcu_fill_message, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_gen_ppe_thresh, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_pm_wake, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_power_save_sched, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_free_pending_tx_skbs, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_pm_queue_skb, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_pm_dequeue_skbs, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_tx_complete_skb, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_write_hw_txp, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_txp_skb_unmap, "_gpl", "");
KSYMTAB_FUNC(mt76_connac_init_tx_queues, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_tx_rate_val, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_write_txwi, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_fill_txs, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_add_txs_skb, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_decode_he_radiotap, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_reverse_frag0_hdr_trans, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_mac_fill_rx_rate, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_tx_check_aggr, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_txwi_free, "_gpl", "");
KSYMTAB_FUNC(mt76_connac2_tx_token_put, "_gpl", "");
KSYMTAB_FUNC(mt76_connac3_mac_decode_he_radiotap, "_gpl", "");
KSYMTAB_FUNC(mt76_connac3_mac_decode_eht_radiotap, "_gpl", "");

SYMBOL_CRC(mt76_connac_mcu_start_firmware, 0xaa1ff13c, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_patch_sem_ctrl, 0x1df0a9d1, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_start_patch, 0xeba738b4, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_init_download, 0xf8f4eba3, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_channel_domain, 0xa9b8a123, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_mac_enable, 0x28e32ce8, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_vif_ps, 0x31b110a4, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_rts_thresh, 0x1771d4ce, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_beacon_loss_iter, 0xcba4d801, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_add_nested_tlv, 0x4b80fd9e, "_gpl");
SYMBOL_CRC(__mt76_connac_mcu_alloc_sta_req, 0xc81a3b6a, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_alloc_wtbl_req, 0x18fc6555, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_bss_omac_tlv, 0xb3fffb9b, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_basic_tlv, 0xc270b0db, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_uapsd, 0xb0173bdf, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_hdr_trans_tlv, 0x0379f026, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_update_hdr_trans, 0xfdd61080, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_update_hdr_trans, 0x43e62848, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_generic_tlv, 0xfeb33c64, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_he_tlv_v2, 0x4e3997fe, "_gpl");
SYMBOL_CRC(mt76_connac_get_phy_mode_v2, 0xfb034c48, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_tlv, 0x87dace8b, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_smps_tlv, 0x640bb386, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_ht_tlv, 0x839de5a5, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_cmd, 0x3b85aae8, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_wtbl_ba_tlv, 0x1e4543a7, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_uni_add_dev, 0x61aefcc4, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_ba_tlv, 0x14f52c7e, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_wed_update, 0xe7dcfff9, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sta_ba, 0x498177c2, "_gpl");
SYMBOL_CRC(mt76_connac_get_phy_mode, 0x683ba161, "_gpl");
SYMBOL_CRC(mt76_connac_get_phy_mode_ext, 0x842e88fb, "_gpl");
SYMBOL_CRC(mt76_connac_get_he_phy_cap, 0x3a0b0c8a, "_gpl");
SYMBOL_CRC(mt76_connac_get_eht_phy_cap, 0x3d579770, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_uni_set_chctx, 0x4e888c85, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_uni_add_bss, 0xd85b6d54, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_build_rnr_scan_param, 0x874549d1, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_hw_scan, 0x2c39e9ed, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_cancel_hw_scan, 0x3b6a5467, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sched_scan_req, 0x5c557e2c, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_sched_scan_enable, 0xf2343e5d, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_chip_config, 0x11285ed5, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_deep_sleep, 0xfb1e8334, "_gpl");
SYMBOL_CRC(mt76_connac_sta_state_dp, 0x842a215a, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_coredump_event, 0x7b495e01, "_gpl");
SYMBOL_CRC(mt76_connac_get_ch_power, 0xa9e82428, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_rate_txpower, 0x47a9eeff, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_update_arp_filter, 0xffab3329, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_p2p_oppps, 0xfe370e3e, "_gpl");
SYMBOL_CRC(mt76_connac_wowlan_support, 0x908ca40c, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_update_gtk_rekey, 0x58c5163f, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_gtk_rekey, 0xa323266c, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_suspend_mode, 0x199e9f1d, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_wow_ctrl, 0xd2d3966d, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_hif_suspend, 0x1ae6727b, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_suspend_iter, 0x217aad35, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_reg_rr, 0x12286b8f, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_reg_wr, 0x1d6cb078, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_add_key, 0xe57fd77e, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_bss_ext_tlv, 0xea03b0c3, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_bss_basic_tlv, 0xac36ede6, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_set_pm, 0xa23852dd, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_restart, 0x9ec33265, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_del_wtbl_all, 0xd44dae3a, "_gpl");
SYMBOL_CRC(mt76_connac_mcu_rdd_cmd, 0x3dd06d37, "_gpl");
SYMBOL_CRC(mt76_connac2_load_ram, 0x609e2224, "_gpl");
SYMBOL_CRC(mt76_connac2_load_patch, 0x3bc40c6b, "_gpl");
SYMBOL_CRC(mt76_connac2_mcu_fill_message, 0x88855e97, "_gpl");
SYMBOL_CRC(mt76_connac_gen_ppe_thresh, 0x42128a8c, "_gpl");
SYMBOL_CRC(mt76_connac_pm_wake, 0x44a86b7a, "_gpl");
SYMBOL_CRC(mt76_connac_power_save_sched, 0x339e8461, "_gpl");
SYMBOL_CRC(mt76_connac_free_pending_tx_skbs, 0xaa6381ab, "_gpl");
SYMBOL_CRC(mt76_connac_pm_queue_skb, 0x39b76a42, "_gpl");
SYMBOL_CRC(mt76_connac_pm_dequeue_skbs, 0xc9272f42, "_gpl");
SYMBOL_CRC(mt76_connac_tx_complete_skb, 0x0a316f39, "_gpl");
SYMBOL_CRC(mt76_connac_write_hw_txp, 0xddaba626, "_gpl");
SYMBOL_CRC(mt76_connac_txp_skb_unmap, 0x1d895b0e, "_gpl");
SYMBOL_CRC(mt76_connac_init_tx_queues, 0x62c839c1, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_tx_rate_val, 0xa93c60fa, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_write_txwi, 0xe4027318, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_fill_txs, 0x61af01bf, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_add_txs_skb, 0xdbcbd166, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_decode_he_radiotap, 0xde5539a3, "_gpl");
SYMBOL_CRC(mt76_connac2_reverse_frag0_hdr_trans, 0xb5ccb1f2, "_gpl");
SYMBOL_CRC(mt76_connac2_mac_fill_rx_rate, 0x15f57bbe, "_gpl");
SYMBOL_CRC(mt76_connac2_tx_check_aggr, 0xed8057fb, "_gpl");
SYMBOL_CRC(mt76_connac2_txwi_free, 0x56fa17f5, "_gpl");
SYMBOL_CRC(mt76_connac2_tx_token_put, 0x8b13b7fa, "_gpl");
SYMBOL_CRC(mt76_connac3_mac_decode_he_radiotap, 0xd28025be, "_gpl");
SYMBOL_CRC(mt76_connac3_mac_decode_eht_radiotap, 0x99c4d0d9, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb9004599, "mt76_tx_status_unlock" },
	{ 0xc6d09aa9, "release_firmware" },
	{ 0x14d6dbbb, "devm_kmalloc" },
	{ 0x34446683, "skb_put" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x57674fd7, "__sw_hweight16" },
	{ 0x3ceb3c58, "consume_skb" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x35d9e144, "mt76_tx_status_skb_get" },
	{ 0x2f40ac81, "mt76_tx_status_lock" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xa585e820, "dma_unmap_page_attrs" },
	{ 0x74c134b9, "__sw_hweight32" },
	{ 0x9b9d415f, "request_firmware" },
	{ 0xa84be08c, "mt76_get_rate" },
	{ 0x5c6f2066, "ieee80211_start_tx_ba_session" },
	{ 0x4829a47e, "memcpy" },
	{ 0x21bd74d7, "ieee80211_beacon_loss" },
	{ 0x2c05b8db, "__mt76_tx_complete_skb" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x4d17e172, "ieee80211_get_hdrlen_from_skb" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0xf7a4af66, "ieee80211_find_sta" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0x59b494b8, "mt76_tx" },
	{ 0x4e8fb31e, "mt76_wcid_add_poll" },
	{ 0xc57c48a3, "idr_get_next" },
	{ 0x09a5b50c, "_dev_info" },
	{ 0x476b165a, "sized_strscpy" },
	{ 0x4343647b, "mt76_init_queue" },
	{ 0x7c250c78, "mt76_tx_status_skb_done" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x2c689d7c, "devm_kfree" },
	{ 0xbb260be5, "_dev_err" },
	{ 0x2de421d7, "skb_pull" },
	{ 0xd7d13ca0, "mt76_mcu_skb_send_and_get_msg" },
	{ 0xdc3fcbc9, "__sw_hweight8" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0x285e9723, "skb_push" },
	{ 0x3f336bf1, "ieee80211_wake_queues" },
	{ 0x6f9dde3c, "mt76_mcu_send_and_get_msg" },
	{ 0x4aa15311, "ieee80211_iter_keys_rcu" },
	{ 0xd95f067b, "__mt76_mcu_msg_alloc" },
	{ 0x8e17b3ae, "idr_destroy" },
	{ 0x3dad9978, "cancel_delayed_work" },
	{ 0x6bedf402, "ieee80211_freq_khz_to_channel" },
	{ 0xdcb764ad, "memset" },
	{ 0x69b18f43, "rfc1042_header" },
	{ 0x19c55e0e, "__mt76_mcu_send_firmware" },
	{ 0xd2d233d7, "ieee80211_stop_queues" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x17f568e9, "mt76_rates" },
	{ 0x7516f060, "mt76_get_rate_power_limits" },
	{ 0x34e4c5d2, "ieee80211_scan_completed" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x522f8bd7, "param_ops_bool" },
	{ 0x56470118, "__warn_printk" },
	{ 0x1219d2f6, "mt76_get_sar_power" },
	{ 0x0c3690fc, "_raw_spin_lock_bh" },
	{ 0xef5bf6d1, "mt76_put_txwi" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0x1879fcbd, "bridge_tunnel_header" },
	{ 0xe141fce0, "dev_kfree_skb_any_reason" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mt76,mac80211,cfg80211");


MODULE_INFO(srcversion, "B21365F6DAC07DDE3DE93A9");
