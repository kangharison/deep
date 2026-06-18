# 2.DeviceTree Booting

> 출처(원문): https://docs.kernel.org/arch/x86/booting-dt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. DeviceTree Booting

> There is one single 32bit entry point to the kernel at code32\_start,
> the decompressor (the real mode entry point goes to the same 32bit
> entry point once it switched into protected mode). That entry point
> supports one calling convention which is documented in
> [The Linux/x86 Boot Protocol](boot.html)
> The physical pointer to the device-tree block is passed via setup\_data
> which requires at least boot protocol 2.09.
> The type filed is defined as
>
> #define SETUP\_DTB 2
>
> This device-tree is used as an extension to the “boot page”. As such it
> does not parse / consider data which is already covered by the boot
> page. This includes memory size, reserved ranges, command line arguments
> or initrd address. It simply holds information which can not be retrieved
> otherwise like interrupt routing or a list of devices behind an I2C bus.
