# lxfb - AMD Geode LX framebuffer driver

> 출처(원문): https://docs.kernel.org/fb/lxfb.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# lxfb - AMD Geode LX framebuffer driver

This is a graphics framebuffer driver for AMD Geode LX based processors.

Advantages:

> * No need to use AMD’s VSA code (or other VESA emulation layer) in the
>   BIOS.
> * It provides a nice large console (128 cols + 48 lines with 1024x768)
>   without using tiny, unreadable fonts.
> * You can run XF68\_FBDev on top of /dev/fb0
> * Most important: boot logo :-)

Disadvantages:

> * graphic mode is slower than text mode...

## How to use it?

Switching modes is done using lxfb.mode\_option=<resolution>... boot
parameter or using fbset program.

See [modedb default video mode support](modedb.html) for more information on modedb
resolutions.

## X11

XF68\_FBDev should generally work fine, but it is non-accelerated.

## Configuration

You can pass kernel command line options to lxfb with lxfb.<option>.
For example, [lxfb.mode\_option=800x600@75](mailto:lxfb.mode_option=800x600%4075).
Accepted options:

|  |  |
| --- | --- |
| mode\_option | specify the video mode. Of the form <x>x<y>[-<bpp>][@<refresh>] |
| vram | size of video ram (normally auto-detected) |
| vt\_switch | enable vt switching during suspend/resume. The vt switch is slow, but harmless. |

Andres Salomon <[dilinger@debian.org](mailto:dilinger%40debian.org)>
