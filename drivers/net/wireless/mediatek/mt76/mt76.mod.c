#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

KSYMTAB_FUNC(mt76_set_irq_mask, "_gpl", "");
KSYMTAB_FUNC(mt76_mmio_init, "_gpl", "");
KSYMTAB_FUNC(__mt76_poll, "_gpl", "");
KSYMTAB_FUNC(____mt76_poll_msec, "_gpl", "");
KSYMTAB_FUNC(mt76_wcid_alloc, "_gpl", "");
KSYMTAB_FUNC(mt76_get_min_avg_rssi, "_gpl", "");
KSYMTAB_FUNC(__mt76_worker_fn, "_gpl", "");
KSYMTAB_DATA(__tracepoint_mac_txdone, "_gpl", "");
KSYMTAB_FUNC(__traceiter_mac_txdone, "_gpl", "");
KSYMTAB_DATA(__SCK__tp_func_mac_txdone, "_gpl", "");
KSYMTAB_DATA(__tracepoint_dev_irq, "_gpl", "");
KSYMTAB_FUNC(__traceiter_dev_irq, "_gpl", "");
KSYMTAB_DATA(__SCK__tp_func_dev_irq, "_gpl", "");
KSYMTAB_FUNC(mt76_get_rxwi, "_gpl", "");
KSYMTAB_FUNC(mt76_put_txwi, "_gpl", "");
KSYMTAB_FUNC(mt76_put_rxwi, "_gpl", "");
KSYMTAB_FUNC(mt76_free_pending_rxwi, "_gpl", "");
KSYMTAB_FUNC(mt76_dma_rx_poll, "_gpl", "");
KSYMTAB_FUNC(mt76_dma_attach, "_gpl", "");
KSYMTAB_FUNC(mt76_dma_cleanup, "_gpl", "");
KSYMTAB_DATA(mt76_rates, "_gpl", "");
KSYMTAB_FUNC(mt76_set_stream_caps, "_gpl", "");
KSYMTAB_FUNC(mt76_alloc_radio_phy, "_gpl", "");
KSYMTAB_FUNC(mt76_alloc_phy, "_gpl", "");
KSYMTAB_FUNC(mt76_register_phy, "_gpl", "");
KSYMTAB_FUNC(mt76_unregister_phy, "_gpl", "");
KSYMTAB_FUNC(mt76_create_page_pool, "_gpl", "");
KSYMTAB_FUNC(mt76_alloc_device, "_gpl", "");
KSYMTAB_FUNC(mt76_register_device, "_gpl", "");
KSYMTAB_FUNC(mt76_unregister_device, "_gpl", "");
KSYMTAB_FUNC(mt76_free_device, "_gpl", "");
KSYMTAB_FUNC(mt76_reset_device, "_gpl", "");
KSYMTAB_FUNC(mt76_vif_phy, "_gpl", "");
KSYMTAB_FUNC(mt76_rx, "_gpl", "");
KSYMTAB_FUNC(mt76_has_tx_pending, "_gpl", "");
KSYMTAB_FUNC(mt76_update_survey_active_time, "_gpl", "");
KSYMTAB_FUNC(mt76_update_survey, "_gpl", "");
KSYMTAB_FUNC(mt76_update_channel, "_gpl", "");
KSYMTAB_FUNC(mt76_get_survey, "_gpl", "");
KSYMTAB_FUNC(mt76_wcid_key_setup, "", "");
KSYMTAB_FUNC(mt76_rx_signal, "", "");
KSYMTAB_FUNC(mt76_rx_poll_complete, "_gpl", "");
KSYMTAB_FUNC(__mt76_sta_remove, "_gpl", "");
KSYMTAB_FUNC(mt76_sta_state, "_gpl", "");
KSYMTAB_FUNC(mt76_sta_pre_rcu_remove, "_gpl", "");
KSYMTAB_FUNC(mt76_wcid_init, "_gpl", "");
KSYMTAB_FUNC(mt76_wcid_cleanup, "_gpl", "");
KSYMTAB_FUNC(mt76_wcid_add_poll, "_gpl", "");
KSYMTAB_FUNC(mt76_get_power_bound, "_gpl", "");
KSYMTAB_FUNC(mt76_get_txpower, "_gpl", "");
KSYMTAB_FUNC(mt76_init_sar_power, "_gpl", "");
KSYMTAB_FUNC(mt76_get_sar_power, "_gpl", "");
KSYMTAB_FUNC(mt76_csa_finish, "_gpl", "");
KSYMTAB_FUNC(mt76_csa_check, "_gpl", "");
KSYMTAB_FUNC(mt76_set_tim, "_gpl", "");
KSYMTAB_FUNC(mt76_insert_ccmp_hdr, "_gpl", "");
KSYMTAB_FUNC(mt76_get_rate, "_gpl", "");
KSYMTAB_FUNC(mt76_sw_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_sw_scan_complete, "_gpl", "");
KSYMTAB_FUNC(mt76_get_antenna, "_gpl", "");
KSYMTAB_FUNC(mt76_init_queue, "_gpl", "");
KSYMTAB_FUNC(mt76_ethtool_worker, "_gpl", "");
KSYMTAB_FUNC(mt76_ethtool_page_pool_stats, "_gpl", "");
KSYMTAB_FUNC(mt76_phy_dfs_state, "_gpl", "");
KSYMTAB_FUNC(mt76_vif_cleanup, "_gpl", "");
KSYMTAB_FUNC(mt76_select_links, "_gpl", "");
KSYMTAB_FUNC(mt76_offchannel_notify, "_gpl", "");
KSYMTAB_FUNC(mt76_rx_beacon, "_gpl", "");
KSYMTAB_FUNC(mt76_beacon_mon_check, "_gpl", "");
KSYMTAB_FUNC(mt76_queues_read, "_gpl", "");
KSYMTAB_FUNC(mt76_seq_puts_array, "_gpl", "");
KSYMTAB_FUNC(mt76_register_debugfs_fops, "_gpl", "");
KSYMTAB_FUNC(mt76_get_of_data_from_mtd, "_gpl", "");
KSYMTAB_FUNC(mt76_get_of_data_from_nvmem, "_gpl", "");
KSYMTAB_FUNC(mt76_eeprom_override, "_gpl", "");
KSYMTAB_FUNC(mt76_find_power_limits_node, "_gpl", "");
KSYMTAB_FUNC(mt76_find_channel_node, "_gpl", "");
KSYMTAB_FUNC(mt76_get_rate_power_limits, "_gpl", "");
KSYMTAB_FUNC(mt76_eeprom_init, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_check_agg_ssn, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_lock, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_unlock, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_skb_done, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_skb_add, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_skb_get, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_status_check, "_gpl", "");
KSYMTAB_FUNC(__mt76_tx_complete_skb, "_gpl", "");
KSYMTAB_FUNC(mt76_tx, "_gpl", "");
KSYMTAB_FUNC(mt76_release_buffered_frames, "_gpl", "");
KSYMTAB_FUNC(mt76_txq_schedule, "_gpl", "");
KSYMTAB_FUNC(mt76_txq_schedule_all, "_gpl", "");
KSYMTAB_FUNC(mt76_tx_worker_run, "_gpl", "");
KSYMTAB_FUNC(mt76_stop_tx_queues, "_gpl", "");
KSYMTAB_FUNC(mt76_wake_tx_queue, "_gpl", "");
KSYMTAB_FUNC(mt76_ac_to_hwq, "_gpl", "");
KSYMTAB_FUNC(mt76_skb_adjust_pad, "_gpl", "");
KSYMTAB_FUNC(mt76_queue_tx_complete, "_gpl", "");
KSYMTAB_FUNC(__mt76_set_tx_blocked, "_gpl", "");
KSYMTAB_FUNC(mt76_token_consume, "_gpl", "");
KSYMTAB_FUNC(mt76_rx_token_consume, "_gpl", "");
KSYMTAB_FUNC(mt76_token_release, "_gpl", "");
KSYMTAB_FUNC(mt76_rx_token_release, "_gpl", "");
KSYMTAB_FUNC(mt76_rx_aggr_start, "_gpl", "");
KSYMTAB_FUNC(mt76_rx_aggr_stop, "_gpl", "");
KSYMTAB_FUNC(__mt76_mcu_msg_alloc, "_gpl", "");
KSYMTAB_FUNC(mt76_mcu_get_response, "_gpl", "");
KSYMTAB_FUNC(mt76_mcu_rx_event, "_gpl", "");
KSYMTAB_FUNC(mt76_mcu_send_and_get_msg, "_gpl", "");
KSYMTAB_FUNC(mt76_mcu_skb_send_and_get_msg, "_gpl", "");
KSYMTAB_FUNC(__mt76_mcu_send_firmware, "_gpl", "");
KSYMTAB_FUNC(mt76_wed_release_rx_buf, "_gpl", "");
KSYMTAB_FUNC(mt76_wed_offload_disable, "_gpl", "");
KSYMTAB_FUNC(mt76_wed_reset_complete, "_gpl", "");
KSYMTAB_FUNC(mt76_wed_net_setup_tc, "_gpl", "");
KSYMTAB_FUNC(mt76_wed_dma_reset, "_gpl", "");
KSYMTAB_FUNC(mt76_abort_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_hw_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_cancel_hw_scan, "_gpl", "");
KSYMTAB_FUNC(mt76_add_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_remove_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_change_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_assign_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_unassign_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_switch_vif_chanctx, "_gpl", "");
KSYMTAB_FUNC(mt76_abort_roc, "_gpl", "");
KSYMTAB_FUNC(mt76_remain_on_channel, "_gpl", "");
KSYMTAB_FUNC(mt76_cancel_remain_on_channel, "_gpl", "");
KSYMTAB_FUNC(mt76_pci_disable_aspm, "_gpl", "");
KSYMTAB_FUNC(mt76_pci_aspm_supported, "_gpl", "");

SYMBOL_CRC(mt76_set_irq_mask, 0x13bb5a4d, "_gpl");
SYMBOL_CRC(mt76_mmio_init, 0x3de83039, "_gpl");
SYMBOL_CRC(__mt76_poll, 0x21a6fb96, "_gpl");
SYMBOL_CRC(____mt76_poll_msec, 0x6497453a, "_gpl");
SYMBOL_CRC(mt76_wcid_alloc, 0xe40b66ef, "_gpl");
SYMBOL_CRC(mt76_get_min_avg_rssi, 0x980dea6b, "_gpl");
SYMBOL_CRC(__mt76_worker_fn, 0x1ec57b4f, "_gpl");
SYMBOL_CRC(__tracepoint_mac_txdone, 0x17ee919d, "_gpl");
SYMBOL_CRC(__traceiter_mac_txdone, 0xad331bd7, "_gpl");
SYMBOL_CRC(__SCK__tp_func_mac_txdone, 0xc6315d8e, "_gpl");
SYMBOL_CRC(__tracepoint_dev_irq, 0x70577545, "_gpl");
SYMBOL_CRC(__traceiter_dev_irq, 0x7405cced, "_gpl");
SYMBOL_CRC(__SCK__tp_func_dev_irq, 0x805fc13a, "_gpl");
SYMBOL_CRC(mt76_get_rxwi, 0x9efadf4c, "_gpl");
SYMBOL_CRC(mt76_put_txwi, 0xef5bf6d1, "_gpl");
SYMBOL_CRC(mt76_put_rxwi, 0x39df9eb0, "_gpl");
SYMBOL_CRC(mt76_free_pending_rxwi, 0x18016e5d, "_gpl");
SYMBOL_CRC(mt76_dma_rx_poll, 0xb7ee5b62, "_gpl");
SYMBOL_CRC(mt76_dma_attach, 0x160fe13e, "_gpl");
SYMBOL_CRC(mt76_dma_cleanup, 0x501a1ae6, "_gpl");
SYMBOL_CRC(mt76_rates, 0x17f568e9, "_gpl");
SYMBOL_CRC(mt76_set_stream_caps, 0xc50331e7, "_gpl");
SYMBOL_CRC(mt76_alloc_radio_phy, 0xe6458e1b, "_gpl");
SYMBOL_CRC(mt76_alloc_phy, 0xed30bedc, "_gpl");
SYMBOL_CRC(mt76_register_phy, 0x179b10c2, "_gpl");
SYMBOL_CRC(mt76_unregister_phy, 0xd11716c0, "_gpl");
SYMBOL_CRC(mt76_create_page_pool, 0x0d44deb3, "_gpl");
SYMBOL_CRC(mt76_alloc_device, 0x361f7230, "_gpl");
SYMBOL_CRC(mt76_register_device, 0x69dc5be7, "_gpl");
SYMBOL_CRC(mt76_unregister_device, 0x03c309a7, "_gpl");
SYMBOL_CRC(mt76_free_device, 0xfe6e6c71, "_gpl");
SYMBOL_CRC(mt76_reset_device, 0xf1d80923, "_gpl");
SYMBOL_CRC(mt76_vif_phy, 0x2bed2943, "_gpl");
SYMBOL_CRC(mt76_rx, 0xdb7bce53, "_gpl");
SYMBOL_CRC(mt76_has_tx_pending, 0x6dbbd66c, "_gpl");
SYMBOL_CRC(mt76_update_survey_active_time, 0x7286f96a, "_gpl");
SYMBOL_CRC(mt76_update_survey, 0x0b9ee70f, "_gpl");
SYMBOL_CRC(mt76_update_channel, 0x34eda94b, "_gpl");
SYMBOL_CRC(mt76_get_survey, 0xa0c06b37, "_gpl");
SYMBOL_CRC(mt76_wcid_key_setup, 0x25e9d0a2, "");
SYMBOL_CRC(mt76_rx_signal, 0x327a9822, "");
SYMBOL_CRC(mt76_rx_poll_complete, 0x531351fb, "_gpl");
SYMBOL_CRC(__mt76_sta_remove, 0x7cc4fad9, "_gpl");
SYMBOL_CRC(mt76_sta_state, 0x28e8a217, "_gpl");
SYMBOL_CRC(mt76_sta_pre_rcu_remove, 0x66c1c31b, "_gpl");
SYMBOL_CRC(mt76_wcid_init, 0xd7f84c02, "_gpl");
SYMBOL_CRC(mt76_wcid_cleanup, 0xe01400b7, "_gpl");
SYMBOL_CRC(mt76_wcid_add_poll, 0x4e8fb31e, "_gpl");
SYMBOL_CRC(mt76_get_power_bound, 0x22f44228, "_gpl");
SYMBOL_CRC(mt76_get_txpower, 0x86478d17, "_gpl");
SYMBOL_CRC(mt76_init_sar_power, 0xbf77c0f8, "_gpl");
SYMBOL_CRC(mt76_get_sar_power, 0x1219d2f6, "_gpl");
SYMBOL_CRC(mt76_csa_finish, 0xd2947a79, "_gpl");
SYMBOL_CRC(mt76_csa_check, 0xda5def67, "_gpl");
SYMBOL_CRC(mt76_set_tim, 0x33a084f3, "_gpl");
SYMBOL_CRC(mt76_insert_ccmp_hdr, 0x0243621f, "_gpl");
SYMBOL_CRC(mt76_get_rate, 0xa84be08c, "_gpl");
SYMBOL_CRC(mt76_sw_scan, 0x7cc40855, "_gpl");
SYMBOL_CRC(mt76_sw_scan_complete, 0xdc47a457, "_gpl");
SYMBOL_CRC(mt76_get_antenna, 0x220a9892, "_gpl");
SYMBOL_CRC(mt76_init_queue, 0x4343647b, "_gpl");
SYMBOL_CRC(mt76_ethtool_worker, 0xab9d88b8, "_gpl");
SYMBOL_CRC(mt76_ethtool_page_pool_stats, 0xa4d7d80e, "_gpl");
SYMBOL_CRC(mt76_phy_dfs_state, 0x2b58e5d8, "_gpl");
SYMBOL_CRC(mt76_vif_cleanup, 0x624ff636, "_gpl");
SYMBOL_CRC(mt76_select_links, 0x4578954a, "_gpl");
SYMBOL_CRC(mt76_offchannel_notify, 0x4f9771a9, "_gpl");
SYMBOL_CRC(mt76_rx_beacon, 0x218e5375, "_gpl");
SYMBOL_CRC(mt76_beacon_mon_check, 0x05442c44, "_gpl");
SYMBOL_CRC(mt76_queues_read, 0x60467150, "_gpl");
SYMBOL_CRC(mt76_seq_puts_array, 0x1722ae12, "_gpl");
SYMBOL_CRC(mt76_register_debugfs_fops, 0x521878e4, "_gpl");
SYMBOL_CRC(mt76_get_of_data_from_mtd, 0x38de4286, "_gpl");
SYMBOL_CRC(mt76_get_of_data_from_nvmem, 0x35a25d30, "_gpl");
SYMBOL_CRC(mt76_eeprom_override, 0x3c2470aa, "_gpl");
SYMBOL_CRC(mt76_find_power_limits_node, 0x257e2682, "_gpl");
SYMBOL_CRC(mt76_find_channel_node, 0x89e7ee01, "_gpl");
SYMBOL_CRC(mt76_get_rate_power_limits, 0x7516f060, "_gpl");
SYMBOL_CRC(mt76_eeprom_init, 0x58997385, "_gpl");
SYMBOL_CRC(mt76_tx_check_agg_ssn, 0xaac7a4ea, "_gpl");
SYMBOL_CRC(mt76_tx_status_lock, 0x2f40ac81, "_gpl");
SYMBOL_CRC(mt76_tx_status_unlock, 0xb9004599, "_gpl");
SYMBOL_CRC(mt76_tx_status_skb_done, 0x7c250c78, "_gpl");
SYMBOL_CRC(mt76_tx_status_skb_add, 0xca646b2e, "_gpl");
SYMBOL_CRC(mt76_tx_status_skb_get, 0x35d9e144, "_gpl");
SYMBOL_CRC(mt76_tx_status_check, 0x7dc3d53e, "_gpl");
SYMBOL_CRC(__mt76_tx_complete_skb, 0x2c05b8db, "_gpl");
SYMBOL_CRC(mt76_tx, 0x59b494b8, "_gpl");
SYMBOL_CRC(mt76_release_buffered_frames, 0xa9c4d9a6, "_gpl");
SYMBOL_CRC(mt76_txq_schedule, 0x57cf1739, "_gpl");
SYMBOL_CRC(mt76_txq_schedule_all, 0x95ec24ac, "_gpl");
SYMBOL_CRC(mt76_tx_worker_run, 0x31b5fc7d, "_gpl");
SYMBOL_CRC(mt76_stop_tx_queues, 0x584b1d62, "_gpl");
SYMBOL_CRC(mt76_wake_tx_queue, 0x823462d4, "_gpl");
SYMBOL_CRC(mt76_ac_to_hwq, 0xc6634315, "_gpl");
SYMBOL_CRC(mt76_skb_adjust_pad, 0xd4bf773a, "_gpl");
SYMBOL_CRC(mt76_queue_tx_complete, 0xb446fd58, "_gpl");
SYMBOL_CRC(__mt76_set_tx_blocked, 0xbe7d03d0, "_gpl");
SYMBOL_CRC(mt76_token_consume, 0xd3a93e12, "_gpl");
SYMBOL_CRC(mt76_rx_token_consume, 0x9ed2a3aa, "_gpl");
SYMBOL_CRC(mt76_token_release, 0x0d1d07d0, "_gpl");
SYMBOL_CRC(mt76_rx_token_release, 0xbb1183c6, "_gpl");
SYMBOL_CRC(mt76_rx_aggr_start, 0xd97849ea, "_gpl");
SYMBOL_CRC(mt76_rx_aggr_stop, 0x142db8a5, "_gpl");
SYMBOL_CRC(__mt76_mcu_msg_alloc, 0xd95f067b, "_gpl");
SYMBOL_CRC(mt76_mcu_get_response, 0xbd09a7fd, "_gpl");
SYMBOL_CRC(mt76_mcu_rx_event, 0x4fc109ee, "_gpl");
SYMBOL_CRC(mt76_mcu_send_and_get_msg, 0x6f9dde3c, "_gpl");
SYMBOL_CRC(mt76_mcu_skb_send_and_get_msg, 0xd7d13ca0, "_gpl");
SYMBOL_CRC(__mt76_mcu_send_firmware, 0x19c55e0e, "_gpl");
SYMBOL_CRC(mt76_wed_release_rx_buf, 0xf3cbd71a, "_gpl");
SYMBOL_CRC(mt76_wed_offload_disable, 0x0cbafb12, "_gpl");
SYMBOL_CRC(mt76_wed_reset_complete, 0x25beb920, "_gpl");
SYMBOL_CRC(mt76_wed_net_setup_tc, 0xed3d8935, "_gpl");
SYMBOL_CRC(mt76_wed_dma_reset, 0x8bc7b388, "_gpl");
SYMBOL_CRC(mt76_abort_scan, 0x25c2f224, "_gpl");
SYMBOL_CRC(mt76_hw_scan, 0x73cf6ad2, "_gpl");
SYMBOL_CRC(mt76_cancel_hw_scan, 0xbfcdc43b, "_gpl");
SYMBOL_CRC(mt76_add_chanctx, 0x2a48c449, "_gpl");
SYMBOL_CRC(mt76_remove_chanctx, 0x6811ae0f, "_gpl");
SYMBOL_CRC(mt76_change_chanctx, 0x265c8910, "_gpl");
SYMBOL_CRC(mt76_assign_vif_chanctx, 0x4da6c4ea, "_gpl");
SYMBOL_CRC(mt76_unassign_vif_chanctx, 0x378a70cd, "_gpl");
SYMBOL_CRC(mt76_switch_vif_chanctx, 0x1d32da4f, "_gpl");
SYMBOL_CRC(mt76_abort_roc, 0x5e2cb3e7, "_gpl");
SYMBOL_CRC(mt76_remain_on_channel, 0xf4e70440, "_gpl");
SYMBOL_CRC(mt76_cancel_remain_on_channel, 0xa613c63f, "_gpl");
SYMBOL_CRC(mt76_pci_disable_aspm, 0x404025ad, "_gpl");
SYMBOL_CRC(mt76_pci_aspm_supported, 0x4adfa720, "_gpl");

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8d1b6861, "ieee80211_sta_register_airtime" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
	{ 0x6b274e07, "pcie_capability_read_word" },
	{ 0x2fe92610, "__skb_pad" },
	{ 0x21fcf695, "simple_attr_open" },
	{ 0x69282e37, "__ieee80211_schedule_txq" },
	{ 0x40af4b18, "debugfs_attr_write" },
	{ 0x4a3ad70e, "wait_for_completion_timeout" },
	{ 0x14d6dbbb, "devm_kmalloc" },
	{ 0xf9428371, "of_node_put" },
	{ 0x34446683, "skb_put" },
	{ 0xec2c4114, "of_property_read_variable_u32_array" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x3c54fbfc, "page_pool_alloc_frag" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xb8342c83, "__kmalloc_noprof" },
	{ 0x57674fd7, "__sw_hweight16" },
	{ 0x3ceb3c58, "consume_skb" },
	{ 0x5a9f1d63, "memmove" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xa6257a2f, "complete" },
	{ 0xb92a4a3f, "dmam_alloc_attrs" },
	{ 0xc1a9d628, "trace_raw_output_prep" },
	{ 0x6b8bf149, "netif_receive_skb_list" },
	{ 0x490c61d5, "skb_dequeue" },
	{ 0x608741b5, "__init_swait_queue_head" },
	{ 0xc678b384, "__trace_trigger_soft_disabled" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x3fe0ba57, "ieee80211_free_hw" },
	{ 0xd1c881ba, "ieee80211_probereq_get" },
	{ 0xa585e820, "dma_unmap_page_attrs" },
	{ 0x8cda6a7f, "ieee80211_txq_schedule_start" },
	{ 0xf3f1225e, "trace_event_printf" },
	{ 0x163b7155, "ieee80211_tx_status_ext" },
	{ 0xe949b386, "debugfs_create_blob" },
	{ 0x0ea3ee0e, "trace_event_raw_init" },
	{ 0x4829a47e, "memcpy" },
	{ 0x21bd74d7, "ieee80211_beacon_loss" },
	{ 0x037a0cba, "kfree" },
	{ 0x0c197b06, "ieee80211_nullfunc_get" },
	{ 0xf95322f4, "kthread_parkme" },
	{ 0x7b557766, "page_pool_destroy" },
	{ 0x9cc35a65, "ieee80211_register_hw" },
	{ 0x557e5e4e, "led_classdev_unregister" },
	{ 0x8bfba944, "__ieee80211_create_tpt_led_trigger" },
	{ 0x04c7e9b1, "netif_napi_add_weight_locked" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x3b336e71, "ieee80211_sta_eosp" },
	{ 0x4d17e172, "ieee80211_get_hdrlen_from_skb" },
	{ 0xe2964344, "__wake_up" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x6f077807, "kthread_park" },
	{ 0x50c0b77d, "sched_set_fifo_low" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x79defbe1, "kthread_should_park" },
	{ 0x7539d01d, "of_get_property" },
	{ 0x70b25da6, "wake_up_process" },
	{ 0x12829f4d, "dev_driver_string" },
	{ 0x8f8269e0, "trace_event_buffer_commit" },
	{ 0xedb16128, "dma_map_page_attrs" },
	{ 0x85540ebc, "nvmem_cell_put" },
	{ 0x3bf47833, "napi_complete_done" },
	{ 0x75c86df0, "ieee80211_get_tx_rates" },
	{ 0xeb1be4b1, "ieee80211_send_bar" },
	{ 0x6b939a61, "cfg80211_reg_check_beaconing" },
	{ 0xaeb9a917, "ieee80211_get_key_rx_seq" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x01000e51, "schedule" },
	{ 0x505dc581, "ieee80211_sta_uapsd_trigger" },
	{ 0xce394caa, "of_get_next_child" },
	{ 0xf3fe8045, "__tracepoint_sched_set_state_tp" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb8585380, "debugfs_create_devm_seqfile" },
	{ 0x0296695f, "refcount_warn_saturate" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0xaef5d983, "__alloc_skb" },
	{ 0xc57c48a3, "idr_get_next" },
	{ 0x09e1a51e, "pci_disable_link_state" },
	{ 0x09a5b50c, "_dev_info" },
	{ 0x4e698588, "of_get_mac_address" },
	{ 0xaef1cd20, "skb_queue_tail" },
	{ 0x99f018c4, "nvmem_cell_read" },
	{ 0x476b165a, "sized_strscpy" },
	{ 0xa1f219c1, "kthread_unpark" },
	{ 0x23f65c8f, "dev_set_threaded" },
	{ 0x7665a95b, "idr_remove" },
	{ 0x07b73ffe, "of_get_child_by_name" },
	{ 0x1d6d44ab, "debugfs_attr_read" },
	{ 0x937250ca, "__dma_sync_single_for_cpu" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x56fee637, "ieee80211_queue_delayed_work" },
	{ 0xda404eab, "devm_kmemdup" },
	{ 0xb6e16cdb, "perf_trace_buf_alloc" },
	{ 0x0d9609a4, "perf_trace_run_bpf_submit" },
	{ 0xbb260be5, "_dev_err" },
	{ 0xf70e4a4d, "preempt_schedule_notrace" },
	{ 0x5b2ea20f, "of_prop_next_string" },
	{ 0x386090fc, "debugfs_create_file_unsafe" },
	{ 0xb8f11603, "idr_alloc" },
	{ 0x64fb973b, "simple_attr_release" },
	{ 0xf4373d2b, "ieee80211_calc_rx_airtime" },
	{ 0xdc3fcbc9, "__sw_hweight8" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0xb1905ade, "of_property_read_bool" },
	{ 0x2a84955a, "ieee80211_find_sta_by_ifaddr" },
	{ 0x5723649e, "ieee80211_next_txq" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x542d4434, "ieee80211_remain_on_channel_expired" },
	{ 0x5fd64fe3, "pcie_capability_clear_and_set_word_locked" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x285e9723, "skb_push" },
	{ 0x3d93dd7e, "napi_enable" },
	{ 0x04a779a7, "ieee80211_beacon_cntdwn_is_complete" },
	{ 0x2781f7e4, "seq_putc" },
	{ 0x8e0afadb, "trace_event_reg" },
	{ 0xaafdc258, "strcasecmp" },
	{ 0xc3832d38, "ieee80211_sta_pspoll" },
	{ 0x64b306f9, "free_netdev" },
	{ 0x77a645b5, "ieee80211_tx_prepare_skb" },
	{ 0x667b38f6, "wiphy_read_of_freq_limits" },
	{ 0xc311bc2d, "of_find_property" },
	{ 0x9844c6f5, "page_pool_put_unrefed_netmem" },
	{ 0x921b07b1, "__cpu_online_mask" },
	{ 0x449ad0a7, "memcmp" },
	{ 0x3c3fce39, "__local_bh_enable_ip" },
	{ 0x56fc4379, "kthread_stop" },
	{ 0x07c1a4a3, "ieee80211_free_txskb" },
	{ 0x59769ab6, "led_classdev_register_ext" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xd6ffcbe8, "of_device_is_available" },
	{ 0x28936efd, "ieee80211_alloc_hw_nm" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x8e17b3ae, "idr_destroy" },
	{ 0xfe0eb6d1, "ieee80211_tx_dequeue" },
	{ 0x259a7acf, "ieee80211_rx_list" },
	{ 0xd3db2b0c, "page_pool_create" },
	{ 0xdcb764ad, "memset" },
	{ 0x69b18f43, "rfc1042_header" },
	{ 0x56e205ff, "debugfs_create_u32" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x0f9d4878, "__pskb_pull_tail" },
	{ 0xe7d9c63c, "ieee80211_ready_on_channel" },
	{ 0xb6986c1f, "__netif_napi_del_locked" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xce5708e3, "kthread_create_on_node" },
	{ 0x7447a4bc, "bpf_trace_run3" },
	{ 0x44c10a52, "kvfree_call_rcu" },
	{ 0x9b8aeb5b, "gro_receive_skb" },
	{ 0xfe271490, "alloc_netdev_dummy" },
	{ 0xd111de91, "trace_event_buffer_reserve" },
	{ 0x34e4c5d2, "ieee80211_scan_completed" },
	{ 0xb6138c76, "ieee80211_iterate_active_interfaces_atomic" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0xc3d94e18, "__dma_sync_single_for_device" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xf4df114f, "ieee80211_unregister_hw" },
	{ 0x440451e9, "napi_build_skb" },
	{ 0x190dd4c1, "seq_write" },
	{ 0x6d5f5b91, "radix_tree_tagged" },
	{ 0x8320a874, "__kmalloc_cache_noprof" },
	{ 0x56470118, "__warn_printk" },
	{ 0xf02050dc, "of_nvmem_cell_get" },
	{ 0xab1af175, "seq_printf" },
	{ 0x5584448a, "ieee80211_channel_to_freq_khz" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x0c3690fc, "_raw_spin_lock_bh" },
	{ 0xf239927a, "cfg80211_chandef_create" },
	{ 0xf9ddb5d9, "timer_init_key" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0x09d48773, "skb_add_rx_frag_netmem" },
	{ 0xf7830f93, "alloc_workqueue_noprof" },
	{ 0x81a64f91, "napi_disable" },
	{ 0x0776a2a7, "ieee80211_sta_ps_transition" },
	{ 0x5f228946, "debugfs_create_u8" },
	{ 0xf1aeee87, "debugfs_create_dir" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x7a1929c1, "trace_handle_return" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0x5bd716e6, "kmalloc_caches" },
	{ 0x53bbefe1, "__trace_set_current_state" },
	{ 0x5677935d, "ieee80211_csa_finish" },
	{ 0x609f1c7e, "synchronize_net" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "mac80211,cfg80211");


MODULE_INFO(srcversion, "FFC119F684AC675B6D8AC7A");
