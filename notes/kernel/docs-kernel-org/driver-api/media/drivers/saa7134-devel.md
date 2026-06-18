# 10.1.9.The saa7134 driver

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/saa7134-devel.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.1.9. The saa7134 driver

Author Gerd Hoffmann

## 10.1.9.1. Card Variations:

Cards can use either of these two crystals (xtal):

* 32.11 MHz -> .audio\_clock=0x187de7
* 24.576MHz -> .audio\_clock=0x200000 (xtal \* .audio\_clock = 51539600)

Some details about 30/34/35:

* saa7130 - low-price chip, doesn’t have mute, that is why all those
  cards should have .mute field defined in their tuner structure.
* saa7134 - usual chip
* saa7133/35 - saa7135 is probably a marketing decision, since all those
  chips identifies itself as 33 on pci.

## 10.1.9.2. LifeView GPIOs

This section was authored by: Peter Missel <[peter.missel@onlinehome.de](mailto:peter.missel%40onlinehome.de)>

* LifeView FlyTV Platinum FM (LR214WF)

  > + GP27 MDT2005 PB4 pin 10
  > + GP26 MDT2005 PB3 pin 9
  > + GP25 MDT2005 PB2 pin 8
  > + GP23 MDT2005 PB1 pin 7
  > + GP22 MDT2005 PB0 pin 6
  > + GP21 MDT2005 PB5 pin 11
  > + GP20 MDT2005 PB6 pin 12
  > + GP19 MDT2005 PB7 pin 13
  > + nc MDT2005 PA3 pin 2
  > + Remote MDT2005 PA2 pin 1
  > + GP18 MDT2005 PA1 pin 18
  > + nc MDT2005 PA0 pin 17 strap low
  > + GP17 Strap “GP7”=High
  > + GP16 Strap “GP6”=High
  >
  >   > - 0=Radio 1=TV
  >   > - Drives SA630D ENCH1 and HEF4052 A1 pinsto do FM radio through
  >   >   SIF input
  > + GP15 nc
  > + GP14 nc
  > + GP13 nc
  > + GP12 Strap “GP5” = High
  > + GP11 Strap “GP4” = High
  > + GP10 Strap “GP3” = High
  > + GP09 Strap “GP2” = Low
  > + GP08 Strap “GP1” = Low
  > + GP07.00 nc

## 10.1.9.3. Credits

[andrew.stevens@philips.com](mailto:andrew.stevens%40philips.com) + [werner.leeb@philips.com](mailto:werner.leeb%40philips.com) for providing
saa7134 hardware specs and sample board.
