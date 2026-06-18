# 10.2.4.Contributors

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/contributors.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.2.4. Contributors

Note

This documentation is outdated. There are several other DVB contributors
that aren’t listed below.

Thanks go to the following people for patches and contributions:

* Michael Hunold <[m.hunold@gmx.de](mailto:m.hunold%40gmx.de)>

  + for the initial saa7146 driver and its recent overhaul
* Christian Theiss

  + for his work on the initial Linux DVB driver
* Marcus Metzler <[mocm@metzlerbros.de](mailto:mocm%40metzlerbros.de)> and
  Ralph Metzler <[rjkm@metzlerbros.de](mailto:rjkm%40metzlerbros.de)>

  + for their continuing work on the DVB driver
* Michael Holzt <[kju@debian.org](mailto:kju%40debian.org)>

  + for his contributions to the dvb-net driver
* Diego Picciani <[d.picciani@novacomp.it](mailto:d.picciani%40novacomp.it)>

  + for CyberLogin for Linux which allows logging onto EON
    (in case you are wondering where CyberLogin is, EON changed its login
    procedure and CyberLogin is no longer used.)
* Martin Schaller <[martin@smurf.franken.de](mailto:martin%40smurf.franken.de)>

  + for patching the cable card decoder driver
* Klaus Schmidinger <[Klaus.Schmidinger@cadsoft.de](mailto:Klaus.Schmidinger%40cadsoft.de)>

  + for various fixes regarding tuning, OSD and CI stuff and his work on VDR
* Steve Brown <[sbrown@cortland.com](mailto:sbrown%40cortland.com)>

  + for his AFC kernel thread
* Christoph Martin <[martin@uni-mainz.de](mailto:martin%40uni-mainz.de)>

  + for his LIRC infrared handler
* Andreas Oberritter <[obi@linuxtv.org](mailto:obi%40linuxtv.org)>,
  Dennis Noermann <[dennis.noermann@noernet.de](mailto:dennis.noermann%40noernet.de)>,
  Felix Domke <[tmbinc@elitedvb.net](mailto:tmbinc%40elitedvb.net)>,
  Florian Schirmer <[jolt@tuxbox.org](mailto:jolt%40tuxbox.org)>,
  Ronny Strutz <[3des@elitedvb.de](mailto:3des%40elitedvb.de)>,
  Wolfram Joost <[dbox2@frokaschwei.de](mailto:dbox2%40frokaschwei.de)>
  and all the other dbox2 people

  + for many bugfixes in the generic DVB Core, frontend drivers and
    their work on the dbox2 port of the DVB driver
* Oliver Endriss <[o.endriss@gmx.de](mailto:o.endriss%40gmx.de)>

  + for many bugfixes
* Andrew de Quincey <[adq\_dvb@lidskialf.net](mailto:adq_dvb%40lidskialf.net)>

  + for the tda1004x frontend driver, and various bugfixes
* Peter Schildmann <[peter.schildmann@web.de](mailto:peter.schildmann%40web.de)>

  + for the driver for the Technisat SkyStar2 PCI DVB card
* Vadim Catana <[skystar@moldova.cc](mailto:skystar%40moldova.cc)>,
  Roberto Ragusa <[r.ragusa@libero.it](mailto:r.ragusa%40libero.it)> and
  Augusto Cardoso <[augusto@carhil.net](mailto:augusto%40carhil.net)>

  + for all the work for the FlexCopII chipset by B2C2,Inc.
* Davor Emard <[emard@softhome.net](mailto:emard%40softhome.net)>

  + for his work on the budget drivers, the demux code,
    the module unloading problems, ...
* Hans-Frieder Vogt <[hfvogt@arcor.de](mailto:hfvogt%40arcor.de)>

  + for his work on calculating and checking the crc’s for the
    TechnoTrend/Hauppauge DEC driver firmware
* Michael Dreher <[michael@5dot1.de](mailto:michael%405dot1.de)> and
  Andreas ‘randy’ Weinberger

  + for the support of the Fujitsu-Siemens Activy budget DVB-S
* Kenneth Aafløy <[ke-aa@frisurf.no](mailto:ke-aa%40frisurf.no)>

  + for adding support for Typhoon DVB-S budget card
* Ernst Peinlich <[e.peinlich@inode.at](mailto:e.peinlich%40inode.at)>

  + for tuning/DiSEqC support for the DEC 3000-s
* Peter Beutner <[p.beutner@gmx.net](mailto:p.beutner%40gmx.net)>

  + for the IR code for the ttusb-dec driver
* Wilson Michaels <[wilsonmichaels@earthlink.net](mailto:wilsonmichaels%40earthlink.net)>

  + for the lgdt330x frontend driver, and various bugfixes
* Michael Krufky <[mkrufky@linuxtv.org](mailto:mkrufky%40linuxtv.org)>

  + for maintaining v4l/dvb inter-tree dependencies
* Taylor Jacob <[rtjacob@earthlink.net](mailto:rtjacob%40earthlink.net)>

  + for the nxt2002 frontend driver
* Jean-Francois Thibert <[jeanfrancois@sagetv.com](mailto:jeanfrancois%40sagetv.com)>

  + for the nxt2004 frontend driver
* Kirk Lapray <[kirk.lapray@gmail.com](mailto:kirk.lapray%40gmail.com)>

  + for the or51211 and or51132 frontend drivers, and
    for merging the nxt2002 and nxt2004 modules into a
    single nxt200x frontend driver.

(If you think you should be in this list, but you are not, drop a
line to the DVB mailing list)
