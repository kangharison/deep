# The sym53c500_cs Driver

> 출처(원문): https://docs.kernel.org/scsi/sym53c500_cs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The sym53c500\_cs Driver

The sym53c500\_cs driver originated as an add-on to David Hinds’ pcmcia-cs
package, and was written by Tom Corner ([tcorner@via.at](mailto:tcorner%40via.at)). A rewrite was
long overdue, and the current version addresses the following concerns:

> 1. extensive kernel changes between 2.4 and 2.6.
> 2. deprecated PCMCIA support outside the kernel.

All the USE\_BIOS code has been ripped out. It was never used, and could
not have worked anyway. The USE\_DMA code is likewise gone. Many thanks
to YOKOTA Hiroshi (nsp\_cs driver) and David Hinds (qlogic\_cs driver) for
the code fragments I shamelessly adapted for this work. Thanks also to
Christoph Hellwig for his patient tutelage while I stumbled about.

The Symbios Logic 53c500 chip was used in the “newer” (circa 1997) version
of the New Media Bus Toaster PCMCIA SCSI controller. Presumably there are
other products using this chip, but I’ve never laid eyes (much less hands)
on one.

Through the years, there have been a number of downloads of the pcmcia-cs
version of this driver, and I guess it worked for those users. It worked
for Tom Corner, and it works for me. Your mileage will probably vary.

Bob Tracy ([rct@frus.com](mailto:rct%40frus.com))
