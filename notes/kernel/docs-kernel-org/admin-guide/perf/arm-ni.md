# Arm Network-on Chip Interconnect PMU

> 출처(원문): https://docs.kernel.org/admin-guide/perf/arm-ni.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Arm Network-on Chip Interconnect PMU

NI-700 and friends implement a distinct PMU for each clock domain within the
interconnect. Correspondingly, the driver exposes multiple PMU devices named
arm\_ni\_<x>\_cd\_<y>, where <x> is an (arbitrary) instance identifier and <y> is
the clock domain ID within that particular instance. If multiple NI instances
exist within a system, the PMU devices can be correlated with the underlying
hardware instance via sysfs parentage.

Each PMU exposes base event aliases for the interface types present in its clock
domain. These require qualifying with the “eventid” and “nodeid” parameters
to specify the event code to count and the interface at which to count it
(per the configured hardware ID as reflected in the xxNI\_NODE\_INFO register).
The exception is the “cycles” alias for the PMU cycle counter, which is encoded
with the PMU node type and needs no further qualification.
