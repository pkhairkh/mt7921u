/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/* Copyright (C) 2025 MediaTek Inc. */
/*
 * This header previously defined an eBPF-based channel survey stub
 * (BPF_PROG_TYPE_WIFI_SURVEY) that depended on kernel BPF subsystem
 * changes not yet upstream.
 *
 * The ACS (Automatic Channel Selection) functionality has been replaced
 * by a direct kernel implementation in acs.c. See mt7921_acs_info and
 * the associated functions declared in mt7921.h.
 *
 * This file is retained only for backward compatibility with any
 * out-of-tree references. New code should use acs.c/mt7921.h.
 */

#ifndef __MT7921_SURVEY_BPF_H
#define __MT7921_SURVEY_BPF_H

/* ACS is now implemented in acs.c — include mt7921.h for declarations. */

#endif /* __MT7921_SURVEY_BPF_H */
