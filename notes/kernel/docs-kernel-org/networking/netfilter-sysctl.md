# Netfilter Sysfs variables

> 출처(원문): https://docs.kernel.org/networking/netfilter-sysctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Netfilter Sysfs variables

## /proc/sys/net/netfilter/\* Variables:

nf\_log\_all\_netns - BOOLEAN
:   * 0 - disabled (default)
    * not 0 - enabled

    By default, only init\_net namespace can log packets into kernel log
    with LOG target; this aims to prevent containers from flooding host
    kernel log. If enabled, this target also works in other network
    namespaces. This variable is only accessible from init\_net.
