# 12.N-Trig touchscreen Driver

> 출처(원문): https://docs.kernel.org/input/devices/ntrig.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 12. N-Trig touchscreen Driver

Copyright:
:   © 2008-2010 Rafi Rubin <[rafi@seas.upenn.edu](mailto:rafi%40seas.upenn.edu)>

Copyright:
:   © 2009-2010 Stephane Chatty

This driver provides support for N-Trig pen and multi-touch sensors. Single
and multi-touch events are translated to the appropriate protocols for
the hid and input systems. Pen events are sufficiently hid compliant and
are left to the hid core. The driver also provides additional filtering
and utility functions accessible with sysfs and module parameters.

This driver has been reported to work properly with multiple N-Trig devices
attached.

## 12.1. Parameters

Note: values set at load time are global and will apply to all applicable
devices. Adjusting parameters with sysfs will override the load time values,
but only for that one device.

The following parameters are used to configure filters to reduce noise:

|  |  |
| --- | --- |
| activate\_slack | number of fingers to ignore before processing events |
| activation\_height, activation\_width | size threshold to activate immediately |
| min\_height, min\_width | size threshold below which fingers are ignored both to decide activation and during activity |
| deactivate\_slack | the number of “no contact” frames to ignore before propagating the end of activity events |

When the last finger is removed from the device, it sends a number of empty
frames. By holding off on deactivation for a few frames we can tolerate false
erroneous disconnects, where the sensor may mistakenly not detect a finger that
is still present. Thus deactivate\_slack addresses problems where a users might
see breaks in lines during drawing, or drop an object during a long drag.

## 12.2. Additional sysfs items

These nodes just provide easy access to the ranges reported by the device.

|  |  |
| --- | --- |
| sensor\_logical\_height, sensor\_logical\_width | the range for positions reported during activity |
| sensor\_physical\_height, sensor\_physical\_width | internal ranges not used for normal events but useful for tuning |

All N-Trig devices with product id of 1 report events in the ranges of

* X: 0-9600
* Y: 0-7200

However not all of these devices have the same physical dimensions. Most
seem to be 12” sensors (Dell Latitude XT and XT2 and the HP TX2), and
at least one model (Dell Studio 17) has a 17” sensor. The ratio of physical
to logical sizes is used to adjust the size based filter parameters.

## 12.3. Filtering

With the release of the early multi-touch firmwares it became increasingly
obvious that these sensors were prone to erroneous events. Users reported
seeing both inappropriately dropped contact and ghosts, contacts reported
where no finger was actually touching the screen.

Deactivation slack helps prevent dropped contact for single touch use, but does
not address the problem of dropping one of more contacts while other contacts
are still active. Drops in the multi-touch context require additional
processing and should be handled in tandem with tacking.

As observed ghost contacts are similar to actual use of the sensor, but they
seem to have different profiles. Ghost activity typically shows up as small
short lived touches. As such, I assume that the longer the continuous stream
of events the more likely those events are from a real contact, and that the
larger the size of each contact the more likely it is real. Balancing the
goals of preventing ghosts and accepting real events quickly (to minimize
user observable latency), the filter accumulates confidence for incoming
events until it hits thresholds and begins propagating. In the interest in
minimizing stored state as well as the cost of operations to make a decision,
I’ve kept that decision simple.

Time is measured in terms of the number of fingers reported, not frames since
the probability of multiple simultaneous ghosts is expected to drop off
dramatically with increasing numbers. Rather than accumulate weight as a
function of size, I just use it as a binary threshold. A sufficiently large
contact immediately overrides the waiting period and leads to activation.

Setting the activation size thresholds to large values will result in deciding
primarily on activation slack. If you see longer lived ghosts, turning up the
activation slack while reducing the size thresholds may suffice to eliminate
the ghosts while keeping the screen quite responsive to firm taps.

Contacts continue to be filtered with min\_height and min\_width even after
the initial activation filter is satisfied. The intent is to provide
a mechanism for filtering out ghosts in the form of an extra finger while
you actually are using the screen. In practice this sort of ghost has
been far less problematic or relatively rare and I’ve left the defaults
set to 0 for both parameters, effectively turning off that filter.

I don’t know what the optimal values are for these filters. If the defaults
don’t work for you, please play with the parameters. If you do find other
values more comfortable, I would appreciate feedback.

The calibration of these devices does drift over time. If ghosts or contact
dropping worsen and interfere with the normal usage of your device, try
recalibrating it.

## 12.4. Calibration

The N-Trig windows tools provide calibration and testing routines. Also an
unofficial unsupported set of user space tools including a calibrator is
available at:
<http://code.launchpad.net/~rafi-seas/+junk/ntrig_calib>

## 12.5. Tracking

As of yet, all tested N-Trig firmwares do not track fingers. When multiple
contacts are active they seem to be sorted primarily by Y position.
