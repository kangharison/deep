# General description of the CPUFreq core and CPUFreq notifiers

> 출처(원문): https://docs.kernel.org/cpu-freq/core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# General description of the CPUFreq core and CPUFreq notifiers

Authors:
:   * Dominik Brodowski <[linux@brodo.de](mailto:linux%40brodo.de)>
    * David Kimdon <[dwhedon@debian.org](mailto:dwhedon%40debian.org)>
    * Rafael J. Wysocki <[rafael.j.wysocki@intel.com](mailto:rafael.j.wysocki%40intel.com)>
    * Viresh Kumar <[viresh.kumar@linaro.org](mailto:viresh.kumar%40linaro.org)>

## 1. General Information

The CPUFreq core code is located in drivers/cpufreq/cpufreq.c. This
cpufreq code offers a standardized interface for the CPUFreq
architecture drivers (those pieces of code that do actual
frequency transitions), as well as to “notifiers”. These are device
drivers or other part of the kernel that need to be informed of
policy changes (ex. thermal modules like ACPI) or of all
frequency changes (ex. timing code) or even need to force certain
speed limits (like LCD drivers on ARM architecture). Additionally, the
kernel “constant” loops\_per\_jiffy is updated on frequency changes
here.

Reference counting of the cpufreq policies is done by cpufreq\_cpu\_get
and cpufreq\_cpu\_put, which make sure that the cpufreq driver is
correctly registered with the core, and will not be unloaded until
cpufreq\_put\_cpu is called. That also ensures that the respective cpufreq
policy doesn’t get freed while being used.

## 2. CPUFreq notifiers

CPUFreq notifiers conform to the standard kernel notifier interface.
See linux/include/linux/notifier.h for details on notifiers.

There are two different CPUFreq notifiers - policy notifiers and
transition notifiers.

### 2.1 CPUFreq policy notifiers

These are notified when a new policy is created or removed.

The phase is specified in the second argument to the notifier. The phase is
CPUFREQ\_CREATE\_POLICY when the policy is first created and it is
CPUFREQ\_REMOVE\_POLICY when the policy is removed.

The third argument, a `void *pointer`, points to a `struct cpufreq_policy`
consisting of several values, including min, max (the lower and upper
frequencies (in kHz) of the new policy).

### 2.2 CPUFreq transition notifiers

These are notified twice for each online CPU in the policy, when the
CPUfreq driver switches the CPU core frequency and this change has no
any external implications.

The second argument specifies the phase - CPUFREQ\_PRECHANGE or
CPUFREQ\_POSTCHANGE.

The third argument is a `struct cpufreq_freqs` with the following
values:

|  |  |
| --- | --- |
| policy | a pointer to the `struct cpufreq_policy` |
| old | old frequency |
| new | new frequency |
| flags | flags of the cpufreq driver |

## 3. CPUFreq Table Generation with Operating Performance Point (OPP)

For details about OPP, see [Operating Performance Points (OPP) Library](../power/opp.html)

dev\_pm\_opp\_init\_cpufreq\_table -
:   This function provides a ready to use conversion routine to translate
    the OPP layer’s internal information about the available frequencies
    into a format readily providable to cpufreq.

    Warning

    Do not use this function in interrupt context.

    Example:

    ```
    soc_pm_init()
    {
           /* Do things */
           r = dev_pm_opp_init_cpufreq_table(dev, &freq_table);
           if (!r)
                   policy->freq_table = freq_table;
           /* Do other things */
    }
    ```

    Note

    This function is available only if CONFIG\_CPU\_FREQ is enabled in
    addition to CONFIG\_PM\_OPP.

dev\_pm\_opp\_free\_cpufreq\_table
:   Free up the table allocated by dev\_pm\_opp\_init\_cpufreq\_table
