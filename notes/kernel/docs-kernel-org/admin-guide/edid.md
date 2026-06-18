# EDID

> 출처(원문): https://docs.kernel.org/admin-guide/edid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# EDID

In the good old days when graphics parameters were configured explicitly
in a file called xorg.conf, even broken hardware could be managed.

Today, with the advent of Kernel Mode Setting, a graphics board is
either correctly working because all components follow the standards -
or the computer is unusable, because the screen remains dark after
booting or it displays the wrong area. Cases when this happens are:

* The graphics board does not recognize the monitor.
* The graphics board is unable to detect any EDID data.
* The graphics board incorrectly forwards EDID data to the driver.
* The monitor sends no or bogus EDID data.
* A KVM sends its own EDID data instead of querying the connected monitor.

Adding the kernel parameter “nomodeset” helps in most cases, but causes
restrictions later on.

As a remedy for such situations, the kernel configuration item
CONFIG\_DRM\_LOAD\_EDID\_FIRMWARE was introduced. It allows to provide an
individually prepared or corrected EDID data set in the /lib/firmware
directory from where it is loaded via the firmware interface.
