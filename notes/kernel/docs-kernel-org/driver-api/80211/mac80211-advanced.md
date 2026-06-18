# mac80211 subsystem (advanced)

> 출처(원문): https://docs.kernel.org/driver-api/80211/mac80211-advanced.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# mac80211 subsystem (advanced)

Information contained within this part of the book is of interest only
for advanced interaction of mac80211 with drivers to exploit more
hardware capabilities and improve performance.

## LED support

Mac80211 supports various ways of blinking LEDs. Wherever possible,
device LEDs should be exposed as LED class devices and hooked up to the
appropriate trigger, which will then be triggered appropriately by
mac80211.

struct ieee80211\_tpt\_blink
:   throughput blink description

**Definition**:

```
struct ieee80211_tpt_blink {
    int throughput;
    int blink_time;
};
```

**Members**

`throughput`
:   throughput in Kbit/sec

`blink_time`
:   blink time in milliseconds
    (full cycle, ie. one off + one on period)

enum ieee80211\_tpt\_led\_trigger\_flags
:   throughput trigger flags

**Constants**

`IEEE80211_TPT_LEDTRIG_FL_RADIO`
:   enable blinking with radio

`IEEE80211_TPT_LEDTRIG_FL_WORK`
:   enable blinking when working

`IEEE80211_TPT_LEDTRIG_FL_CONNECTED`
:   enable blinking when at least one
    interface is connected in some way, including being an AP

const char \*ieee80211\_get\_tx\_led\_name(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw)
:   get name of TX LED

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware to get the LED trigger name for

**Description**

mac80211 creates a transmit LED trigger for each wireless hardware
that can be used to drive LEDs if your driver registers a LED device.
This function returns the name (or `NULL` if not configured for LEDs)
of the trigger so you can automatically link the LED device.

**Return**

The name of the LED trigger. `NULL` if not configured for LEDs.

const char \*ieee80211\_get\_rx\_led\_name(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw)
:   get name of RX LED

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware to get the LED trigger name for

**Description**

mac80211 creates a receive LED trigger for each wireless hardware
that can be used to drive LEDs if your driver registers a LED device.
This function returns the name (or `NULL` if not configured for LEDs)
of the trigger so you can automatically link the LED device.

**Return**

The name of the LED trigger. `NULL` if not configured for LEDs.

const char \*ieee80211\_get\_assoc\_led\_name(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw)
:   get name of association LED

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware to get the LED trigger name for

**Description**

mac80211 creates a association LED trigger for each wireless hardware
that can be used to drive LEDs if your driver registers a LED device.
This function returns the name (or `NULL` if not configured for LEDs)
of the trigger so you can automatically link the LED device.

**Return**

The name of the LED trigger. `NULL` if not configured for LEDs.

const char \*ieee80211\_get\_radio\_led\_name(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw)
:   get name of radio LED

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware to get the LED trigger name for

**Description**

mac80211 creates a radio change LED trigger for each wireless hardware
that can be used to drive LEDs if your driver registers a LED device.
This function returns the name (or `NULL` if not configured for LEDs)
of the trigger so you can automatically link the LED device.

**Return**

The name of the LED trigger. `NULL` if not configured for LEDs.

const char \*ieee80211\_create\_tpt\_led\_trigger(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, unsigned int flags, const struct [ieee80211\_tpt\_blink](#c.ieee80211_tpt_blink "ieee80211_tpt_blink") \*blink\_table, unsigned int blink\_table\_len)
:   create throughput LED trigger

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware to create the trigger for

`unsigned int flags`
:   trigger flags, see [`enum ieee80211_tpt_led_trigger_flags`](#c.ieee80211_tpt_led_trigger_flags "ieee80211_tpt_led_trigger_flags")

`const struct ieee80211_tpt_blink *blink_table`
:   the blink table -- needs to be ordered by throughput

`unsigned int blink_table_len`
:   size of the blink table

**Return**

`NULL` (in case of error, or if no LED triggers are
configured) or the name of the new trigger.

**Note**

This function must be called before [`ieee80211_register_hw()`](mac80211.html#c.ieee80211_register_hw "ieee80211_register_hw").

## Hardware crypto acceleration

mac80211 is capable of taking advantage of many hardware
acceleration designs for encryption and decryption operations.

The `set_key()` callback in the [`struct ieee80211_ops`](mac80211.html#c.ieee80211_ops "ieee80211_ops") for a given
device is called to enable hardware acceleration of encryption and
decryption. The callback takes a **sta** parameter that will be NULL
for default keys or keys used for transmission only, or point to
the station information for the peer for individual keys.
Multiple transmission keys with the same key index may be used when
VLANs are configured for an access point.

When transmitting, the TX control data will use the **hw\_key\_idx**
selected by the driver by modifying the [`struct ieee80211_key_conf`](#c.ieee80211_key_conf "ieee80211_key_conf")
pointed to by the **key** parameter to the `set_key()` function.

The `set_key()` call for the `SET_KEY` command should return 0 if
the key is now in use, -`EOPNOTSUPP` or -`ENOSPC` if it couldn’t be
added; if you return 0 then hw\_key\_idx must be assigned to the
hardware key index. You are free to use the full u8 range.

Note that in the case that the **IEEE80211\_HW\_SW\_CRYPTO\_CONTROL** flag is
set, mac80211 will not automatically fall back to software crypto if
enabling hardware crypto failed. The `set_key()` call may also return the
value 1 to permit this specific key/algorithm to be done in software.

When the cmd is `DISABLE_KEY` then it must succeed.

Note that it is permissible to not decrypt a frame even if a key
for it has been uploaded to hardware. The stack will not make any
decision based on whether a key has been uploaded or not but rather
based on the receive flags.

The [`struct ieee80211_key_conf`](#c.ieee80211_key_conf "ieee80211_key_conf") structure pointed to by the **key**
parameter is guaranteed to be valid until another call to `set_key()`
removes it, but it can only be used as a cookie to differentiate
keys.

In TKIP some HW need to be provided a phase 1 key, for RX decryption
acceleration (i.e. iwlwifi). Those drivers should provide update\_tkip\_key
handler.
The `update_tkip_key()` call updates the driver with the new phase 1 key.
This happens every time the iv16 wraps around (every 65536 packets). The
`set_key()` call will happen only once for each key (unless the AP did
rekeying); it will not include a valid phase 1 key. The valid phase 1 key is
provided by update\_tkip\_key only. The trigger that makes mac80211 call this
handler is software decryption with wrap around of iv16.

The `set_default_unicast_key()` call updates the default WEP key index
configured to the hardware for WEP encryption type. This is required
for devices that support offload of data packets (e.g. ARP responses).

Mac80211 drivers should set the **NL80211\_EXT\_FEATURE\_CAN\_REPLACE\_PTK0** flag
when they are able to replace in-use PTK keys according to the following
requirements:
1) They do not hand over frames decrypted with the old key to mac80211

enum ieee80211\_key\_flags
:   key flags

**Constants**

`IEEE80211_KEY_FLAG_GENERATE_IV_MGMT`
:   This flag should be set by the
    driver for a CCMP/GCMP key to indicate that is requires IV generation
    only for management frames (MFP).

`IEEE80211_KEY_FLAG_GENERATE_IV`
:   This flag should be set by the
    driver to indicate that it requires IV generation for this
    particular key. Setting this flag does not necessarily mean that SKBs
    will have sufficient tailroom for ICV or MIC.

`IEEE80211_KEY_FLAG_GENERATE_MMIC`
:   This flag should be set by
    the driver for a TKIP key if it requires Michael MIC
    generation in software.

`IEEE80211_KEY_FLAG_PAIRWISE`
:   Set by mac80211, this flag indicates
    that the key is pairwise rather then a shared key.

`IEEE80211_KEY_FLAG_SW_MGMT_TX`
:   This flag should be set by the driver for a
    CCMP/GCMP key if it requires CCMP/GCMP encryption of management frames
    (MFP) to be done in software.

`IEEE80211_KEY_FLAG_PUT_IV_SPACE`
:   This flag should be set by the driver
    if space should be prepared for the IV, but the IV
    itself should not be generated. Do not set together with
    **IEEE80211\_KEY\_FLAG\_GENERATE\_IV** on the same key. Setting this flag does
    not necessarily mean that SKBs will have sufficient tailroom for ICV or
    MIC.

`IEEE80211_KEY_FLAG_RX_MGMT`
:   This key will be used to decrypt received
    management frames. The flag can help drivers that have a hardware
    crypto implementation that doesn’t deal with management frames
    properly by allowing them to not upload the keys to hardware and
    fall back to software crypto. Note that this flag deals only with
    RX, if your crypto engine can’t deal with TX you can also set the
    `IEEE80211_KEY_FLAG_SW_MGMT_TX` flag to encrypt such frames in SW.

`IEEE80211_KEY_FLAG_RESERVE_TAILROOM`
:   This flag should be set by the
    driver for a key to indicate that sufficient tailroom must always
    be reserved for ICV or MIC, even when HW encryption is enabled.

`IEEE80211_KEY_FLAG_PUT_MIC_SPACE`
:   This flag should be set by the driver for
    a TKIP key if it only requires MIC space. Do not set together with
    **IEEE80211\_KEY\_FLAG\_GENERATE\_MMIC** on the same key.

`IEEE80211_KEY_FLAG_NO_AUTO_TX`
:   Key needs explicit Tx activation.

`IEEE80211_KEY_FLAG_GENERATE_MMIE`
:   This flag should be set by the driver
    for a AES\_CMAC or a AES\_GMAC key to indicate that it requires sequence
    number generation only

`IEEE80211_KEY_FLAG_SPP_AMSDU`
:   SPP A-MSDUs can be used with this key
    (set by mac80211 from the sta->spp\_amsdu flag)

**Description**

These flags are used for communication about keys between the driver
and mac80211, with the **flags** parameter of [`struct ieee80211_key_conf`](#c.ieee80211_key_conf "ieee80211_key_conf").

struct ieee80211\_key\_conf
:   key information

**Definition**:

```
struct ieee80211_key_conf {
    atomic64_t tx_pn;
    u32 cipher;
    u8 icv_len;
    u8 iv_len;
    u8 hw_key_idx;
    s8 keyidx;
    u16 flags;
    s8 link_id;
    u8 keylen;
    u8 key[];
};
```

**Members**

`tx_pn`
:   PN used for TX keys, may be used by the driver as well if it
    needs to do software PN assignment by itself (e.g. due to TSO)

`cipher`
:   The key’s cipher suite selector.

`icv_len`
:   The ICV length for this key type

`iv_len`
:   The IV length for this key type

`hw_key_idx`
:   To be set by the driver, this is the key index the driver
    wants to be given when a frame is transmitted and needs to be
    encrypted in hardware.

`keyidx`
:   the key index (0-7)

`flags`
:   key flags, see [`enum ieee80211_key_flags`](#c.ieee80211_key_flags "ieee80211_key_flags").

`link_id`
:   the link ID, 0 for non-MLO, or -1 for pairwise keys

`keylen`
:   key material length

`key`
:   key material. For ALG\_TKIP the key is encoded as a 256-bit (32 byte)
    data block:
    - Temporal Encryption Key (128 bits)
    - Temporal Authenticator Tx MIC Key (64 bits)
    - Temporal Authenticator Rx MIC Key (64 bits)

**Description**

This key information is given by mac80211 to the driver by
the `set_key()` callback in [`struct ieee80211_ops`](mac80211.html#c.ieee80211_ops "ieee80211_ops").

enum set\_key\_cmd
:   key command

**Constants**

`SET_KEY`
:   a key is set

`DISABLE_KEY`
:   a key must be disabled

**Description**

Used with the `set_key()` callback in [`struct ieee80211_ops`](mac80211.html#c.ieee80211_ops "ieee80211_ops"), this
indicates whether a key is being removed or added.

void ieee80211\_get\_tkip\_p1k\_iv(struct [ieee80211\_key\_conf](#c.ieee80211_key_conf "ieee80211_key_conf") \*keyconf, u32 iv32, u16 \*p1k)
:   get a TKIP phase 1 key for IV32

**Parameters**

`struct ieee80211_key_conf *keyconf`
:   the parameter passed with the set key

`u32 iv32`
:   IV32 to get the P1K for

`u16 *p1k`
:   a buffer to which the key will be written, as 5 u16 values

**Description**

This function returns the TKIP phase 1 key for the given IV32.

void ieee80211\_get\_tkip\_p1k(struct [ieee80211\_key\_conf](#c.ieee80211_key_conf "ieee80211_key_conf") \*keyconf, struct [sk\_buff](../../networking/kapi.html#c.sk_buff "sk_buff") \*skb, u16 \*p1k)
:   get a TKIP phase 1 key

**Parameters**

`struct ieee80211_key_conf *keyconf`
:   the parameter passed with the set key

`struct sk_buff *skb`
:   the packet to take the IV32 value from that will be encrypted
    with this P1K

`u16 *p1k`
:   a buffer to which the key will be written, as 5 u16 values

**Description**

This function returns the TKIP phase 1 key for the IV32 taken
from the given packet.

void ieee80211\_get\_tkip\_p2k(struct [ieee80211\_key\_conf](#c.ieee80211_key_conf "ieee80211_key_conf") \*keyconf, struct [sk\_buff](../../networking/kapi.html#c.sk_buff "sk_buff") \*skb, u8 \*p2k)
:   get a TKIP phase 2 key

**Parameters**

`struct ieee80211_key_conf *keyconf`
:   the parameter passed with the set key

`struct sk_buff *skb`
:   the packet to take the IV32/IV16 values from that will be
    encrypted with this key

`u8 *p2k`
:   a buffer to which the key will be written, 16 bytes

**Description**

This function computes the TKIP RC4 key for the IV values
in the packet.

## Powersave support

mac80211 has support for various powersave implementations.

First, it can support hardware that handles all powersaving by itself;
such hardware should simply set the `IEEE80211_HW_SUPPORTS_PS` hardware
flag. In that case, it will be told about the desired powersave mode
with the `IEEE80211_CONF_PS` flag depending on the association status.
The hardware must take care of sending nullfunc frames when necessary,
i.e. when entering and leaving powersave mode. The hardware is required
to look at the AID in beacons and signal to the AP that it woke up when
it finds traffic directed to it.

`IEEE80211_CONF_PS` flag enabled means that the powersave mode defined in
IEEE 802.11-2007 section 11.2 is enabled. This is not to be confused
with hardware wakeup and sleep states. Driver is responsible for waking
up the hardware before issuing commands to the hardware and putting it
back to sleep at appropriate times.

When PS is enabled, hardware needs to wakeup for beacons and receive the
buffered multicast/broadcast frames after the beacon. Also it must be
possible to send frames and receive the acknowledment frame.

Other hardware designs cannot send nullfunc frames by themselves and also
need software support for parsing the TIM bitmap. This is also supported
by mac80211 by combining the `IEEE80211_HW_SUPPORTS_PS` and
`IEEE80211_HW_PS_NULLFUNC_STACK` flags. The hardware is of course still
required to pass up beacons. The hardware is still required to handle
waking up for multicast traffic; if it cannot the driver must handle that
as best as it can; mac80211 is too slow to do that.

Dynamic powersave is an extension to normal powersave in which the
hardware stays awake for a user-specified period of time after sending a
frame so that reply frames need not be buffered and therefore delayed to
the next wakeup. It’s a compromise of getting good enough latency when
there’s data traffic and still saving significantly power in idle
periods.

Dynamic powersave is simply supported by mac80211 enabling and disabling
PS based on traffic. Driver needs to only set `IEEE80211_HW_SUPPORTS_PS`
flag and mac80211 will handle everything automatically. Additionally,
hardware having support for the dynamic PS feature may set the
`IEEE80211_HW_SUPPORTS_DYNAMIC_PS` flag to indicate that it can support
dynamic PS mode itself. The driver needs to look at the
**dynamic\_ps\_timeout** hardware configuration value and use it that value
whenever `IEEE80211_CONF_PS` is set. In this case mac80211 will disable
dynamic PS feature in stack and will just keep `IEEE80211_CONF_PS`
enabled whenever user has enabled powersave.

Driver informs U-APSD client support by enabling
`IEEE80211_VIF_SUPPORTS_UAPSD` flag. The mode is configured through the
uapsd parameter in `conf_tx()` operation. Hardware needs to send the QoS
Nullfunc frames and stay awake until the service period has ended. To
utilize U-APSD, dynamic powersave is disabled for voip AC and all frames
from that AC are transmitted with powersave enabled.

Note: U-APSD client mode is not yet supported with
`IEEE80211_HW_PS_NULLFUNC_STACK`.

## Beacon filter support

Some hardware have beacon filter support to reduce host cpu wakeups
which will reduce system power consumption. It usually works so that
the firmware creates a checksum of the beacon but omits all constantly
changing elements (TSF, TIM etc). Whenever the checksum changes the
beacon is forwarded to the host, otherwise it will be just dropped. That
way the host will only receive beacons where some relevant information
(for example ERP protection or WMM settings) have changed.

Beacon filter support is advertised with the `IEEE80211_VIF_BEACON_FILTER`
interface capability. The driver needs to enable beacon filter support
whenever power save is enabled, that is `IEEE80211_CONF_PS` is set. When
power save is enabled, the stack will not check for beacon loss and the
driver needs to notify about loss of beacons with [`ieee80211_beacon_loss()`](#c.ieee80211_beacon_loss "ieee80211_beacon_loss").

The time (or number of beacons missed) until the firmware notifies the
driver of a beacon loss event (which in turn causes the driver to call
[`ieee80211_beacon_loss()`](#c.ieee80211_beacon_loss "ieee80211_beacon_loss")) should be configurable and will be controlled
by mac80211 and the roaming algorithm in the future.

Since there may be constantly changing information elements that nothing
in the software stack cares about, we will, in the future, have mac80211
tell the driver which information elements are interesting in the sense
that we want to see changes in them. This will include

> * a list of information element IDs
> * a list of OUIs for the vendor information element

Ideally, the hardware would filter out any beacons without changes in the
requested elements, but if it cannot support that it may, at the expense
of some efficiency, filter out only a subset. For example, if the device
doesn’t support checking for OUIs it should pass up all changes in all
vendor information elements.

Note that change, for the sake of simplification, also includes information
elements appearing or disappearing from the beacon.

Some hardware supports an “ignore list” instead. Just make sure nothing
that was requested is on the ignore list, and include commonly changing
information element IDs in the ignore list, for example 11 (BSS load) and
the various vendor-assigned IEs with unknown contents (128, 129, 133-136,
149, 150, 155, 156, 173, 176, 178, 179, 219); for forward compatibility
it could also include some currently unused IDs.

In addition to these capabilities, hardware should support notifying the
host of changes in the beacon RSSI. This is relevant to implement roaming
when no traffic is flowing (when traffic is flowing we see the RSSI of
the received data packets). This can consist of notifying the host when
the RSSI changes significantly or when it drops below or rises above
configurable thresholds. In the future these thresholds will also be
configured by mac80211 (which gets them from userspace) to implement
them as the roaming algorithm requires.

If the hardware cannot implement this, the driver should ask it to
periodically pass beacon frames to the host so that software can do the
signal strength threshold checking.

void ieee80211\_beacon\_loss(struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif)
:   inform hardware does not receive beacons

**Parameters**

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback.

**Description**

When beacon filtering is enabled with `IEEE80211_VIF_BEACON_FILTER` and
`IEEE80211_CONF_PS` is set, the driver needs to inform whenever the
hardware is not receiving beacons with this function.

## Multiple queues and QoS support

TBD

struct ieee80211\_tx\_queue\_params
:   transmit queue configuration

**Definition**:

```
struct ieee80211_tx_queue_params {
    u16 txop;
    u16 cw_min;
    u16 cw_max;
    u8 aifs;
    bool acm;
    bool uapsd;
    bool mu_edca;
    struct ieee80211_he_mu_edca_param_ac_rec mu_edca_param_rec;
};
```

**Members**

`txop`
:   maximum burst time in units of 32 usecs, 0 meaning disabled

`cw_min`
:   minimum contention window [a value of the form
    2^n-1 in the range 1..32767]

`cw_max`
:   maximum contention window [like **cw\_min**]

`aifs`
:   arbitration interframe space [0..255]

`acm`
:   is mandatory admission control required for the access category

`uapsd`
:   is U-APSD mode enabled for the queue

`mu_edca`
:   is the MU EDCA configured

`mu_edca_param_rec`
:   MU EDCA Parameter Record for HE

**Description**

The information provided in this structure is required for QoS
transmit queue configuration. Cf. IEEE 802.11 7.3.2.29.

## Access point mode support

TBD

Some parts of the if\_conf should be discussed here instead

Insert notes about VLAN interfaces with hw crypto here or in the hw
crypto chapter.

### support for powersaving clients

In order to implement AP and P2P GO modes, mac80211 has support for
client powersaving, both “legacy” PS (PS-Poll/null data) and uAPSD.
There currently is no support for sAPSD.

There is one assumption that mac80211 makes, namely that a client
will not poll with PS-Poll and trigger with uAPSD at the same time.
Both are supported, and both can be used by the same client, but
they can’t be used concurrently by the same client. This simplifies
the driver code.

The first thing to keep in mind is that there is a flag for complete
driver implementation: `IEEE80211_HW_AP_LINK_PS`. If this flag is set,
mac80211 expects the driver to handle most of the state machine for
powersaving clients and will ignore the PM bit in incoming frames.
Drivers then use [`ieee80211_sta_ps_transition()`](#c.ieee80211_sta_ps_transition "ieee80211_sta_ps_transition") to inform mac80211 of
stations’ powersave transitions. In this mode, mac80211 also doesn’t
handle PS-Poll/uAPSD.

In the mode without `IEEE80211_HW_AP_LINK_PS`, mac80211 will check the
PM bit in incoming frames for client powersave transitions. When a
station goes to sleep, we will stop transmitting to it. There is,
however, a race condition: a station might go to sleep while there is
data buffered on hardware queues. If the device has support for this
it will reject frames, and the driver should give the frames back to
mac80211 with the `IEEE80211_TX_STAT_TX_FILTERED` flag set which will
cause mac80211 to retry the frame when the station wakes up. The
driver is also notified of powersave transitions by calling its
**sta\_notify** callback.

When the station is asleep, it has three choices: it can wake up,
it can PS-Poll, or it can possibly start a uAPSD service period.
Waking up is implemented by simply transmitting all buffered (and
filtered) frames to the station. This is the easiest case. When
the station sends a PS-Poll or a uAPSD trigger frame, mac80211
will inform the driver of this with the **allow\_buffered\_frames**
callback; this callback is optional. mac80211 will then transmit
the frames as usual and set the `IEEE80211_TX_CTL_NO_PS_BUFFER`
on each frame. The last frame in the service period (or the only
response to a PS-Poll) also has `IEEE80211_TX_STATUS_EOSP` set to
indicate that it ends the service period; as this frame must have
TX status report it also sets `IEEE80211_TX_CTL_REQ_TX_STATUS`.
When TX status is reported for this frame, the service period is
marked has having ended and a new one can be started by the peer.

Additionally, non-bufferable MMPDUs can also be transmitted by
mac80211 with the `IEEE80211_TX_CTL_NO_PS_BUFFER` set in them.

Another race condition can happen on some devices like iwlwifi
when there are frames queued for the station and it wakes up
or polls; the frames that are already queued could end up being
transmitted first instead, causing reordering and/or wrong
processing of the EOSP. The cause is that allowing frames to be
transmitted to a certain station is out-of-band communication to
the device. To allow this problem to be solved, the driver can
call [`ieee80211_sta_block_awake()`](#c.ieee80211_sta_block_awake "ieee80211_sta_block_awake") if frames are buffered when it
is notified that the station went to sleep. When all these frames
have been filtered (see above), it must call the function again
to indicate that the station is no longer blocked.

If the driver buffers frames in the driver for aggregation in any
way, it must use the [`ieee80211_sta_set_buffered()`](#c.ieee80211_sta_set_buffered "ieee80211_sta_set_buffered") call when it is
notified of the station going to sleep to inform mac80211 of any
TIDs that have frames buffered. Note that when a station wakes up
this information is reset (hence the requirement to call it when
informed of the station going to sleep). Then, when a service
period starts for any reason, **release\_buffered\_frames** is called
with the number of frames to be released and which TIDs they are
to come from. In this case, the driver is responsible for setting
the EOSP (for uAPSD) and MORE\_DATA bits in the released frames.
To help the **more\_data** parameter is passed to tell the driver if
there is more data on other TIDs -- the TIDs to release frames
from are ignored since mac80211 doesn’t know how many frames the
buffers for those TIDs contain.

If the driver also implement GO mode, where absence periods may
shorten service periods (or abort PS-Poll responses), it must
filter those response frames except in the case of frames that
are buffered in the driver -- those must remain buffered to avoid
reordering. Because it is possible that no frames are released
in this case, the driver must call [`ieee80211_sta_eosp()`](#c.ieee80211_sta_eosp "ieee80211_sta_eosp")
to indicate to mac80211 that the service period ended anyway.

Finally, if frames from multiple TIDs are released from mac80211
but the driver might reorder them, it must clear & set the flags
appropriately (only the last frame may have `IEEE80211_TX_STATUS_EOSP`)
and also take care of the EOSP and MORE\_DATA bits in the frame.
The driver may also use [`ieee80211_sta_eosp()`](#c.ieee80211_sta_eosp "ieee80211_sta_eosp") in this case.

Note that if the driver ever buffers frames other than QoS-data
frames, it must take care to never send a non-QoS-data frame as
the last frame in a service period, adding a QoS-nulldata frame
after a non-QoS-data frame if needed.

enum ieee80211\_frame\_release\_type
:   frame release reason

**Constants**

`IEEE80211_FRAME_RELEASE_PSPOLL`
:   frame released for PS-Poll

`IEEE80211_FRAME_RELEASE_UAPSD`
:   frame(s) released due to
    frame received on trigger-enabled AC

int ieee80211\_sta\_ps\_transition(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*sta, bool start)
:   PS transition for connected sta

**Parameters**

`struct ieee80211_sta *sta`
:   currently connected sta

`bool start`
:   start or stop PS

**Description**

When operating in AP mode with the `IEEE80211_HW_AP_LINK_PS`
flag set, use this function to inform mac80211 about a connected station
entering/leaving PS mode.

This function may not be called in IRQ context or with softirqs enabled.

Calls to this function for a single hardware must be synchronized against
each other.

**Return**

0 on success. -EINVAL when the requested PS mode is already set.

int ieee80211\_sta\_ps\_transition\_ni(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*sta, bool start)
:   PS transition for connected sta (in process context)

**Parameters**

`struct ieee80211_sta *sta`
:   currently connected sta

`bool start`
:   start or stop PS

**Description**

Like [`ieee80211_sta_ps_transition()`](#c.ieee80211_sta_ps_transition "ieee80211_sta_ps_transition") but can be called in process context
(internally disables bottom halves). Concurrent call restriction still
applies.

**Return**

Like [`ieee80211_sta_ps_transition()`](#c.ieee80211_sta_ps_transition "ieee80211_sta_ps_transition").

void ieee80211\_sta\_set\_buffered(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*sta, u8 tid, bool buffered)
:   inform mac80211 about driver-buffered frames

**Parameters**

`struct ieee80211_sta *sta`
:   [`struct ieee80211_sta`](#c.ieee80211_sta "ieee80211_sta") pointer for the sleeping station

`u8 tid`
:   the TID that has buffered frames

`bool buffered`
:   indicates whether or not frames are buffered for this TID

**Description**

If a driver buffers frames for a powersave station instead of passing
them back to mac80211 for retransmission, the station may still need
to be told that there are buffered frames via the TIM bit.

This function informs mac80211 whether or not there are frames that are
buffered in the driver for a given TID; mac80211 can then use this data
to set the TIM bit (NOTE: This may call back into the driver’s set\_tim
call! Beware of the locking!)

If all frames are released to the station (due to PS-poll or uAPSD)
then the driver needs to inform mac80211 that there no longer are
frames buffered. However, when the station wakes up mac80211 assumes
that all buffered frames will be transmitted and clears this data,
drivers need to make sure they inform mac80211 about all buffered
frames on the sleep transition (`sta_notify()` with `STA_NOTIFY_SLEEP`).

Note that technically mac80211 only needs to know this per AC, not per
TID, but since driver buffering will inevitably happen per TID (since
it is related to aggregation) it is easier to make mac80211 map the
TID to the AC as required instead of keeping track in all drivers that
use this API.

struct [sk\_buff](../../networking/kapi.html#c.sk_buff "sk_buff") \*ieee80211\_beacon\_get(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif, unsigned int link\_id)
:   beacon generation function

**Parameters**

`struct ieee80211_hw *hw`
:   pointer obtained from [`ieee80211_alloc_hw()`](mac80211.html#c.ieee80211_alloc_hw "ieee80211_alloc_hw").

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback.

`unsigned int link_id`
:   the link id to which the beacon belongs (or 0 for an AP STA
    that is not associated with AP MLD).

**Description**

See `ieee80211_beacon_get_tim()`.

**Return**

See `ieee80211_beacon_get_tim()`.

struct [sk\_buff](../../networking/kapi.html#c.sk_buff "sk_buff") \*ieee80211\_get\_buffered\_bc(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif)
:   accessing buffered broadcast and multicast frames

**Parameters**

`struct ieee80211_hw *hw`
:   pointer as obtained from [`ieee80211_alloc_hw()`](mac80211.html#c.ieee80211_alloc_hw "ieee80211_alloc_hw").

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback.

**Description**

Function for accessing buffered broadcast and multicast frames. If
hardware/firmware does not implement buffering of broadcast/multicast
frames when power saving is used, 802.11 code buffers them in the host
memory. The low-level driver uses this function to fetch next buffered
frame. In most cases, this is used when generating beacon frame.

**Return**

A pointer to the next buffered skb or NULL if no more buffered
frames are available.

**Note**

buffered frames are returned only after DTIM beacon frame was
generated with [`ieee80211_beacon_get()`](#c.ieee80211_beacon_get "ieee80211_beacon_get") and the low-level driver must thus
call [`ieee80211_beacon_get()`](#c.ieee80211_beacon_get "ieee80211_beacon_get") first. [`ieee80211_get_buffered_bc()`](#c.ieee80211_get_buffered_bc "ieee80211_get_buffered_bc") returns
NULL if the previous generated beacon was not DTIM, so the low-level driver
does not need to check for DTIM beacons separately and should be able to
use common code for all beacons.

void ieee80211\_sta\_block\_awake(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*pubsta, bool block)
:   block station from waking up

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware

`struct ieee80211_sta *pubsta`
:   the station

`bool block`
:   whether to block or unblock

**Description**

Some devices require that all frames that are on the queues
for a specific station that went to sleep are flushed before
a poll response or frames after the station woke up can be
delivered to that it. Note that such frames must be rejected
by the driver as filtered, with the appropriate status flag.

This function allows implementing this mode in a race-free
manner.

To do this, a driver must keep track of the number of frames
still enqueued for a specific station. If this number is not
zero when the station goes to sleep, the driver must call
this function to force mac80211 to consider the station to
be asleep regardless of the station’s actual state. Once the
number of outstanding frames reaches zero, the driver must
call this function again to unblock the station. That will
cause mac80211 to be able to send ps-poll responses, and if
the station queried in the meantime then frames will also
be sent out as a result of this. Additionally, the driver
will be notified that the station woke up some time after
it is unblocked, regardless of whether the station actually
woke up while blocked or not.

void ieee80211\_sta\_eosp(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*pubsta)
:   notify mac80211 about end of SP

**Parameters**

`struct ieee80211_sta *pubsta`
:   the station

**Description**

When a device transmits frames in a way that it can’t tell
mac80211 in the TX status about the EOSP, it must clear the
`IEEE80211_TX_STATUS_EOSP` bit and call this function instead.
This applies for PS-Poll as well as uAPSD.

Note that just like with `_tx_status()` and `_rx()` drivers must
not mix calls to irqsafe/non-irqsafe versions, this function
must not be mixed with those either. Use the all irqsafe, or
all non-irqsafe, don’t mix!

NB: the \_irqsafe version of this function doesn’t exist, no
:   driver needs it right now. Don’t call this function if
    you’d need the \_irqsafe version, look at the git history
    and restore the \_irqsafe version!

## Supporting multiple virtual interfaces

TBD

Note: WDS with identical MAC address should almost always be OK

Insert notes about having multiple virtual interfaces with different MAC
addresses here, note which configurations are supported by mac80211, add
notes about supporting hw crypto with it.

void ieee80211\_iterate\_active\_interfaces(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, u32 iter\_flags, void (\*iterator)(void \*data, u8 \*mac, struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif), void \*data)
:   iterate active interfaces

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware `struct of` which the interfaces should be iterated over

`u32 iter_flags`
:   iteration flags, see `enum ieee80211_interface_iteration_flags`

`void (*iterator)(void *data, u8 *mac, struct ieee80211_vif *vif)`
:   the iterator function to call

`void *data`
:   first argument of the iterator function

**Description**

This function iterates over the interfaces associated with a given
hardware that are currently active and calls the callback for them.
This function allows the iterator function to sleep, when the iterator
function is atomic **ieee80211\_iterate\_active\_interfaces\_atomic** can
be used.
Does not iterate over a new interface during `add_interface()`.

void ieee80211\_iterate\_active\_interfaces\_atomic(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, u32 iter\_flags, void (\*iterator)(void \*data, u8 \*mac, struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif), void \*data)
:   iterate active interfaces

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware `struct of` which the interfaces should be iterated over

`u32 iter_flags`
:   iteration flags, see `enum ieee80211_interface_iteration_flags`

`void (*iterator)(void *data, u8 *mac, struct ieee80211_vif *vif)`
:   the iterator function to call, cannot sleep

`void *data`
:   first argument of the iterator function

**Description**

This function iterates over the interfaces associated with a given
hardware that are currently active and calls the callback for them.
This function requires the iterator callback function to be atomic,
if that is not desired, use **ieee80211\_iterate\_active\_interfaces** instead.
Does not iterate over a new interface during `add_interface()`.

## Station handling

TODO

struct ieee80211\_sta
:   station table entry

**Definition**:

```
struct ieee80211_sta {
    u8 addr[ETH_ALEN];
    u16 aid;
    u16 max_rx_aggregation_subframes;
    bool wme;
    u8 uapsd_queues;
    u8 max_sp;
    struct ieee80211_sta_rates *rates;
    bool tdls;
    bool tdls_initiator;
    bool mfp;
    bool mlo;
    bool spp_amsdu;
    u8 max_amsdu_subframes;
    u16 eml_cap;
    struct ieee80211_sta_aggregates *cur;
    bool support_p2p_ps;
    struct ieee80211_txq *txq[IEEE80211_NUM_TIDS + 1];
    u16 valid_links;
    bool epp_peer;
    struct ieee80211_link_sta deflink;
    struct ieee80211_link_sta *link[IEEE80211_MLD_MAX_NUM_LINKS];
    struct ieee80211_sta *nmi;
    struct ieee80211_nan_peer_sched *nan_sched;
    u8 drv_priv[];
};
```

**Members**

`addr`
:   MAC address

`aid`
:   AID we assigned to the station if we’re an AP

`max_rx_aggregation_subframes`
:   maximal amount of frames in a single AMPDU
    that this station is allowed to transmit to us.
    Can be modified by driver.

`wme`
:   indicates whether the STA supports QoS/WME (if local devices does,
    otherwise always false)

`uapsd_queues`
:   bitmap of queues configured for uapsd. Only valid
    if wme is supported. The bits order is like in
    IEEE80211\_WMM\_IE\_STA\_QOSINFO\_AC\_\*.

`max_sp`
:   max Service Period. Only valid if wme is supported.

`rates`
:   rate control selection table

`tdls`
:   indicates whether the STA is a TDLS peer

`tdls_initiator`
:   indicates the STA is an initiator of the TDLS link. Only
    valid if the STA is a TDLS peer in the first place.

`mfp`
:   indicates whether the STA uses management frame protection or not.

`mlo`
:   indicates whether the STA is MLO station.

`spp_amsdu`
:   indicates whether the STA uses SPP A-MSDU or not.

`max_amsdu_subframes`
:   indicates the maximal number of MSDUs in a single
    A-MSDU. Taken from the Extended Capabilities element. 0 means
    unlimited.

`eml_cap`
:   EML capabilities of this MLO station

`cur`
:   currently valid data as aggregated from the active links
    For non MLO STA it will point to the deflink data. For MLO STA
    `ieee80211_sta_recalc_aggregates()` must be called to update it.

`support_p2p_ps`
:   indicates whether the STA supports P2P PS mechanism or not.

`txq`
:   per-TID data TX queues; note that the last entry (`IEEE80211_NUM_TIDS`)
    is used for non-data frames

`valid_links`
:   bitmap of valid links, or 0 for non-MLO

`epp_peer`
:   indicates that the peer is an EPP peer.

`deflink`
:   This holds the default link STA information, for non MLO STA all link
    specific STA information is accessed through **deflink** or through
    link[0] which points to address of **deflink**. For MLO Link STA
    the first added link STA will point to deflink.

`link`
:   reference to Link Sta entries. For Non MLO STA, except 1st link,
    i.e link[0] all links would be assigned to NULL by default and
    would access link information via **deflink** or link[0]. For MLO
    STA, first link STA being added will point its link pointer to
    **deflink** address and remaining would be allocated and the address
    would be assigned to link[link\_id] where link\_id is the id assigned
    by the AP.

`nmi`
:   For NDI stations, pointer to the NMI station of the peer.

`nan_sched`
:   NAN peer schedule for this station. Valid only for NMI stations.

`drv_priv`
:   data area for driver use, will always be aligned to
    sizeof(void \*), size is determined in hw information.

**Description**

A station table entry represents a station we are possibly
communicating with. Since stations are RCU-managed in
mac80211, any ieee80211\_sta pointer you get access to must
either be protected by [`rcu_read_lock()`](../../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock") explicitly or implicitly,
or you must take good care to not use such a pointer after a
call to your sta\_remove callback that removed it.
This also represents the MLD STA in case of MLO association
and holds pointers to various link STA’s

enum sta\_notify\_cmd
:   sta notify command

**Constants**

`STA_NOTIFY_SLEEP`
:   a station is now sleeping

`STA_NOTIFY_AWAKE`
:   a sleeping station woke up

**Description**

Used with the `sta_notify()` callback in [`struct ieee80211_ops`](mac80211.html#c.ieee80211_ops "ieee80211_ops"), this
indicates if an associated station made a power state transition.

struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*ieee80211\_find\_sta(struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif, const u8 \*addr)
:   find a station

**Parameters**

`struct ieee80211_vif *vif`
:   virtual interface to look for station on

`const u8 *addr`
:   station’s address

**Return**

The station, if found. `NULL` otherwise.

**Note**

This function must be called under RCU lock and the
resulting pointer is only valid under RCU lock as well.

struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*ieee80211\_find\_sta\_by\_ifaddr(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, const u8 \*addr, const u8 \*localaddr)
:   find a station on hardware

**Parameters**

`struct ieee80211_hw *hw`
:   pointer as obtained from [`ieee80211_alloc_hw()`](mac80211.html#c.ieee80211_alloc_hw "ieee80211_alloc_hw")

`const u8 *addr`
:   remote station’s address

`const u8 *localaddr`
:   local address (vif->sdata->vif.addr). Use NULL for ‘any’.

**Return**

The station, if found. `NULL` otherwise.

**Note**

This function must be called under RCU lock and the
resulting pointer is only valid under RCU lock as well.

**NOTE**

You may pass NULL for localaddr, but then you will just get
:   the first STA that matches the remote address ‘addr’.
    We can have multiple STA associated with multiple
    logical stations (e.g. consider a station connecting to another
    BSSID on the same AP hardware without disconnecting first).
    In this case, the result of this method with localaddr NULL
    is not reliable.

DO NOT USE THIS FUNCTION with localaddr NULL if at all possible.

## Hardware scan offload

TBD

void ieee80211\_scan\_completed(struct [ieee80211\_hw](mac80211.html#c.ieee80211_hw "ieee80211_hw") \*hw, struct cfg80211\_scan\_info \*info)
:   completed hardware scan

**Parameters**

`struct ieee80211_hw *hw`
:   the hardware that finished the scan

`struct cfg80211_scan_info *info`
:   information about the completed scan

**Description**

When hardware scan offload is used (i.e. the `hw_scan()` callback is
assigned) this function needs to be called by the driver to notify
mac80211 that the scan finished. This function can be called from
any context, including hardirq context.

## Aggregation

### TX A-MPDU aggregation

Aggregation on the TX side requires setting the hardware flag
`IEEE80211_HW_AMPDU_AGGREGATION`. The driver will then be handed
packets with a flag indicating A-MPDU aggregation. The driver
or device is responsible for actually aggregating the frames,
as well as deciding how many and which to aggregate.

When TX aggregation is started by some subsystem (usually the rate
control algorithm would be appropriate) by calling the
[`ieee80211_start_tx_ba_session()`](#c.ieee80211_start_tx_ba_session "ieee80211_start_tx_ba_session") function, the driver will be
notified via its **ampdu\_action** function, with the
`IEEE80211_AMPDU_TX_START` action.

In response to that, the driver is later required to call the
[`ieee80211_start_tx_ba_cb_irqsafe()`](#c.ieee80211_start_tx_ba_cb_irqsafe "ieee80211_start_tx_ba_cb_irqsafe") function, which will really
start the aggregation session after the peer has also responded.
If the peer responds negatively, the session will be stopped
again right away. Note that it is possible for the aggregation
session to be stopped before the driver has indicated that it
is done setting it up, in which case it must not indicate the
setup completion.

Also note that, since we also need to wait for a response from
the peer, the driver is notified of the completion of the
handshake by the `IEEE80211_AMPDU_TX_OPERATIONAL` action to the
**ampdu\_action** callback.

Similarly, when the aggregation session is stopped by the peer
or something calling [`ieee80211_stop_tx_ba_session()`](#c.ieee80211_stop_tx_ba_session "ieee80211_stop_tx_ba_session"), the driver’s
**ampdu\_action** function will be called with the action
`IEEE80211_AMPDU_TX_STOP`. In this case, the call must not fail,
and the driver must later call [`ieee80211_stop_tx_ba_cb_irqsafe()`](#c.ieee80211_stop_tx_ba_cb_irqsafe "ieee80211_stop_tx_ba_cb_irqsafe").
Note that the sta can get destroyed before the BA tear down is
complete.

### RX A-MPDU aggregation

Aggregation on the RX side requires only implementing the
**ampdu\_action** callback that is invoked to start/stop any
block-ack sessions for RX aggregation.

When RX aggregation is started by the peer, the driver is
notified via **ampdu\_action** function, with the
`IEEE80211_AMPDU_RX_START` action, and may reject the request
in which case a negative response is sent to the peer, if it
accepts it a positive response is sent.

While the session is active, the device/driver are required
to de-aggregate frames and pass them up one by one to mac80211,
which will handle the reorder buffer.

When the aggregation session is stopped again by the peer or
ourselves, the driver’s **ampdu\_action** function will be called
with the action `IEEE80211_AMPDU_RX_STOP`. In this case, the
call must not fail.

enum ieee80211\_ampdu\_mlme\_action
:   A-MPDU actions

**Constants**

`IEEE80211_AMPDU_RX_START`
:   start RX aggregation

`IEEE80211_AMPDU_RX_STOP`
:   stop RX aggregation

`IEEE80211_AMPDU_TX_START`
:   start TX aggregation, the driver must either
    call [`ieee80211_start_tx_ba_cb_irqsafe()`](#c.ieee80211_start_tx_ba_cb_irqsafe "ieee80211_start_tx_ba_cb_irqsafe") or
    call [`ieee80211_start_tx_ba_cb_irqsafe()`](#c.ieee80211_start_tx_ba_cb_irqsafe "ieee80211_start_tx_ba_cb_irqsafe") with status
    `IEEE80211_AMPDU_TX_START_DELAY_ADDBA` to delay addba after
    ieee80211\_start\_tx\_ba\_cb\_irqsafe is called, or just return the special
    status `IEEE80211_AMPDU_TX_START_IMMEDIATE`.

`IEEE80211_AMPDU_TX_STOP_CONT`
:   stop TX aggregation but continue transmitting
    queued packets, now unaggregated. After all packets are transmitted the
    driver has to call [`ieee80211_stop_tx_ba_cb_irqsafe()`](#c.ieee80211_stop_tx_ba_cb_irqsafe "ieee80211_stop_tx_ba_cb_irqsafe").

`IEEE80211_AMPDU_TX_STOP_FLUSH`
:   stop TX aggregation and flush all packets,
    called when the station is removed. There’s no need or reason to call
    [`ieee80211_stop_tx_ba_cb_irqsafe()`](#c.ieee80211_stop_tx_ba_cb_irqsafe "ieee80211_stop_tx_ba_cb_irqsafe") in this case as mac80211 assumes the
    session is gone and removes the station.

`IEEE80211_AMPDU_TX_STOP_FLUSH_CONT`
:   called when TX aggregation is stopped
    but the driver hasn’t called [`ieee80211_stop_tx_ba_cb_irqsafe()`](#c.ieee80211_stop_tx_ba_cb_irqsafe "ieee80211_stop_tx_ba_cb_irqsafe") yet and
    now the connection is dropped and the station will be removed. Drivers
    should clean up and drop remaining packets when this is called.

`IEEE80211_AMPDU_TX_OPERATIONAL`
:   TX aggregation has become operational

**Description**

These flags are used with the `ampdu_action()` callback in
[`struct ieee80211_ops`](mac80211.html#c.ieee80211_ops "ieee80211_ops") to indicate which action is needed.

Note that drivers MUST be able to deal with a TX aggregation
session being stopped even before they OK’ed starting it by
calling ieee80211\_start\_tx\_ba\_cb\_irqsafe, because the peer
might receive the addBA frame and send a delBA right away!

## Spatial Multiplexing Powersave (SMPS)

SMPS (Spatial multiplexing power save) is a mechanism to conserve
power in an 802.11n implementation. For details on the mechanism
and rationale, please refer to 802.11 (as amended by 802.11n-2009)
“11.2.3 SM power save”.

The mac80211 implementation is capable of sending action frames
to update the AP about the station’s SMPS mode, and will instruct
the driver to enter the specific mode. It will also announce the
requested SMPS mode during the association handshake. Hardware
support for this feature is required, and can be indicated by
hardware flags.

The default mode will be “automatic”, which nl80211/cfg80211
defines to be dynamic SMPS in (regular) powersave, and SMPS
turned off otherwise.

To support this feature, the driver must set the appropriate
hardware support flags, and handle the SMPS flag to the `config()`
operation. It will then with this mechanism be instructed to
enter the requested SMPS mode while associated to an HT AP.

enum ieee80211\_smps\_mode
:   spatial multiplexing power save mode

**Constants**

`IEEE80211_SMPS_AUTOMATIC`
:   automatic

`IEEE80211_SMPS_OFF`
:   off

`IEEE80211_SMPS_STATIC`
:   static

`IEEE80211_SMPS_DYNAMIC`
:   dynamic

`IEEE80211_SMPS_NUM_MODES`
:   internal, don’t use

void ieee80211\_request\_smps(struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif, unsigned int link\_id, enum [ieee80211\_smps\_mode](#c.ieee80211_smps_mode "ieee80211_smps_mode") smps\_mode)
:   request SM PS transition

**Parameters**

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback.

`unsigned int link_id`
:   link ID for MLO, or 0

`enum ieee80211_smps_mode smps_mode`
:   new SM PS mode

**Description**

This allows the driver to request an SM PS transition in managed
mode. This is useful when the driver has more information than
the stack about possible interference, for example by bluetooth.

TBD

This part of the book describes the rate control algorithm interface and
how it relates to mac80211 and drivers.

## Rate Control API

TBD

enum ieee80211\_rate\_control\_changed
:   flags to indicate what changed

**Constants**

`IEEE80211_RC_BW_CHANGED`
:   The bandwidth that can be used to transmit
    to this station changed. The actual bandwidth is in the station
    information -- for HT20/40 the IEEE80211\_HT\_CAP\_SUP\_WIDTH\_20\_40
    flag changes, for HT and VHT the bandwidth field changes.

`IEEE80211_RC_SMPS_CHANGED`
:   The SMPS state of the station changed.

`IEEE80211_RC_SUPP_RATES_CHANGED`
:   The supported rate set of this peer
    changed (in IBSS mode) due to discovering more information about
    the peer.

`IEEE80211_RC_NSS_CHANGED`
:   N\_SS (number of spatial streams) was changed
    by the peer

int ieee80211\_start\_tx\_ba\_session(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*sta, u16 tid, u16 timeout)
:   Start a tx Block Ack session.

**Parameters**

`struct ieee80211_sta *sta`
:   the station for which to start a BA session

`u16 tid`
:   the TID to BA on.

`u16 timeout`
:   session timeout value (in TUs)

**Return**

success if addBA request was sent, failure otherwise

**Description**

Although mac80211/low level driver/user space application can estimate
the need to start aggregation on a certain RA/TID, the session level
will be managed by the mac80211.

void ieee80211\_start\_tx\_ba\_cb\_irqsafe(struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif, const u8 \*ra, u16 tid)
:   low level driver ready to aggregate.

**Parameters**

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback

`const u8 *ra`
:   receiver address of the BA session recipient.

`u16 tid`
:   the TID to BA on.

**Description**

This function must be called by low level driver once it has
finished with preparations for the BA session. It can be called
from any context.

int ieee80211\_stop\_tx\_ba\_session(struct [ieee80211\_sta](#c.ieee80211_sta "ieee80211_sta") \*sta, u16 tid)
:   Stop a Block Ack session.

**Parameters**

`struct ieee80211_sta *sta`
:   the station whose BA session to stop

`u16 tid`
:   the TID to stop BA.

**Return**

negative error if the TID is invalid, or no aggregation active

**Description**

Although mac80211/low level driver/user space application can estimate
the need to stop aggregation on a certain RA/TID, the session level
will be managed by the mac80211.

void ieee80211\_stop\_tx\_ba\_cb\_irqsafe(struct [ieee80211\_vif](mac80211.html#c.ieee80211_vif "ieee80211_vif") \*vif, const u8 \*ra, u16 tid)
:   low level driver ready to stop aggregate.

**Parameters**

`struct ieee80211_vif *vif`
:   [`struct ieee80211_vif`](mac80211.html#c.ieee80211_vif "ieee80211_vif") pointer from the add\_interface callback

`const u8 *ra`
:   receiver address of the BA session recipient.

`u16 tid`
:   the desired TID to BA on.

**Description**

This function must be called by low level driver once it has
finished with preparations for the BA session tear down. It
can be called from any context.

struct ieee80211\_tx\_rate\_control
:   rate control information for/from RC algo

**Definition**:

```
struct ieee80211_tx_rate_control {
    struct ieee80211_hw *hw;
    struct ieee80211_supported_band *sband;
    struct ieee80211_bss_conf *bss_conf;
    struct sk_buff *skb;
    struct ieee80211_tx_rate reported_rate;
    bool rts, short_preamble;
    u32 rate_idx_mask;
    u8 *rate_idx_mcs_mask;
    bool bss;
};
```

**Members**

`hw`
:   The hardware the algorithm is invoked for.

`sband`
:   The band this frame is being transmitted on.

`bss_conf`
:   the current BSS configuration

`skb`
:   the skb that will be transmitted, the control information in it needs
    to be filled in

`reported_rate`
:   The rate control algorithm can fill this in to indicate
    which rate should be reported to userspace as the current rate and
    used for rate calculations in the mesh network.

`rts`
:   whether RTS will be used for this frame because it is longer than the
    RTS threshold

`short_preamble`
:   whether mac80211 will request short-preamble transmission
    if the selected rate supports it

`rate_idx_mask`
:   user-requested (legacy) rate mask

`rate_idx_mcs_mask`
:   user-requested MCS rate mask (NULL if not in use)

`bss`
:   whether this frame is sent out in AP or IBSS mode

TBD

This part of the book describes mac80211 internals.

## Key handling

### Key handling basics

Key handling in mac80211 is done based on per-interface (sub\_if\_data)
keys and per-station keys. Since each station belongs to an interface,
each station key also belongs to that interface.

Hardware acceleration is done on a best-effort basis for algorithms
that are implemented in software, for each key the hardware is asked
to enable that key for offloading but if it cannot do that the key is
simply kept for software encryption (unless it is for an algorithm
that isn’t implemented in software).
There is currently no way of knowing whether a key is handled in SW
or HW except by looking into debugfs.

All key management is internally protected by a mutex. Within all
other parts of mac80211, key references are, just as STA structure
references, protected by RCU. Note, however, that some things are
unprotected, namely the key->sta dereferences within the hardware
acceleration functions. This means that `sta_info_destroy()` must
remove the key which waits for an RCU grace period.

### MORE TBD

TBD

## Receive processing

TBD

## Transmit processing

TBD

## Station info handling

### Programming information

enum ieee80211\_sta\_info\_flags
:   Stations flags

**Constants**

`WLAN_STA_AUTH`
:   Station is authenticated.

`WLAN_STA_ASSOC`
:   Station is associated.

`WLAN_STA_PS_STA`
:   Station is in power-save mode

`WLAN_STA_AUTHORIZED`
:   Station is authorized to send/receive traffic.
    This bit is always checked so needs to be enabled for all stations
    when virtual port control is not in use.

`WLAN_STA_SHORT_PREAMBLE`
:   Station is capable of receiving short-preamble
    frames.

`WLAN_STA_WDS`
:   Station is one of our WDS peers.

`WLAN_STA_CLEAR_PS_FILT`
:   Clear PS filter in hardware (using the
    IEEE80211\_TX\_CTL\_CLEAR\_PS\_FILT control flag) when the next
    frame to this station is transmitted.

`WLAN_STA_MFP`
:   Management frame protection is used with this STA.

`WLAN_STA_BLOCK_BA`
:   Used to deny ADDBA requests (both TX and RX)
    during suspend/resume and station removal.

`WLAN_STA_PS_DRIVER`
:   driver requires keeping this station in
    power-save mode logically to flush frames that might still
    be in the queues

`WLAN_STA_PSPOLL`
:   Station sent PS-poll while driver was keeping
    station in power-save mode, reply when the driver unblocks.

`WLAN_STA_TDLS_PEER`
:   Station is a TDLS peer.

`WLAN_STA_TDLS_PEER_AUTH`
:   This TDLS peer is authorized to send direct
    packets. This means the link is enabled.

`WLAN_STA_TDLS_INITIATOR`
:   We are the initiator of the TDLS link with this
    station.

`WLAN_STA_TDLS_CHAN_SWITCH`
:   This TDLS peer supports TDLS channel-switching

`WLAN_STA_TDLS_OFF_CHANNEL`
:   The local STA is currently off-channel with this
    TDLS peer

`WLAN_STA_TDLS_WIDER_BW`
:   This TDLS peer supports working on a wider bw on
    the BSS base channel.

`WLAN_STA_UAPSD`
:   Station requested unscheduled SP while driver was
    keeping station in power-save mode, reply when the driver
    unblocks the station.

`WLAN_STA_SP`
:   Station is in a service period, so don’t try to
    reply to other uAPSD trigger frames or PS-Poll.

`WLAN_STA_4ADDR_EVENT`
:   4-addr event was already sent for this frame.

`WLAN_STA_INSERTED`
:   This station is inserted into the hash table.

`WLAN_STA_RATE_CONTROL`
:   rate control was initialized for this station.

`WLAN_STA_TOFFSET_KNOWN`
:   toffset calculated for this station is valid.

`WLAN_STA_MPSP_OWNER`
:   local STA is owner of a mesh Peer Service Period.

`WLAN_STA_MPSP_RECIPIENT`
:   local STA is recipient of a MPSP.

`WLAN_STA_PS_DELIVER`
:   station woke up, but we’re still blocking TX
    until pending frames are delivered

`WLAN_STA_USES_ENCRYPTION`
:   This station was configured for encryption,
    so drop all packets without a key later.

`WLAN_STA_DECAP_OFFLOAD`
:   This station uses rx decap offload

`NUM_WLAN_STA_FLAGS`
:   number of defined flags

**Description**

These flags are used with [`struct sta_info`](#c.sta_info "sta_info")’s **flags** member, but
only indirectly with `set_sta_flag()` and friends.

struct sta\_info
:   STA information

**Definition**:

```
struct sta_info {
    struct list_head list, free_list;
    struct rcu_head rcu_head;
    struct rhlist_head hash_node;
    u8 addr[ETH_ALEN];
    struct ieee80211_local *local;
    struct ieee80211_sub_if_data *sdata;
    struct ieee80211_key *ptk[NUM_DEFAULT_KEYS];
    u8 ptk_idx;
    struct rate_control_ref *rate_ctrl;
    void *rate_ctrl_priv;
    spinlock_t rate_ctrl_lock;
    spinlock_t lock;
    struct ieee80211_fast_tx *fast_tx;
    struct ieee80211_fast_rx *fast_rx;
#ifdef CONFIG_MAC80211_MESH;
    struct mesh_sta *mesh;
#endif;
    struct work_struct drv_deliver_wk;
    u16 listen_interval;
    bool dead;
    bool removed;
    bool uploaded;
    enum ieee80211_sta_state sta_state;
    unsigned long _flags;
    spinlock_t ps_lock;
    struct sk_buff_head ps_tx_buf[IEEE80211_NUM_ACS];
    struct sk_buff_head tx_filtered[IEEE80211_NUM_ACS];
    unsigned long driver_buffered_tids;
    unsigned long txq_buffered_tids;
    u64 assoc_at;
    long last_connected;
    __le16 last_seq_ctrl[IEEE80211_NUM_TIDS + 1];
    u16 tid_seq[IEEE80211_QOS_CTL_TID_MASK + 1];
    struct airtime_info airtime[IEEE80211_NUM_ACS];
    u16 airtime_weight;
    struct sta_ampdu_mlme ampdu_mlme;
#ifdef CONFIG_MAC80211_DEBUGFS;
    struct dentry *debugfs_dir;
#endif;
    u8 reserved_tid;
    s8 amsdu_mesh_control;
    struct cfg80211_chan_def tdls_chandef;
    struct ieee80211_fragment_cache frags;
    struct ieee80211_sta_aggregates cur;
    struct link_sta_info deflink;
    struct link_sta_info *link[IEEE80211_MLD_MAX_NUM_LINKS];
    struct ieee80211_sta_removed_link_stats rem_link_stats;
    struct ieee80211_sta sta;
};
```

**Members**

`list`
:   global linked list entry

`free_list`
:   list entry for keeping track of stations to free

`rcu_head`
:   RCU head used for freeing this station struct

`hash_node`
:   hash node for rhashtable

`addr`
:   station’s MAC address - duplicated from public part to
    let the hash table work with just a single cacheline

`local`
:   pointer to the global information

`sdata`
:   virtual interface this station belongs to

`ptk`
:   peer keys negotiated with this station, if any

`ptk_idx`
:   last installed peer key index

`rate_ctrl`
:   rate control algorithm reference

`rate_ctrl_priv`
:   rate control private per-STA pointer

`rate_ctrl_lock`
:   spinlock used to protect rate control data
    (data inside the algorithm, so serializes calls there)

`lock`
:   used for locking all fields that require locking, see comments
    in the header file.

`fast_tx`
:   TX fastpath information

`fast_rx`
:   RX fastpath information

`mesh`
:   mesh STA information

`drv_deliver_wk`
:   used for delivering frames after driver PS unblocking

`listen_interval`
:   listen interval of this station, when we’re acting as AP

`dead`
:   set to true when sta is unlinked

`removed`
:   set to true when sta is being removed from sta\_list

`uploaded`
:   set to true when sta is uploaded to the driver

`sta_state`
:   duplicates information about station state (for debug)

`_flags`
:   STA flags, see [`enum ieee80211_sta_info_flags`](#c.ieee80211_sta_info_flags "ieee80211_sta_info_flags"), do not use directly

`ps_lock`
:   used for powersave (when mac80211 is the AP) related locking

`ps_tx_buf`
:   buffers (per AC) of frames to transmit to this station
    when it leaves power saving state or polls

`tx_filtered`
:   buffers (per AC) of frames we already tried to
    transmit but were filtered by hardware due to STA having
    entered power saving state, these are also delivered to
    the station when it leaves powersave or polls for frames

`driver_buffered_tids`
:   bitmap of TIDs the driver has data buffered on

`txq_buffered_tids`
:   bitmap of TIDs that mac80211 has txq data buffered on

`assoc_at`
:   clock boottime (in ns) of last association

`last_connected`
:   time (in seconds) when a station got connected

`last_seq_ctrl`
:   last received seq/frag number from this STA (per TID
    plus one for non-QoS frames)

`tid_seq`
:   per-TID sequence numbers for sending to this STA

`airtime`
:   per-AC `struct airtime_info` describing airtime statistics for this
    station

`airtime_weight`
:   station weight for airtime fairness calculation purposes

`ampdu_mlme`
:   A-MPDU state machine state

`debugfs_dir`
:   debug filesystem directory dentry

`reserved_tid`
:   reserved TID (if any, otherwise IEEE80211\_TID\_UNRESERVED)

`amsdu_mesh_control`
:   track the mesh A-MSDU format used by the peer:

`tdls_chandef`
:   a TDLS peer can have a wider chandef that is compatible to
    the BSS one.

`frags`
:   fragment cache

`cur`
:   storage for aggregation data
    [`struct ieee80211_sta`](#c.ieee80211_sta "ieee80211_sta") points either here or to deflink.agg.

`deflink`
:   This is the default link STA information, for non MLO STA all link
    specific STA information is accessed through **deflink** or through
    link[0] which points to address of **deflink**. For MLO Link STA
    the first added link STA will point to deflink.

`link`
:   reference to Link Sta entries. For Non MLO STA, except 1st link,
    i.e link[0] all links would be assigned to NULL by default and
    would access link information via **deflink** or link[0]. For MLO
    STA, first link STA being added will point its link pointer to
    **deflink** address and remaining would be allocated and the address
    would be assigned to link[link\_id] where link\_id is the id assigned
    by the AP.

`rem_link_stats`
:   accumulated removed link stats

`sta`
:   station information we share with the driver

**Description**

This structure collects information about a station that
mac80211 is communicating with.

> * -1: not yet known
> * 0: non-mesh A-MSDU length field
> * 1: big-endian mesh A-MSDU length field
> * 2: little-endian mesh A-MSDU length field

### STA information lifetime rules

STA info structures ([`struct sta_info`](#c.sta_info "sta_info")) are managed in a hash table
for faster lookup and a list for iteration. They are managed using
RCU, i.e. access to the list and hash table is protected by RCU.

Upon allocating a STA info structure with `sta_info_alloc()`, the caller
owns that structure. It must then insert it into the hash table using
either `sta_info_insert()` or `sta_info_insert_rcu()`; only in the latter
case (which acquires an rcu read section but must not be called from
within one) will the pointer still be valid after the call. Note that
the caller may not do much with the STA info before inserting it; in
particular, it may not start any mesh peer link management or add
encryption keys.

When the insertion fails (`sta_info_insert()`) returns non-zero), the
structure will have been freed by `sta_info_insert()`!

Station entries are added by mac80211 when you establish a link with a
peer. This means different things for the different type of interfaces
we support. For a regular station this mean we add the AP sta when we
receive an association response from the AP. For IBSS this occurs when
get to know about a peer on the same IBSS. For WDS we add the sta for
the peer immediately upon device open. When using AP mode we add stations
for each respective station upon request from userspace through nl80211.

In order to remove a STA info structure, various sta\_info\_destroy\_\*()
calls are available.

There is no concept of ownership on a STA entry; each structure is
owned by the global hash table/list until it is removed. All users of
the structure need to be RCU protected so that the structure won’t be
freed before they are done using it.

## Aggregation Functions

struct tid\_ampdu\_tx
:   TID aggregation information (Tx).

**Definition**:

```
struct tid_ampdu_tx {
    struct rcu_head rcu_head;
    struct timer_list session_timer;
    struct timer_list addba_resp_timer;
    struct sk_buff_head pending;
    struct sta_info *sta;
    unsigned long state;
    unsigned long last_tx;
    u16 timeout;
    u8 dialog_token;
    u8 stop_initiator;
    bool tx_stop;
    u16 buf_size;
    u16 ssn;
    u16 failed_bar_ssn;
    bool bar_pending;
    bool amsdu;
    bool ndp;
    u8 tid;
};
```

**Members**

`rcu_head`
:   rcu head for freeing structure

`session_timer`
:   check if we keep Tx-ing on the TID (by timeout value)

`addba_resp_timer`
:   timer for peer’s response to addba request

`pending`
:   pending frames queue -- use sta’s spinlock to protect

`sta`
:   station we are attached to

`state`
:   session state (see above)

`last_tx`
:   jiffies of last tx activity

`timeout`
:   session timeout value to be filled in ADDBA requests

`dialog_token`
:   dialog token for aggregation session

`stop_initiator`
:   initiator of a session stop

`tx_stop`
:   TX DelBA frame when stopping

`buf_size`
:   reorder buffer size at receiver

`ssn`
:   starting sequence number of the session

`failed_bar_ssn`
:   ssn of the last failed BAR tx attempt

`bar_pending`
:   BAR needs to be re-sent

`amsdu`
:   support A-MSDU within A-MDPU

`ndp`
:   this session is using NDP Block ACKs

`tid`
:   TID number

**Description**

This structure’s lifetime is managed by RCU, assignments to
the array holding it must hold the aggregation mutex.

The TX path can access it under RCU lock-free if, and
only if, the state has the flag `HT_AGG_STATE_OPERATIONAL`
set. Otherwise, the TX path must also acquire the spinlock
and re-check the state, see comments in the tx code
touching it.

struct tid\_ampdu\_rx
:   TID aggregation information (Rx).

**Definition**:

```
struct tid_ampdu_rx {
    struct rcu_head rcu_head;
    spinlock_t reorder_lock;
    u64 reorder_buf_filtered;
    struct sk_buff_head *reorder_buf;
    unsigned long *reorder_time;
    struct sta_info *sta;
    struct timer_list session_timer;
    struct timer_list reorder_timer;
    unsigned long last_rx;
    u16 head_seq_num;
    u16 stored_mpdu_num;
    u16 ssn;
    u16 buf_size;
    u16 timeout;
    u8 tid;
    u8 auto_seq:1, removed:1, started:1;
};
```

**Members**

`rcu_head`
:   RCU head used for freeing this struct

`reorder_lock`
:   serializes access to reorder buffer, see below.

`reorder_buf_filtered`
:   bitmap indicating where there are filtered frames in
    the reorder buffer that should be ignored when releasing frames

`reorder_buf`
:   buffer to reorder incoming aggregated MPDUs. An MPDU may be an
    A-MSDU with individually reported subframes.

`reorder_time`
:   jiffies when skb was added

`sta`
:   station we are attached to

`session_timer`
:   check if peer keeps Tx-ing on the TID (by timeout value)

`reorder_timer`
:   releases expired frames from the reorder buffer.

`last_rx`
:   jiffies of last rx activity

`head_seq_num`
:   head sequence number in reordering buffer.

`stored_mpdu_num`
:   number of MPDUs in reordering buffer

`ssn`
:   Starting Sequence Number expected to be aggregated.

`buf_size`
:   buffer size for incoming A-MPDUs

`timeout`
:   reset timer value (in TUs).

`tid`
:   TID number

`auto_seq`
:   used for offloaded BA sessions to automatically pick head\_seq\_and
    and ssn.

`removed`
:   this session is removed (but might have been found due to RCU)

`started`
:   this session has started (head ssn or higher was received)

**Description**

This structure’s lifetime is managed by RCU, assignments to
the array holding it must hold the aggregation mutex.

The **reorder\_lock** is used to protect the members of this
struct, except for **timeout**, **buf\_size** and **dialog\_token**,
which are constant across the lifetime of the struct (the
dialog token being used only for debugging).

struct sta\_ampdu\_mlme
:   STA aggregation information.

**Definition**:

```
struct sta_ampdu_mlme {
    struct tid_ampdu_rx *tid_rx[IEEE80211_NUM_TIDS];
    u8 tid_rx_token[IEEE80211_NUM_TIDS];
    unsigned long tid_rx_timer_expired[BITS_TO_LONGS(IEEE80211_NUM_TIDS)];
    unsigned long tid_rx_stop_requested[BITS_TO_LONGS(IEEE80211_NUM_TIDS)];
    unsigned long tid_rx_manage_offl[BITS_TO_LONGS(2 * IEEE80211_NUM_TIDS)];
    unsigned long agg_session_valid[BITS_TO_LONGS(IEEE80211_NUM_TIDS)];
    unsigned long unexpected_agg[BITS_TO_LONGS(IEEE80211_NUM_TIDS)];
    struct wiphy_work work;
    struct tid_ampdu_tx *tid_tx[IEEE80211_NUM_TIDS];
    struct tid_ampdu_tx *tid_start_tx[IEEE80211_NUM_TIDS];
    unsigned long last_addba_req_time[IEEE80211_NUM_TIDS];
    u8 addba_req_num[IEEE80211_NUM_TIDS];
    u8 dialog_token_allocator;
};
```

**Members**

`tid_rx`
:   aggregation info for Rx per TID -- RCU protected

`tid_rx_token`
:   dialog tokens for valid aggregation sessions

`tid_rx_timer_expired`
:   bitmap indicating on which TIDs the
    RX timer expired until the work for it runs

`tid_rx_stop_requested`
:   bitmap indicating which BA sessions per TID the
    driver requested to close until the work for it runs

`tid_rx_manage_offl`
:   bitmap indicating which BA sessions were requested
    to be treated as started/stopped due to offloading

`agg_session_valid`
:   bitmap indicating which TID has a rx BA session open on

`unexpected_agg`
:   bitmap indicating which TID already sent a delBA due to
    unexpected aggregation related frames outside a session

`work`
:   work struct for starting/stopping aggregation

`tid_tx`
:   aggregation info for Tx per TID

`tid_start_tx`
:   sessions where start was requested, not just protected
    by wiphy mutex but also sta->lock

`last_addba_req_time`
:   timestamp of the last addBA request.

`addba_req_num`
:   number of times addBA request has been sent.

`dialog_token_allocator`
:   dialog token enumerator for each new session;

## Synchronisation Functions

TBD

Locking, lots of RCU
