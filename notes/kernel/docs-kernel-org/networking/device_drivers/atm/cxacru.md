# ATM cxacru device driver

> 출처(원문): https://docs.kernel.org/networking/device_drivers/atm/cxacru.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ATM cxacru device driver

Firmware is required for this device: <http://accessrunner.sourceforge.net/>

While it is capable of managing/maintaining the ADSL connection without the
module loaded, the device will sometimes stop responding after unloading the
driver and it is necessary to unplug/remove power to the device to fix this.

Note: support for cxacru-cf.bin has been removed. It was not loaded correctly
so it had no effect on the device configuration. Fixing it could have stopped
existing devices working when an invalid configuration is supplied.

There is a script cxacru-cf.py to convert an existing file to the sysfs form.

Detected devices will appear as ATM devices named “cxacru”. In /sys/class/atm/
these are directories named cxacruN where N is the device number. A symlink
named device points to the USB interface device’s directory which contains
several sysfs attribute files for retrieving device statistics:

* adsl\_controller\_version
* adsl\_headend
* adsl\_headend\_environment

  > + Information about the remote headend.
* adsl\_config

  > + Configuration writing interface.
  > + Write parameters in hexadecimal format <index>=<value>,
  >   separated by whitespace, e.g.:
  >
  >   > “1=0 a=5”
  > + Up to 7 parameters at a time will be sent and the modem will restart
  >   the ADSL connection when any value is set. These are logged for future
  >   reference.
* downstream\_attenuation (dB)
* downstream\_bits\_per\_frame
* downstream\_rate (kbps)
* downstream\_snr\_margin (dB)

  > + Downstream stats.
* upstream\_attenuation (dB)
* upstream\_bits\_per\_frame
* upstream\_rate (kbps)
* upstream\_snr\_margin (dB)
* transmitter\_power (dBm/Hz)

  > + Upstream stats.
* downstream\_crc\_errors
* downstream\_fec\_errors
* downstream\_hec\_errors
* upstream\_crc\_errors
* upstream\_fec\_errors
* upstream\_hec\_errors

  > + Error counts.
* line\_startable

  > + Indicates that ADSL support on the device
  >   is/can be enabled, see adsl\_start.
* line\_status

  > > + “initialising”
  > > + “down”
  > > + “attempting to activate”
  > > + “training”
  > > + “channel analysis”
  > > + “exchange”
  > > + “waiting”
  > > + “up”
  >
  > Changes between “down” and “attempting to activate”
  > if there is no signal.
* link\_status

  > + “not connected”
  > + “connected”
  > + “lost”
* mac\_address
* modulation

  > + “” (when not connected)
  > + “ANSI T1.413”
  > + “ITU-T G.992.1 (G.DMT)”
  > + “ITU-T G.992.2 (G.LITE)”
* startup\_attempts

  > + Count of total attempts to initialise ADSL.

To enable/disable ADSL, the following can be written to the adsl\_state file:

> * “start”
> * “stop
> * “restart” (stops, waits 1.5s, then starts)
> * “poll” (used to resume status polling if it was disabled due to failure)

Changes in adsl/line state are reported via kernel log messages:

```
[4942145.150704] ATM dev 0: ADSL state: running
[4942243.663766] ATM dev 0: ADSL line: down
[4942249.665075] ATM dev 0: ADSL line: attempting to activate
[4942253.654954] ATM dev 0: ADSL line: training
[4942255.666387] ATM dev 0: ADSL line: channel analysis
[4942259.656262] ATM dev 0: ADSL line: exchange
[2635357.696901] ATM dev 0: ADSL line: up (8128 kb/s down | 832 kb/s up)
```
