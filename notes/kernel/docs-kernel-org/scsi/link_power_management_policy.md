# Link Power Managent Policy

> 출처(원문): https://docs.kernel.org/scsi/link_power_management_policy.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Link Power Managent Policy

This parameter allows the user to set the link (interface) power management.
There are 6 possible options:

| Value | Effect |
| --- | --- |
| min\_power | Enable slumber mode(no partial mode) for the link to use the least possible power when possible. This may sacrifice some performance due to increased latency when coming out of lower power states. |
| max\_performance | Generally, this means no power management. Tell the controller to have performance be a priority over power management. |
| medium\_power | Tell the controller to enter a lower power state when possible, but do not enter the lowest power state, thus improving latency over min\_power setting. |
| keep\_firmware\_settings | Do not change the current firmware settings for Power management. This is the default setting. |
| med\_power\_with\_dipm | Same as medium\_power, but additionally with Device-initiated power management(DIPM) enabled, as Intel Rapid Storage Technology(IRST) does. |
| min\_power\_with\_partial | Same as min\_power, but additionally with partial power state enabled, which may improve performance over min\_power setting. |
