# Old Microcode

> 출처(원문): https://docs.kernel.org/admin-guide/hw-vuln/old_microcode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Old Microcode

The kernel keeps a table of released microcode. Systems that had
microcode older than this at boot will say “Vulnerable”. This means
that the system was vulnerable to some known CPU issue. It could be
security or functional, the kernel does not know or care.

You should update the CPU microcode to mitigate any exposure. This is
usually accomplished by updating the files in
/lib/firmware/intel-ucode/ via normal distribution updates. Intel also
distributes these files in a github repo:

> <https://github.com/intel/Intel-Linux-Processor-Microcode-Data-Files.git>

Just like all the other hardware vulnerabilities, exposure is
determined at boot. Runtime microcode updates do not change the status
of this vulnerability.
