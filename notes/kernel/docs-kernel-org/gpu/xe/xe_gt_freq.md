# Xe GT Frequency Management

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_gt_freq.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xe GT Frequency Management

This component is responsible for the raw GT frequency management, including
the sysfs API.

Underneath, Xe enables GuC SLPC automated frequency management. GuC is then
allowed to request PCODE any frequency between the Minimum and the Maximum
selected by this component. Furthermore, it is important to highlight that
PCODE is the ultimate decision maker of the actual running frequency, based
on thermal and other running conditions.

Xe’s Freq provides a sysfs API for frequency management under
`<device>/tile#/gt#/freq0/` directory.

**Read-only** attributes:

* `act_freq`: The actual resolved frequency decided by PCODE.
* `cur_freq`: The current one requested by GuC PC to the PCODE.
* `rpn_freq`: The Render Performance (RP) N level, which is the minimal one.
* `rpa_freq`: The Render Performance (RP) A level, which is the achievable one.
  :   Calculated by PCODE at runtime based on multiple running conditions
* `rpe_freq`: The Render Performance (RP) E level, which is the efficient one.
  :   Calculated by PCODE at runtime based on multiple running conditions
* `rp0_freq`: The Render Performance (RP) 0 level, which is the maximum one.

**Read-write** attributes:

* `min_freq`: Min frequency request.
* `max_freq`: Max frequency request.
  :   If max <= min, then freq\_min becomes a fixed frequency
      request.

The GT frequency may be throttled by hardware/firmware for various reasons
that are provided through attributes under the `freq0/throttle/` directory.
Their availability depend on the platform and some may not be visible if that
reason is not available.

The `reasons` attribute can be used by sysadmin to monitor all possible
reasons for throttling and report them. It’s preferred over monitoring
`status` and then reading the reason from individual attributes since that
is racy. If there’s no throttling happening, “none” is returned.

The following attributes are available on Crescent Island platform:

* `status`: Overall throttle status (0: no throttling, 1: throttling)
* `reasons`: Array of reasons causing throttling separated by space
* `reason_pl1`: package PL1
* `reason_pl2`: package PL2
* `reason_pl4`: package PL4
* `reason_prochot`: prochot
* `reason_soc_thermal`: SoC thermal
* `reason_mem_thermal`: Memory thermal
* `reason_vr_thermal`: VR thermal
* `reason_iccmax`: ICCMAX
* `reason_ratl`: RATL thermal algorithm
* `reason_soc_avg_thermal`: SoC average temp
* `reason_fastvmode`: VR is hitting FastVMode
* `reason_psys_pl1`: PSYS PL1
* `reason_psys_pl2`: PSYS PL2
* `reason_p0_freq`: P0 frequency
* `reason_psys_crit`: PSYS critical

Other platforms support the following reasons:

* `status`: Overall throttle status (0: no throttling, 1: throttling)
* `reasons`: Array of reasons causing throttling separated by space
* `reason_pl1`: package PL1
* `reason_pl2`: package PL2
* `reason_pl4`: package PL4, Iccmax etc.
* `reason_thermal`: thermal
* `reason_prochot`: prochot
* `reason_ratl`: RATL hermal algorithm
* `reason_vr_thermalert`: VR THERMALERT
* `reason_vr_tdc`: VR TDC

## Internal API

int xe\_gt\_freq\_init(struct xe\_gt \*gt)
:   Initialize Xe Freq component

**Parameters**

`struct xe_gt *gt`
:   Xe GT object

**Description**

It needs to be initialized after GT Sysfs and GuC PC components are ready.

**Return**

Returns error value for failure and 0 for success.
