# 3.Infrared remote control support in video4linux drivers

> 출처(원문): https://docs.kernel.org/admin-guide/media/remote-controller.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Infrared remote control support in video4linux drivers

Authors: Gerd Hoffmann, Mauro Carvalho Chehab

## 3.1. Basics

Most analog and digital TV boards support remote controllers. Several of
them have a microprocessor that receives the IR carriers, convert into
pulse/space sequences and then to scan codes, returning such codes to
userspace (“scancode mode”). Other boards return just the pulse/space
sequences (“raw mode”).

The support for remote controller in scancode mode is provided by the
standard Linux input layer. The support for raw mode is provided via LIRC.

In order to check the support and test it, it is suggested to download
the [v4l-utils](https://git.linuxtv.org/v4l-utils.git/). It provides
two tools to handle remote controllers:

* ir-keytable: provides a way to query the remote controller, list the
  protocols it supports, enable in-kernel support for IR decoder or
  switch the protocol and to test the reception of scan codes;
* ir-ctl: provide tools to handle remote controllers that support raw mode
  via LIRC interface.

Usually, the remote controller module is auto-loaded when the TV card is
detected. However, for a few devices, you need to manually load the
ir-kbd-i2c module.

## 3.2. How it works

The modules register the remote as keyboard within the linux input
layer, i.e. you’ll see the keys of the remote as normal key strokes
(if CONFIG\_INPUT\_KEYBOARD is enabled).

Using the event devices (CONFIG\_INPUT\_EVDEV) it is possible for
applications to access the remote via /dev/input/event<n> devices.
The udev/systemd will automatically create the devices. If you install
the [v4l-utils](https://git.linuxtv.org/v4l-utils.git/), it may also
automatically load a different keytable than the default one. Please see
[v4l-utils](https://git.linuxtv.org/v4l-utils.git/) ir-keytable.1
man page for details.

The ir-keytable tool is nice for trouble shooting, i.e. to check
whenever the input device is really present, which of the devices it
is, check whenever pressing keys on the remote actually generates
events and the like. You can also use any other input utility that changes
the keymaps, like the input kbd utility.

### 3.2.1. Using with lircd

The latest versions of the lircd daemon supports reading events from the
linux input layer (via event device). It also supports receiving IR codes
in lirc mode.

### 3.2.2. Using without lircd

Xorg recognizes several IR keycodes that have its numerical value lower
than 247. With the advent of Wayland, the input driver got updated too,
and should now accept all keycodes. Yet, you may want to just reassign
the keycodes to something that your favorite media application likes.

This can be done by setting
[v4l-utils](https://git.linuxtv.org/v4l-utils.git/) to load your own
keytable in runtime. Please read ir-keytable.1 man page for details.
