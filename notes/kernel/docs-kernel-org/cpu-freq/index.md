# CPUFreq - CPU frequency and voltage scaling code in the Linux(TM) kernel

> 출처(원문): https://docs.kernel.org/cpu-freq/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CPUFreq - CPU frequency and voltage scaling code in the Linux(TM) kernel

Author: Dominik Brodowski <[linux@brodo.de](mailto:linux%40brodo.de)>

> Clock scaling allows you to change the clock speed of the CPUs on the
> fly. This is a nice method to save battery power, because the lower
> the clock speed, the less power the CPU consumes.

* [General description of the CPUFreq core and CPUFreq notifiers](core.html)
* [How to Implement a new CPUFreq Processor Driver](cpu-drivers.html)
* [General Description of sysfs CPUFreq Stats](cpufreq-stats.html)

## Mailing List

There is a CPU frequency general list where you can report bugs,
problems or submit patches. To post a message, send an email to
[linux-pm@vger.kernel.org](mailto:linux-pm%40vger.kernel.org).

## Links

the FTP archives:
\* <ftp://ftp.linux.org.uk/pub/linux/cpufreq/>

the CPUFreq Mailing list:
\* <http://vger.kernel.org/vger-lists.html#linux-pm>

Clock and voltage scaling for the SA-1100:
\* <http://www.lartmaker.nl/projects/scaling>
