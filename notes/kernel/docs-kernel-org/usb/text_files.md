# Linux CDC ACM inf

> 출처(원문): https://docs.kernel.org/usb/text_files.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux CDC ACM inf

```
; Windows USB CDC ACM Setup File

; Based on INF template which was:
;     Copyright (c) 2000 Microsoft Corporation
;     Copyright (c) 2007 Microchip Technology Inc.
; likely to be covered by the MLPL as found at:
;    <http://msdn.microsoft.com/en-us/cc300389.aspx#MLPL>.
; For use only on Windows operating systems.

[Version]
Signature="$Windows NT$"
Class=Ports
ClassGuid={4D36E978-E325-11CE-BFC1-08002BE10318}
Provider=%Linux%
DriverVer=11/15/2007,5.1.2600.0

[Manufacturer]
%Linux%=DeviceList, NTamd64

[DestinationDirs]
DefaultDestDir=12


;------------------------------------------------------------------------------
;  Windows 2000/XP/Vista-32bit Sections
;------------------------------------------------------------------------------

[DriverInstall.nt]
include=mdmcpq.inf
CopyFiles=DriverCopyFiles.nt
AddReg=DriverInstall.nt.AddReg

[DriverCopyFiles.nt]
usbser.sys,,,0x20

[DriverInstall.nt.AddReg]
HKR,,DevLoader,,*ntkern
HKR,,NTMPDriver,,USBSER.sys
HKR,,EnumPropPages32,,"MsPorts.dll,SerialPortPropPageProvider"

[DriverInstall.nt.Services]
AddService=usbser, 0x00000002, DriverService.nt

[DriverService.nt]
DisplayName=%SERVICE%
ServiceType=1
StartType=3
ErrorControl=1
ServiceBinary=%12%\USBSER.sys

;------------------------------------------------------------------------------
;  Vista-64bit Sections
;------------------------------------------------------------------------------

[DriverInstall.NTamd64]
include=mdmcpq.inf
CopyFiles=DriverCopyFiles.NTamd64
AddReg=DriverInstall.NTamd64.AddReg

[DriverCopyFiles.NTamd64]
USBSER.sys,,,0x20

[DriverInstall.NTamd64.AddReg]
HKR,,DevLoader,,*ntkern
HKR,,NTMPDriver,,USBSER.sys
HKR,,EnumPropPages32,,"MsPorts.dll,SerialPortPropPageProvider"

[DriverInstall.NTamd64.Services]
AddService=usbser, 0x00000002, DriverService.NTamd64

[DriverService.NTamd64]
DisplayName=%SERVICE%
ServiceType=1
StartType=3
ErrorControl=1
ServiceBinary=%12%\USBSER.sys


;------------------------------------------------------------------------------
;  Vendor and Product ID Definitions
;------------------------------------------------------------------------------
; When developing your USB device, the VID and PID used in the PC side
; application program and the firmware on the microcontroller must match.
; Modify the below line to use your VID and PID.  Use the format as shown
; below.
; Note: One INF file can be used for multiple devices with different
;       VID and PIDs.  For each supported device, append
;       ",USB\VID_xxxx&PID_yyyy" to the end of the line.
;------------------------------------------------------------------------------
[SourceDisksFiles]
[SourceDisksNames]
[DeviceList]
%DESCRIPTION%=DriverInstall, USB\VID_0525&PID_A4A7, USB\VID_1D6B&PID_0104&MI_02, USB\VID_1D6B&PID_0106&MI_00

[DeviceList.NTamd64]
%DESCRIPTION%=DriverInstall, USB\VID_0525&PID_A4A7, USB\VID_1D6B&PID_0104&MI_02, USB\VID_1D6B&PID_0106&MI_00


;------------------------------------------------------------------------------
;  String Definitions
;------------------------------------------------------------------------------
;Modify these strings to customize your device
;------------------------------------------------------------------------------
[Strings]
Linux               = "Linux Developer Community"
DESCRIPTION         = "Gadget Serial"
SERVICE             = "USB RS-232 Emulation Driver"
```

# Linux inf

```
; Based on template INF file found at
;    <https://msdn.microsoft.com/en-us/library/ff570620.aspx>
; which was:
;    Copyright (c) Microsoft Corporation
; and released under the MLPL as found at:
;    <http://msdn.microsoft.com/en-us/cc300389.aspx#MLPL>.
; For use only on Windows operating systems.

[Version]
Signature           = "$Windows NT$"
Class               = Net
ClassGUID           = {4d36e972-e325-11ce-bfc1-08002be10318}
Provider            = %Linux%
DriverVer           = 06/21/2006,6.0.6000.16384

[Manufacturer]
%Linux%             = LinuxDevices,NTx86,NTamd64,NTia64

; Decoration for x86 architecture
[LinuxDevices.NTx86]
%LinuxDevice%       = RNDIS.NT.5.1, USB\VID_0525&PID_a4a2, USB\VID_1d6b&PID_0104&MI_00

; Decoration for x64 architecture
[LinuxDevices.NTamd64]
%LinuxDevice%       = RNDIS.NT.5.1, USB\VID_0525&PID_a4a2, USB\VID_1d6b&PID_0104&MI_00

; Decoration for ia64 architecture
[LinuxDevices.NTia64]
%LinuxDevice%       = RNDIS.NT.5.1, USB\VID_0525&PID_a4a2, USB\VID_1d6b&PID_0104&MI_00

;@@@ This is the common setting for setup
[ControlFlags]
ExcludeFromSelect=*

; DDInstall section
; References the in-build Netrndis.inf
[RNDIS.NT.5.1]
Characteristics     = 0x84   ; NCF_PHYSICAL + NCF_HAS_UI
BusType             = 15
; NEVER REMOVE THE FOLLOWING REFERENCE FOR NETRNDIS.INF
include             = netrndis.inf
needs               = Usb_Rndis.ndi
AddReg              = Rndis_AddReg_Vista

; DDInstal.Services section
[RNDIS.NT.5.1.Services]
include             = netrndis.inf
needs               = Usb_Rndis.ndi.Services

; Optional registry settings. You can modify as needed.
[RNDIS_AddReg_Vista]
HKR, NDI\params\VistaProperty, ParamDesc,  0, %Vista_Property%
HKR, NDI\params\VistaProperty, type,       0, "edit"
HKR, NDI\params\VistaProperty, LimitText,  0, "12"
HKR, NDI\params\VistaProperty, UpperCase,  0, "1"
HKR, NDI\params\VistaProperty, default,    0, " "
HKR, NDI\params\VistaProperty, optional,   0, "1"

; No sys copyfiles - the sys files are already in-build
; (part of the operating system).
; We do not support XP SP1-, 2003 SP1-, ME, 9x.

[Strings]
Linux                 = "Linux Developer Community"
LinuxDevice           = "Linux USB Ethernet/RNDIS Gadget"
Vista_Property        = "Optional Vista Property"
```

# USB devfs drop permissions source

```
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include <linux/usbdevice_fs.h>

/* For building without an updated set of headers */
#ifndef USBDEVFS_DROP_PRIVILEGES
#define USBDEVFS_DROP_PRIVILEGES		_IOW('U', 30, __u32)
#define USBDEVFS_CAP_DROP_PRIVILEGES		0x40
#endif

void drop_privileges(int fd, uint32_t mask)
{
	int res;

	res = ioctl(fd, USBDEVFS_DROP_PRIVILEGES, &mask);
	if (res)
		printf("ERROR: USBDEVFS_DROP_PRIVILEGES returned %d\n", res);
	else
		printf("OK: privileges dropped!\n");
}

void reset_device(int fd)
{
	int res;

	res = ioctl(fd, USBDEVFS_RESET);
	if (!res)
		printf("OK: USBDEVFS_RESET succeeded\n");
	else
		printf("ERROR: reset failed! (%d - %s)\n",
		       -res, strerror(-res));
}

void claim_some_intf(int fd)
{
	int i, res;

	for (i = 0; i < 4; i++) {
		res = ioctl(fd, USBDEVFS_CLAIMINTERFACE, &i);
		if (!res)
			printf("OK: claimed if %d\n", i);
		else
			printf("ERROR claiming if %d (%d - %s)\n",
			       i, -res, strerror(-res));
	}
}

int main(int argc, char *argv[])
{
	uint32_t mask, caps;
	int c, fd;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Failed to open file\n");
		goto err_fd;
	}

	/*
	 * check if dropping privileges is supported,
	 * bail on systems where the capability is not present
	 */
	ioctl(fd, USBDEVFS_GET_CAPABILITIES, &caps);
	if (!(caps & USBDEVFS_CAP_DROP_PRIVILEGES)) {
		printf("DROP_PRIVILEGES not supported\n");
		goto err;
	}

	/*
	 * Drop privileges but keep the ability to claim all
	 * free interfaces (i.e., those not used by kernel drivers)
	 */
	drop_privileges(fd, -1U);

	printf("Available options:\n"
		"[0] Exit now\n"
		"[1] Reset device. Should fail if device is in use\n"
		"[2] Claim 4 interfaces. Should succeed where not in use\n"
		"[3] Narrow interface permission mask\n"
		"Which option shall I run?: ");

	while (scanf("%d", &c) == 1) {
		switch (c) {
		case 0:
			goto exit;
		case 1:
			reset_device(fd);
			break;
		case 2:
			claim_some_intf(fd);
			break;
		case 3:
			printf("Insert new mask: ");
			scanf("%x", &mask);
			drop_privileges(fd, mask);
			break;
		default:
			printf("I don't recognize that\n");
		}

		printf("Which test shall I run next?: ");
	}

exit:
	close(fd);
	return 0;

err:
	close(fd);
err_fd:
	return 1;
}
```

# Credits

```
Credits for the Simple Linux USB Driver:

The following people have contributed to this code (in alphabetical
order by last name).  I'm sure this list should be longer, it's
difficult to maintain, add yourself with a patch if desired.

  Georg Acher <acher@informatik.tu-muenchen.de>
  David Brownell <dbrownell@users.sourceforge.net>
  Alan Cox <alan@lxorguk.ukuu.org.uk>
  Randy Dunlap <randy.dunlap@intel.com>
  Johannes Erdfelt <johannes@erdfelt.com>
  Deti Fliegl <deti@fliegl.de>
  ham <ham@unsuave.com>
  Bradley M Keryan <keryan@andrew.cmu.edu>
  Greg Kroah-Hartman <greg@kroah.com>
  Pavel Machek <pavel@suse.cz>
  Paul Mackerras <paulus@cs.anu.edu.au>
  Petko Manlolov <petkan@dce.bg>
  David E. Nelson <dnelson@jump.net>
  Vojtech Pavlik <vojtech@suse.cz>
  Bill Ryder <bryder@sgi.com>
  Thomas Sailer <sailer@ife.ee.ethz.ch>
  Gregory P. Smith <greg@electricrain.com>
  Linus Torvalds <torvalds@linux-foundation.org>
  Roman Weissgaerber <weissg@vienna.at>
  <Kazuki.Yasumatsu@fujixerox.co.jp>

Special thanks to:

  Inaky Perez Gonzalez <inaky@peloncho.fis.ucm.es> for starting the
  Linux USB driver effort and writing much of the larger uusbd driver.
  Much has been learned from that effort.

  The NetBSD & FreeBSD USB developers.  For being on the Linux USB list
  and offering suggestions and sharing implementation experiences.

Additional thanks to the following companies and people for donations
of hardware, support, time and development (this is from the original
THANKS file in Inaky's driver):

        The following corporations have helped us in the development
        of Linux USB / UUSBD:

        - 3Com GmbH for donating a ISDN Pro TA and supporting me
          in technical questions and with test equipment. I'd never 
          expect such a great help.

        - USAR Systems provided us with one of their excellent USB
          Evaluation Kits. It allows us to test the Linux-USB driver
          for compliance with the latest USB specification. USAR
          Systems recognized the importance of an up-to-date open
          Operating System and supports this project with
          Hardware. Thanks!.

        - Thanks to Intel Corporation for their precious help.

        - We teamed up with Cherry to make Linux the first OS with
          built-in USB support. Cherry is one of the biggest keyboard
          makers in the world.

        - CMD Technology, Inc. sponsored us kindly donating a CSA-6700
          PCI-to-USB Controller Board to test the OHCI implementation.

        - Due to their support to us, Keytronic can be sure that they
          will sell keyboards to some of the 3 million (at least)
          Linux users.

        - Many thanks to ing büro h doran [http://www.ibhdoran.com]!
          It was almost impossible to get a PC backplate USB connector
          for the motherboard here at Europe (mine, home-made, was
          quite lousy :). Now I know where to acquire nice USB stuff!

        - Genius Germany donated a USB mouse to test the mouse boot
          protocol. They've also donated a F-23 digital joystick and a
          NetMouse Pro. Thanks! 

        - AVM GmbH Berlin is supporting the development of the Linux
          USB driver for the AVM ISDN Controller B1 USB. AVM is a
          leading manufacturer for active and passive ISDN Controllers
          and CAPI 2.0-based software. The active design of the AVM B1
          is open for all OS platforms, including Linux.

        - Thanks to Y-E Data, Inc. for donating their FlashBuster-U
          USB Floppy Disk Drive, so we could test the bulk transfer
          code.

        - Many thanks to Logitech for contributing a three axis USB
          mouse. 

          Logitech designs, manufactures and markets
          Human Interface Devices, having a long history and
          experience in making devices such as keyboards, mice,
          trackballs, cameras, loudspeakers and control devices for
          gaming and professional use.

          Being a recognized vendor and seller for all these devices,
          they have donated USB mice, a joystick and a scanner, as a
          way to acknowledge the importance of Linux and to allow
          Logitech customers to enjoy support in their favorite
          operating systems and all Linux users to use Logitech and
          other USB hardware.

          Logitech is official sponsor of the Linux Conference on
          Feb. 11th 1999 in Vienna, where we'll will present the
          current state of the Linux USB effort.

        - CATC has provided means to uncover dark corners of the UHCI
          inner workings with a USB Inspector.

        - Thanks to Entrega for providing PCI to USB cards, hubs and
          converter products for development. 

        - Thanks to ConnectTech for providing a WhiteHEAT usb to
          serial converter, and the documentation for the device to
          allow a driver to be written.

        - Thanks to ADMtek for providing Pegasus and Pegasus II
          evaluation boards, specs and valuable advices during
          the driver development.
        
        And thanks go to (hey! in no particular order :)

        - Oren Tirosh <orenti@hishome.net>, for standing so patiently
          all my doubts'bout USB and giving lots of cool ideas.

        - Jochen Karrer <karrer@wpfd25.physik.uni-wuerzburg.de>, for
          pointing out mortal bugs and giving advice.

        - Edmund Humemberger <ed@atnet.at>, for his great work on
          public relationships and general management stuff for the
          Linux-USB effort.

        - Alberto Menegazzi <flash@flash.iol.it> is starting the
          documentation for the UUSBD. Go for it!

        - Ric Klaren <ia_ric@cs.utwente.nl> for doing nice
          introductory documents (competing with Alberto's :).

        - Christian Groessler <cpg@aladdin.de>, for his help on those
          itchy bits ... :)

        - Paul MacKerras for polishing OHCI and pushing me harder for
          the iMac support, giving improvements and enhancements.

        - Fernando Herrera <fherrera@eurielec.etsit.upm.es> has taken
          charge of composing, maintaining and feeding the
          long-awaited, unique and marvelous UUSBD FAQ! Tadaaaa!!!

        - Rasca Gmelch <thron@gmx.de> has revived the raw driver and
          pointed bugs, as well as started the uusbd-utils package.

        - Peter Dettori <dettori@ozy.dec.com> is uncovering bugs like
          crazy, as well as making cool suggestions, great :)

        - All the Free Software and Linux community, the FSF & the GNU
          project, the MIT X consortium, the TeX people ... everyone!
          You know who you are!

        - Big thanks to Richard Stallman for creating Emacs!

        - The people at the linux-usb mailing list, for reading so
          many messages :) Ok, no more kidding; for all your advises!

        - All the people at the USB Implementers Forum for their
          help and assistance.

        - Nathan Myers <ncm@cantrip.org>, for his advice! (hope you
          liked Cibeles' party).

        - Linus Torvalds, for starting, developing and managing Linux.

        - Mike Smith, Craig Keithley, Thierry Giron and Janet Schank
          for convincing me USB Standard hubs are not that standard
          and that's good to allow for vendor specific quirks on the
          standard hub driver.
```
