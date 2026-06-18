# Passing boot parameters to the kernel

> 출처(원문): https://docs.kernel.org/arch/xtensa/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Passing boot parameters to the kernel

Boot parameters are represented as a TLV list in the memory. Please see
arch/xtensa/include/asm/bootparam.h for definition of the bp\_tag structure and
tag value constants. First entry in the list must have type BP\_TAG\_FIRST, last
entry must have type BP\_TAG\_LAST. The address of the first list entry is
passed to the kernel in the register a2. The address type depends on MMU type:

* For configurations without MMU, with region protection or with MPU the
  address must be the physical address.
* For configurations with region translarion MMU or with MMUv3 and CONFIG\_MMU=n
  the address must be a valid address in the current mapping. The kernel will
  not change the mapping on its own.
* For configurations with MMUv2 the address must be a virtual address in the
  default virtual mapping (0xd0000000..0xffffffff).
* For configurations with MMUv3 and CONFIG\_MMU=y the address may be either a
  virtual or physical address. In either case it must be within the default
  virtual mapping. It is considered physical if it is within the range of
  physical addresses covered by the default KSEG mapping (XCHAL\_KSEG\_PADDR..
  XCHAL\_KSEG\_PADDR + XCHAL\_KSEG\_SIZE), otherwise it is considered virtual.
