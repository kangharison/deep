# 7.1.The bttv driver

> 출처(원문): https://docs.kernel.org/admin-guide/media/bttv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.1. The bttv driver

## 7.1.1. Release notes for bttv

You’ll need at least these config options for bttv:

```
./scripts/config -e PCI
./scripts/config -m I2C
./scripts/config -m INPUT
./scripts/config -m MEDIA_SUPPORT
./scripts/config -e MEDIA_PCI_SUPPORT
./scripts/config -e MEDIA_ANALOG_TV_SUPPORT
./scripts/config -e MEDIA_DIGITAL_TV_SUPPORT
./scripts/config -e MEDIA_RADIO_SUPPORT
./scripts/config -e RC_CORE
./scripts/config -m VIDEO_BT848
```

If your board has digital TV, you’ll also need:

```
./scripts/config -m DVB_BT8XX
```

In this case, please see [How to get the bt8xx cards working](bt8xx.html)
for additional notes.

## 7.1.2. Make bttv work with your card

If you have bttv compiled and installed, just booting the Kernel
should be enough for it to try probing it. However, depending
on the model, the Kernel may require additional information about
the hardware, as the device may not be able to provide such info
directly to the Kernel.

If it doesn’t bttv likely could not autodetect your card and needs some
insmod options. The most important insmod option for bttv is “card=n”
to select the correct card type. If you get video but no sound you’ve
very likely specified the wrong (or no) card type. A list of supported
cards is in [BTTV cards list](bttv-cardlist.html).

If bttv takes very long to load (happens sometimes with the cheap
cards which have no tuner), try adding this to your modules configuration
file (usually, it is either `/etc/modules.conf` or some file at
`/etc/modules-load.d/`, but the actual place depends on your
distribution):

```
options i2c-algo-bit bit_test=1
```

Some cards may require an extra firmware file to work. For example,
for the WinTV/PVR you need one firmware file from its driver CD,
called: `hcwamc.rbf`. It is inside a self-extracting zip file
called `pvr45xxx.exe`. Just placing it at the `/etc/firmware`
directory should be enough for it to be autoload during the driver’s
probing mode (e. g. when the Kernel boots or when the driver is
manually loaded via `modprobe` command).

If your card isn’t listed in [BTTV cards list](bttv-cardlist.html)
or if you have trouble making audio work, please read [Still doesn’t work?](#still-doesnt-work).

## 7.1.3. Autodetecting cards

bttv uses the PCI Subsystem ID to autodetect the card type. lspci lists
the Subsystem ID in the second line, looks like this:

```
00:0a.0 Multimedia video controller: Brooktree Corporation Bt878 (rev 02)
        Subsystem: Hauppauge computer works Inc. WinTV/GO
        Flags: bus master, medium devsel, latency 32, IRQ 5
        Memory at e2000000 (32-bit, prefetchable) [size=4K]
```

only bt878-based cards can have a subsystem ID (which does not mean
that every card really has one). bt848 cards can’t have a Subsystem
ID and therefore can’t be autodetected. There is a list with the ID’s
at [BTTV cards list](bttv-cardlist.html)
(in case you are interested or want to mail patches with updates).

## 7.1.4. Still doesn’t work?

I do NOT have a lab with 30+ different grabber boards and a
PAL/NTSC/SECAM test signal generator at home, so I often can’t
reproduce your problems. This makes debugging very difficult for me.

If you have some knowledge and spare time, please try to fix this
yourself (patches very welcome of course...) You know: The linux
slogan is “Do it yourself”.

There is a mailing list at
<http://vger.kernel.org/vger-lists.html#linux-media>

If you have trouble with some specific TV card, try to ask there
instead of mailing me directly. The chance that someone with the
same card listens there is much higher...

For problems with sound: There are a lot of different systems used
for TV sound all over the world. And there are also different chips
which decode the audio signal. Reports about sound problems (“stereo
doesn’t work”) are pretty useless unless you include some details
about your hardware and the TV sound scheme used in your country (or
at least the country you are living in).

## 7.1.5. Modprobe options

Note

The following argument list can be outdated, as we might add more
options if ever needed. In case of doubt, please check with
`modinfo <module>`.

This command prints various information about a kernel
module, among them a complete and up-to-date list of insmod options.

bttv
:   > The bt848/878 (grabber chip) driver

    insmod args:

    ```
        card=n              card type, see CARDLIST for a list.
        tuner=n             tuner type, see CARDLIST for a list.
        radio=0/1   card supports radio
        pll=0/1/2   pll settings

                        0: don't use PLL
                        1: 28 MHz crystal installed
                        2: 35 MHz crystal installed

        triton1=0/1     for Triton1 (+others) compatibility
        vsfx=0/1    yet another chipset bug compatibility bit
                        see README.quirks for details on these two.

        bigendian=n Set the endianness of the gfx framebuffer.
                        Default is native endian.
        fieldnr=0/1 Count fields.  Some TV descrambling software
                        needs this, for others it only generates
                        50 useless IRQs/sec.  default is 0 (off).
        autoload=0/1        autoload helper modules (tuner, audio).
                        default is 1 (on).
        bttv_verbose=0/1/2  verbose level (at insmod time, while
                        looking at the hardware).  default is 1.
        bttv_debug=0/1      debug messages (for capture).
                        default is 0 (off).
        irq_debug=0/1       irq handler debug messages.
                        default is 0 (off).
        gbuffers=2-32       number of capture buffers for mmap'ed capture.
                        default is 4.
        gbufsize=   size of capture buffers. default and
                        maximum value is 0x208000 (~2MB)
        no_overlay=0        Enable overlay on broken hardware.  There
                        are some chipsets (SIS for example) which
                        are known to have problems with the PCI DMA
                        push used by bttv.  bttv will disable overlay
                        by default on this hardware to avoid crashes.
                        With this insmod option you can override this.
        no_overlay=1        Disable overlay. It should be used by broken
                        hardware that doesn't support PCI2PCI direct
                        transfers.
        automute=0/1        Automatically mutes the sound if there is
                        no TV signal, on by default.  You might try
                        to disable this if you have bad input signal
                        quality which leading to unwanted sound
                        dropouts.
        chroma_agc=0/1      AGC of chroma signal, off by default.
        adc_crush=0/1       Luminance ADC crush, on by default.
        i2c_udelay=     Allow reduce I2C speed. Default is 5 usecs
                        (meaning 66,67 Kbps). The default is the
                        maximum supported speed by kernel bitbang
                        algorithm. You may use lower numbers, if I2C
                        messages are lost (16 is known to work on
                        all supported cards).

        bttv_gpio=0/1
        gpiomask=
        audioall=
        audiomux=
                        See Sound-FAQ for a detailed description.

    remap, card, radio and pll accept up to four comma-separated arguments
    (for multiple boards).
    ```

tuner
:   The tuner driver. You need this unless you want to use only
    with a camera or the board doesn’t provide analog TV tuning.

    insmod args:

    ```
    debug=1         print some debug info to the syslog
    type=n          type of the tuner chip. n as follows:
                    see CARDLIST for a complete list.
    pal=[bdgil]     select PAL variant (used for some tuners
                    only, important for the audio carrier).
    ```

tvaudio
:   Provide a single driver for all simple i2c audio control
    chips (tda/tea\*).

    insmod args:

    ```
    tda8425  = 1    enable/disable the support for the
    tda9840  = 1    various chips.
    tda9850  = 1    The tea6300 can't be autodetected and is
    tda9855  = 1    therefore off by default, if you have
    tda9873  = 1    this one on your card (STB uses these)
    tda9874a = 1    you have to enable it explicitly.
    tea6300  = 0    The two tda985x chips use the same i2c
    tea6420  = 1    address and can't be disturgished from
    pic16c54 = 1    each other, you might have to disable
                    the wrong one.
    debug = 1       print debug messages
    ```

msp3400
:   The driver for the msp34xx sound processor chips. If you have a
    stereo card, you probably want to insmod this one.

    insmod args:

    ```
    debug=1/2       print some debug info to the syslog,
                    2 is more verbose.
    simple=1        Use the "short programming" method.  Newer
                    msp34xx versions support this.  You need this
                    for dbx stereo.  Default is on if supported by
                    the chip.
    once=1          Don't check the TV-stations Audio mode
                    every few seconds, but only once after
                    channel switches.
    amsound=1       Audio carrier is AM/NICAM at 6.5 Mhz.  This
                    should improve things for french people, the
                    carrier autoscan seems to work with FM only...
    ```

## 7.1.6. If the box freezes hard with bttv

It might be a bttv driver bug. It also might be bad hardware. It also
might be something else ...

Just mailing me “bttv freezes” isn’t going to help much. This README
has a few hints how you can help to pin down the problem.

### 7.1.6.1. bttv bugs

If some version works and another doesn’t it is likely to be a driver
bug. It is very helpful if you can tell where exactly it broke
(i.e. the last working and the first broken version).

With a hard freeze you probably doesn’t find anything in the logfiles.
The only way to capture any kernel messages is to hook up a serial
console and let some terminal application log the messages. /me uses
screen. See [Linux Serial Console](../serial-console.html) for details on
setting up a serial console.

Read [Bug hunting](../bug-hunting.html) to learn how to get any useful
information out of a register+stack dump printed by the kernel on
protection faults (so-called “kernel oops”).

If you run into some kind of deadlock, you can try to dump a call trace
for each process using sysrq-t (see [Linux Magic System Request Key Hacks](../sysrq.html)).
This way it is possible to figure where *exactly* some process in “D”
state is stuck.

I’ve seen reports that bttv 0.7.x crashes whereas 0.8.x works rock solid
for some people. Thus probably a small buglet left somewhere in bttv
0.7.x. I have no idea where exactly, it works stable for me and a lot of
other people. But in case you have problems with the 0.7.x versions you
can give 0.8.x a try ...

### 7.1.6.2. hardware bugs

Some hardware can’t deal with PCI-PCI transfers (i.e. grabber => vga).
Sometimes problems show up with bttv just because of the high load on
the PCI bus. The bt848/878 chips have a few workarounds for known
incompatibilities, see README.quirks.

Some folks report that increasing the pci latency helps too,
althrought I’m not sure whenever this really fixes the problems or
only makes it less likely to happen. Both bttv and btaudio have a
insmod option to set the PCI latency of the device.

Some mainboard have problems to deal correctly with multiple devices
doing DMA at the same time. bttv + ide seems to cause this sometimes,
if this is the case you likely see freezes only with video and hard disk
access at the same time. Updating the IDE driver to get the latest and
greatest workarounds for hardware bugs might fix these problems.

### 7.1.6.3. other

If you use some binary-only yunk (like nvidia module) try to reproduce
the problem without.

IRQ sharing is known to cause problems in some cases. It works just
fine in theory and many configurations. Neverless it might be worth a
try to shuffle around the PCI cards to give bttv another IRQ or make
it share the IRQ with some other piece of hardware. IRQ sharing with
VGA cards seems to cause trouble sometimes. I’ve also seen funny
effects with bttv sharing the IRQ with the ACPI bridge (and
apci-enabled kernel).

## 7.1.7. Bttv quirks

Below is what the bt878 data book says about the PCI bug compatibility
modes of the bt878 chip.

The triton1 insmod option sets the EN\_TBFX bit in the control register.
The vsfx insmod option does the same for EN\_VSFX bit. If you have
stability problems you can try if one of these options makes your box
work solid.

drivers/pci/quirks.c knows about these issues, this way these bits are
enabled automagically for known-buggy chipsets (look at the kernel
messages, bttv tells you).

### 7.1.7.1. Normal PCI Mode

The PCI REQ signal is the logical-or of the incoming function requests.
The inter-nal GNT[0:1] signals are gated asynchronously with GNT and
demultiplexed by the audio request signal. Thus the arbiter defaults to
the video function at power-up and parks there during no requests for
bus access. This is desirable since the video will request the bus more
often. However, the audio will have highest bus access priority. Thus
the audio will have first access to the bus even when issuing a request
after the video request but before the PCI external arbiter has granted
access to the Bt879. Neither function can preempt the other once on the
bus. The duration to empty the entire video PCI FIFO onto the PCI bus is
very short compared to the bus access latency the audio PCI FIFO can
tolerate.

### 7.1.7.2. 430FX Compatibility Mode

When using the 430FX PCI, the following rules will ensure
compatibility:

> 1. Deassert REQ at the same time as asserting FRAME.
> 2. Do not reassert REQ to request another bus transaction until after
>    finish-ing the previous transaction.

Since the individual bus masters do not have direct control of REQ, a
simple logical-or of video and audio requests would violate the rules.
Thus, both the arbiter and the initiator contain 430FX compatibility
mode logic. To enable 430FX mode, set the EN\_TBFX bit as indicated in
Device Control Register on page 104.

When EN\_TBFX is enabled, the arbiter ensures that the two compatibility
rules are satisfied. Before GNT is asserted by the PCI arbiter, this
internal arbiter may still logical-or the two requests. However, once
the GNT is issued, this arbiter must lock in its decision and now route
only the granted request to the REQ pin. The arbiter decision lock
happens regardless of the state of FRAME because it does not know when
FRAME will be asserted (typically - each initiator will assert FRAME on
the cycle following GNT). When FRAME is asserted, it is the initiator s
responsibility to remove its request at the same time. It is the
arbiters responsibility to allow this request to flow through to REQ and
not allow the other request to hold REQ asserted. The decision lock may
be removed at the end of the transaction: for example, when the bus is
idle (FRAME and IRDY). The arbiter decision may then continue
asynchronously until GNT is again asserted.

### 7.1.7.3. Interfacing with Non-PCI 2.1 Compliant Core Logic

A small percentage of core logic devices may start a bus transaction
during the same cycle that GNT is de-asserted. This is non PCI 2.1
compliant. To ensure compatibility when using PCs with these PCI
controllers, the EN\_VSFX bit must be enabled (refer to Device Control
Register on page 104). When in this mode, the arbiter does not pass GNT
to the internal functions unless REQ is asserted. This prevents a bus
transaction from starting the same cycle as GNT is de-asserted. This
also has the side effect of not being able to take advantage of bus
parking, thus lowering arbitration performance. The Bt879 drivers must
query for these non-compliant devices, and set the EN\_VSFX bit only if
required.

### 7.1.7.4. Other elements of the tvcards array

If you are trying to make a new card work you might find it useful to
know what the other elements in the tvcards array are good for:

```
video_inputs    - # of video inputs the card has
audio_inputs    - historical cruft, not used any more.
tuner           - which input is the tuner
svhs            - which input is svhs (all others are labeled composite)
muxsel          - video mux, input->registervalue mapping
pll             - same as pll= insmod option
tuner_type      - same as tuner= insmod option
*_modulename    - hint whenever some card needs this or that audio
                module loaded to work properly.
has_radio       - whenever this TV card has a radio tuner.
no_msp34xx      - "1" disables loading of msp3400.o module
no_tda9875      - "1" disables loading of tda9875.o module
needs_tvaudio   - set to "1" to load tvaudio.o module
```

If some config item is specified both from the tvcards array and as
insmod option, the insmod option takes precedence.

## 7.1.8. Cards

Note

For a more updated list, please check
<https://linuxtv.org/wiki/index.php/Hardware_Device_Information>

### 7.1.8.1. Supported cards: Bt848/Bt848a/Bt849/Bt878/Bt879 cards

All cards with Bt848/Bt848a/Bt849/Bt878/Bt879 and normal
Composite/S-VHS inputs are supported. Teletext and Intercast support
(PAL only) for ALL cards via VBI sample decoding in software.

Some cards with additional multiplexing of inputs or other additional
fancy chips are only partially supported (unless specifications by the
card manufacturer are given). When a card is listed here it isn’t
necessarily fully supported.

All other cards only differ by additional components as tuners, sound
decoders, EEPROMs, teletext decoders ...

### 7.1.8.2. MATRIX Vision

MV-Delta
- Bt848A
- 4 Composite inputs, 1 S-VHS input (shared with 4th composite)
- EEPROM

<http://www.matrix-vision.de/>

This card has no tuner but supports all 4 composite (1 shared with an
S-VHS input) of the Bt848A.
Very nice card if you only have satellite TV but several tuners connected
to the card via composite.

Many thanks to Matrix-Vision for giving us 2 cards for free which made
Bt848a/Bt849 single crystal operation support possible!!!

### 7.1.8.3. Miro/Pinnacle PCTV

* Bt848
  some (all??) come with 2 crystals for PAL/SECAM and NTSC
* PAL, SECAM or NTSC TV tuner (Philips or TEMIC)
* MSP34xx sound decoder on add on board
  decoder is supported but AFAIK does not yet work
  (other sound MUX setting in GPIO port needed??? somebody who fixed this???)
* 1 tuner, 1 composite and 1 S-VHS input
* tuner type is autodetected

<http://www.miro.de/>
<http://www.miro.com/>

Many thanks for the free card which made first NTSC support possible back
in 1997!

### 7.1.8.4. Hauppauge Win/TV pci

There are many different versions of the Hauppauge cards with different
tuners (TV+Radio ...), teletext decoders.
Note that even cards with same model numbers have (depending on the revision)
different chips on it.

* Bt848 (and others but always in 2 crystal operation???)
  newer cards have a Bt878
* PAL, SECAM, NTSC or tuner with or without Radio support

e.g.:

* PAL:

  + TDA5737: VHF, hyperband and UHF mixer/oscillator for TV and VCR 3-band tuners
  + TSA5522: 1.4 GHz I2C-bus controlled synthesizer, I2C 0xc2-0xc3
* NTSC:

  + TDA5731: VHF, hyperband and UHF mixer/oscillator for TV and VCR 3-band tuners
  + TSA5518: no datasheet available on Philips site
* Philips SAA5246 or SAA5284 ( or no) Teletext decoder chip
  with buffer RAM (e.g. Winbond W24257AS-35: 32Kx8 CMOS static RAM)
  SAA5246 (I2C 0x22) is supported
* 256 bytes EEPROM: Microchip 24LC02B or Philips 8582E2Y
  with configuration information
  I2C address 0xa0 (24LC02B also responds to 0xa2-0xaf)
* 1 tuner, 1 composite and (depending on model) 1 S-VHS input
* 14052B: mux for selection of sound source
* sound decoder: TDA9800, MSP34xx (stereo cards)

### 7.1.8.5. Askey CPH-Series

Developed by TelSignal(?), OEMed by many vendors (Typhoon, Anubis, Dynalink)

* Card series:
  - CPH01x: BT848 capture only
  - CPH03x: BT848
  - CPH05x: BT878 with FM
  - CPH06x: BT878 (w/o FM)
  - CPH07x: BT878 capture only
* TV standards:
  - CPH0x0: NTSC-M/M
  - CPH0x1: PAL-B/G
  - CPH0x2: PAL-I/I
  - CPH0x3: PAL-D/K
  - CPH0x4: SECAM-L/L
  - CPH0x5: SECAM-B/G
  - CPH0x6: SECAM-D/K
  - CPH0x7: PAL-N/N
  - CPH0x8: PAL-B/H
  - CPH0x9: PAL-M/M
* CPH03x was often sold as “TV capturer”.

Identifying:

> 1. 878 cards can be identified by PCI Subsystem-ID:
>    - 144f:3000 = CPH06x
>    - 144F:3002 = CPH05x w/ FM
>    - 144F:3005 = CPH06x\_LC (w/o remote control)
> 2. The cards have a sticker with “CPH”-model on the back.
> 3. These cards have a number printed on the PCB just above the tuner metal box:
>    - “80-CP2000300-x” = CPH03X
>    - “80-CP2000500-x” = CPH05X
>    - “80-CP2000600-x” = CPH06X / CPH06x\_LC
>
> Askey sells these cards as “Magic TView series”, Brand “MagicXpress”.
> Other OEM often call these “Tview”, “TView99” or else.

### 7.1.8.6. Lifeview Flyvideo Series:

The naming of these series differs in time and space.

Identifying:
:   1. Some models can be identified by PCI subsystem ID:

       * 1852:1852 = Flyvideo 98 FM
       * 1851:1850 = Flyvideo 98
       * 1851:1851 = Flyvideo 98 EZ (capture only)
    2. There is a print on the PCB:

       * LR25 = Flyvideo (Zoran ZR36120, SAA7110A)
       * LR26 Rev.N = Flyvideo II (Bt848)
       * LR26 Rev.O = Flyvideo II (Bt878)
       * LR37 Rev.C = Flyvideo EZ (Capture only, ZR36120 + SAA7110)
       * LR38 Rev.A1= Flyvideo II EZ (Bt848 capture only)
       * LR50 Rev.Q = Flyvideo 98 (w/eeprom and PCI subsystem ID)
       * LR50 Rev.W = Flyvideo 98 (no eeprom)
       * LR51 Rev.E = Flyvideo 98 EZ (capture only)
       * LR90 = Flyvideo 2000 (Bt878)
       * LR90 Flyvideo 2000S (Bt878) w/Stereo TV (Package incl. LR91 daughterboard)
       * LR91 = Stereo daughter card for LR90
       * LR97 = Flyvideo DVBS
       * LR99 Rev.E = Low profile card for OEM integration (only internal audio!) bt878
       * LR136 = Flyvideo 2100/3100 (Low profile, SAA7130/SAA7134)
       * LR137 = Flyvideo DV2000/DV3000 (SAA7130/SAA7134 + IEEE1394)
       * LR138 Rev.C= Flyvideo 2000 (SAA7130)
       * LR138 Flyvideo 3000 (SAA7134) w/Stereo TV

         > + These exist in variations w/FM and w/Remote sometimes denoted
         >   by suffixes “FM” and “R”.
    3. You have a laptop (miniPCI card):

       > * Product = FlyTV Platinum Mini
       > * Model/Chip = LR212/saa7135
       > * Lifeview.com.tw states (Feb. 2002):
       >   “The FlyVideo2000 and FlyVideo2000s product name have renamed to FlyVideo98.”
       >   Their Bt8x8 cards are listed as discontinued.
       > * Flyvideo 2000S was probably sold as Flyvideo 3000 in some countries(Europe?).
       >   The new Flyvideo 2000/3000 are SAA7130/SAA7134 based.

“Flyvideo II” had been the name for the 848 cards, nowadays (in Germany)
this name is re-used for LR50 Rev.W.

The Lifeview website mentioned Flyvideo III at some time, but such a card
has not yet been seen (perhaps it was the german name for LR90 [stereo]).
These cards are sold by many OEMs too.

FlyVideo A2 (Elta 8680)= LR90 Rev.F (w/Remote, w/o FM, stereo TV by tda9821) {Germany}

Lifeview 3000 (Elta 8681) as sold by Plus(April 2002), Germany = LR138 w/ saa7134

#### 7.1.8.6.1. lifeview config coding on gpio pins 0-9

* LR50 rev. Q (“PARTS: 7031505116), Tuner wurde als Nr. 5 erkannt, Eingänge
  SVideo, TV, Composite, Audio, Remote:

> * CP9..1=100001001 (1: 0-Ohm-Widerstand gegen GND unbestückt; 0: bestückt)

### 7.1.8.7. Typhoon TV card series:

These can be CPH, Flyvideo, Pixelview or KNC1 series.

Typhoon is the brand of Anubis.

Model 50680 got re-used, some model no. had different contents over time.

Models:

> * 50680 “TV Tuner PCI Pal BG”(old,red package)=can be CPH03x(bt848) or CPH06x(bt878)
> * 50680 “TV Tuner Pal BG” (blue package)= Pixelview PV-BT878P+ (Rev 9B)
> * 50681 “TV Tuner PCI Pal I” (variant of 50680)
> * 50682 “TView TV/FM Tuner Pal BG” = Flyvideo 98FM (LR50 Rev.Q)
>
> Note
>
> The package has a picture of CPH05x (which would be a real TView)
>
> * 50683 “TV Tuner PCI SECAM” (variant of 50680)
> * 50684 “TV Tuner Pal BG” = Pixelview 878TV(Rev.3D)
> * 50686 “TV Tuner” = KNC1 TV Station
> * 50687 “TV Tuner stereo” = KNC1 TV Station pro
> * 50688 “TV Tuner RDS” (black package) = KNC1 TV Station RDS
> * 50689 TV SAT DVB-S CARD CI PCI (SAA7146AH, SU1278?) = “KNC1 TV Station DVB-S”
> * 50692 “TV/FM Tuner” (small PCB)
> * 50694 TV TUNER CARD RDS (PHILIPS CHIPSET SAA7134HL)
> * 50696 TV TUNER STEREO (PHILIPS CHIPSET SAA7134HL, MK3ME Tuner)
> * 50804 PC-SAT TV/Audio Karte = Techni-PC-Sat (ZORAN 36120PQC, Tuner:Alps)
> * 50866 TVIEW SAT RECEIVER+ADR
> * 50868 “TV/FM Tuner Pal I” (variant of 50682)
> * 50999 “TV/FM Tuner Secam” (variant of 50682)

### 7.1.8.8. Guillemot

Models:

* Maxi-TV PCI (ZR36120)
* Maxi TV Video 2 = LR50 Rev.Q (FI1216MF, PAL BG+SECAM)
* Maxi TV Video 3 = CPH064 (PAL BG + SECAM)

### 7.1.8.9. Mentor

Mentor TV card (“55-878TV-U1”) = Pixelview 878TV(Rev.3F) (w/FM w/Remote)

### 7.1.8.10. Prolink

* TV cards:

  + PixelView Play TV pro - (Model: PV-BT878P+ REV 8E)
  + PixelView Play TV pro - (Model: PV-BT878P+ REV 9D)
  + PixelView Play TV pro - (Model: PV-BT878P+ REV 4C / 8D / 10A )
  + PixelView Play TV - (Model: PV-BT848P+)
  + 878TV - (Model: PV-BT878TV)
* Multimedia TV packages (card + software pack):

  + PixelView Play TV Theater - (Model: PV-M4200) = PixelView Play TV pro + Software
  + PixelView Play TV PAK - (Model: PV-BT878P+ REV 4E)
  + PixelView Play TV/VCR - (Model: PV-M3200 REV 4C / 8D / 10A )
  + PixelView Studio PAK - (Model: M2200 REV 4C / 8D / 10A )
  + PixelView PowerStudio PAK - (Model: PV-M3600 REV 4E)
  + PixelView DigitalVCR PAK - (Model: PV-M2400 REV 4C / 8D / 10A )
  + PixelView PlayTV PAK II (TV/FM card + usb camera) PV-M3800
  + PixelView PlayTV XP PV-M4700,PV-M4700(w/FM)
  + PixelView PlayTV DVR PV-M4600 package contents:PixelView PlayTV pro, windvr & videoMail s/w
* Further Cards:

  + PV-BT878P+rev.9B (Play TV Pro, opt. w/FM w/NICAM)
  + PV-BT878P+rev.2F
  + PV-BT878P Rev.1D (bt878, capture only)
  + XCapture PV-CX881P (cx23881)
  + PlayTV HD PV-CX881PL+, PV-CX881PL+(w/FM) (cx23881)
  + DTV3000 PV-DTV3000P+ DVB-S CI = Twinhan VP-1030
  + DTV2000 DVB-S = Twinhan VP-1020
* Video Conferencing:

  + PixelView Meeting PAK - (Model: PV-BT878P)
  + PixelView Meeting PAK Lite - (Model: PV-BT878P)
  + PixelView Meeting PAK plus - (Model: PV-BT878P+rev 4C/8D/10A)
  + PixelView Capture - (Model: PV-BT848P)
  + PixelView PlayTV USB pro
  + Model No. PV-NT1004+, PV-NT1004+ (w/FM) = NT1004 USB decoder chip + SAA7113 video decoder chip

### 7.1.8.11. Dynalink

These are CPH series.

### 7.1.8.12. Phoebemicro

* TV Master = CPH030 or CPH060
* TV Master FM = CPH050

### 7.1.8.13. Genius/Kye

* Video Wonder/Genius Internet Video Kit = LR37 Rev.C
* Video Wonder Pro II (848 or 878) = LR26

### 7.1.8.14. Tekram

* VideoCap C205 (Bt848)
* VideoCap C210 (zr36120 +Philips)
* CaptureTV M200 (ISA)
* CaptureTV M205 (Bt848)

### 7.1.8.15. Lucky Star

* Image World Conference TV = LR50 Rev. Q

### 7.1.8.16. Leadtek

* WinView 601 (Bt848)
* WinView 610 (Zoran)
* WinFast2000
* WinFast2000 XP

#### 7.1.8.16.1. Support for the Leadtek WinView 601 TV/FM

Author of this section: Jon Tombs <[jon@gte.esi.us.es](mailto:jon%40gte.esi.us.es)>

This card is basically the same as all the rest (Bt484A, Philips tuner),
the main difference is that they have attached a programmable attenuator to 3
GPIO lines in order to give some volume control. They have also stuck an
infra-red remote control decoded on the board, I will add support for this
when I get time (it simple generates an interrupt for each key press, with
the key code is placed in the GPIO port).

I don’t yet have any application to test the radio support. The tuner
frequency setting should work but it is possible that the audio multiplexer
is wrong. If it doesn’t work, send me email.

* No Thanks to Leadtek they refused to answer any questions about their
  hardware. The driver was written by visual inspection of the card. If you
  use this driver, send an email insult to them, and tell them you won’t
  continue buying their hardware unless they support Linux.
* Little thanks to Princeton Technology Corp (<http://www.princeton.com.tw>)
  who make the audio attenuator. Their publicly available data-sheet available
  on their web site doesn’t include the chip programming information! Hidden
  on their server are the full data-sheets, but don’t ask how I found it.

To use the driver I use the following options, the tuner and pll settings might
be different in your country. You can force it via modprobe parameters.
For example:

```
modprobe bttv  tuner=1 pll=28 radio=1 card=17
```

Sets tuner type 1 (Philips PAL\_I), PLL with a 28 MHz crystal, enables
FM radio and selects bttv card ID 17 (Leadtek WinView 601).

### 7.1.8.17. KNC One

* TV-Station
* TV-Station SE (+Software Bundle)
* TV-Station pro (+TV stereo)
* TV-Station FM (+Radio)
* TV-Station RDS (+RDS)
* TV Station SAT (analog satellite)
* TV-Station DVB-S

Note

newer Cards have saa7134, but model name stayed the same?

### 7.1.8.18. Provideo

* PV951 or PV-951, now named PV-951T
  (also are sold as:
  Boeder TV-FM Video Capture Card,
  Titanmedia Supervision TV-2400,
  Provideo PV951 TF,
  3DeMon PV951,
  MediaForte TV-Vision PV951,
  Yoko PV951,
  Vivanco Tuner Card PCI Art.-Nr.: 68404
  )
* Surveillance Series:

> * PV-141
> * PV-143
> * PV-147
> * PV-148 (capture only)
> * PV-150
> * PV-151

* TV-FM Tuner Series:

> * PV-951TDV (tv tuner + 1394)
> * PV-951T/TF
> * PV-951PT/TF
> * PV-956T/TF Low Profile
> * PV-911

### 7.1.8.19. Highscreen

Models:

* TV Karte = LR50 Rev.S
* TV-Boostar = Terratec Terra TV+ Version 1.0 (Bt848, tda9821) “ceb105.pcb”

### 7.1.8.20. Zoltrix

Models:

* Face to Face Capture (Bt848 capture only) (PCB “VP-2848”)
* Face To Face TV MAX (Bt848) (PCB “VP-8482 Rev1.3”)
* Genie TV (Bt878) (PCB “VP-8790 Rev 2.1”)
* Genie Wonder Pro

### 7.1.8.21. AVerMedia

* AVer FunTV Lite (ISA, AV3001 chipset) “M101.C”
* AVerTV
* AVerTV Stereo
* AVerTV Studio (w/FM)
* AVerMedia TV98 with Remote
* AVerMedia TV/FM98 Stereo
* AVerMedia TVCAM98
* TVCapture (Bt848)
* TVPhone (Bt848)
* TVCapture98 (=”AVerMedia TV98” in USA) (Bt878)
* TVPhone98 (Bt878, w/FM)

| PCB | PCI-ID | Model-Name | Eeprom | Tuner | Sound | Country |
| --- | --- | --- | --- | --- | --- | --- |
| M101.C | ISA ! |  |  |  |  |  |
| M108-B | Bt848 |  | -- | FR1236 |  | US [[2]](#f2), [[3]](#f3) |
| M1A8-A | Bt848 | AVer TV-Phone |  | FM1216 | -- |  |
| M168-T | 1461:0003 | AVerTV Studio | 48:17 | FM1216 | TDA9840T | D [[1]](#f1) w/FM w/Remote |
| M168-U | 1461:0004 | TVCapture98 | 40:11 | FI1216 | -- | D w/Remote |
| M168II-B | 1461:0003 | Medion MD9592 | 48:16 | FM1216 | TDA9873H | D w/FM |

[[1](#id3)]

Daughterboard MB68-A with TDA9820T and TDA9840T


[[2](#id1)]

Sony NE41S soldered (stereo sound?)


[[3](#id2)]

Daughterboard M118-A w/ pic 16c54 and 4 MHz quartz

* US site has different drivers for (as of 09/2002):

  + EZ Capture/InterCam PCI (BT-848 chip)
  + EZ Capture/InterCam PCI (BT-878 chip)
  + TV-Phone (BT-848 chip)
  + TV98 (BT-848 chip)
  + TV98 With Remote (BT-848 chip)
  + TV98 (BT-878 chip)
  + TV98 With Remote (BT-878)
  + TV/FM98 (BT-878 chip)
  + AVerTV
  + AverTV Stereo
  + AVerTV Studio

DE hat diverse Treiber fuer diese Modelle (Stand 09/2002):

> * TVPhone (848) mit Philips tuner FR12X6 (w/ FM radio)
> * TVPhone (848) mit Philips tuner FM12X6 (w/ FM radio)
> * TVCapture (848) w/Philips tuner FI12X6
> * TVCapture (848) non-Philips tuner
> * TVCapture98 (Bt878)
> * TVPhone98 (Bt878)
> * AVerTV und TVCapture98 w/VCR (Bt 878)
> * AVerTVStudio und TVPhone98 w/VCR (Bt878)
> * AVerTV GO Series (Kein SVideo Input)
> * AVerTV98 (BT-878 chip)
> * AVerTV98 mit Fernbedienung (BT-878 chip)
> * AVerTV/FM98 (BT-878 chip)
> * VDOmate (www.averm.com.cn) = M168U ?

### 7.1.8.22. Aimslab

Models:

* Video Highway or “Video Highway TR200” (ISA)
* Video Highway Xtreme (aka “VHX”) (Bt848, FM w/ TEA5757)

### 7.1.8.23. IXMicro (former: IMS=Integrated Micro Solutions)

Models:

* IXTV BT848 (=TurboTV)
* IXTV BT878
* IMS TurboTV (Bt848)

### 7.1.8.24. Lifetec/Medion/Tevion/Aldi

Models:

* LT9306/MD9306 = CPH061
* LT9415/MD9415 = LR90 Rev.F or Rev.G
* MD9592 = Avermedia TVphone98 (PCI\_ID=1461:0003), PCB-Rev=M168II-B (w/TDA9873H)
* MD9717 = KNC One (Rev D4, saa7134, FM1216 MK2 tuner)
* MD5044 = KNC One (Rev D4, saa7134, FM1216ME MK3 tuner)

### 7.1.8.25. Modular Technologies (www.modulartech.com) UK

Models:

* MM100 PCTV (Bt848)
* MM201 PCTV (Bt878, Bt832) w/ Quartzsight camera
* MM202 PCTV (Bt878, Bt832, tda9874)
* MM205 PCTV (Bt878)
* MM210 PCTV (Bt878) (Galaxy TV, Galaxymedia ?)

### 7.1.8.26. Terratec

Models:

* Terra TV+ Version 1.0 (Bt848), “ceb105.PCB” printed on the PCB, TDA9821
* Terra TV+ Version 1.1 (Bt878), “LR74 Rev.E” printed on the PCB, TDA9821
* Terra TValueRadio, “LR102 Rev.C” printed on the PCB
* Terra TV/Radio+ Version 1.0, “80-CP2830100-0” TTTV3 printed on the PCB,
  “CPH010-E83” on the back, SAA6588T, TDA9873H
* Terra TValue Version BT878, “80-CP2830110-0 TTTV4” printed on the PCB,
  “CPH011-D83” on back
* Terra TValue Version 1.0 “ceb105.PCB” (really identical to Terra TV+ Version 1.0)
* Terra TValue New Revision “LR102 Rec.C”
* Terra Active Radio Upgrade (tea5757h, saa6588t)
* LR74 is a newer PCB revision of ceb105 (both incl. connector for Active Radio Upgrade)
* Cinergy 400 (saa7134), “E877 11(S)”, “PM820092D” printed on PCB
* Cinergy 600 (saa7134)

### 7.1.8.27. Technisat

Models:

* Discos ADR PC-Karte ISA (no TV!)
* Discos ADR PC-Karte PCI (probably no TV?)
* Techni-PC-Sat (Sat. analog)
  Rev 1.2 (zr36120, vpx3220, stv0030, saa5246, BSJE3-494A)
* Mediafocus I (zr36120/zr36125, drp3510, Sat. analog + ADR Radio)
* Mediafocus II (saa7146, Sat. analog)
* SatADR Rev 2.1 (saa7146a, saa7113h, stv0056a, msp3400c, drp3510a, BSKE3-307A)
* SkyStar 1 DVB (AV7110) = Technotrend Premium
* SkyStar 2 DVB (B2C2) (=Sky2PC)

### 7.1.8.28. Siemens

Multimedia eXtension Board (MXB) (SAA7146, SAA7111)

### 7.1.8.29. Powercolor

Models:

* MTV878
  :   Package comes with different contents:

      > 1. pcb “MTV878” (CARD=75)
      > 2. Pixelview Rev. 4\_
* MTV878R w/Remote Control
* MTV878F w/Remote Control w/FM radio

### 7.1.8.30. Pinnacle

PCTV models:

* Mirovideo PCTV (Bt848)
* Mirovideo PCTV SE (Bt848)
* Mirovideo PCTV Pro (Bt848 + Daughterboard for TV Stereo and FM)
* Studio PCTV Rave (Bt848 Version = Mirovideo PCTV)
* Studio PCTV Rave (Bt878 package w/o infrared)
* Studio PCTV (Bt878)
* Studio PCTV Pro (Bt878 stereo w/ FM)
* Pinnacle PCTV (Bt878, MT2032)
* Pinnacle PCTV Pro (Bt878, MT2032)
* Pinncale PCTV Sat (bt878a, HM1821/1221) [“Conexant CX24110 with CX24108 tuner, aka HM1221/HM1811”]
* Pinnacle PCTV Sat XE

M(J)PEG capture and playback models:

* DC1+ (ISA)
* DC10 (zr36057, zr36060, saa7110, adv7176)
* DC10+ (zr36067, zr36060, saa7110, adv7176)
* DC20 (ql16x24b,zr36050, zr36016, saa7110, saa7187 ...)
* DC30 (zr36057, zr36050, zr36016, vpx3220, adv7176, ad1843, tea6415, miro FST97A1)
* DC30+ (zr36067, zr36050, zr36016, vpx3220, adv7176)
* DC50 (zr36067, zr36050, zr36016, saa7112, adv7176 (2 pcs.?), ad1843, miro FST97A1, Lattice ???)

### 7.1.8.31. Lenco

Models:

* MXR-9565 (=Technisat Mediafocus?)
* MXR-9571 (Bt848) (=CPH031?)
* MXR-9575
* MXR-9577 (Bt878) (=Prolink 878TV Rev.3x)
* MXTV-9578CP (Bt878) (= Prolink PV-BT878P+4E)

### 7.1.8.32. Iomega

Buz (zr36067, zr36060, saa7111, saa7185)

### 7.1.8.33. LML

> LML33 (zr36067, zr36060, bt819, bt856)

### 7.1.8.34. Grandtec

Models:

* Grand Video Capture (Bt848)
* Multi Capture Card (Bt878)

### 7.1.8.35. Koutech

Models:

* KW-606 (Bt848)
* KW-607 (Bt848 capture only)
* KW-606RSF
* KW-607A (capture only)
* KW-608 (Zoran capture only)

### 7.1.8.36. IODATA (jp)

Models:

* GV-BCTV/PCI
* GV-BCTV2/PCI
* GV-BCTV3/PCI
* GV-BCTV4/PCI
* GV-VCP/PCI (capture only)
* GV-VCP2/PCI (capture only)

### 7.1.8.37. Canopus (jp)

WinDVR = Kworld “KW-TVL878RF”

### 7.1.8.38. www.sigmacom.co.kr

Sigma Cyber TV II

### 7.1.8.39. www.sasem.co.kr

Litte OnAir TV

### 7.1.8.40. hama

TV/Radio-Tuner Card, PCI (Model 44677) = CPH051

### 7.1.8.41. Sigma Designs

Hollywood plus (em8300, em9010, adv7175), (PCB “M340-10”) MPEG DVD decoder

### 7.1.8.42. Formac

Models:

* iProTV (Card for iMac Mezzanine slot, Bt848+SCSI)
* ProTV (Bt848)
* ProTV II = ProTV Stereo (Bt878) [“stereo” means FM stereo, tv is still mono]

### 7.1.8.43. ATI

Models:

* TV-Wonder
* TV-Wonder VE

### 7.1.8.44. Diamond Multimedia

DTV2000 (Bt848, tda9875)

### 7.1.8.45. Aopen

* VA1000 Plus (w/ Stereo)
* VA1000 Lite
* VA1000 (=LR90)

### 7.1.8.46. Intel

Models:

* Smart Video Recorder (ISA full-length)
* Smart Video Recorder pro (ISA half-length)
* Smart Video Recorder III (Bt848)

### 7.1.8.47. STB

Models:

* STB Gateway 6000704 (bt878)
* STB Gateway 6000699 (bt848)
* STB Gateway 6000402 (bt848)
* STB TV130 PCI

### 7.1.8.48. Videologic

Models:

* Captivator Pro/TV (ISA?)
* Captivator PCI/VC (Bt848 bundled with camera) (capture only)

### 7.1.8.49. Technotrend

Models:

* TT-SAT PCI (PCB “Sat-PCI Rev.:1.3.1”; zr36125, vpx3225d, stc0056a, Tuner:BSKE6-155A
* TT-DVB-Sat
  :   + revisions 1.1, 1.3, 1.5, 1.6 and 2.1
      + This card is sold as OEM from:

        > - Siemens DVB-s Card
        > - Hauppauge WinTV DVB-S
        > - Technisat SkyStar 1 DVB
        > - Galaxis DVB Sat
      + Now this card is called TT-PCline Premium Family
      + TT-Budget (saa7146, bsru6-701a)
        This card is sold as OEM from:

        > - Hauppauge WinTV Nova
        > - Satelco Standard PCI (DVB-S)
      + TT-DVB-C PCI

### 7.1.8.50. Teles

> DVB-s (Rev. 2.2, BSRV2-301A, data only?)

### 7.1.8.51. Remote Vision

MX RV605 (Bt848 capture only)

### 7.1.8.52. Boeder

Models:

* PC ChatCam (Model 68252) (Bt848 capture only)
* Tv/Fm Capture Card (Model 68404) = PV951

### 7.1.8.53. Media-Surfer (esc-kathrein.de)

Models:

* Sat-Surfer (ISA)
* Sat-Surfer PCI = Techni-PC-Sat
* Cable-Surfer 1
* Cable-Surfer 2
* Cable-Surfer PCI (zr36120)
* Audio-Surfer (ISA Radio card)

### 7.1.8.54. Jetway (www.jetway.com.tw)

Models:

* JW-TV 878M
* JW-TV 878 = KWorld KW-TV878RF

### 7.1.8.55. Galaxis

Models:

* Galaxis DVB Card S CI
* Galaxis DVB Card C CI
* Galaxis DVB Card S
* Galaxis DVB Card C
* Galaxis plug.in S [neuer Name: Galaxis DVB Card S CI

### 7.1.8.56. Hauppauge

Models:

* many many WinTV models ...
* WinTV DVBs = Technotrend Premium 1.3
* WinTV NOVA = Technotrend Budget 1.1 “S-DVB DATA”
* WinTV NOVA-CI “SDVBACI”
* WinTV Nova USB (=Technotrend USB 1.0)
* WinTV-Nexus-s (=Technotrend Premium 2.1 or 2.2)
* WinTV PVR
* WinTV PVR 250
* WinTV PVR 450

US models

-990 WinTV-PVR-350 (249USD) (iTVC15 chipset + radio)
-980 WinTV-PVR-250 (149USD) (iTVC15 chipset)
-880 WinTV-PVR-PCI (199USD) (KFIR chipset + bt878)
-881 WinTV-PVR-USB
-190 WinTV-GO
-191 WinTV-GO-FM
-404 WinTV
-401 WinTV-radio
-495 WinTV-Theater
-602 WinTV-USB
-621 WinTV-USB-FM
-600 USB-Live
-698 WinTV-HD
-697 WinTV-D
-564 WinTV-Nexus-S

Deutsche Modelle:

-603 WinTV GO
-719 WinTV Primio-FM
-718 WinTV PCI-FM
-497 WinTV Theater
-569 WinTV USB
-568 WinTV USB-FM
-882 WinTV PVR
-981 WinTV PVR 250
-891 WinTV-PVR-USB
-541 WinTV Nova
-488 WinTV Nova-Ci
-564 WinTV-Nexus-s
-727 WinTV-DVB-c
-545 Common Interface
-898 WinTV-Nova-USB

UK models:

-607 WinTV Go
-693,793 WinTV Primio FM
-647,747 WinTV PCI FM
-498 WinTV Theater
-883 WinTV PVR
-893 WinTV PVR USB (Duplicate entry)
-566 WinTV USB (UK)
-573 WinTV USB FM
-429 Impact VCB (bt848)
-600 USB Live (Video-In 1x Comp, 1xSVHS)
-542 WinTV Nova
-717 WinTV DVB-S
-909 Nova-t PCI
-893 Nova-t USB (Duplicate entry)
-802 MyTV
-804 MyView
-809 MyVideo
-872 MyTV2Go FM
-546 WinTV Nova-S CI
-543 WinTV Nova
-907 Nova-S USB
-908 Nova-T USB
-717 WinTV Nexus-S
-157 DEC3000-s Standalone + USB

Spain:

-685 WinTV-Go
-690 WinTV-PrimioFM
-416 WinTV-PCI Nicam Estereo
-677 WinTV-PCI-FM
-699 WinTV-Theater
-683 WinTV-USB
-678 WinTV-USB-FM
-983 WinTV-PVR-250
-883 WinTV-PVR-PCI
-993 WinTV-PVR-350
-893 WinTV-PVR-USB
-728 WinTV-DVB-C PCI
-832 MyTV2Go
-869 MyTV2Go-FM
-805 MyVideo (USB)

### 7.1.8.57. Matrix-Vision

Models:

* MATRIX-Vision MV-Delta
* MATRIX-Vision MV-Delta 2
* MVsigma-SLC (Bt848)

### 7.1.8.58. Conceptronic (.net)

Models:

* TVCON FM, TV card w/ FM = CPH05x
* TVCON = CPH06x

### 7.1.8.59. BestData

Models:

* HCC100 = VCC100rev1 + camera
* VCC100 rev1 (bt848)
* VCC100 rev2 (bt878)

### 7.1.8.60. Gallant (www.gallantcom.com) www.minton.com.tw

Models:

* Intervision IV-510 (capture only bt8x8)
* Intervision IV-550 (bt8x8)
* Intervision IV-100 (zoran)
* Intervision IV-1000 (bt8x8)

### 7.1.8.61. Asonic (www.asonic.com.cn) (website down)

SkyEye tv 878

### 7.1.8.62. Hoontech

878TV/FM

### 7.1.8.63. Teppro (www.itcteppro.com.tw)

Models:

* ITC PCITV (Card Ver 1.0) “Teppro TV1/TVFM1 Card”
* ITC PCITV (Card Ver 2.0)
* ITC PCITV (Card Ver 3.0) = “PV-BT878P+ (REV.9D)”
* ITC PCITV (Card Ver 4.0)
* TEPPRO IV-550 (For BT848 Main Chip)
* ITC DSTTV (bt878, satellite)
* ITC VideoMaker (saa7146, StreamMachine sm2110, tvtuner) “PV-SM2210P+ (REV:1C)”

### 7.1.8.64. Kworld (www.kworld.com.tw)

PC TV Station:

* KWORLD KW-TV878R TV (no radio)
* KWORLD KW-TV878RF TV (w/ radio)
* KWORLD KW-TVL878RF (low profile)
* KWORLD KW-TV713XRF (saa7134)

> MPEG TV Station (same cards as above plus WinDVR Software MPEG en/decoder)

* KWORLD KW-TV878R -Pro TV (no Radio)
* KWORLD KW-TV878RF-Pro TV (w/ Radio)
* KWORLD KW-TV878R -Ultra TV (no Radio)
* KWORLD KW-TV878RF-Ultra TV (w/ Radio)

### 7.1.8.65. JTT/ Justy Corp.(<http://www.jtt.ne.jp/>)

JTT-02 (JTT TV) “TV watchmate pro” (bt848)

### 7.1.8.66. ADS www.adstech.com

Models:

* Channel Surfer TV ( CHX-950 )
* Channel Surfer TV+FM ( CHX-960FM )

### 7.1.8.67. AVEC www.prochips.com

AVEC Intercapture (bt848, tea6320)

### 7.1.8.68. NoBrand

TV Excel = Australian Name for “PV-BT878P+ 8E” or “878TV Rev.3\_”

### 7.1.8.69. Mach www.machspeed.com

Mach TV 878

### 7.1.8.70. Eline www.eline-net.com/

Models:

* Eline Vision TVMaster / TVMaster FM (ELV-TVM/ ELV-TVM-FM) = LR26 (bt878)
* Eline Vision TVMaster-2000 (ELV-TVM-2000, ELV-TVM-2000-FM)= LR138 (saa713x)

### 7.1.8.71. Spirit

* Spirit TV Tuner/Video Capture Card (bt848)

### 7.1.8.72. Boser www.boser.com.tw

Models:

* HS-878 Mini PCI Capture Add-on Card
* HS-879 Mini PCI 3D Audio and Capture Add-on Card (w/ ES1938 Solo-1)

### 7.1.8.73. Satelco www.citycom-gmbh.de, www.satelco.de

Models:

* TV-FM =KNC1 saa7134
* Standard PCI (DVB-S) = Technotrend Budget
* Standard PCI (DVB-S) w/ CI
* Satelco Highend PCI (DVB-S) = Technotrend Premium

### 7.1.8.74. Sensoray www.sensoray.com

Models:

* Sensoray 311 (PC/104 bus)
* Sensoray 611 (PCI)

### 7.1.8.75. CEI (Chartered Electronics Industries Pte Ltd [CEI] [FCC ID HBY])

Models:

* TV Tuner - HBY-33A-RAFFLES Brooktree Bt848KPF + Philips
* TV Tuner MG9910 - HBY33A-TVO CEI + Philips SAA7110 + OKI M548262 + ST STV8438CV
* Primetime TV (ISA)

  + acquired by Singapore Technologies
  + now operating as Chartered Semiconductor Manufacturing
  + Manufacturer of video cards is listed as:

    - Cogent Electronics Industries [CEI]

### 7.1.8.76. AITech

Models:

* Wavewatcher TV (ISA)
* AITech WaveWatcher TV-PCI = can be LR26 (Bt848) or LR50 (BT878)
* WaveWatcher TVR-202 TV/FM Radio Card (ISA)

### 7.1.8.77. MAXRON

Maxron MaxTV/FM Radio (KW-TV878-FNT) = Kworld or JW-TV878-FBK

### 7.1.8.78. www.ids-imaging.de

Models:

* Falcon Series (capture only)

In USA: <http://www.theimagingsource.com/>
- DFG/LC1

### 7.1.8.79. www.sknet-web.co.jp

SKnet Monster TV (saa7134)

### 7.1.8.80. A-Max www.amaxhk.com (Colormax, Amax, Napa)

APAC Viewcomp 878

### 7.1.8.81. Cybertainment

Models:

* CyberMail AV Video Email Kit w/ PCI Capture Card (capture only)
* CyberMail Xtreme

These are Flyvideo

### 7.1.8.82. VCR (<http://www.vcrinc.com/>)

Video Catcher 16

### 7.1.8.83. Twinhan

Models:

* DST Card/DST-IP (bt878, twinhan asic) VP-1020
  - Sold as:

  > + KWorld DVBS Satellite TV-Card
  > + Powercolor DSTV Satellite Tuner Card
  > + Prolink Pixelview DTV2000
  > + Provideo PV-911 Digital Satellite TV Tuner Card With Common Interface ?
* DST-CI Card (DVB Satellite) VP-1030
* DCT Card (DVB cable)

### 7.1.8.84. MSI

Models:

* MSI [TV@nywhere](mailto:TV%40nywhere) Tuner Card (MS-8876) (CX23881/883) Not Bt878 compatible.
* MS-8401 DVB-S

### 7.1.8.85. Focus www.focusinfo.com

InVideo PCI (bt878)

### 7.1.8.86. Sdisilk www.sdisilk.com/

Models:

* SDI Silk 100
* SDI Silk 200 SDI Input Card

### 7.1.8.87. www.euresys.com

PICOLO series

### 7.1.8.88. PMC/Pace

www.pacecom.co.uk website closed

### 7.1.8.89. Mercury www.kobian.com (UK and FR)

Models:

* LR50
* LR138RBG-Rx == LR138

### 7.1.8.90. TEC sound

TV-Mate = Zoltrix VP-8482

Though educated googling found: www.techmakers.com

(package and manuals don’t have any other manufacturer info) TecSound

### 7.1.8.91. Lorenzen www.lorenzen.de

SL DVB-S PCI = Technotrend Budget PCI (su1278 or bsru version)

### 7.1.8.92. Origo (.uk) www.origo2000.com

PC TV Card = LR50

### 7.1.8.93. I/O Magic www.iomagic.com

PC PVR - Desktop TV Personal Video Recorder DR-PCTV100 = Pinnacle ROB2D-51009464 4.0 + Cyberlink PowerVCR II

### 7.1.8.94. Arowana

TV-Karte / Poso Power TV (?) = Zoltrix VP-8482 (?)

### 7.1.8.95. iTVC15 boards

kuroutoshikou.com ITVC15
yuan.com MPG160 PCI TV (Internal PCI MPEG2 encoder card plus TV-tuner)

### 7.1.8.96. Asus www.asuscom.com

Models:

* Asus TV Tuner Card 880 NTSC (low profile, cx23880)
* Asus TV (saa7134)

### 7.1.8.97. Hoontech

<http://www.hoontech.de/>

* HART Vision 848 (H-ART Vision 848)
* HART Vision 878 (H-Art Vision 878)

## 7.1.9. Chips used at bttv devices

* all boards:

  + Brooktree Bt848/848A/849/878/879: video capture chip
* Board specific

  + Miro PCTV:

    - Philips or Temic Tuner
  + Hauppauge Win/TV pci (version 405):

    - Microchip 24LC02B or Philips 8582E2Y:

      > * 256 Byte EEPROM with configuration information
      > * I2C 0xa0-0xa1, (24LC02B also responds to 0xa2-0xaf)
    - Philips SAA5246AGP/E: Videotext decoder chip, I2C 0x22-0x23
    - TDA9800: sound decoder
    - Winbond W24257AS-35: 32Kx8 CMOS static RAM (Videotext buffer mem)
    - 14052B: analog switch for selection of sound source
* PAL:

  + TDA5737: VHF, hyperband and UHF mixer/oscillator for TV and VCR 3-band tuners
  + TSA5522: 1.4 GHz I2C-bus controlled synthesizer, I2C 0xc2-0xc3
* NTSC:

  + TDA5731: VHF, hyperband and UHF mixer/oscillator for TV and VCR 3-band tuners
  + TSA5518: no datasheet available on Philips site
* STB TV pci:

  + ???
  + if you want better support for STB cards send me info!
    Look at the board! What chips are on it?

## 7.1.10. Specs

Philips <http://www.Semiconductors.COM/pip/>

Conexant <http://www.conexant.com/>

Micronas <http://www.micronas.com/en/home/index.html>

## 7.1.11. Thanks

Many thanks to:

* Markus Schroeder <[schroedm@uni-duesseldorf.de](mailto:schroedm%40uni-duesseldorf.de)> for information on the Bt848
  and tuner programming and his control program xtvc.
* Martin Buck <[martin-2.buck@student.uni-ulm.de](mailto:martin-2.buck%40student.uni-ulm.de)> for his great Videotext
  package.
* Gerd Hoffmann for the MSP3400 support and the modular
  I2C, tuner, ... support.
* MATRIX Vision for giving us 2 cards for free, which made support of
  single crystal operation possible.
* MIRO for providing a free PCTV card and detailed information about the
  components on their cards. (E.g. how the tuner type is detected)
  Without their card I could not have debugged the NTSC mode.
* Hauppauge for telling how the sound input is selected and what components
  they do and will use on their radio cards.
  Also many thanks for faxing me the FM1216 data sheet.

## 7.1.12. Contributors

Michael Chu <[mmchu@pobox.com](mailto:mmchu%40pobox.com)>
:   AverMedia fix and more flexible card recognition

Alan Cox <[alan@lxorguk.ukuu.org.uk](mailto:alan%40lxorguk.ukuu.org.uk)>
:   Video4Linux interface and 2.1.x kernel adaptation

Chris Kleitsch
:   Hardware I2C

Gerd Hoffmann
:   Radio card (ITT sound processor)

bigfoot <[bigfoot@net-way.net](mailto:bigfoot%40net-way.net)>

Ragnar Hojland Espinosa <[ragnar@macula.net](mailto:ragnar%40macula.net)>
:   ConferenceTV card

* many more (please mail me if you are missing in this list and would
  :   like to be mentioned)
