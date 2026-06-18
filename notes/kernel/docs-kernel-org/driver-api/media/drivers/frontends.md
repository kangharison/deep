# 10.2.2.Frontend drivers

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/frontends.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.2.2. Frontend drivers

## 10.2.2.1. Frontend attach headers

struct a8293\_platform\_data
:   Platform data for the a8293 driver

**Definition**:

```
struct a8293_platform_data {
    struct dvb_frontend *dvb_frontend;
    int volt_slew_nanos_per_mv;
};
```

**Members**

`dvb_frontend`
:   DVB frontend.

`volt_slew_nanos_per_mv`
:   Slew rate when increasing LNB voltage,
    in nanoseconds per millivolt.

struct af9013\_platform\_data
:   Platform data for the af9013 driver

**Definition**:

```
struct af9013_platform_data {
    u32 clk;
#define AF9013_TUNER_MXL5003D      3 ;
#define AF9013_TUNER_MXL5005D     13 ;
#define AF9013_TUNER_MXL5005R     30 ;
#define AF9013_TUNER_ENV77H11D5  129 ;
#define AF9013_TUNER_MT2060      130 ;
#define AF9013_TUNER_MC44S803    133 ;
#define AF9013_TUNER_QT1010      134 ;
#define AF9013_TUNER_UNKNOWN     140 ;
#define AF9013_TUNER_MT2060_2    147 ;
#define AF9013_TUNER_TDA18271    156 ;
#define AF9013_TUNER_QT1010A     162 ;
#define AF9013_TUNER_MXL5007T    177 ;
#define AF9013_TUNER_TDA18218    179 ;
    u8 tuner;
    u32 if_frequency;
#define AF9013_TS_MODE_USB       0;
#define AF9013_TS_MODE_PARALLEL  1;
#define AF9013_TS_MODE_SERIAL    2;
    u8 ts_mode;
    u8 ts_output_pin;
    bool spec_inv;
    u8 api_version[4];
#define AF9013_GPIO_ON (1 << 0);
#define AF9013_GPIO_EN (1 << 1);
#define AF9013_GPIO_O  (1 << 2);
#define AF9013_GPIO_I  (1 << 3);
#define AF9013_GPIO_LO (AF9013_GPIO_ON|AF9013_GPIO_EN);
#define AF9013_GPIO_HI (AF9013_GPIO_ON|AF9013_GPIO_EN|AF9013_GPIO_O);
#define AF9013_GPIO_TUNER_ON  (AF9013_GPIO_ON|AF9013_GPIO_EN);
#define AF9013_GPIO_TUNER_OFF (AF9013_GPIO_ON|AF9013_GPIO_EN|AF9013_GPIO_O);
    u8 gpio[4];
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
    struct i2c_adapter* (*get_i2c_adapter)(struct i2c_client *);
    int (*pid_filter_ctrl)(struct dvb_frontend *, int);
    int (*pid_filter)(struct dvb_frontend *, u8, u16, int);
};
```

**Members**

`clk`
:   Clock frequency.

`tuner`
:   Used tuner model.

`if_frequency`
:   IF frequency.

`ts_mode`
:   TS mode.

`ts_output_pin`
:   TS output pin.

`spec_inv`
:   Input spectrum inverted.

`api_version`
:   Firmware API version.

`gpio`
:   GPIOs.

`get_dvb_frontend`
:   Get DVB frontend callback.

`get_i2c_adapter`
:   Get I2C adapter.

`pid_filter_ctrl`
:   Control PID filter.

`pid_filter`
:   Set PID to PID filter.

struct ascot2e\_config
:   the configuration of Ascot2E tuner driver

**Definition**:

```
struct ascot2e_config {
    u8 i2c_address;
    u8 xtal_freq_mhz;
    void *set_tuner_priv;
    int (*set_tuner_callback)(void *, int);
};
```

**Members**

`i2c_address`
:   I2C address of the tuner

`xtal_freq_mhz`
:   Oscillator frequency, MHz

`set_tuner_priv`
:   Callback function private context

`set_tuner_callback`
:   Callback function that notifies the parent driver
    which tuner is active now

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*ascot2e\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct [ascot2e\_config](#c.ascot2e_config "ascot2e_config") \*config, struct i2c\_adapter \*i2c)
:   Attach an ascot2e tuner

**Parameters**

`struct dvb_frontend *fe`
:   frontend to be attached

`const struct ascot2e_config *config`
:   pointer to [`struct ascot2e_config`](#c.ascot2e_config "ascot2e_config") with tuner configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct cxd2820r\_platform\_data
:   Platform data for the cxd2820r driver

**Definition**:

```
struct cxd2820r_platform_data {
    u8 ts_mode;
    bool ts_clk_inv;
    bool if_agc_polarity;
    bool spec_inv;
    int **gpio_chip_base;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
};
```

**Members**

`ts_mode`
:   TS mode.

`ts_clk_inv`
:   TS clock inverted.

`if_agc_polarity`
:   IF AGC polarity.

`spec_inv`
:   Input spectrum inverted.

`gpio_chip_base`
:   GPIO.

`get_dvb_frontend`
:   Get DVB frontend.

struct cxd2820r\_config
:   configuration for cxd2020r demod

**Definition**:

```
struct cxd2820r_config {
    u8 i2c_address;
    u8 ts_mode;
    bool ts_clock_inv;
    bool if_agc_polarity;
    bool spec_inv;
};
```

**Members**

`i2c_address`
:   Demodulator I2C address. Driver determines DVB-C slave I2C
    address automatically from master address.
    Default: none, must set. Values: 0x6c, 0x6d.

`ts_mode`
:   TS output mode. Default: none, must set. Values: FIXME?

`ts_clock_inv`
:   TS clock inverted. Default: 0. Values: 0, 1.

`if_agc_polarity`
:   Default: 0. Values: 0, 1

`spec_inv`
:   Spectrum inversion. Default: 0. Values: 0, 1.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*cxd2820r\_attach(const struct [cxd2820r\_config](#c.cxd2820r_config "cxd2820r_config") \*config, struct i2c\_adapter \*i2c, int \*gpio\_chip\_base)
:   Attach a cxd2820r demod

**Parameters**

`const struct cxd2820r_config *config`
:   pointer to [`struct cxd2820r_config`](#c.cxd2820r_config "cxd2820r_config") with demod configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

`int *gpio_chip_base`
:   if zero, disables GPIO setting. Otherwise, if
    CONFIG\_GPIOLIB is set dynamically allocate
    gpio base; if is not set, use its value to
    setup the GPIO pins.

**Return**

FE pointer on success, NULL on failure.

struct drxk\_config
:   Configure the initial parameters for DRX-K

**Definition**:

```
struct drxk_config {
    u8 adr;
    bool single_master;
    bool no_i2c_bridge;
    bool parallel_ts;
    bool dynamic_clk;
    bool enable_merr_cfg;
    bool antenna_dvbt;
    u16 antenna_gpio;
    u8 mpeg_out_clk_strength;
    int chunk_size;
    const char      *microcode_name;
    int qam_demod_parameter_count;
};
```

**Members**

`adr`
:   I2C address of the DRX-K

`single_master`
:   Device is on the single master mode

`no_i2c_bridge`
:   Don’t switch the I2C bridge to talk with tuner

`parallel_ts`
:   True means that the device uses parallel TS,
    Serial otherwise.

`dynamic_clk`
:   True means that the clock will be dynamically
    adjusted. Static clock otherwise.

`enable_merr_cfg`
:   Enable SIO\_PDR\_PERR\_CFG/SIO\_PDR\_MVAL\_CFG.

`antenna_dvbt`
:   GPIO bit for changing antenna to DVB-C. A value of 1
    means that 1=DVBC, 0 = DVBT. Zero means the opposite.

`antenna_gpio`
:   GPIO bit used to control the antenna

`mpeg_out_clk_strength`
:   DRXK Mpeg output clock drive strength.

`chunk_size`
:   maximum size for I2C messages

`microcode_name`
:   Name of the firmware file with the microcode

`qam_demod_parameter_count`
:   The number of parameters used for the command
    to set the demodulator parameters. All
    firmwares are using the 2-parameter command.
    An exception is the `drxk_a3.mc` firmware,
    which uses the 4-parameter command.
    A value of 0 (default) or lower indicates that
    the correct number of parameters will be
    automatically detected.

**Description**

On the `*_gpio` vars, bit 0 is UIO-1, bit 1 is UIO-2 and bit 2 is
UIO-3.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*drxk\_attach(const struct [drxk\_config](#c.drxk_config "drxk_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a drxk demod

**Parameters**

`const struct drxk_config *config`
:   pointer to [`struct drxk_config`](#c.drxk_config "drxk_config") with demod configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*dvb\_pll\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, int pll\_addr, struct i2c\_adapter \*i2c, unsigned int pll\_desc\_id)
:   Attach a dvb-pll to the supplied frontend structure.

**Parameters**

`struct dvb_frontend *fe`
:   Frontend to attach to.

`int pll_addr`
:   i2c address of the PLL (if used).

`struct i2c_adapter *i2c`
:   i2c adapter to use (set to NULL if not used).

`unsigned int pll_desc_id`
:   dvb\_pll\_desc to use.

**Return**

Frontend pointer on success, NULL on failure

struct helene\_config
:   the configuration of ‘Helene’ tuner driver

**Definition**:

```
struct helene_config {
    u8 i2c_address;
    u8 xtal_freq_mhz;
    void *set_tuner_priv;
    int (*set_tuner_callback)(void *, int);
    enum helene_xtal xtal;
    struct dvb_frontend *fe;
};
```

**Members**

`i2c_address`
:   I2C address of the tuner

`xtal_freq_mhz`
:   Oscillator frequency, MHz

`set_tuner_priv`
:   Callback function private context

`set_tuner_callback`
:   Callback function that notifies the parent driver
    which tuner is active now

`xtal`
:   Cristal frequency as described by `enum helene_xtal`

`fe`
:   Frontend for which connects this tuner

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*helene\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct [helene\_config](#c.helene_config "helene_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a helene tuner (terrestrial and cable standards)

**Parameters**

`struct dvb_frontend *fe`
:   frontend to be attached

`const struct helene_config *config`
:   pointer to [`struct helene_config`](#c.helene_config "helene_config") with tuner configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*helene\_attach\_s(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct [helene\_config](#c.helene_config "helene_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a helene tuner (satellite standards)

**Parameters**

`struct dvb_frontend *fe`
:   frontend to be attached

`const struct helene_config *config`
:   pointer to [`struct helene_config`](#c.helene_config "helene_config") with tuner configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct horus3a\_config
:   the configuration of Horus3A tuner driver

**Definition**:

```
struct horus3a_config {
    u8 i2c_address;
    u8 xtal_freq_mhz;
    void *set_tuner_priv;
    int (*set_tuner_callback)(void *, int);
};
```

**Members**

`i2c_address`
:   I2C address of the tuner

`xtal_freq_mhz`
:   Oscillator frequency, MHz

`set_tuner_priv`
:   Callback function private context

`set_tuner_callback`
:   Callback function that notifies the parent driver
    which tuner is active now

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*horus3a\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct [horus3a\_config](#c.horus3a_config "horus3a_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a horus3a tuner

**Parameters**

`struct dvb_frontend *fe`
:   frontend to be attached

`const struct horus3a_config *config`
:   pointer to [`struct helene_config`](#c.helene_config "helene_config") with tuner configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct ix2505v\_config
:   ix2505 attachment configuration

**Definition**:

```
struct ix2505v_config {
    u8 tuner_address;
    u8 tuner_gain;
    u8 tuner_chargepump;
    int min_delay_ms;
    u8 tuner_write_only;
};
```

**Members**

`tuner_address`
:   tuner address

`tuner_gain`
:   Baseband AMP gain control 0/1=0dB(default) 2=-2bB 3=-4dB

`tuner_chargepump`
:   Charge pump output +/- 0=120 1=260 2=555 3=1200(default)

`min_delay_ms`
:   delay after tune

`tuner_write_only`
:   disables reads

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*ix2505v\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct [ix2505v\_config](#c.ix2505v_config "ix2505v_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a ix2505v tuner to the supplied frontend structure.

**Parameters**

`struct dvb_frontend *fe`
:   Frontend to attach to.

`const struct ix2505v_config *config`
:   pointer to [`struct ix2505v_config`](#c.ix2505v_config "ix2505v_config")

`struct i2c_adapter *i2c`
:   pointer to `struct i2c_adapter`.

**Return**

FE pointer on success, NULL on failure.

enum m88ds3103\_ts\_mode
:   TS connection mode

**Constants**

`M88DS3103_TS_SERIAL`
:   TS output pin D0, normal

`M88DS3103_TS_SERIAL_D7`
:   TS output pin D7

`M88DS3103_TS_PARALLEL`
:   TS Parallel mode

`M88DS3103_TS_CI`
:   TS CI Mode

enum m88ds3103\_clock\_out

**Constants**

`M88DS3103_CLOCK_OUT_DISABLED`
:   Clock output is disabled

`M88DS3103_CLOCK_OUT_ENABLED`
:   Clock output is enabled with crystal
    clock.

`M88DS3103_CLOCK_OUT_ENABLED_DIV2`
:   Clock output is enabled with half
    crystal clock.

struct m88ds3103\_platform\_data
:   Platform data for the m88ds3103 driver

**Definition**:

```
struct m88ds3103_platform_data {
    u32 clk;
    u16 i2c_wr_max;
    enum m88ds3103_ts_mode ts_mode;
    u32 ts_clk;
    enum m88ds3103_clock_out clk_out;
    u8 ts_clk_pol:1;
    u8 spec_inv:1;
    u8 agc;
    u8 agc_inv:1;
    u8 envelope_mode:1;
    u8 lnb_hv_pol:1;
    u8 lnb_en_pol:1;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
    struct i2c_adapter* (*get_i2c_adapter)(struct i2c_client *);
};
```

**Members**

`clk`
:   Clock frequency.

`i2c_wr_max`
:   Max bytes I2C adapter can write at once.

`ts_mode`
:   TS mode.

`ts_clk`
:   TS clock (KHz).

`clk_out`
:   Clock output.

`ts_clk_pol`
:   TS clk polarity. 1-active at falling edge; 0-active at rising
    edge.

`spec_inv`
:   Input spectrum inversion.

`agc`
:   AGC configuration.

`agc_inv`
:   AGC polarity.

`envelope_mode`
:   DiSEqC envelope mode.

`lnb_hv_pol`
:   LNB H/V pin polarity. 0: pin high set to VOLTAGE\_18, pin low to
    set VOLTAGE\_13. 1: pin high set to VOLTAGE\_13, pin low to set VOLTAGE\_18.

`lnb_en_pol`
:   LNB enable pin polarity. 0: pin high to disable, pin low to
    enable. 1: pin high to enable, pin low to disable.

`get_dvb_frontend`
:   Get DVB frontend.

`get_i2c_adapter`
:   Get I2C adapter.

struct m88ds3103\_config
:   m88ds3102 configuration

**Definition**:

```
struct m88ds3103_config {
    u8 i2c_addr;
    u32 clock;
    u16 i2c_wr_max;
    u8 ts_mode;
    u32 ts_clk;
    u8 ts_clk_pol:1;
    u8 spec_inv:1;
    u8 agc_inv:1;
    u8 clock_out;
    u8 envelope_mode:1;
    u8 agc;
    u8 lnb_hv_pol:1;
    u8 lnb_en_pol:1;
};
```

**Members**

`i2c_addr`
:   I2C address. Default: none, must set. Example: 0x68, ...

`clock`
:   Device’s clock. Default: none, must set. Example: 27000000

`i2c_wr_max`
:   Max bytes I2C provider is asked to write at once.
    Default: none, must set. Example: 33, 65, ...

`ts_mode`
:   TS output mode, as defined by [`enum m88ds3103_ts_mode`](#c.m88ds3103_ts_mode "m88ds3103_ts_mode").
    Default: M88DS3103\_TS\_SERIAL.

`ts_clk`
:   TS clk in KHz. Default: 0.

`ts_clk_pol`
:   TS clk polarity.Default: 0.
    1-active at falling edge; 0-active at rising edge.

`spec_inv`
:   Spectrum inversion. Default: 0.

`agc_inv`
:   AGC polarity. Default: 0.

`clock_out`
:   Clock output, as defined by [`enum m88ds3103_clock_out`](#c.m88ds3103_clock_out "m88ds3103_clock_out").
    Default: M88DS3103\_CLOCK\_OUT\_DISABLED.

`envelope_mode`
:   DiSEqC envelope mode. Default: 0.

`agc`
:   AGC configuration. Default: none, must set.

`lnb_hv_pol`
:   LNB H/V pin polarity. Default: 0. Values:
    1: pin high set to VOLTAGE\_13, pin low to set VOLTAGE\_18;
    0: pin high set to VOLTAGE\_18, pin low to set VOLTAGE\_13.

`lnb_en_pol`
:   LNB enable pin polarity. Default: 0. Values:
    1: pin high to enable, pin low to disable;
    0: pin high to disable, pin low to enable.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*m88ds3103\_attach(const struct [m88ds3103\_config](#c.m88ds3103_config "m88ds3103_config") \*config, struct i2c\_adapter \*i2c, struct i2c\_adapter \*\*tuner\_i2c)
:   Attach a m88ds3103 demod

**Parameters**

`const struct m88ds3103_config *config`
:   pointer to [`struct m88ds3103_config`](#c.m88ds3103_config "m88ds3103_config") with demod configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

`struct i2c_adapter **tuner_i2c`
:   on success, returns the I2C adapter associated with
    m88ds3103 tuner.

**Return**

FE pointer on success, NULL on failure.

**Note**

Do not add new [`m88ds3103_attach()`](#c.m88ds3103_attach "m88ds3103_attach") users! Use I2C bindings instead.

struct mb86a20s\_config
:   Define the per-device attributes of the frontend

**Definition**:

```
struct mb86a20s_config {
    u32 fclk;
    u8 demod_address;
    bool is_serial;
};
```

**Members**

`fclk`
:   Clock frequency. If zero, assumes the default
    (32.57142 Mhz)

`demod_address`
:   the demodulator’s i2c address

`is_serial`
:   if true, TS is serial. Otherwise, TS is parallel

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*mb86a20s\_attach(const struct [mb86a20s\_config](#c.mb86a20s_config "mb86a20s_config") \*config, struct i2c\_adapter \*i2c)
:   Attach a mb86a20s demod

**Parameters**

`const struct mb86a20s_config *config`
:   pointer to [`struct mb86a20s_config`](#c.mb86a20s_config "mb86a20s_config") with demod configuration.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct mn88472\_config
:   Platform data for the mn88472 driver

**Definition**:

```
struct mn88472_config {
    unsigned int xtal;
#define MN88472_TS_MODE_SERIAL      0;
#define MN88472_TS_MODE_PARALLEL    1;
    int ts_mode;
#define MN88472_TS_CLK_FIXED        0;
#define MN88472_TS_CLK_VARIABLE     1;
    int ts_clock;
    u16 i2c_wr_max;
    struct dvb_frontend **fe;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
};
```

**Members**

`xtal`
:   Clock frequency.

`ts_mode`
:   TS mode.

`ts_clock`
:   TS clock config.

`i2c_wr_max`
:   Max number of bytes driver writes to I2C at once.

`fe`
:   pointer to a frontend pointer

`get_dvb_frontend`
:   Get DVB frontend callback.

struct rtl2830\_platform\_data
:   Platform data for the rtl2830 driver

**Definition**:

```
struct rtl2830_platform_data {
    u32 clk;
    bool spec_inv;
    u8 vtop;
    u8 krf;
    u8 agc_targ_val;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
    struct i2c_adapter* (*get_i2c_adapter)(struct i2c_client *);
    int (*pid_filter)(struct dvb_frontend *, u8, u16, int);
    int (*pid_filter_ctrl)(struct dvb_frontend *, int);
};
```

**Members**

`clk`
:   Clock frequency (4000000, 16000000, 25000000, 28800000).

`spec_inv`
:   Spectrum inversion.

`vtop`
:   AGC take-over point.

`krf`
:   AGC ratio.

`agc_targ_val`
:   AGC.

`get_dvb_frontend`
:   Get DVB frontend.

`get_i2c_adapter`
:   Get I2C adapter.

`pid_filter`
:   Set PID to PID filter.

`pid_filter_ctrl`
:   Control PID filter.

struct rtl2832\_platform\_data
:   Platform data for the rtl2832 driver

**Definition**:

```
struct rtl2832_platform_data {
    u32 clk;
#define RTL2832_TUNER_FC2580    0x21;
#define RTL2832_TUNER_TUA9001   0x24;
#define RTL2832_TUNER_FC0012    0x26;
#define RTL2832_TUNER_E4000     0x27;
#define RTL2832_TUNER_FC0013    0x29;
#define RTL2832_TUNER_R820T     0x2a;
#define RTL2832_TUNER_R828D     0x2b;
#define RTL2832_TUNER_SI2157    0x2c;
    u8 tuner;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
    struct i2c_adapter* (*get_i2c_adapter)(struct i2c_client *);
    int (*slave_ts_ctrl)(struct i2c_client *, bool);
    int (*pid_filter)(struct dvb_frontend *, u8, u16, int);
    int (*pid_filter_ctrl)(struct dvb_frontend *, int);
};
```

**Members**

`clk`
:   Clock frequency (4000000, 16000000, 25000000, 28800000).

`tuner`
:   Used tuner model.

`get_dvb_frontend`
:   Get DVB frontend.

`get_i2c_adapter`
:   Get I2C adapter.

`slave_ts_ctrl`
:   Control slave TS interface.

`pid_filter`
:   Set PID to PID filter.

`pid_filter_ctrl`
:   Control PID filter.

struct rtl2832\_sdr\_platform\_data
:   Platform data for the rtl2832\_sdr driver

**Definition**:

```
struct rtl2832_sdr_platform_data {
    u32 clk;
#define RTL2832_SDR_TUNER_FC2580    0x21;
#define RTL2832_SDR_TUNER_TUA9001   0x24;
#define RTL2832_SDR_TUNER_FC0012    0x26;
#define RTL2832_SDR_TUNER_E4000     0x27;
#define RTL2832_SDR_TUNER_FC0013    0x29;
#define RTL2832_SDR_TUNER_R820T     0x2a;
#define RTL2832_SDR_TUNER_R828D     0x2b;
    u8 tuner;
    struct regmap *regmap;
    struct dvb_frontend *dvb_frontend;
    struct v4l2_subdev *v4l2_subdev;
    struct dvb_usb_device *dvb_usb_device;
};
```

**Members**

`clk`
:   Clock frequency (4000000, 16000000, 25000000, 28800000).

`tuner`
:   Used tuner model.

`regmap`
:   pointer to `struct regmap`.

`dvb_frontend`
:   rtl2832 DVB frontend.

`v4l2_subdev`
:   Tuner v4l2 controls.

`dvb_usb_device`
:   DVB USB interface for USB streaming.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*stb6000\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, int addr, struct i2c\_adapter \*i2c)
:   Attach a stb6000 tuner to the supplied frontend structure.

**Parameters**

`struct dvb_frontend *fe`
:   Frontend to attach to.

`int addr`
:   i2c address of the tuner.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

**Return**

FE pointer on success, NULL on failure.

struct tda10071\_platform\_data
:   Platform data for the tda10071 driver

**Definition**:

```
struct tda10071_platform_data {
    u32 clk;
    u16 i2c_wr_max;
#define TDA10071_TS_SERIAL        0;
#define TDA10071_TS_PARALLEL      1;
    u8 ts_mode;
    bool spec_inv;
    u8 pll_multiplier;
    u8 tuner_i2c_addr;
    struct dvb_frontend* (*get_dvb_frontend)(struct i2c_client *);
};
```

**Members**

`clk`
:   Clock frequency.

`i2c_wr_max`
:   Max bytes I2C adapter can write at once.

`ts_mode`
:   TS mode.

`spec_inv`
:   Input spectrum inversion.

`pll_multiplier`
:   PLL multiplier.

`tuner_i2c_addr`
:   CX24118A tuner I2C address (0x14, 0x54, ...).

`get_dvb_frontend`
:   Get DVB frontend.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*tda826x\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, int addr, struct i2c\_adapter \*i2c, int has\_loopthrough)
:   Attach a tda826x tuner to the supplied frontend structure.

**Parameters**

`struct dvb_frontend *fe`
:   Frontend to attach to.

`int addr`
:   i2c address of the tuner.

`struct i2c_adapter *i2c`
:   i2c adapter to use.

`int has_loopthrough`
:   Set to 1 if the card has a loopthrough RF connector.

**Return**

FE pointer on success, NULL on failure.

struct zd1301\_demod\_platform\_data
:   Platform data for the zd1301\_demod driver

**Definition**:

```
struct zd1301_demod_platform_data {
    void *reg_priv;
    int (*reg_read)(void *, u16, u8 *);
    int (*reg_write)(void *, u16, u8);
};
```

**Members**

`reg_priv`
:   First argument of reg\_read and reg\_write callbacks.

`reg_read`
:   Register read callback.

`reg_write`
:   Register write callback.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*zd1301\_demod\_get\_dvb\_frontend(struct platform\_device \*pdev)
:   Get pointer to DVB frontend

**Parameters**

`struct platform_device *pdev`
:   Pointer to platform device

**Return**

Pointer to DVB frontend which given platform device owns.

struct i2c\_adapter \*zd1301\_demod\_get\_i2c\_adapter(struct platform\_device \*pdev)
:   Get pointer to I2C adapter

**Parameters**

`struct platform_device *pdev`
:   Pointer to platform device

**Return**

Pointer to I2C adapter which given platform device owns.

struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*zl10036\_attach(struct [dvb\_frontend](../dtv-frontend.html#c.dvb_frontend "dvb_frontend") \*fe, const struct zl10036\_config \*config, struct i2c\_adapter \*i2c)
:   Attach a zl10036 tuner to the supplied frontend structure.

**Parameters**

`struct dvb_frontend *fe`
:   Frontend to attach to.

`const struct zl10036_config *config`
:   zl10036\_config structure.

`struct i2c_adapter *i2c`
:   pointer to `struct i2c_adapter`.

**Return**

FE pointer on success, NULL on failure.
