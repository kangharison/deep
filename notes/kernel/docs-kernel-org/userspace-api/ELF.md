# Linux-specific ELF idiosyncrasies

> 출처(원문): https://docs.kernel.org/userspace-api/ELF.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux-specific ELF idiosyncrasies

## Definitions

“First” program header is the one with the smallest offset in the file:
e\_phoff.

“Last” program header is the one with the biggest offset in the file:
e\_phoff + (e\_phnum - 1) \* sizeof(Elf\_Phdr).

## PT\_INTERP

First PT\_INTERP program header is used to locate the filename of ELF
interpreter. Other PT\_INTERP headers are ignored (since Linux 2.4.11).

## PT\_GNU\_STACK

Last PT\_GNU\_STACK program header defines userspace stack executability
(since Linux 2.6.6). Other PT\_GNU\_STACK headers are ignored.

## PT\_GNU\_PROPERTY

ELF interpreter’s last PT\_GNU\_PROPERTY program header is used (since
Linux 5.8). If interpreter doesn’t have one, then the last PT\_GNU\_PROPERTY
program header of an executable is used. Other PT\_GNU\_PROPERTY headers
are ignored.
