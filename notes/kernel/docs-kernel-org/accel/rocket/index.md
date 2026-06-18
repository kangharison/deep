# accel/rocket Rockchip NPU driver

> 출처(원문): https://docs.kernel.org/accel/rocket/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# accel/rocket Rockchip NPU driver

The accel/rocket driver supports the Neural Processing Units (NPUs) inside some
Rockchip SoCs such as the RK3588. Rockchip calls it RKNN and sometimes RKNPU.

The hardware is described in chapter 36 in the RK3588 TRM.

This driver just powers the hardware on and off, allocates and maps buffers to
the device and submits jobs to the frontend unit. Everything else is done in
userspace, as a Gallium driver (also called rocket) that is part of the Mesa3D
project.

Hardware currently supported:

* RK3588
