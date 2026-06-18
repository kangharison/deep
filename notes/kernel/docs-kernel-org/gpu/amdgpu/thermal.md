# GPU Power/Thermal Controls and Monitoring

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/thermal.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPU Power/Thermal Controls and Monitoring

## HWMON Interfaces

The amdgpu driver exposes the following sensor interfaces:

* GPU temperature (via the on-die sensor)
* GPU voltage
* Northbridge voltage (APUs only)
* GPU power
* GPU fan
* GPU gfx/compute engine clock
* GPU memory clock (dGPU only)

hwmon interfaces for GPU temperature:

* temp[1-3]\_input: the on die GPU temperature in millidegrees Celsius
  - temp2\_input and temp3\_input are supported on SOC15 dGPUs only
* temp[1-3]\_label: temperature channel label
  - temp2\_label and temp3\_label are supported on SOC15 dGPUs only
* temp[1-3]\_crit: temperature critical max value in millidegrees Celsius
  - temp2\_crit and temp3\_crit are supported on SOC15 dGPUs only
* temp[1-3]\_crit\_hyst: temperature hysteresis for critical limit in millidegrees Celsius
  - temp2\_crit\_hyst and temp3\_crit\_hyst are supported on SOC15 dGPUs only
* temp[1-3]\_emergency: temperature emergency max value(asic shutdown) in millidegrees Celsius
  - these are supported on SOC15 dGPUs only

hwmon interfaces for GPU voltage:

* in0\_input: the voltage on the GPU in millivolts
* in1\_input: the voltage on the Northbridge in millivolts

hwmon interfaces for GPU power:

* power1\_average: average power used by the SoC in microWatts. On APUs this includes the CPU.
* power1\_input: instantaneous power used by the SoC in microWatts. On APUs this includes the CPU.
* power1\_cap\_min: minimum cap supported in microWatts
* power1\_cap\_max: maximum cap supported in microWatts
* power1\_cap: selected power cap in microWatts

hwmon interfaces for GPU fan:

* pwm1: pulse width modulation fan level (0-255)
* pwm1\_enable: pulse width modulation fan control method (0: no fan speed control, 1: manual fan speed control using pwm interface, 2: automatic fan speed control)
* pwm1\_min: pulse width modulation fan control minimum level (0)
* pwm1\_max: pulse width modulation fan control maximum level (255)
* fan1\_min: a minimum value Unit: revolution/min (RPM)
* fan1\_max: a maximum value Unit: revolution/max (RPM)
* fan1\_input: fan speed in RPM
* fan[1-\*]\_target: Desired fan speed Unit: revolution/min (RPM)
* fan[1-\*]\_enable: Enable or disable the sensors.1: Enable 0: Disable

NOTE: DO NOT set the fan speed via “pwm1” and “fan[1-\*]\_target” interfaces at the same time.
:   That will get the former one overridden.

hwmon interfaces for GPU clocks:

* freq1\_input: the gfx/compute clock in hertz
* freq2\_input: the memory clock in hertz

You can use hwmon tools like sensors to view this information on your system.

## GPU sysfs Power State Interfaces

GPU power controls are exposed via sysfs files.

### power\_dpm\_state

The power\_dpm\_state file is a legacy interface and is only provided for
backwards compatibility. The amdgpu driver provides a sysfs API for adjusting
certain power related parameters. The file power\_dpm\_state is used for this.
It accepts the following arguments:

* battery
* balanced
* performance

battery

On older GPUs, the vbios provided a special power state for battery
operation. Selecting battery switched to this state. This is no
longer provided on newer GPUs so the option does nothing in that case.

balanced

On older GPUs, the vbios provided a special power state for balanced
operation. Selecting balanced switched to this state. This is no
longer provided on newer GPUs so the option does nothing in that case.

performance

On older GPUs, the vbios provided a special power state for performance
operation. Selecting performance switched to this state. This is no
longer provided on newer GPUs so the option does nothing in that case.

### power\_dpm\_force\_performance\_level

The amdgpu driver provides a sysfs API for adjusting certain power
related parameters. The file power\_dpm\_force\_performance\_level is
used for this. It accepts the following arguments:

* auto
* low
* high
* manual
* profile\_standard
* profile\_min\_sclk
* profile\_min\_mclk
* profile\_peak

auto

When auto is selected, the driver will attempt to dynamically select
the optimal power profile for current conditions in the driver.

low

When low is selected, the clocks are forced to the lowest power state.

high

When high is selected, the clocks are forced to the highest power state.

manual

When manual is selected, the user can manually adjust which power states
are enabled for each clock domain via the sysfs pp\_dpm\_mclk, pp\_dpm\_sclk,
and pp\_dpm\_pcie files and adjust the power state transition heuristics
via the pp\_power\_profile\_mode sysfs file.

profile\_standard
profile\_min\_sclk
profile\_min\_mclk
profile\_peak

When the profiling modes are selected, clock and power gating are
disabled and the clocks are set for different profiling cases. This
mode is recommended for profiling specific work loads where you do
not want clock or power gating for clock fluctuation to interfere
with your results. profile\_standard sets the clocks to a fixed clock
level which varies from asic to asic. profile\_min\_sclk forces the sclk
to the lowest level. profile\_min\_mclk forces the mclk to the lowest level.
profile\_peak sets all clocks (mclk, sclk, pcie) to the highest levels.

### pp\_table

The amdgpu driver provides a sysfs API for uploading new powerplay
tables. The file pp\_table is used for this. Reading the file
will dump the current power play table. Writing to the file
will attempt to upload a new powerplay table and re-initialize
powerplay using that new table.

### pp\_od\_clk\_voltage

The amdgpu driver provides a sysfs API for adjusting the clocks and voltages
in each power level within a power state. The pp\_od\_clk\_voltage is used for
this.

Note that the actual memory controller clock rate are exposed, not
the effective memory clock of the DRAMs. To translate it, use the
following formula:

Clock conversion (Mhz):

HBM: effective\_memory\_clock = memory\_controller\_clock \* 1

G5: effective\_memory\_clock = memory\_controller\_clock \* 1

G6: effective\_memory\_clock = memory\_controller\_clock \* 2

DRAM data rate (MT/s):

HBM: effective\_memory\_clock \* 2 = data\_rate

G5: effective\_memory\_clock \* 4 = data\_rate

G6: effective\_memory\_clock \* 8 = data\_rate

Bandwidth (MB/s):

data\_rate \* vram\_bit\_width / 8 = memory\_bandwidth

Some examples:

G5 on RX460:

memory\_controller\_clock = 1750 Mhz

effective\_memory\_clock = 1750 Mhz \* 1 = 1750 Mhz

data rate = 1750 \* 4 = 7000 MT/s

memory\_bandwidth = 7000 \* 128 bits / 8 = 112000 MB/s

G6 on RX5700:

memory\_controller\_clock = 875 Mhz

effective\_memory\_clock = 875 Mhz \* 2 = 1750 Mhz

data rate = 1750 \* 8 = 14000 MT/s

memory\_bandwidth = 14000 \* 256 bits / 8 = 448000 MB/s

< For Vega10 and previous ASICs >

Reading the file will display:

* a list of engine clock levels and voltages labeled OD\_SCLK
* a list of memory clock levels and voltages labeled OD\_MCLK
* a list of valid ranges for sclk, mclk, and voltage labeled OD\_RANGE

To manually adjust these settings, first select manual using
power\_dpm\_force\_performance\_level. Enter a new value for each
level by writing a string that contains “s/m level clock voltage” to
the file. E.g., “s 1 500 820” will update sclk level 1 to be 500 MHz
at 820 mV; “m 0 350 810” will update mclk level 0 to be 350 MHz at
810 mV. When you have edited all of the states as needed, write
“c” (commit) to the file to commit your changes. If you want to reset to the
default power levels, write “r” (reset) to the file to reset them.

< For Vega20 and newer ASICs >

Reading the file will display:

* minimum and maximum engine clock labeled OD\_SCLK
* minimum(not available for Vega20 and Navi1x) and maximum memory
  clock labeled OD\_MCLK
* minimum and maximum fabric clock labeled OD\_FCLK (SMU13)
* three <frequency, voltage> points labeled OD\_VDDC\_CURVE.
  They can be used to calibrate the sclk voltage curve. This is
  available for Vega20 and NV1X.
* voltage offset(in mV) applied on target voltage calculation.
  This is available for Sienna Cichlid, Navy Flounder, Dimgrey
  Cavefish and some later SMU13 ASICs. For these ASICs, the target
  voltage calculation can be illustrated by “voltage = voltage
  calculated from v/f curve + overdrive vddgfx offset”
* a list of valid ranges for sclk, mclk, voltage curve points
  or voltage offset labeled OD\_RANGE

< For APUs >

Reading the file will display:

* minimum and maximum engine clock labeled OD\_SCLK
* a list of valid ranges for sclk labeled OD\_RANGE

< For VanGogh >

Reading the file will display:

* minimum and maximum engine clock labeled OD\_SCLK
* minimum and maximum core clocks labeled OD\_CCLK
* a list of valid ranges for sclk and cclk labeled OD\_RANGE

To manually adjust these settings:

* First select manual using power\_dpm\_force\_performance\_level
* For clock frequency setting, enter a new value by writing a
  string that contains “s/m/f index clock” to the file. The index
  should be 0 if to set minimum clock. And 1 if to set maximum
  clock. E.g., “s 0 500” will update minimum sclk to be 500 MHz.
  “m 1 800” will update maximum mclk to be 800Mhz. “f 1 1600” will
  update maximum fabric clock to be 1600Mhz. For core
  clocks on VanGogh, the string contains “p core index clock”.
  E.g., “p 2 0 800” would set the minimum core clock on core
  2 to 800Mhz.

  For sclk voltage curve supported by Vega20 and NV1X, enter the new
  values by writing a string that contains “vc point clock voltage”
  to the file. The points are indexed by 0, 1 and 2. E.g., “vc 0 300
  600” will update point1 with clock set as 300Mhz and voltage as 600mV.
  “vc 2 1000 1000” will update point3 with clock set as 1000Mhz and
  voltage 1000mV.

  For voltage offset supported by Sienna Cichlid, Navy Flounder, Dimgrey
  Cavefish and some later SMU13 ASICs, enter the new value by writing a
  string that contains “vo offset”. E.g., “vo -10” will update the extra
  voltage offset applied to the whole v/f curve line as -10mv.
* When you have edited all of the states as needed, write “c” (commit)
  to the file to commit your changes
* If you want to reset to the default power levels, write “r” (reset)
  to the file to reset them

### pp\_dpm\_\*

The amdgpu driver provides a sysfs API for adjusting what power levels
are enabled for a given power state. The files pp\_dpm\_sclk, pp\_dpm\_mclk,
pp\_dpm\_socclk, pp\_dpm\_fclk, pp\_dpm\_dcefclk and pp\_dpm\_pcie are used for
this.

pp\_dpm\_socclk and pp\_dpm\_dcefclk interfaces are only available for
Vega10 and later ASICs.
pp\_dpm\_fclk interface is only available for Vega20 and later ASICs.

Reading back the files will show you the available power levels within
the power state and the clock information for those levels. If deep sleep is
applied to a clock, the level will be denoted by a special level ‘S:’
E.g.,

```
S: 19Mhz *
0: 615Mhz
1: 800Mhz
2: 888Mhz
3: 1000Mhz
```

To manually adjust these states, first select manual using
power\_dpm\_force\_performance\_level.
Secondly, enter a new value for each level by inputing a string that
contains “ echo xx xx xx > pp\_dpm\_sclk/mclk/pcie”
E.g.,

```
echo "4 5 6" > pp_dpm_sclk
```

will enable sclk levels 4, 5, and 6.

NOTE: change to the dcefclk max dpm level is not supported now

### pp\_power\_profile\_mode

The amdgpu driver provides a sysfs API for adjusting the heuristics
related to switching between power levels in a power state. The file
pp\_power\_profile\_mode is used for this.

Reading this file outputs a list of all of the predefined power profiles
and the relevant heuristics settings for that profile.

To select a profile or create a custom profile, first select manual using
power\_dpm\_force\_performance\_level. Writing the number of a predefined
profile to pp\_power\_profile\_mode will enable those heuristics. To
create a custom set of heuristics, write a string of numbers to the file
starting with the number of the custom profile along with a setting
for each heuristic parameter. Due to differences across asic families
the heuristic parameters vary from family to family. Additionally,
you can apply the custom heuristics to different clock domains. Each
clock domain is considered a distinct operation so if you modify the
gfxclk heuristics and then the memclk heuristics, the all of the
custom heuristics will be retained until you switch to another profile.

### pm\_policy

Certain SOCs can support different power policies to optimize application
performance. However, this policy is provided only at SOC level and not at a
per-process level. This is useful especially when entire SOC is utilized for
dedicated workload.

The amdgpu driver provides a sysfs API for selecting the policy. Presently,
only two types of policies are supported through this interface.

> Pstate Policy Selection - This is to select different Pstate profiles which
> decides clock/throttling preferences.
>
> XGMI PLPD Policy Selection - When multiple devices are connected over XGMI,
> this helps to select policy to be applied for per link power down.

The list of available policies and policy levels vary between SOCs. They can
be viewed under pm\_policy node directory. If SOC doesn’t support any policy,
this node won’t be available. The different policies supported will be
available as separate nodes under pm\_policy.

> cat /sys/bus/pci/devices/.../pm\_policy/<policy\_type>

Reading the policy file shows the different levels supported. The level which
is applied presently is denoted by \* (asterisk). E.g.,

```
cat /sys/bus/pci/devices/.../pm_policy/soc_pstate
0 : soc_pstate_default
1 : soc_pstate_0
2 : soc_pstate_1*
3 : soc_pstate_2

cat /sys/bus/pci/devices/.../pm_policy/xgmi_plpd
0 : plpd_disallow
1 : plpd_default
2 : plpd_optimized*
```

To apply a specific policy

“echo <level> > /sys/bus/pci/devices/.../pm\_policy/<policy\_type>”

For the levels listed in the example above, to select “plpd\_optimized” for
XGMI and “soc\_pstate\_2” for soc pstate policy -

```
echo "2" > /sys/bus/pci/devices/.../pm_policy/xgmi_plpd
echo "3" > /sys/bus/pci/devices/.../pm_policy/soc_pstate
```

### \*\_busy\_percent

The amdgpu driver provides a sysfs API for reading how busy the GPU
is as a percentage. The file gpu\_busy\_percent is used for this.
The SMU firmware computes a percentage of load based on the
aggregate activity level in the IP cores.

The amdgpu driver provides a sysfs API for reading how busy the VRAM
is as a percentage. The file mem\_busy\_percent is used for this.
The SMU firmware computes a percentage of load based on the
aggregate activity level in the IP cores.

### gpu\_metrics

The amdgpu driver provides a sysfs API for retrieving current gpu
metrics data. The file gpu\_metrics is used for this. Reading the
file will dump all the current gpu metrics data.

These data include temperature, frequency, engines utilization,
power consume, throttler status, fan speed and cpu core statistics(
available for APU only). That’s it will give a snapshot of all sensors
at the same time.

### fan\_curve

The amdgpu driver provides a sysfs API for checking and adjusting the fan
control curve line.

Reading back the file shows you the current settings(temperature in Celsius
degree and fan speed in pwm) applied to every anchor point of the curve line
and their permitted ranges if changable.

Writing a desired string(with the format like “anchor\_point\_index temperature
fan\_speed\_in\_pwm”) to the file, change the settings for the specific anchor
point accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them

There are two fan control modes supported: auto and manual. With auto mode,
PMFW handles the fan speed control(how fan speed reacts to ASIC temperature).
While with manual mode, users can set their own fan curve line as what
described here. Normally the ASIC is booted up with auto mode. Any
settings via this interface will switch the fan control to manual mode
implicitly.

### acoustic\_limit\_rpm\_threshold

The amdgpu driver provides a sysfs API for checking and adjusting the
acoustic limit in RPM for fan control.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them

This setting works under auto fan control mode only. It adjusts the PMFW’s
behavior about the maximum speed in RPM the fan can spin. Setting via this
interface will switch the fan control to auto mode implicitly.

### acoustic\_target\_rpm\_threshold

The amdgpu driver provides a sysfs API for checking and adjusting the
acoustic target in RPM for fan control.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them

This setting works under auto fan control mode only. It can co-exist with
other settings which can work also under auto mode. It adjusts the PMFW’s
behavior about the maximum speed in RPM the fan can spin when ASIC
temperature is not greater than target temperature. Setting via this
interface will switch the fan control to auto mode implicitly.

### fan\_target\_temperature

The amdgpu driver provides a sysfs API for checking and adjusting the
target tempeature in Celsius degree for fan control.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them

This setting works under auto fan control mode only. It can co-exist with
other settings which can work also under auto mode. Paring with the
acoustic\_target\_rpm\_threshold setting, they define the maximum speed in
RPM the fan can spin when ASIC temperature is not greater than target
temperature. Setting via this interface will switch the fan control to
auto mode implicitly.

### fan\_minimum\_pwm

The amdgpu driver provides a sysfs API for checking and adjusting the
minimum fan speed in PWM.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them

This setting works under auto fan control mode only. It can co-exist with
other settings which can work also under auto mode. It adjusts the PMFW’s
behavior about the minimum fan speed in PWM the fan should spin. Setting
via this interface will switch the fan control to auto mode implicitly.

### fan\_zero\_rpm\_enable

The amdgpu driver provides a sysfs API for checking and adjusting the
zero RPM feature.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them.

### fan\_zero\_rpm\_stop\_temperature

The amdgpu driver provides a sysfs API for checking and adjusting the
zero RPM stop temperature feature.

Reading back the file shows you the current setting and the permitted
ranges if changable.

Writing an integer to the file, change the setting accordingly.

When you have finished the editing, write “c” (commit) to the file to commit
your changes.

If you want to reset to the default value, write “r” (reset) to the file to
reset them.

This setting works only if the Zero RPM setting is enabled. It adjusts the
temperature below which the fan can stop.

## GFXOFF

GFXOFF is a feature found in most recent GPUs that saves power at runtime. The
card’s RLC (RunList Controller) firmware powers off the gfx engine
dynamically when there is no workload on gfx or compute pipes. GFXOFF is on by
default on supported GPUs.

Userspace can interact with GFXOFF through a debugfs interface (all values in
uint32\_t, unless otherwise noted):

### `amdgpu_gfxoff`

Use it to enable/disable GFXOFF, and to check if it’s current enabled/disabled:

```
$ xxd -l1 -p /sys/kernel/debug/dri/0/amdgpu_gfxoff
01
```

* Write 0 to disable it, and 1 to enable it.
* Read 0 means it’s disabled, 1 it’s enabled.

If it’s enabled, that means that the GPU is free to enter into GFXOFF mode as
needed. Disabled means that it will never enter GFXOFF mode.

### `amdgpu_gfxoff_status`

Read it to check current GFXOFF’s status of a GPU:

```
$ xxd -l1 -p /sys/kernel/debug/dri/0/amdgpu_gfxoff_status
02
```

* 0: GPU is in GFXOFF state, the gfx engine is powered down.
* 1: Transition out of GFXOFF state
* 2: Not in GFXOFF state
* 3: Transition into GFXOFF state

If GFXOFF is enabled, the value will be transitioning around [0, 3], always
getting into 0 when possible. When it’s disabled, it’s always at 2. Returns
`-EINVAL` if it’s not supported.

### `amdgpu_gfxoff_count`

Read it to get the total GFXOFF entry count at the time of query since system
power-up. The value is an uint64\_t type, however, due to firmware limitations,
it can currently overflow as an uint32\_t. *Only supported in vangogh*

### `amdgpu_gfxoff_residency`

Write 1 to amdgpu\_gfxoff\_residency to start logging, and 0 to stop. Read it to
get average GFXOFF residency % multiplied by 100 during the last logging
interval. E.g. a value of 7854 means 78.54% of the time in the last logging
interval the GPU was in GFXOFF mode. *Only supported in vangogh*
