# CPU and Device Power Management

> 출처(원문): https://docs.kernel.org/driver-api/pm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CPU and Device Power Management

* [CPU Idle Time Management](cpuidle.html)
  + [CPU Idle Time Management Subsystem](cpuidle.html#cpu-idle-time-management-subsystem)
  + [CPU Idle Time Governors](cpuidle.html#cpu-idle-time-governors)
  + [CPU Idle Time Management Drivers](cpuidle.html#cpu-idle-time-management-drivers)
* [Device Power Management Basics](devices.html)
  + [Two Models for Device Power Management](devices.html#two-models-for-device-power-management)
  + [Interfaces for Entering System Sleep States](devices.html#interfaces-for-entering-system-sleep-states)
    - [Device Power Management Operations](devices.html#device-power-management-operations)
    - [Subsystem-Level Methods](devices.html#subsystem-level-methods)
    - [`/sys/devices/.../power/wakeup` files](devices.html#sys-devices-power-wakeup-files)
    - [`/sys/devices/.../power/control` files](devices.html#sys-devices-power-control-files)
  + [Calling Drivers to Enter and Leave System Sleep States](devices.html#calling-drivers-to-enter-and-leave-system-sleep-states)
    - [Call Sequence Guarantees](devices.html#call-sequence-guarantees)
    - [System Power Management Phases](devices.html#system-power-management-phases)
    - [Entering System Suspend](devices.html#entering-system-suspend)
    - [Leaving System Suspend](devices.html#leaving-system-suspend)
    - [Entering Hibernation](devices.html#entering-hibernation)
    - [Leaving Hibernation](devices.html#leaving-hibernation)
  + [Power Management Notifiers](devices.html#power-management-notifiers)
  + [Device Low-Power (suspend) States](devices.html#device-low-power-suspend-states)
  + [Device Power Management Domains](devices.html#device-power-management-domains)
  + [Runtime Power Management](devices.html#runtime-power-management)
    - [The `DPM_FLAG_SMART_SUSPEND` Driver Flag](devices.html#the-dpm-flag-smart-suspend-driver-flag)
    - [The `DPM_FLAG_MAY_SKIP_RESUME` Driver Flag](devices.html#the-dpm-flag-may-skip-resume-driver-flag)
* [Suspend/Hibernation Notifiers](notifiers.html)
* [Device Power Management Data Types](types.html)
