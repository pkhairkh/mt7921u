// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2025 mt7921u Forensic Audit Project
 *
 * TWT (Target Wake Time) debugfs statistics for mt7921.
 * Provides visibility into active TWT agreements and service period
 * counters via /sys/kernel/debug/ieee80211/phyX/mt76/twt_stats.
 */

#include <linux/debugfs.h>
#include "mt7921.h"

static int
mt7921_twt_stats_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = dev_get_drvdata(s->private);
        struct mt7921_twt_agrt_stats *stats = &dev->twt.stats;
        struct mt76_wcid *wcid;
        struct mt792x_sta *msta;
        struct mt7921_twt_flow *flow;
        int i, j;

        rcu_read_lock();

        seq_printf(s, "total agreements: %u\n", dev->twt.n_agrt);
        seq_printf(s, "missed SPs:       %u\n", stats->n_missed_sp);
        seq_puts(s, "\n");

        if (!dev->twt.n_agrt)
                goto out;

        seq_puts(s, "     wcid | table_id | flow_id | mantissa | exp | duration");
        seq_puts(s, " |            tsf | prot | type | trig | missed_sp | active\n");

        for (i = 0; i < MT76_N_WCIDS; i++) {
                wcid = rcu_dereference(dev->mt76.wcid[i]);
                if (!wcid)
                        continue;

                if (!wcid->sta)
                        continue;

                msta = container_of(wcid, struct mt792x_sta, deflink.wcid);

                for (j = 0; j < MT7921_MAX_STA_TWT_AGRT; j++) {
                        if (!(msta->twt.flowid_mask & BIT(j)))
                                continue;

                        flow = &msta->twt.flow[j];
                        seq_printf(s,
                                   "%9d | %8d | %7d | %8d | %3d | %8d | %14lld | %4c | %4c | %4c | %9u | %6c\n",
                                   flow->wcid, flow->table_id, flow->id,
                                   le16_to_cpu(flow->mantissa), flow->exp,
                                   flow->duration, flow->tsf,
                                   flow->protection ? 'p' : '-',
                                   flow->flowtype ? 'i' : 'a',
                                   flow->trigger ? 't' : '-',
                                   stats->missed_sp[flow->table_id],
                                   stats->sp_active[flow->table_id] ? 'Y' : 'N');
                }
        }

out:
        rcu_read_unlock();

        return 0;
}

static int
mt7921_twt_stats_open(struct inode *inode, struct file *file)
{
        return single_open(file, mt7921_twt_stats_show, inode->i_private);
}

static const struct file_operations mt7921_twt_stats_fops = {
        .owner          = THIS_MODULE,
        .open           = mt7921_twt_stats_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

void mt7921_twt_debugfs_init(struct mt792x_dev *dev)
{
        struct dentry *dir = dev->debugfs_dir;

        if (!dir)
                return;

        /* RUNTIME_VERIFY: test with TWT-capable AP */
        debugfs_create_file("twt_stats", 0400, dir, dev,
                            &mt7921_twt_stats_fops);
}

void mt7921_twt_debugfs_remove(struct mt792x_dev *dev)
{
        /* debugfs entries are removed automatically when the parent
         * directory is removed. The TWT stats file under debugfs_dir
         * does not require separate cleanup — debugfs_remove_recursive
         * on the parent handles it.
         */
}
