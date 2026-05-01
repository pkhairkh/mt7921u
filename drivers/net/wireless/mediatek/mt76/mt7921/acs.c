// SPDX-License-Identifier: BSD-3-Clause-Clear
/* Copyright (C) 2025 MediaTek Inc. */
/*
 * ACS (Automatic Channel Selection) for mt7921.
 * Implements a scoring-based channel selection algorithm using
 * survey data from the driver's channel state. Replaces the
 * earlier eBPF stub with a direct kernel implementation.
 *
 * Algorithm:
 * 1. Walk all supported channels (2.4 GHz, 5 GHz, 6 GHz)
 * 2. For each channel, compute a score from survey data:
 *    score = 1000 - (busy_pct * 4) - (noise_offset * 3) - (bss_count * 2)
 * 3. Apply band preference weights
 * 4. Select highest-scoring channel
 */

#include <linux/debugfs.h>
#include <linux/jiffies.h>
#include <linux/sort.h>
#include "mt7921.h"

#define ACS_BASE_SCORE          1000
#define ACS_BUSY_WEIGHT         4
#define ACS_NOISE_WEIGHT        3
#define ACS_BSS_WEIGHT          2

#define ACS_BAND_BONUS_6GHZ     200
#define ACS_BAND_BONUS_5GHZ     100
#define ACS_BAND_BONUS_2GHZ     0

#define ACS_DFS_PENALTY         50
#define ACS_WIDE_CHAN_BONUS     50

/* Helper: look up the mt76_channel_state for a given ieee80211_channel.
 * Replicates the logic of the static mt76_channel_state() in mac80211.c.
 */
static struct mt76_channel_state *
acs_channel_state(struct mt76_phy *mphy, struct ieee80211_channel *c)
{
        struct mt76_sband *msband;
        int idx;

        if (c->band == NL80211_BAND_2GHZ)
                msband = &mphy->sband_2g;
        else if (c->band == NL80211_BAND_6GHZ)
                msband = &mphy->sband_6g;
        else
                msband = &mphy->sband_5g;

        idx = c - msband->sband.channels;
        if (idx < 0 || idx >= msband->sband.n_channels)
                return NULL;

        return &msband->chan[idx];
}

/* Check if a channel is capable of 40 MHz or wider operation.
 * For 5/6 GHz: most non-DFS channels support at least 40 MHz.
 * For 2.4 GHz: channels 1-9 can be 40 MHz upper, 5-13 lower.
 */
static bool acs_channel_wide_capable(struct ieee80211_channel *chan)
{
        if (chan->flags & IEEE80211_CHAN_DISABLED)
                return false;

        if (chan->band == NL80211_BAND_2GHZ)
                return chan->hw_value >= 1 && chan->hw_value <= 13;

        if (chan->band == NL80211_BAND_6GHZ)
                return true;

        /* 5 GHz: channels that are not 20 MHz only */
        if (chan->flags & IEEE80211_CHAN_NO_40MHZ)
                return false;

        return true;
}

static s32 acs_compute_score(struct mt792x_acs_chan_score *cs)
{
        s32 score = ACS_BASE_SCORE;
        u32 busy_pct = cs->busy_pct;
        s32 noise_offset;
        int band_bonus = 0;

        /* Busy time penalty: lower is better */
        score -= (s32)(busy_pct * ACS_BUSY_WEIGHT);

        /* Noise offset: noise is in dBm (typically -80 to -100).
         * Lower (more negative) is better. Offset from -100 so that
         * a noise floor of -100 dBm gives offset 0 (best case),
         * and -80 dBm gives offset 20 (noisier).
         */
        noise_offset = cs->noise - (-100);
        if (noise_offset < 0)
                noise_offset = 0;
        score -= noise_offset * ACS_NOISE_WEIGHT;

        /* Band preference */
        switch (cs->band) {
        case NL80211_BAND_6GHZ:
                band_bonus = ACS_BAND_BONUS_6GHZ;
                break;
        case NL80211_BAND_5GHZ:
                band_bonus = ACS_BAND_BONUS_5GHZ;
                break;
        case NL80211_BAND_2GHZ:
        default:
                band_bonus = ACS_BAND_BONUS_2GHZ;
                break;
        }
        score += band_bonus;

        /* DFS penalty: radar detection required, more overhead */
        if (cs->dfs)
                score -= ACS_DFS_PENALTY;

        /* Wide channel bonus: wider channels offer more throughput */
        if (cs->wide_capable)
                score += ACS_WIDE_CHAN_BONUS;

        return score;
}

static void acs_score_band(struct mt792x_dev *dev,
                           struct mt76_sband *msband,
                           enum nl80211_band band,
                           unsigned int *out_idx)
{
        struct mt76_phy *mphy = &dev->mphy;
        struct ieee80211_supported_band *sband = &msband->sband;
        struct ieee80211_channel *chan;
        struct mt76_channel_state *state;
        struct mt792x_acs_chan_score *cs;
        u64 busy, active;
        unsigned int i;

        for (i = 0; i < sband->n_channels; i++) {
                chan = &sband->channels[i];

                if (chan->flags & IEEE80211_CHAN_DISABLED)
                        continue;

                if (*out_idx >= MT792X_ACS_MAX_CHANNELS)
                        return;

                cs = &dev->acs.scores[*out_idx];
                cs->center_freq = chan->center_freq;
                cs->band = band;
                cs->hw_value = chan->hw_value;
                cs->dfs = !!(chan->flags & IEEE80211_CHAN_RADAR);
                cs->wide_capable = acs_channel_wide_capable(chan);

                state = acs_channel_state(mphy, chan);
                if (state) {
                        busy = state->cc_busy;
                        active = state->cc_active;

                        /* busy_pct = (busy * 100) / (active ?: 1) */
                        if (active)
                                cs->busy_pct = div_u64(busy * 100, active);
                        else
                                cs->busy_pct = 0;

                        cs->noise = state->noise;
                } else {
                        cs->busy_pct = 0;
                        cs->noise = 0;
                }

                cs->score = acs_compute_score(cs);
                (*out_idx)++;
        }
}

void mt7921_acs_update(struct mt792x_dev *dev)
{
        struct mt76_phy *mphy = &dev->mphy;
        unsigned int idx = 0;
        unsigned int best_idx = 0;
        s32 best_score = S32_MIN;
        unsigned int i;

        if (!dev->acs.enabled)
                return;

        dev->acs.num_scores = 0;

        /* Score all bands that the device supports */
        if (mphy->sband_2g.sband.n_channels)
                acs_score_band(dev, &mphy->sband_2g,
                               NL80211_BAND_2GHZ, &idx);

        if (mphy->sband_5g.sband.n_channels)
                acs_score_band(dev, &mphy->sband_5g,
                               NL80211_BAND_5GHZ, &idx);

        if (mphy->sband_6g.sband.n_channels)
                acs_score_band(dev, &mphy->sband_6g,
                               NL80211_BAND_6GHZ, &idx);

        dev->acs.num_scores = idx;

        /* Select the best channel */
        for (i = 0; i < idx; i++) {
                if (dev->acs.scores[i].score > best_score) {
                        best_score = dev->acs.scores[i].score;
                        best_idx = i;
                }
        }

        if (idx > 0) {
                dev->acs.recommended_freq =
                        dev->acs.scores[best_idx].center_freq;
                dev->acs.recommended_hw_value =
                        dev->acs.scores[best_idx].hw_value;
                dev->acs.recommended_score = best_score;
        } else {
                dev->acs.recommended_freq = 0;
                dev->acs.recommended_hw_value = 0;
                dev->acs.recommended_score = 0;
        }

        dev->acs.last_update_jiffies = jiffies;
}

void mt7921_acs_init(struct mt792x_dev *dev)
{
        memset(&dev->acs, 0, sizeof(dev->acs));
        dev->acs.enabled = true;
        dev->acs.last_update_jiffies = jiffies;

        /* Do NOT call mt7921_acs_update() here — during early device
         * registration the hardware is not yet initialized, so survey
         * data would be all zeros. The first real update happens in
         * mt7921_init_work() after hardware init completes.
         */
}

void mt7921_acs_cleanup(struct mt792x_dev *dev)
{
        dev->acs.enabled = false;
        /* debugfs entries are cleaned up automatically when the parent
         * directory is removed by the debugfs core.
         */
        dev->acs.debugfs_dir = NULL;
}

int mt7921_acs_get_recommendation(struct mt792x_dev *dev, u32 *freq)
{
        if (!dev->acs.enabled || !dev->acs.recommended_freq)
                return -ENOENT;

        /* Refresh scores if data is stale (older than 10 seconds) */
        if (time_after(jiffies,
                       dev->acs.last_update_jiffies + 10 * HZ))
                mt7921_acs_update(dev);

        *freq = dev->acs.recommended_freq;
        return 0;
}

/* debugfs: show ACS recommendation and per-channel scores */
static int mt7921_acs_show(struct seq_file *s, void *data)
{
        struct mt792x_dev *dev = s->private;
        unsigned long age_jiffies;
        unsigned int i;
        const char *band_name;

        if (!dev->acs.enabled) {
                seq_puts(s, "ACS disabled\n");
                return 0;
        }

        /* Refresh if stale */
        if (time_after(jiffies,
                       dev->acs.last_update_jiffies + 10 * HZ))
                mt7921_acs_update(dev);

        age_jiffies = jiffies - dev->acs.last_update_jiffies;

        if (dev->acs.recommended_freq) {
                seq_printf(s, "recommended_channel: %u (%u MHz)\n",
                           dev->acs.recommended_hw_value,
                           dev->acs.recommended_freq);
                seq_printf(s, "score: %d\n", dev->acs.recommended_score);
                seq_printf(s, "last_update: %lu jiffies ago\n", age_jiffies);
        } else {
                seq_puts(s, "no recommendation available\n");
        }

        seq_puts(s, "\nChannel scores:\n");
        for (i = 0; i < dev->acs.num_scores; i++) {
                struct mt792x_acs_chan_score *cs = &dev->acs.scores[i];

                switch (cs->band) {
                case NL80211_BAND_2GHZ:
                        band_name = "2GHz";
                        break;
                case NL80211_BAND_5GHZ:
                        band_name = "5GHz";
                        break;
                case NL80211_BAND_6GHZ:
                        band_name = "6GHz";
                        break;
                default:
                        band_name = "?";
                        break;
                }

                seq_printf(s, "  %3u (%u MHz) [%s]: %d  busy=%u%% noise=%d dBm%s%s\n",
                           cs->hw_value, cs->center_freq, band_name,
                           cs->score, cs->busy_pct, cs->noise,
                           cs->dfs ? " DFS" : "",
                           cs->wide_capable ? " wide" : "");
        }

        return 0;
}

/* debugfs: trigger an ACS update on write */
static ssize_t mt7921_acs_write(struct file *file,
                                const char __user *user_buf,
                                size_t count, loff_t *ppos)
{
        struct seq_file *s = file->private_data;
        struct mt792x_dev *dev = s->private;

        mt7921_acs_update(dev);

        return count;
}

static int mt7921_acs_open(struct inode *inode, struct file *file)
{
        return single_open(file, mt7921_acs_show, inode->i_private);
}

static const struct file_operations mt7921_acs_fops = {
        .owner          = THIS_MODULE,
        .open           = mt7921_acs_open,
        .read           = seq_read,
        .write          = mt7921_acs_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

void mt7921_acs_debugfs_init(struct mt792x_dev *dev)
{
        struct dentry *dir;

        if (!dev->debugfs_dir)
                return;

        dir = debugfs_create_dir("acs", dev->debugfs_dir);
        if (IS_ERR_OR_NULL(dir))
                return;

        dev->acs.debugfs_dir = dir;

        debugfs_create_file("recommendation", 0600, dir, dev,
                            &mt7921_acs_fops);
}
