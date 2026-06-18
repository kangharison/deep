# ARM

> 출처(원문): https://docs.kernel.org/virt/kvm/arm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ARM

* [ARM firmware pseudo-registers interface](fw-pseudo-registers.html)
  + [Bitmap Feature Firmware Registers](fw-pseudo-registers.html#bitmap-feature-firmware-registers)
* [Internal ABI between the kernel and HYP](hyp-abi.html)
* [KVM/arm64-specific hypercalls exposed to guests](hypercalls.html)
  + [`ARM_SMCCC_VENDOR_HYP_KVM_FEATURES_FUNC_ID`](hypercalls.html#arm-smccc-vendor-hyp-kvm-features-func-id)
  + [`ARM_SMCCC_VENDOR_HYP_KVM_PTP_FUNC_ID`](hypercalls.html#arm-smccc-vendor-hyp-kvm-ptp-func-id)
  + [`ARM_SMCCC_KVM_FUNC_HYP_MEMINFO`](hypercalls.html#arm-smccc-kvm-func-hyp-meminfo)
  + [`ARM_SMCCC_KVM_FUNC_MEM_SHARE`](hypercalls.html#arm-smccc-kvm-func-mem-share)
  + [`ARM_SMCCC_KVM_FUNC_MEM_UNSHARE`](hypercalls.html#arm-smccc-kvm-func-mem-unshare)
  + [`ARM_SMCCC_KVM_FUNC_MMIO_GUARD`](hypercalls.html#arm-smccc-kvm-func-mmio-guard)
  + [`ARM_SMCCC_VENDOR_HYP_KVM_DISCOVER_IMPL_VER_FUNC_ID`](hypercalls.html#arm-smccc-vendor-hyp-kvm-discover-impl-ver-func-id)
  + [`ARM_SMCCC_VENDOR_HYP_KVM_DISCOVER_IMPL_CPUS_FUNC_ID`](hypercalls.html#arm-smccc-vendor-hyp-kvm-discover-impl-cpus-func-id)
* [Protected KVM (pKVM)](pkvm.html)
  + [Overview](pkvm.html#overview)
  + [Isolation mechanisms](pkvm.html#isolation-mechanisms)
  + [Resources](pkvm.html#resources)
* [Paravirtualized time support for arm64](pvtime.html)
  + [Stolen Time](pvtime.html#stolen-time)
* [PTP\_KVM support for arm/arm64](ptp_kvm.html)
  + [`ARM_SMCCC_VENDOR_HYP_KVM_PTP_FUNC_ID`](ptp_kvm.html#arm-smccc-vendor-hyp-kvm-ptp-func-id)
* [vCPU feature selection on arm64](vcpu-features.html)
  + [KVM\_ARM\_VCPU\_INIT](vcpu-features.html#kvm-arm-vcpu-init)
  + [The ID Registers](vcpu-features.html#the-id-registers)
