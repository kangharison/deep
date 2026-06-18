# MEI NFC

> 출처(원문): https://docs.kernel.org/driver-api/mei/nfc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# MEI NFC

Some Intel 8 and 9 Series chipsets support NFC devices connected behind
the Intel Management Engine controller.
MEI client bus exposes the NFC chips as NFC phy devices and enables
binding with Microread and NXP PN544 NFC device driver from the Linux NFC
subsystem.

![MEI NFC digraph](../../_images/DOT-d09fae00dfc981dee81fc238831b72204bfdaa15.svg)

**MEI NFC** Stack
