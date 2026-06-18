# DeviceTree Booting

> 출처(원문): https://docs.kernel.org/arch/sh/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DeviceTree Booting

> Device-tree compatible SH bootloaders are expected to provide the physical
> address of the device tree blob in r4. Since legacy bootloaders did not
> guarantee any particular initial register state, kernels built to
> inter-operate with old bootloaders must either use a builtin DTB or
> select a legacy board option (something other than CONFIG\_SH\_DEVICE\_TREE)
> that does not use device tree. Support for the latter is being phased out
> in favor of device tree.
