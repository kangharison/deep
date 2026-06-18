# 5.Digital TV Network API

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/net.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5. Digital TV Network API

The Digital TV net device controls the mapping of data packages that are part
of a transport stream to be mapped into a virtual network interface,
visible through the standard Linux network protocol stack.

Currently, two encapsulations are supported:

* [Multi Protocol Encapsulation (MPE)](http://en.wikipedia.org/wiki/Multiprotocol_Encapsulation)
* [Ultra Lightweight Encapsulation (ULE)](http://en.wikipedia.org/wiki/Unidirectional_Lightweight_Encapsulation)

In order to create the Linux virtual network interfaces, an application
needs to tell to the Kernel what are the PIDs and the encapsulation
types that are present on the transport stream. This is done through
`/dev/dvb/adapter?/net?` device node. The data will be available via
virtual `dvb?_?` network interfaces, and will be controlled/routed via
the standard ip tools (like ip, route, netstat, ifconfig, etc).

Data types and ioctl definitions are defined via `linux/dvb/net.h`
header.

## 5.1. Digital TV net Function Calls

* [5.1.1. Net Data Types](net-types.html)
* [5.1.2. ioctl NET\_ADD\_IF](net-add-if.html)
* [5.1.3. ioctl NET\_REMOVE\_IF](net-remove-if.html)
* [5.1.4. ioctl NET\_GET\_IF](net-get-if.html)
