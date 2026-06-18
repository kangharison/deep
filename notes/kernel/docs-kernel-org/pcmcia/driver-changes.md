# Driver changes

> 출처(원문): https://docs.kernel.org/pcmcia/driver-changes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Driver changes

This file details changes in 2.6 which affect PCMCIA card driver authors:

* pcmcia\_loop\_config() and autoconfiguration (as of 2.6.36)
  :   If `struct pcmcia_device` \*p\_dev->config\_flags is set accordingly,
      `pcmcia_loop_config()` now sets up certain configuration values
      automatically, though the driver may still override the settings
      in the callback function. The following autoconfiguration options
      are provided at the moment:

      > + CONF\_AUTO\_CHECK\_VCC : check for matching Vcc
      > + CONF\_AUTO\_SET\_VPP : set Vpp
      > + CONF\_AUTO\_AUDIO : auto-enable audio line, if required
      > + CONF\_AUTO\_SET\_IO : set ioport resources (->resource[0,1])
      > + CONF\_AUTO\_SET\_IOMEM : set first iomem resource (->resource[2])
* pcmcia\_request\_configuration -> pcmcia\_enable\_device (as of 2.6.36)
  :   `pcmcia_request_configuration()` got renamed to `pcmcia_enable_device()`,
      as it mirrors `pcmcia_disable_device()`. Configuration settings are now
      stored in `struct pcmcia_device`, e.g. in the fields config\_flags,
      config\_index, config\_base, vpp.
* pcmcia\_request\_window changes (as of 2.6.36)
  :   Instead of win\_req\_t, drivers are now requested to fill out
      `struct pcmcia_device` \*p\_dev->resource[2,3,4,5] for up to four ioport
      ranges. After a call to `pcmcia_request_window()`, the regions found there
      are reserved and may be used immediately -- until `pcmcia_release_window()`
      is called.
* pcmcia\_request\_io changes (as of 2.6.36)
  :   Instead of io\_req\_t, drivers are now requested to fill out
      `struct pcmcia_device` \*p\_dev->resource[0,1] for up to two ioport
      ranges. After a call to `pcmcia_request_io()`, the ports found there
      are reserved, after calling `pcmcia_request_configuration()`, they may
      be used.
* No dev\_info\_t, no cs\_types.h (as of 2.6.36)
  :   dev\_info\_t and a few other typedefs are removed. No longer use them
      in PCMCIA device drivers. Also, do not include pcmcia/cs\_types.h, as
      this file is gone.
* No dev\_node\_t (as of 2.6.35)
  :   There is no more need to fill out a “dev\_node\_t” structure.
* New IRQ request rules (as of 2.6.35)
  :   Instead of the old `pcmcia_request_irq()` interface, drivers may now
      choose between:

      + calling request\_irq/free\_irq directly. Use the IRQ from \*p\_dev->irq.
      + use pcmcia\_request\_irq(p\_dev, handler\_t); the PCMCIA core will
        clean up automatically on calls to `pcmcia_disable_device()` or
        device ejection.
* no cs\_error / CS\_CHECK / CONFIG\_PCMCIA\_DEBUG (as of 2.6.33)
  :   Instead of the `cs_error()` callback or the `CS_CHECK()` macro, please use
      Linux-style checking of return values, and -- if necessary -- debug
      messages using “`dev_dbg()`” or “[`pr_debug()`](../core-api/printk-basics.html#c.pr_debug "pr_debug")”.
* New CIS tuple access (as of 2.6.33)
  :   Instead of pcmcia\_get\_{first,next}`_tuple()`, `pcmcia_get_tuple_data()` and
      `pcmcia_parse_tuple()`, a driver shall use “`pcmcia_get_tuple()`” if it is
      only interested in one (raw) tuple, or “`pcmcia_loop_tuple()`” if it is
      interested in all tuples of one type. To decode the MAC from CISTPL\_FUNCE,
      a new helper “`pcmcia_get_mac_from_cis()`” was added.
* New configuration loop helper (as of 2.6.28)
  :   By calling `pcmcia_loop_config()`, a driver can iterate over all available
      configuration options. During a driver’s `probe()` phase, one doesn’t need
      to use pcmcia\_get\_{first,next}\_tuple, pcmcia\_get\_tuple\_data and
      pcmcia\_parse\_tuple directly in most if not all cases.
* New release helper (as of 2.6.17)
  :   Instead of calling pcmcia\_release\_{configuration,io,irq,win}, all that’s
      necessary now is calling pcmcia\_disable\_device. As there is no valid
      reason left to call pcmcia\_release\_io and pcmcia\_release\_irq, the
      exports for them were removed.
* Unify detach and REMOVAL event code, as well as attach and INSERTION
  code (as of 2.6.16):

  ```
  void (*remove)          (struct pcmcia_device *dev);
  int (*probe)            (struct pcmcia_device *dev);
  ```
* Move suspend, resume and reset out of event handler (as of 2.6.16):

  ```
  int (*suspend)          (struct pcmcia_device *dev);
  int (*resume)           (struct pcmcia_device *dev);
  ```

  should be initialized in `struct pcmcia_driver`, and handle
  (SUSPEND == RESET\_PHYSICAL) and (RESUME == CARD\_RESET) events
* event handler initialization in struct pcmcia\_driver (as of 2.6.13)
  :   The event handler is notified of all events, and must be initialized
      as the `event()` callback in the driver’s `struct pcmcia_driver`.
* pcmcia/version.h should not be used (as of 2.6.13)
  :   This file will be removed eventually.
* in-kernel device<->driver matching (as of 2.6.13)
  :   PCMCIA devices and their correct drivers can now be matched in
      kernelspace. See ‘[Device table](devicetable.html)’ for details.
* Device model integration (as of 2.6.11)
  :   A `struct pcmcia_device` is registered with the device model core,
      and can be used (e.g. for SET\_NETDEV\_DEV) by using
      handle\_to\_dev(client\_handle\_t \* handle).
* Convert internal I/O port addresses to unsigned int (as of 2.6.11)
  :   ioaddr\_t should be replaced by unsigned int in PCMCIA card drivers.
* irq\_mask and irq\_list parameters (as of 2.6.11)
  :   The irq\_mask and irq\_list parameters should no longer be used in
      PCMCIA card drivers. Instead, it is the job of the PCMCIA core to
      determine which IRQ should be used. Therefore, link->irq.IRQInfo2
      is ignored.
* client->PendingEvents is gone (as of 2.6.11)
  :   client->PendingEvents is no longer available.
* client->Attributes are gone (as of 2.6.11)
  :   client->Attributes is unused, therefore it is removed from all
      PCMCIA card drivers
* core functions no longer available (as of 2.6.11)
  :   The following functions have been removed from the kernel source
      because they are unused by all in-kernel drivers, and no external
      driver was reported to rely on them:

      ```
      pcmcia_get_first_region()
      pcmcia_get_next_region()
      pcmcia_modify_window()
      pcmcia_set_event_mask()
      pcmcia_get_first_window()
      pcmcia_get_next_window()
      ```
* device list iteration upon module removal (as of 2.6.10)
  :   It is no longer necessary to iterate on the driver’s internal
      client list and call the ->`detach()` function upon module removal.
* Resource management. (as of 2.6.8)
  :   Although the PCMCIA subsystem will allocate resources for cards,
      it no longer marks these resources busy. This means that driver
      authors are now responsible for claiming your resources as per
      other drivers in Linux. You should use `request_region()` to mark
      your IO regions in-use, and `request_mem_region()` to mark your
      memory regions in-use. The name argument should be a pointer to
      your driver name. Eg, for pcnet\_cs, name should point to the
      string “pcnet\_cs”.
* CardServices is gone
  `CardServices()` in 2.4 is just a big switch statement to call various
  services. In 2.6, all of those entry points are exported and called
  directly (except for `pcmcia_report_error()`, just use `cs_error()` instead).
* `struct pcmcia_driver`
  You need to use `struct pcmcia_driver` and pcmcia\_{un,}register\_driver
  instead of {un,}register\_pccard\_driver
