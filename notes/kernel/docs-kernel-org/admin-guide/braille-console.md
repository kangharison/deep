# Linux Braille Console

> 출처(원문): https://docs.kernel.org/admin-guide/braille-console.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Braille Console

To get early boot messages on a braille device (before userspace screen
readers can start), you first need to compile the support for the usual serial
console (see [Documentation/admin-guide/serial-console.rst](serial-console.html#serial-console)), and
for braille device
(in Device Drivers ‣ Accessibility support ‣ Console on braille device).

Then you need to specify a `console=brl`, option on the kernel command line, the
format is:

```
console=brl,serial_options...
```

where `serial_options...` are the same as described in
[Documentation/admin-guide/serial-console.rst](serial-console.html#serial-console).

So for instance you can use `console=brl,ttyS0` if the braille device is connected to the first serial port, and `console=brl,ttyS0,115200` to
override the baud rate to 115200, etc.

By default, the braille device will just show the last kernel message (console
mode). To review previous messages, press the Insert key to switch to the VT
review mode. In review mode, the arrow keys permit to browse in the VT content,
PAGE-UP/PAGE-DOWN keys go at the top/bottom of the screen, and
the HOME key goes back
to the cursor, hence providing very basic screen reviewing facility.

Sound feedback can be obtained by adding the `braille_console.sound=1` kernel
parameter.

For simplicity, only one braille console can be enabled, other uses of
`console=brl,...` will be discarded. Also note that it does not interfere with
the console selection mechanism described in
[Documentation/admin-guide/serial-console.rst](serial-console.html#serial-console).

For now, only the VisioBraille device is supported.

Samuel Thibault <[samuel.thibault@ens-lyon.org](mailto:samuel.thibault%40ens-lyon.org)>
