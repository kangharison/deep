# 8.2.How to get the bt8xx cards working

> 출처(원문): https://docs.kernel.org/admin-guide/media/bt8xx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8.2. How to get the bt8xx cards working

Authors:
:   Richard Walker,
    Jamie Honan,
    Michael Hunold,
    Manu Abraham,
    Uwe Bugla,
    Michael Krufky

## 8.2.1. General information

This class of cards has a bt878a as the PCI interface, and require the bttv
driver for accessing the i2c bus and the gpio pins of the bt8xx chipset.

Please see [BTTV cards list](bttv-cardlist.html) for a complete
list of Cards based on the Conexant Bt8xx PCI bridge supported by the
Linux Kernel.

In order to be able to compile the kernel, some config options should be
enabled:

```
./scripts/config -e PCI
./scripts/config -e INPUT
./scripts/config -m I2C
./scripts/config -m MEDIA_SUPPORT
./scripts/config -e MEDIA_PCI_SUPPORT
./scripts/config -e MEDIA_ANALOG_TV_SUPPORT
./scripts/config -e MEDIA_DIGITAL_TV_SUPPORT
./scripts/config -e MEDIA_RADIO_SUPPORT
./scripts/config -e RC_CORE
./scripts/config -m VIDEO_BT848
./scripts/config -m DVB_BT8XX
```

If you want to automatically support all possible variants of the Bt8xx
cards, you should also do:

```
./scripts/config -e MEDIA_SUBDRV_AUTOSELECT
```

Note

Please use the following options with care as deselection of drivers which
are in fact necessary may result in DVB devices that cannot be tuned due
to lack of driver support.

If your goal is to just support an specific board, you may, instead,
disable MEDIA\_SUBDRV\_AUTOSELECT and manually select the frontend drivers
required by your board. With that, you can save some RAM.

You can do that by calling make xconfig/qconfig/menuconfig and look at
the options on those menu options (only enabled if
`Autoselect ancillary drivers` is disabled:

1. `Device drivers` => `Multimedia support` => `Customize TV tuners`
2. `Device drivers` => `Multimedia support` => `Customize DVB frontends`

Then, on each of the above menu, please select your card-specific
frontend and tuner modules.

## 8.2.2. Loading Modules

Regular case: If the bttv driver detects a bt8xx-based DVB card, all
frontend and backend modules will be loaded automatically.

Exceptions are:

* Old TV cards without EEPROMs, sharing a common PCI subsystem ID;
* Old TwinHan DST cards or clones with or without CA slot and not
  containing an Eeprom.

In the following cases overriding the PCI type detection for bttv and
for dvb-bt8xx drivers by passing modprobe parameters may be necessary.

### 8.2.2.1. Running TwinHan and Clones

As shown at [BTTV cards list](bttv-cardlist.html), TwinHan and
clones use `card=113` modprobe parameter. So, in order to properly
detect it for devices without EEPROM, you should use:

```
$ modprobe bttv card=113
$ modprobe dst
```

Useful parameters for verbosity level and debugging the dst module:

```
verbose=0:              messages are disabled
        1:              only error messages are displayed
        2:              notifications are displayed
        3:              other useful messages are displayed
        4:              debug setting
dst_addons=0:           card is a free to air (FTA) card only
        0x20:   card has a conditional access slot for scrambled channels
dst_algo=0:             (default) Software tuning algorithm
         1:             Hardware tuning algorithm
```

The autodetected values are determined by the cards’ “response string”.

In your logs see f. ex.: dst\_get\_device\_id: Recognize [DSTMCI].

For bug reports please send in a complete log with verbose=4 activated.
Please also see [Digital TV Conditional Access Interface](ci.html).

### 8.2.2.2. Running multiple cards

See [BTTV cards list](bttv-cardlist.html) for a complete list of
Card ID. Some examples:

> | Brand name | ID |
> | --- | --- |
> | Pinnacle PCTV Sat | 94 |
> | Nebula Electronics Digi TV | 104 |
> | pcHDTV HD-2000 TV | 112 |
> | Twinhan DST and clones | 113 |
> | Avermedia AverTV DVB-T 77: | 123 |
> | Avermedia AverTV DVB-T 761 | 124 |
> | DViCO FusionHDTV DVB-T Lite | 128 |
> | DViCO FusionHDTV 5 Lite | 135 |

Note

When you have multiple cards, the order of the card ID should
match the order where they’re detected by the system. Please notice
that removing/inserting other PCI cards may change the detection
order.

Example:

```
$ modprobe bttv card=113 card=135
```

In case of further problems please subscribe and send questions to
the mailing list: [linux-media@vger.kernel.org](mailto:linux-media%40vger.kernel.org).

### 8.2.2.3. Probing the cards with broken PCI subsystem ID

There are some TwinHan cards whose EEPROM has become corrupted for some
reason. The cards do not have a correct PCI subsystem ID.
Still, it is possible to force probing the cards with:

```
$ echo 109e 0878 $subvendor $subdevice > \
        /sys/bus/pci/drivers/bt878/new_id
```

The two numbers there are:

```
109e: PCI_VENDOR_ID_BROOKTREE
0878: PCI_DEVICE_ID_BROOKTREE_878
```
