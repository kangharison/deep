# 3.26.Tuner functions and data structures

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-tuner.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.26. Tuner functions and data structures

enum tuner\_mode
:   Mode of the tuner

> **Constants**
>
> `T_RADIO`
> :   Tuner core will work in radio mode
>
> `T_ANALOG_TV`
> :   Tuner core will work in analog TV mode

**Description**

Older boards only had a single tuner device, but some devices have a
separate tuner for radio. In any case, the tuner-core needs to know if
the tuner chip(s) will be used in radio mode or analog TV mode, as, on
radio mode, frequencies are specified on a different range than on TV
mode. This `enum is` used by the tuner core in order to work with the
proper tuner range and eventually use a different tuner chip while in
radio mode.

struct tuner\_setup
:   setup the tuner chipsets

> **Definition**:
>
> ```
> struct tuner_setup {
>       unsigned short  addr;
>       unsigned int    type;
>       unsigned int    mode_mask;
>       void *config;
>       int (*tuner_callback)(void *dev, int component, int cmd, int arg);
> };
> ```

**Members**

`addr`
:   I2C address used to control the tuner device/chipset

`type`
:   Type of the tuner, as defined at the TUNER\_\* macros.
    Each different tuner model should have an unique
    identifier.

`mode_mask`
:   Mask with the allowed tuner modes: V4L2\_TUNER\_RADIO,
    V4L2\_TUNER\_ANALOG\_TV and/or V4L2\_TUNER\_DIGITAL\_TV,
    describing if the tuner should be used to support
    Radio, analog TV and/or digital TV.

`config`
:   Used to send tuner-specific configuration for complex
    tuners that require extra parameters to be set.
    Only a very few tuners require it and its usage on
    newer tuners should be avoided.

`tuner_callback`
:   Some tuners require to call back the bridge driver,
    in order to do some tasks like rising a GPIO at the
    bridge chipset, in order to do things like resetting
    the device.

**Description**

Older boards only had a single tuner device. Nowadays multiple tuner
devices may be present on a single board. Using TUNER\_SET\_TYPE\_ADDR
to pass the tuner\_setup structure it is possible to setup each tuner
device in turn.

Since multiple devices may be present it is no longer sufficient to
send a command to a single i2c device. Instead you should broadcast
the command to all i2c devices.

By setting the mode\_mask correctly you can select which commands are
accepted by a specific tuner device. For example, set mode\_mask to
T\_RADIO if the device is a radio-only tuner. That specific tuner will
only accept commands when the tuner is in radio mode and ignore them
when the tuner is set to TV mode.

enum param\_type
:   type of the tuner pameters

**Constants**

`TUNER_PARAM_TYPE_RADIO`
:   Tuner params are for FM and/or AM radio

`TUNER_PARAM_TYPE_PAL`
:   Tuner params are for PAL color TV standard

`TUNER_PARAM_TYPE_SECAM`
:   Tuner params are for SECAM color TV standard

`TUNER_PARAM_TYPE_NTSC`
:   Tuner params are for NTSC color TV standard

`TUNER_PARAM_TYPE_DIGITAL`
:   Tuner params are for digital TV

struct tuner\_range
:   define the frequencies supported by the tuner

> **Definition**:
>
> ```
> struct tuner_range {
>       unsigned short limit;
>       unsigned char config;
>       unsigned char cb;
> };
> ```

**Members**

`limit`
:   Max frequency supported by that range, in 62.5 kHz
    (TV) or 62.5 Hz (Radio), as defined by
    V4L2\_TUNER\_CAP\_LOW.

`config`
:   Value of the band switch byte (BB) to setup this mode.

`cb`
:   Value of the CB byte to setup this mode.

**Description**

Please notice that digital tuners like xc3028/xc4000/xc5000 don’t use
those ranges, as they’re defined inside the driver. This is used by
analog tuners that are compatible with the “Philips way” to setup the
tuners. On those devices, the tuner set is done via 4 bytes:

> 1. divider byte1 (DB1)
> 2. divider byte 2 (DB2)
> 3. Control byte (CB)
> 4. band switch byte (BB)

Some tuners also have an additional optional Auxiliary byte (AB).

struct tuner\_params
:   Parameters to be used to setup the tuner. Those are used by drivers/media/tuners/tuner-types.c in order to specify the tuner properties. Most of the parameters are for tuners based on tda9887 IF-PLL multi-standard analog TV/Radio demodulator, with is very common on legacy analog tuners.

> **Definition**:
>
> ```
> struct tuner_params {
>       enum param_type type;
>       unsigned int cb_first_if_lower_freq:1;
>       unsigned int has_tda9887:1;
>       unsigned int port1_fm_high_sensitivity:1;
>       unsigned int port2_fm_high_sensitivity:1;
>       unsigned int fm_gain_normal:1;
>       unsigned int intercarrier_mode:1;
>       unsigned int port1_active:1;
>       unsigned int port2_active:1;
>       unsigned int port1_invert_for_secam_lc:1;
>       unsigned int port2_invert_for_secam_lc:1;
>       unsigned int port1_set_for_fm_mono:1;
>       unsigned int default_pll_gating_18:1;
>       unsigned int radio_if:2;
>       signed int default_top_low:5;
>       signed int default_top_mid:5;
>       signed int default_top_high:5;
>       signed int default_top_secam_low:5;
>       signed int default_top_secam_mid:5;
>       signed int default_top_secam_high:5;
>       u16 iffreq;
>       unsigned int count;
>       const struct tuner_range *ranges;
> };
> ```

**Members**

`type`
:   Type of the tuner parameters, as defined at
    [`enum param_type`](#c.param_type "param_type"). If the tuner supports multiple
    standards, an array should be used, with one
    row per different standard.

`cb_first_if_lower_freq`
:   Many Philips-based tuners have a comment in
    their datasheet like
    “For channel selection involving band
    switching, and to ensure smooth tuning to the
    desired channel without causing unnecessary
    charge pump action, it is recommended to
    consider the difference between wanted channel
    frequency and the current channel frequency.
    Unnecessary charge pump action will result
    in very low tuning voltage which may drive the
    oscillator to extreme conditions”.
    Set cb\_first\_if\_lower\_freq to 1, if this check
    is required for this tuner. I tested this for
    PAL by first setting the TV frequency to
    203 MHz and then switching to 96.6 MHz FM
    radio. The result was static unless the
    control byte was sent first.

`has_tda9887`
:   Set to 1 if this tuner uses a tda9887

`port1_fm_high_sensitivity`
:   Many Philips tuners use tda9887 PORT1 to select
    the FM radio sensitivity. If this setting is 1,
    then set PORT1 to 1 to get proper FM reception.

`port2_fm_high_sensitivity`
:   Some Philips tuners use tda9887 PORT2 to select
    the FM radio sensitivity. If this setting is 1,
    then set PORT2 to 1 to get proper FM reception.

`fm_gain_normal`
:   Some Philips tuners use tda9887 cGainNormal to
    select the FM radio sensitivity. If this
    setting is 1, e register will use cGainNormal
    instead of cGainLow.

`intercarrier_mode`
:   Most tuners with a tda9887 use QSS mode.
    Some (cheaper) tuners use Intercarrier mode.
    If this setting is 1, then the tuner needs to
    be set to intercarrier mode.

`port1_active`
:   This setting sets the default value for PORT1.
    0 means inactive, 1 means active. Note: the
    actual bit value written to the tda9887 is
    inverted. So a 0 here means a 1 in the B6 bit.

`port2_active`
:   This setting sets the default value for PORT2.
    0 means inactive, 1 means active. Note: the
    actual bit value written to the tda9887 is
    inverted. So a 0 here means a 1 in the B7 bit.

`port1_invert_for_secam_lc`
:   Sometimes PORT1 is inverted when the SECAM-L’
    standard is selected. Set this bit to 1 if this
    is needed.

`port2_invert_for_secam_lc`
:   Sometimes PORT2 is inverted when the SECAM-L’
    standard is selected. Set this bit to 1 if this
    is needed.

`port1_set_for_fm_mono`
:   Some cards require PORT1 to be 1 for mono Radio
    FM and 0 for stereo.

`default_pll_gating_18`
:   Select 18% (or according to datasheet 0%)
    L standard PLL gating, vs the driver default
    of 36%.

`radio_if`
:   IF to use in radio mode. Tuners with a
    separate radio IF filter seem to use 10.7,
    while those without use 33.3 for PAL/SECAM
    tuners and 41.3 for NTSC tuners.
    0 = 10.7, 1 = 33.3, 2 = 41.3

`default_top_low`
:   Default tda9887 TOP value in dB for the low
    band. Default is 0. Range: -16:+15

`default_top_mid`
:   Default tda9887 TOP value in dB for the mid
    band. Default is 0. Range: -16:+15

`default_top_high`
:   Default tda9887 TOP value in dB for the high
    band. Default is 0. Range: -16:+15

`default_top_secam_low`
:   Default tda9887 TOP value in dB for SECAM-L/L’
    for the low band. Default is 0. Several tuners
    require a different TOP value for the
    SECAM-L/L’ standards. Range: -16:+15

`default_top_secam_mid`
:   Default tda9887 TOP value in dB for SECAM-L/L’
    for the mid band. Default is 0. Several tuners
    require a different TOP value for the
    SECAM-L/L’ standards. Range: -16:+15

`default_top_secam_high`
:   Default tda9887 TOP value in dB for SECAM-L/L’
    for the high band. Default is 0. Several tuners
    require a different TOP value for the
    SECAM-L/L’ standards. Range: -16:+15

`iffreq`
:   Intermediate frequency (IF) used by the tuner
    on digital mode.

`count`
:   Size of the ranges array.

`ranges`
:   Array with the frequency ranges supported by
    the tuner.

struct tunertype
:   describes the known tuners.

> **Definition**:
>
> ```
> struct tunertype {
>       char *name;
>       unsigned int count;
>       const struct tuner_params *params;
>       u16 min;
>       u16 max;
>       u32 stepsize;
>       u8 *initdata;
>       u8 *sleepdata;
> };
> ```

**Members**

`name`
:   string with the tuner’s name.

`count`
:   size of [`struct tuner_params`](#c.tuner_params "tuner_params") array.

`params`
:   pointer to [`struct tuner_params`](#c.tuner_params "tuner_params") array.

`min`
:   minimal tuner frequency, in 62.5 kHz step.
    should be multiplied to 16 to convert to MHz.

`max`
:   minimal tuner frequency, in 62.5 kHz step.
    Should be multiplied to 16 to convert to MHz.

`stepsize`
:   frequency step, in Hz.

`initdata`
:   optional byte sequence to initialize the tuner.

`sleepdata`
:   optional byte sequence to power down the tuner.
