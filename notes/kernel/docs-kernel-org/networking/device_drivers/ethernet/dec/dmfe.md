# Davicom DM9102(A)/DM9132/DM9801 fast ethernet driver for Linux

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/dec/dmfe.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Davicom DM9102(A)/DM9132/DM9801 fast ethernet driver for Linux

Note: This driver doesn’t have a maintainer.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

This driver provides kernel support for Davicom DM9102(A)/DM9132/DM9801 ethernet cards ( CNET
10/100 ethernet cards uses Davicom chipset too, so this driver supports CNET cards too ).If you
didn’t compile this driver as a module, it will automatically load itself on boot and print a
line similar to:

```
dmfe: Davicom DM9xxx net driver, version 1.36.4 (2002-01-17)
```

If you compiled this driver as a module, you have to load it on boot.You can load it with command:

```
insmod dmfe
```

This way it will autodetect the device mode.This is the suggested way to load the module.Or you can pass
a mode= setting to module while loading, like:

```
insmod dmfe mode=0 # Force 10M Half Duplex
insmod dmfe mode=1 # Force 100M Half Duplex
insmod dmfe mode=4 # Force 10M Full Duplex
insmod dmfe mode=5 # Force 100M Full Duplex
```

Next you should configure your network interface with a command similar to:

```
ifconfig eth0 172.22.3.18
              ^^^^^^^^^^^
             Your IP Address
```

Then you may have to modify the default routing table with command:

```
route add default eth0
```

Now your ethernet card should be up and running.

TODO:

* Implement pci\_driver::`suspend()` and pci\_driver::`resume()` power management methods.
* Check on 64 bit boxes.
* Check and fix on big endian boxes.
* Test and make sure PCI latency is now correct for all cases.

Authors:

Sten Wang <[sten\_wang@davicom.com.tw](mailto:sten_wang%40davicom.com.tw) > : Original Author

Contributors:

* Marcelo Tosatti <[marcelo@conectiva.com.br](mailto:marcelo%40conectiva.com.br)>
* Alan Cox <[alan@lxorguk.ukuu.org.uk](mailto:alan%40lxorguk.ukuu.org.uk)>
* Jeff Garzik <[jgarzik@pobox.com](mailto:jgarzik%40pobox.com)>
* Vojtech Pavlik <[vojtech@suse.cz](mailto:vojtech%40suse.cz)>
