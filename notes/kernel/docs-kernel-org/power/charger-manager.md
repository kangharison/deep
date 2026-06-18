# Charger Manager

> 출처(원문): https://docs.kernel.org/power/charger-manager.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Charger Manager

> 3. 2011 MyungJoo Ham <[myungjoo.ham@samsung.com](mailto:myungjoo.ham%40samsung.com)>, GPL

Charger Manager provides in-kernel battery charger management that
requires temperature monitoring during suspend-to-RAM state
and where each battery may have multiple chargers attached and the userland
wants to look at the aggregated information of the multiple chargers.

Charger Manager is a platform\_driver with power-supply-class entries.
An instance of Charger Manager (a platform-device created with Charger-Manager)
represents an independent battery with chargers. If there are multiple
batteries with their own chargers acting independently in a system,
the system may need multiple instances of Charger Manager.

## 1. Introduction

Charger Manager supports the following:

* Support for multiple chargers (e.g., a device with USB, AC, and solar panels)
  :   A system may have multiple chargers (or power sources) and some of
      they may be activated at the same time. Each charger may have its
      own power-supply-class and each power-supply-class can provide
      different information about the battery status. This framework
      aggregates charger-related information from multiple sources and
      shows combined information as a single power-supply-class.
* Support for in suspend-to-RAM polling (with suspend\_again callback)
  :   While the battery is being charged and the system is in suspend-to-RAM,
      we may need to monitor the battery health by looking at the ambient or
      battery temperature. We can accomplish this by waking up the system
      periodically. However, such a method wakes up devices unnecessarily for
      monitoring the battery health and tasks, and user processes that are
      supposed to be kept suspended. That, in turn, incurs unnecessary power
      consumption and slow down charging process. Or even, such peak power
      consumption can stop chargers in the middle of charging
      (external power input < device power consumption), which not
      only affects the charging time, but the lifespan of the battery.

      Charger Manager provides a function “cm\_suspend\_again” that can be
      used as suspend\_again callback of platform\_suspend\_ops. If the platform
      requires tasks other than cm\_suspend\_again, it may implement its own
      suspend\_again callback that calls cm\_suspend\_again in the middle.
      Normally, the platform will need to resume and suspend some devices
      that are used by Charger Manager.
* Support for premature full-battery event handling
  :   If the battery voltage drops by “fullbatt\_vchkdrop\_uV” after
      “fullbatt\_vchkdrop\_ms” from the full-battery event, the framework
      restarts charging. This check is also performed while suspended by
      setting wakeup time accordingly and using suspend\_again.
* Support for uevent-notify
  :   With the charger-related events, the device sends
      notification to users with UEVENT.

## 2. Global Charger-Manager Data related with suspend\_again

In order to setup Charger Manager with suspend-again feature
(in-suspend monitoring), the user should provide charger\_global\_desc
with setup\_charger\_manager(`struct charger_global_desc` \*).
This charger\_global\_desc data for in-suspend monitoring is global
as the name suggests. Thus, the user needs to provide only once even
if there are multiple batteries. If there are multiple batteries, the
multiple instances of Charger Manager share the same charger\_global\_desc
and it will manage in-suspend monitoring for all instances of Charger Manager.

The user needs to provide all the three entries to `struct charger_global_desc`
properly in order to activate in-suspend monitoring:

char \*rtc\_name;
:   The name of rtc (e.g., “rtc0”) used to wakeup the system from
    suspend for Charger Manager. The alarm interrupt (AIE) of the rtc
    should be able to wake up the system from suspend. Charger Manager
    saves and restores the alarm value and use the previously-defined
    alarm if it is going to go off earlier than Charger Manager so that
    Charger Manager does not interfere with previously-defined alarms.

bool (\*rtc\_only\_wakeup)(void);
:   This callback should let CM know whether
    the wakeup-from-suspend is caused only by the alarm of “rtc” in the
    same struct. If there is any other wakeup source triggered the
    wakeup, it should return false. If the “rtc” is the only wakeup
    reason, it should return true.

bool assume\_timer\_stops\_in\_suspend;
:   if true, Charger Manager assumes that
    the timer (CM uses jiffies as timer) stops during suspend. Then, CM
    assumes that the suspend-duration is same as the alarm length.

## 3. How to setup suspend\_again

Charger Manager provides a function “extern bool cm\_suspend\_again(void)”.
When cm\_suspend\_again is called, it monitors every battery. The suspend\_ops
callback of the system’s platform\_suspend\_ops can call cm\_suspend\_again
function to know whether Charger Manager wants to suspend again or not.
If there are no other devices or tasks that want to use suspend\_again
feature, the platform\_suspend\_ops may directly refer to cm\_suspend\_again
for its suspend\_again callback.

The `cm_suspend_again()` returns true (meaning “I want to suspend again”)
if the system was woken up by Charger Manager and the polling
(in-suspend monitoring) results in “normal”.

## 4. Charger-Manager Data (struct charger\_desc)

For each battery charged independently from other batteries (if a series of
batteries are charged by a single charger, they are counted as one independent
battery), an instance of Charger Manager is attached to it. The following

`struct charger_desc` elements:

char \*psy\_name;
:   The power-supply-class name of the battery. Default is
    “battery” if psy\_name is NULL. Users can access the psy entries
    at “/sys/class/power\_supply/[psy\_name]/”.

enum polling\_modes polling\_mode;
:   CM\_POLL\_DISABLE:
    :   do not poll this battery.

    CM\_POLL\_ALWAYS:
    :   always poll this battery.

    CM\_POLL\_EXTERNAL\_POWER\_ONLY:
    :   poll this battery if and only if an external power
        source is attached.

    CM\_POLL\_CHARGING\_ONLY:
    :   poll this battery if and only if the battery is being charged.

unsigned int fullbatt\_vchkdrop\_ms; / unsigned int fullbatt\_vchkdrop\_uV;
:   If both have non-zero values, Charger Manager will check the
    battery voltage drop fullbatt\_vchkdrop\_ms after the battery is fully
    charged. If the voltage drop is over fullbatt\_vchkdrop\_uV, Charger
    Manager will try to recharge the battery by disabling and enabling
    chargers. Recharge with voltage drop condition only (without delay
    condition) is needed to be implemented with hardware interrupts from
    fuel gauges or charger devices/chips.

unsigned int fullbatt\_uV;
:   If specified with a non-zero value, Charger Manager assumes
    that the battery is full (capacity = 100) if the battery is not being
    charged and the battery voltage is equal to or greater than
    fullbatt\_uV.

unsigned int polling\_interval\_ms;
:   Required polling interval in ms. Charger Manager will poll
    this battery every polling\_interval\_ms or more frequently.

enum data\_source battery\_present;
:   CM\_BATTERY\_PRESENT:
    :   assume that the battery exists.

    CM\_NO\_BATTERY:
    :   assume that the battery does not exists.

    CM\_FUEL\_GAUGE:
    :   get battery presence information from fuel gauge.

    CM\_CHARGER\_STAT:
    :   get battery presence from chargers.

char \*\*psy\_charger\_stat;
:   An array ending with NULL that has power-supply-class names of
    chargers. Each power-supply-class should provide “PRESENT” (if
    battery\_present is “CM\_CHARGER\_STAT”), “ONLINE” (shows whether an
    external power source is attached or not), and “STATUS” (shows whether
    the battery is {“FULL” or not FULL} or {“FULL”, “Charging”,
    “Discharging”, “NotCharging”}).

int num\_charger\_regulators; / struct regulator\_bulk\_data \*charger\_regulators;
:   Regulators representing the chargers in the form for
    regulator framework’s bulk functions.

char \*psy\_fuel\_gauge;
:   Power-supply-class name of the fuel gauge.

int (\*temperature\_out\_of\_range)(int \*mC); / bool measure\_battery\_temp;
:   This callback returns 0 if the temperature is safe for charging,
    a positive number if it is too hot to charge, and a negative number
    if it is too cold to charge. With the variable mC, the callback returns
    the temperature in 1/1000 of centigrade.
    The source of temperature can be battery or ambient one according to
    the value of measure\_battery\_temp.

## 5. Other Considerations

At the charger/battery-related events such as battery-pulled-out,
charger-pulled-out, charger-inserted, DCIN-over/under-voltage, charger-stopped,
and others critical to chargers, the system should be configured to wake up.
At least the following should wake up the system from a suspend:
a) charger-on/off b) external-power-in/out c) battery-in/out (while charging)

It is usually accomplished by configuring the PMIC as a wakeup source.
