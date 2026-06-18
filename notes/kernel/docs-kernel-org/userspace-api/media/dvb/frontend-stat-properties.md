# 2.3.2.Frontend statistics indicators

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend-stat-properties.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3.2. Frontend statistics indicators

The values are returned via `dtv_property.stat`. If the property is
supported, `dtv_property.stat.len` is bigger than zero.

For most delivery systems, `dtv_property.stat.len` will be 1 if the
stats is supported, and the properties will return a single value for
each parameter.

It should be noted, however, that new OFDM delivery systems like ISDB
can use different modulation types for each group of carriers. On such
standards, up to 3 groups of statistics can be provided, and
`dtv_property.stat.len` is updated to reflect the “global” metrics,
plus one metric per each carrier group (called “layer” on ISDB).

So, in order to be consistent with other delivery systems, the first
value at [`dtv_property.stat.dtv_stats`](frontend-header.html#c.dtv_stats "dtv_stats") array refers
to the global metric. The other elements of the array represent each
layer, starting from layer A(index 1), layer B (index 2) and so on.

The number of filled elements are stored at `dtv_property.stat.len`.

Each element of the `dtv_property.stat.dtv_stats` array consists on
two elements:

* `svalue` or `uvalue`, where `svalue` is for signed values of
  the measure (dB measures) and `uvalue` is for unsigned values
  (counters, relative scale)
* `scale` - Scale for the value. It can be:

  + `FE_SCALE_NOT_AVAILABLE` - The parameter is supported by the
    frontend, but it was not possible to collect it (could be a
    transitory or permanent condition)
  + `FE_SCALE_DECIBEL` - parameter is a signed value, measured in
    1/1000 dB
  + `FE_SCALE_RELATIVE` - parameter is a unsigned value, where 0
    means 0% and 65535 means 100%.
  + `FE_SCALE_COUNTER` - parameter is a unsigned value that counts
    the occurrence of an event, like bit error, block error, or lapsed
    time.

## 2.3.2.1. DTV\_STAT\_SIGNAL\_STRENGTH

Indicates the signal strength level at the analog part of the tuner or
of the demod.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_DECIBEL` - signal strength is in 0.001 dBm units, power
  measured in miliwatts. This value is generally negative.
* `FE_SCALE_RELATIVE` - The frontend provides a 0% to 100%
  measurement for power (actually, 0 to 65535).

## 2.3.2.2. DTV\_STAT\_CNR

Indicates the Signal to Noise ratio for the main carrier.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_DECIBEL` - Signal/Noise ratio is in 0.001 dB units.
* `FE_SCALE_RELATIVE` - The frontend provides a 0% to 100%
  measurement for Signal/Noise (actually, 0 to 65535).

## 2.3.2.3. DTV\_STAT\_PRE\_ERROR\_BIT\_COUNT

Measures the number of bit errors before the forward error correction
(FEC) on the inner coding block (before Viterbi, LDPC or other inner
code).

This measure is taken during the same interval as
`DTV_STAT_PRE_TOTAL_BIT_COUNT`.

In order to get the BER (Bit Error Rate) measurement, it should be
divided by
[DTV\_STAT\_PRE\_TOTAL\_BIT\_COUNT](#dtv-stat-pre-total-bit-count).

This measurement is monotonically increased, as the frontend gets more
bit count measurements. The frontend may reset it when a
channel/transponder is tuned.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of error bits counted before the inner
  coding.

## 2.3.2.4. DTV\_STAT\_PRE\_TOTAL\_BIT\_COUNT

Measures the amount of bits received before the inner code block, during
the same period as
[DTV\_STAT\_PRE\_ERROR\_BIT\_COUNT](#dtv-stat-pre-error-bit-count)
measurement was taken.

It should be noted that this measurement can be smaller than the total
amount of bits on the transport stream, as the frontend may need to
manually restart the measurement, losing some data between each
measurement interval.

This measurement is monotonically increased, as the frontend gets more
bit count measurements. The frontend may reset it when a
channel/transponder is tuned.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of bits counted while measuring
  [DTV\_STAT\_PRE\_ERROR\_BIT\_COUNT](#dtv-stat-pre-error-bit-count).

## 2.3.2.5. DTV\_STAT\_POST\_ERROR\_BIT\_COUNT

Measures the number of bit errors after the forward error correction
(FEC) done by inner code block (after Viterbi, LDPC or other inner
code).

This measure is taken during the same interval as
`DTV_STAT_POST_TOTAL_BIT_COUNT`.

In order to get the BER (Bit Error Rate) measurement, it should be
divided by
[DTV\_STAT\_POST\_TOTAL\_BIT\_COUNT](#dtv-stat-post-total-bit-count).

This measurement is monotonically increased, as the frontend gets more
bit count measurements. The frontend may reset it when a
channel/transponder is tuned.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of error bits counted after the inner
  coding.

## 2.3.2.6. DTV\_STAT\_POST\_TOTAL\_BIT\_COUNT

Measures the amount of bits received after the inner coding, during the
same period as
[DTV\_STAT\_POST\_ERROR\_BIT\_COUNT](#dtv-stat-post-error-bit-count)
measurement was taken.

It should be noted that this measurement can be smaller than the total
amount of bits on the transport stream, as the frontend may need to
manually restart the measurement, losing some data between each
measurement interval.

This measurement is monotonically increased, as the frontend gets more
bit count measurements. The frontend may reset it when a
channel/transponder is tuned.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of bits counted while measuring
  [DTV\_STAT\_POST\_ERROR\_BIT\_COUNT](#dtv-stat-post-error-bit-count).

## 2.3.2.7. DTV\_STAT\_ERROR\_BLOCK\_COUNT

Measures the number of block errors after the outer forward error
correction coding (after Reed-Solomon or other outer code).

This measurement is monotonically increased, as the frontend gets more
bit count measurements. The frontend may reset it when a
channel/transponder is tuned.

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of error blocks counted after the outer
  coding.

## 2.3.2.8. DTV-STAT\_TOTAL\_BLOCK\_COUNT

Measures the total number of blocks received during the same period as
[DTV\_STAT\_ERROR\_BLOCK\_COUNT](#dtv-stat-error-block-count)
measurement was taken.

It can be used to calculate the PER indicator, by dividing
[DTV\_STAT\_ERROR\_BLOCK\_COUNT](#dtv-stat-error-block-count) by
[DTV-STAT\_TOTAL\_BLOCK\_COUNT](#dtv-stat-total-block-count).

Possible scales for this metric are:

* `FE_SCALE_NOT_AVAILABLE` - it failed to measure it, or the
  measurement was not complete yet.
* `FE_SCALE_COUNTER` - Number of blocks counted while measuring
  [DTV\_STAT\_ERROR\_BLOCK\_COUNT](#dtv-stat-error-block-count).
