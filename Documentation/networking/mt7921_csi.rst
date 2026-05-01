.. SPDX-License-Identifier: BSD-3-Clause-Clear

===========================================
MT7921 CSI (Channel State Information) Tool
===========================================

Overview
========

Channel State Information (CSI) captures the per-subcarrier channel
response between a Wi-Fi transmitter and receiver.  For the MT7921,
the firmware can report I/Q samples for every received PPDU, making
the adapter suitable for Wi-Fi sensing research — human presence
detection, gesture recognition, respiration monitoring, and similar
applications.

This document describes the open-source CSI extraction path added to
the mt76/mt7921 driver.  It is the first MediaTek USB Wi-Fi adapter
with open CSI access, based on the vendor driver's CSI control
command and data structures.

Research basis: "Enhancing CSI-Based Wireless Sensing With Open-Source
Linux 802.11ax CSI Tool" (IEEE, May 2025); WhoFi system achieves
95.5% accuracy for human presence detection.


Firmware Interface
==================

The MT7921 firmware exposes two messages for CSI:

.. table:: CSI Firmware Messages

   ==================  ======  =======================================
   Message             ID      Direction
   ==================  ======  =======================================
   CMD_ID_CSI_CONTROL  0x4C    Host → Firmware (start/stop/configure)
   EVENT_ID_CSI_DATA   0x3C    Firmware → Host (CSI I/Q samples)
   ==================  ======  =======================================

CSI Control Command (0x4C)
--------------------------

The host sends ``CMD_ID_CSI_CONTROL`` to start, stop, or configure
CSI capture.  The command payload is::

  struct CMD_CSI_CONTROL_T {
      u8 band_idx;   /* 0 = primary band */
      u8 mode;       /* 0=STOP, 1=START, 2=SET */
      u8 cfg_item;   /* config item selector (SET mode only) */
      u8 value1;     /* first config value */
      u8 value2;     /* second config value */
  };

Config items (used when mode = SET):

.. table:: CSI Configuration Items

   ==========================  =====  =====================================
   Item                        Value  Description
   ==========================  =====  =====================================
   CSI_CONFIG_RSVD1            0      Reserved
   CSI_CONFIG_WF               1      Wi-Fi interface selector
   CSI_CONFIG_RSVD2            2      Reserved
   CSI_CONFIG_FRAME_TYPE       3      Frame type filter
   CSI_CONFIG_TX_PATH          4      TX path selection
   CSI_CONFIG_OUTPUT_FORMAT    5      Output format (raw/tone-masked)
   CSI_CONFIG_INFO             6      Extra info flags
   ==========================  =====  =====================================

Output formats:

.. table:: CSI Output Formats

   =================================  =====  ========================
   Format                             Value  Description
   =================================  =====  ========================
   CSI_OUTPUT_RAW                     0      Raw I/Q per subcarrier
   CSI_OUTPUT_TONE_MASKED             1      Tone-masked output
   CSI_OUTPUT_TONE_MASKED_SHIFTED     2      Tone-masked + shifted
   =================================  =====  ========================


CSI Data Event (0x3C)
---------------------

The firmware sends ``EVENT_ID_CSI_DATA`` for each received PPDU
when CSI capture is active.  The event payload uses a TLV
(Type-Length-Value) format::

  struct CSI_TLV_ELEMENT {
      __le16 tag_type;
      __le16 body_len;
      u8 body[];
  };

TLV tags:

.. table:: CSI Event TLV Tags

   =======================  =====  ==============================
   Tag                      ID     Body Contents
   =======================  =====  ==============================
   CSI_EVENT_VERSION        0      u8 firmware CSI version
   CSI_EVENT_CBW            1      u8 channel bandwidth
   CSI_EVENT_RSSI           2      s8 RSSI
   CSI_EVENT_SNR            3      u8 SNR
   CSI_EVENT_BAND           4      u8 band index
   CSI_EVENT_CSI_NUM        5      u16 number of subcarriers
   CSI_EVENT_CSI_I_DATA     6      int16[] I samples
   CSI_EVENT_CSI_Q_DATA     7      int16[] Q samples
   CSI_EVENT_DBW            8      u8 data bandwidth
   CSI_EVENT_CH_IDX         9      u8 primary channel index
   CSI_EVENT_TA             10     u8[6] transmitter MAC
   CSI_EVENT_EXTRA_INFO     11     u32 extra info flags
   CSI_EVENT_RX_MODE        12     u8 RX mode (SISO/MIMO)
   CSI_EVENT_H_IDX          13     u16 H-matrix index
   CSI_EVENT_TX_RX_IDX      14     u32 TX/RX antenna indices
   =======================  =====  ==============================


nl80211 Vendor Command API
==========================

The CSI interface is exposed to userspace via cfg80211 vendor
commands under the MediaTek OUI (0x00E70C).

Vendor OUI: 0x00, 0x0C, 0xE7 (MediaTek)
Vendor subcmd namespace: CSI sub-commands

Commands
--------

.. table:: CSI Vendor Commands

   ==========================  =====  ============================
   Command                     ID     Description
   ==========================  =====  ============================
   MT7921_NL_CMD_CSI_START     0      Begin CSI capture
   MT7921_NL_CMD_CSI_STOP      1      Stop CSI capture
   MT7921_NL_CMD_CSI_GET       2      Read CSI sample from buffer
   MT7921_NL_CMD_CSI_CONFIG    3      Set CSI configuration
   ==========================  =====  ============================

Attributes
----------

.. table:: CSI Vendor Attributes

   ==========================  =====  ============================
   Attribute                   Type   Description
   ==========================  =====  ============================
   MT7921_NL_ATTR_CSI_DATA    bin    Raw CSI I/Q data blob
   MT7921_NL_ATTR_CSI_MODE    u8     Start/stop/set mode
   MT7921_NL_ATTR_CSI_BAND_IDX u8    Band index (0 = primary)
   MT7921_NL_ATTR_CSI_CFG_ITEM u8    Config item selector
   MT7921_NL_ATTR_CSI_VAL1    u8     Config value 1
   MT7921_NL_ATTR_CSI_VAL2    u8     Config value 2
   MT7921_NL_ATTR_CSI_BW      u8     Channel bandwidth
   MT7921_NL_ATTR_CSI_RSSI    s8     RSSI
   MT7921_NL_ATTR_CSI_SNR     u8     SNR
   MT7921_NL_ATTR_CSI_TA      6-bin  Transmitter MAC address
   MT7921_NL_ATTR_CSI_COUNT   u16    Number of CSI samples
   ==========================  =====  ============================


CSI_START
~~~~~~~~~

Starts CSI capture on the specified band.

Required attributes:

- ``MT7921_NL_ATTR_CSI_BAND_IDX`` — Band index (0 for primary)

Optional attributes:

- ``MT7921_NL_ATTR_CSI_MODE`` — Output format (0=raw, 1=tone-masked,
  2=tone-masked-shifted).  Defaults to raw (0).

Returns: 0 on success, negative errno on failure.


CSI_STOP
~~~~~~~~

Stops CSI capture on the specified band.

Required attributes:

- ``MT7921_NL_ATTR_CSI_BAND_IDX`` — Band index

Returns: 0 on success, negative errno on failure.


CSI_GET
~~~~~~~

Reads one CSI sample from the driver's ring buffer and returns it
as a vendor command reply.

The reply contains:

- ``MT7921_NL_ATTR_CSI_DATA`` — Interleaved I/Q binary blob
  (I0,Q0,I1,Q1,... as int16 pairs, little-endian)
- ``MT7921_NL_ATTR_CSI_BW`` — Channel bandwidth
- ``MT7921_NL_ATTR_CSI_RSSI`` — RSSI
- ``MT7921_NL_ATTR_CSI_SNR`` — SNR
- ``MT7921_NL_ATTR_CSI_TA`` — Transmitter address (6 bytes)
- ``MT7921_NL_ATTR_CSI_COUNT`` — Number of subcarriers

Returns: 0 with reply on success, ``-EAGAIN`` if buffer is empty,
``-ENOENT`` if CSI capture is not enabled.

.. note::

   Until hardware confirms the exact firmware response format,
   the raw firmware event data is encapsulated as a binary blob.
   Userspace must parse it according to the TLV format described
   above.


CSI_CONFIG
~~~~~~~~~~

Sets CSI configuration parameters via the firmware control command.

Required attributes:

- ``MT7921_NL_ATTR_CSI_CFG_ITEM`` — Config item (see table above)
- ``MT7921_NL_ATTR_CSI_VAL1`` — First config value
- ``MT7921_NL_ATTR_CSI_VAL2`` — Second config value

Optional attributes:

- ``MT7921_NL_ATTR_CSI_BAND_IDX`` — Band index

Returns: 0 on success, negative errno on failure.


CSI Data Format
===============

I/Q Sample Layout
-----------------

The CSI I/Q data consists of signed 16-bit integers (little-endian),
organized per subcarrier:

- **Raw format**: I and Q values are provided for each subcarrier.
  In the nl80211 response, they are interleaved as
  I₀,Q₀,I₁,Q₁,...,Iₙ,Qₙ where each value is an ``int16_t``.

- **Subcarrier count** depends on bandwidth:

.. table:: Subcarrier Count by Bandwidth

   ============  ===============  ================  =================
   Bandwidth     Subcarriers      I/Q Data Size     Total I/Q Bytes
   ============  ===============  ================  =================
   20 MHz        64               64 × 2 × 2       256
   40 MHz        128              128 × 2 × 2      512
   80 MHz        256              256 × 2 × 2      1024
   ============  ===============  ================  =================

- Subcarriers are ordered from lowest to highest frequency.
- I and Q are in the same format as the firmware event TLV
  (``CSI_EVENT_CSI_I_DATA`` and ``CSI_EVENT_CSI_Q_DATA``).

Ring Buffer
-----------

The driver maintains a ring buffer of ``MT7921_CSI_RING_SIZE``
(1000) entries.  Each entry stores one CSI sample.  When the
buffer is full, the oldest entry is overwritten.  Userspace
reads entries sequentially via ``CSI_GET``.


Sample Userspace Parser
========================

The following C skeleton demonstrates how to use libnl to
interact with the CSI vendor commands::

  /* mt7921_csi_parser.c — Sample CSI parser using libnl
   *
   * Compile: gcc -o mt7921_csi_parser mt7921_csi_parser.c -lnl-3 -lnl-genl-3
   *
   * SPDX-License-Identifier: BSD-3-Clause-Clear
   */

  #include <netlink/genl/genl.h>
  #include <netlink/genl/family.h>
  #include <netlink/genl/ctrl.h>
  #include <netlink/msg.h>
  #include <netlink/attr.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <errno.h>

  #define MT7921_CSI_VENDOR_OUI   0x00E70C

  enum mt7921_nl_attr_csi {
      MT7921_NL_ATTR_CSI_DATA     = 1,
      MT7921_NL_ATTR_CSI_MODE     = 2,
      MT7921_NL_ATTR_CSI_BAND_IDX = 3,
      MT7921_NL_ATTR_CSI_CFG_ITEM = 4,
      MT7921_NL_ATTR_CSI_VAL1     = 5,
      MT7921_NL_ATTR_CSI_VAL2     = 6,
      MT7921_NL_ATTR_CSI_BW       = 7,
      MT7921_NL_ATTR_CSI_RSSI     = 8,
      MT7921_NL_ATTR_CSI_SNR      = 9,
      MT7921_NL_ATTR_CSI_TA       = 10,
      MT7921_NL_ATTR_CSI_COUNT    = 11,
  };

  enum mt7921_nl_cmd_csi {
      MT7921_NL_CMD_CSI_START  = 0,
      MT7921_NL_CMD_CSI_STOP   = 1,
      MT7921_NL_CMD_CSI_GET    = 2,
      MT7921_NL_CMD_CSI_CONFIG = 3,
  };

  static struct nl_sock *sock;
  static int vendor_id;

  static int csi_vendor_cb(struct nl_msg *msg, void *arg)
  {
      struct nlattr *attrs[MT7921_NL_ATTR_CSI_COUNT + 1];
      struct genlmsghdr *ghdr = nlmsg_data(nlmsg_hdr(msg));
      int len, i, count;
      const int16_t *iq_data;
      const uint8_t *ta;
      int8_t rssi;
      uint8_t bw, snr;

      if (nla_parse(attrs, MT7921_NL_ATTR_CSI_COUNT,
                    genlmsg_attrdata(ghdr, 0),
                    genlmsg_attrlen(ghdr, 0), NULL) < 0)
          return NL_SKIP;

      if (!attrs[MT7921_NL_ATTR_CSI_DATA] ||
          !attrs[MT7921_NL_ATTR_CSI_BW] ||
          !attrs[MT7921_NL_ATTR_CSI_COUNT])
          return NL_SKIP;

      bw = nla_get_u8(attrs[MT7921_NL_ATTR_CSI_BW]);
      count = nla_get_u16(attrs[MT7921_NL_ATTR_CSI_COUNT]);
      rssi = attrs[MT7921_NL_ATTR_CSI_RSSI] ?
             nla_get_s8(attrs[MT7921_NL_ATTR_CSI_RSSI]) : 0;
      snr = attrs[MT7921_NL_ATTR_CSI_SNR] ?
            nla_get_u8(attrs[MT7921_NL_ATTR_CSI_SNR]) : 0;

      printf("BW=%u RSSI=%d SNR=%u count=%u\n", bw, rssi, snr, count);

      if (attrs[MT7921_NL_ATTR_CSI_TA]) {
          ta = nla_data(attrs[MT7921_NL_ATTR_CSI_TA]);
          printf("TA=%02x:%02x:%02x:%02x:%02x:%02x\n",
                 ta[0], ta[1], ta[2], ta[3], ta[4], ta[5]);
      }

      /* Parse interleaved I/Q data */
      iq_data = nla_data(attrs[MT7921_NL_ATTR_CSI_DATA]);
      len = nla_len(attrs[MT7921_NL_ATTR_CSI_DATA]);

      for (i = 0; i + 1 < len / sizeof(int16_t); i += 2) {
          printf("  sc[%3d] I=%6d Q=%6d\n", i / 2, iq_data[i], iq_data[i+1]);
      }

      return NL_OK;
  }

  static int send_csi_cmd(uint8_t subcmd, struct nlattr *extra_attrs[],
                          int n_extra)
  {
      struct nl_msg *msg;
      int i, ret;

      msg = nlmsg_alloc();
      if (!msg)
          return -ENOMEM;

      genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, vendor_id,
                  0, 0, subcmd, MT7921_CSI_VENDOR_OUI);

      for (i = 0; i < n_extra; i++)
          nla_put_attr(msg, 0, &extra_attrs[i]);

      ret = nl_send_auto(sock, msg);
      nlmsg_free(msg);

      if (ret < 0)
          return ret;

      return nl_recvmsgs_default(sock);
  }

  int main(int argc, char *argv[])
  {
      int err;

      sock = nl_socket_alloc();
      if (!sock)
          return 1;

      genl_connect(sock);
      vendor_id = genl_ctrl_resolve(sock, "nl80211");

      nl_socket_modify_cb(sock, NL_CB_MSG_IN, NL_CB_CUSTOM,
                           csi_vendor_cb, NULL);

      /* Start CSI on band 0, raw format */
      /* In practice, use iw vendor command or direct nl80211 calls */

      nl_socket_free(sock);
      return 0;
  }


Known Limitations
=================

1. **Binary blob format**: Until the exact firmware event format is
   confirmed on real hardware, the CSI_GET response encapsulates the
   raw firmware I/Q data as a binary blob.  Userspace must parse it
   according to the TLV format.  This will be replaced with a
   structured netlink API once firmware behavior is validated.

2. **Radiotap not yet filled at runtime**: The radiotap
   vendor-extension field for CSI is defined (see ``mt7921.h``) but
   not yet populated at runtime.  This requires firmware event format
   confirmation on real hardware before the radiotap header can be
   correctly filled during RX processing.

3. **Single-band only**: The current implementation supports CSI
   capture on one band at a time.  Multi-band simultaneous capture
   may require additional firmware support.

4. **Ring buffer overflow**: When userspace reads CSI data slower
   than the firmware produces it, the ring buffer (1000 entries)
   will overflow and older samples are silently dropped.  No
   backpressure mechanism is implemented.

5. **No debugfs interface yet**: CSI data is available only via the
   nl80211 vendor command.  A future debugfs interface for quick
   testing may be added.


Configuration
=============

Module Parameters
-----------------

The CSI feature is always compiled into the driver when
``CONFIG_MT7921_COMMON`` is enabled.  No separate module parameter
is required to enable it.

debugfs Entries
---------------

The following debugfs entries are available under
``/sys/kernel/debug/ieee80211/phyX/mt76/``:

- ``csi_status`` — Read-only.  Shows CSI capture state (enabled/disabled),
  ring buffer head/tail positions, and total samples captured.

(Planned; not yet implemented in this phase.)


Vendor Command Usage with iw
-----------------------------

Starting CSI capture::

  iw dev wlan0 vendor send 00e70c 0 <band_idx> [output_format]

Stopping CSI capture::

  iw dev wlan0 vendor send 00e70c 1 <band_idx>

Reading CSI data::

  iw dev wlan0 vendor send 00e70c 2

Configuring CSI parameters::

  iw dev wlan0 vendor send 00e70c 3 <cfg_item> <val1> <val2> [band_idx]

.. note::

   The exact ``iw vendor send`` syntax requires the nl80211 vendor
   command registration to be recognized by the kernel.  The OUI
   ``00:0C:E7`` and subcommand numbers must match those registered
   by the driver.
