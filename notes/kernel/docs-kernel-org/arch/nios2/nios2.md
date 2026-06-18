# 1.Linux on the Nios II architecture

> 출처(원문): https://docs.kernel.org/arch/nios2/nios2.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. Linux on the Nios II architecture

This is a port of Linux to Nios II (nios2) processor.

In order to compile for Nios II, you need a version of GCC with support for the generic
system call ABI. Please see this link for more information on how compiling and booting
software for the Nios II platform:
<http://www.rocketboards.org/foswiki/Documentation/NiosIILinuxUserManual>

For reference, please see the following link:
<http://www.altera.com/literature/lit-nio2.jsp>

## 1.1. What is Nios II?

Nios II is a 32-bit embedded-processor architecture designed specifically for the
Altera family of FPGAs. In order to support Linux, Nios II needs to be configured
with MMU and hardware multiplier enabled.

## 1.2. Nios II ABI

Please refer to chapter “Application Binary Interface” in Nios II Processor Reference
Handbook.
