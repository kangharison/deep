# mlx5 devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/mlx5.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# mlx5 devlink support

This document describes the devlink features implemented by the `mlx5`
device driver.

## Parameters

Generic parameters implemented

|  |  |  |  |
| --- | --- | --- | --- |
| Name | Mode | Validation | Notes |
| `enable_roce` | driverinit | Boolean | If the device supports RoCE disablement, RoCE enablement state controls device support for RoCE capability. Otherwise, the control occurs in the driver stack. When RoCE is disabled at the driver level, only raw ethernet QPs are supported. |
| `io_eq_size` | driverinit | The range is between 64 and 4096. |  |
| `event_eq_size` | driverinit | The range is between 64 and 4096. |  |
| `max_macs` | driverinit | The range is between 1 and 2^31. Only power of 2 values are supported. |  |
| `enable_sriov` | permanent | Boolean | Applies to each physical function (PF) independently, if the device supports it. Otherwise, it applies symmetrically to all PFs. |
| `total_vfs` | permanent | The range is between 1 and a device-specific max. | Applies to each physical function (PF) independently, if the device supports it. Otherwise, it applies symmetrically to all PFs. |

Note: permanent parameters such as `enable_sriov` and `total_vfs` require FW reset to take effect

```
# setup parameters
devlink dev param set pci/0000:01:00.0 name enable_sriov value true cmode permanent
devlink dev param set pci/0000:01:00.0 name total_vfs value 8 cmode permanent

# Fw reset
devlink dev reload pci/0000:01:00.0 action fw_activate

# for PCI related config such as sriov PCI reset/rescan is required:
echo 1 >/sys/bus/pci/devices/0000:01:00.0/remove
echo 1 >/sys/bus/pci/rescan
grep ^ /sys/bus/pci/devices/0000:01:00.0/sriov_*

* - ``num_doorbells``
  - driverinit
  - This controls the number of channel doorbells used by the netdev. In all
    cases, an additional doorbell is allocated and used for non-channel
    communication (e.g. for PTP, HWS, etc.). Supported values are:

    - 0: No channel-specific doorbells, use the global one for everything.
    - [1, max_num_channels]: Spread netdev channels equally across these
      doorbells.
```

The `mlx5` driver also implements the following driver-specific
parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `flow_steering_mode` | string | runtime | Controls the flow steering mode of the driver   * `dmfs` Device managed flow steering. In DMFS mode, the HW   steering entities are created and managed through firmware. * `smfs` Software managed flow steering. In SMFS mode, the HW   steering entities are created and manage through the driver without   firmware intervention. * `hmfs` Hardware managed flow steering. In HMFS mode, the driver   is configuring steering rules directly to the HW using Work Queues with   a special new type of WQE (Work Queue Element).   SMFS mode is faster and provides better rule insertion rate compared to default DMFS mode. |
| `fdb_large_groups` | u32 | driverinit | Control the number of large groups (size > 1) in the FDB table.   * The default value is 15, and the range is between 1 and 1024. |
| `esw_multiport` | Boolean | runtime | Control MultiPort E-Switch shared fdb mode.  An experimental mode where a single E-Switch is used and all the vports and physical ports on the NIC are connected to it.  An example is to send traffic from a VF that is created on PF0 to an uplink that is natively associated with the uplink of PF1  Note: Future devices, ConnectX-8 and onward, will eventually have this as the default to allow forwarding between all NIC ports in a single E-switch environment and the dual E-switch mode will likely get deprecated.  Default: disabled |
| `esw_port_metadata` | Boolean | runtime | When applicable, disabling eswitch metadata can increase packet rate up to 20% depending on the use case and packet sizes.  Eswitch port metadata state controls whether to internally tag packets with metadata. Metadata tagging must be enabled for multi-port RoCE, failover between representors and stacked devices. By default metadata is enabled on the supported devices in E-switch. Metadata is applicable only for E-switch in switchdev mode and users may disable it when NONE of the below use cases will be in use: 1. HCA is in Dual/multi-port RoCE mode. 2. VF/SF representor bonding (Usually used for Live migration) 3. Stacked devices  When metadata is disabled, the above use cases will fail to initialize if users try to enable them.  Note: Setting this parameter does not take effect immediately. Setting must happen in legacy mode and eswitch port metadata takes effect after enabling switchdev mode. |
| `hairpin_num_queues` | u32 | driverinit | We refer to a TC NIC rule that involves forwarding as “hairpin”. Hairpin queues are mlx5 hardware specific implementation for hardware forwarding of such packets.  Control the number of hairpin queues. |
| `hairpin_queue_size` | u32 | driverinit | Control the size (in packets) of the hairpin queues. |
| `pcie_cong_inbound_high` | u16 | driverinit | High threshold configuration for PCIe congestion events. The firmware will send an event once device side inbound PCIe traffic went above the configured high threshold for a long enough period (at least 200ms).  See pci\_bw\_inbound\_high ethtool stat.  Units are 0.01 %. Accepted values are in range [0, 10000]. pcie\_cong\_inbound\_low < pcie\_cong\_inbound\_high. Default value: 9000 (Corresponds to 90%). |
| `pcie_cong_inbound_low` | u16 | driverinit | Low threshold configuration for PCIe congestion events. The firmware will send an event once device side inbound PCIe traffic went below the configured low threshold, only after having been previously in a congested state.  See pci\_bw\_inbound\_low ethtool stat.  Units are 0.01 %. Accepted values are in range [0, 10000]. pcie\_cong\_inbound\_low < pcie\_cong\_inbound\_high. Default value: 7500. |
| `pcie_cong_outbound_high` | u16 | driverinit | High threshold configuration for PCIe congestion events. The firmware will send an event once device side outbound PCIe traffic went above the configured high threshold for a long enough period (at least 200ms).  See pci\_bw\_outbound\_high ethtool stat.  Units are 0.01 %. Accepted values are in range [0, 10000]. pcie\_cong\_outbound\_low < pcie\_cong\_outbound\_high. Default value: 9000 (Corresponds to 90%). |
| `pcie_cong_outbound_low` | u16 | driverinit | Low threshold configuration for PCIe congestion events. The firmware will send an event once device side outbound PCIe traffic went below the configured low threshold, only after having been previously in a congested state.  See pci\_bw\_outbound\_low ethtool stat.  Units are 0.01 %. Accepted values are in range [0, 10000]. pcie\_cong\_outbound\_low < pcie\_cong\_outbound\_high. Default value: 7500. |
| `cqe_compress_type` | string | permanent | Configure which mechanism/algorithm should be used by the NIC that will affect the rate (aggressiveness) of compressed CQEs depending on PCIe bus conditions and other internal NIC factors. This mode affects all queues that enable compression. \* `balanced` : Merges fewer CQEs, resulting in a moderate compression ratio but maintaining a balance between bandwidth savings and performance \* `aggressive` : Merges more CQEs into a single entry, achieving a higher compression rate and maximizing performance, particularly under high traffic loads |
| `swp_l4_csum_mode` | string | permanent | Configure how the L4 checksum is calculated by the device when using Software Parser (SWP) hints for header locations.   * `default` : Use the device’s default checksum calculation   mode. The driver will discover during init whether or   full\_csum or l4\_only is in use. Setting this value explicitly   from userspace is not allowed, but some firmware versions may   return this value on param read. * `full_csum` : Calculate full checksum including the pseudo-header * `l4_only` : Calculate L4-only checksum, excluding the pseudo-header |

The `mlx5` driver supports reloading via `DEVLINK_CMD_RELOAD`

## Info versions

The `mlx5` driver reports the following versions

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `fw.psid` | fixed | Used to represent the board id of the device. |
| `fw.version` | stored, running | Three digit major.minor.subminor firmware version number. |

## Health reporters

### tx reporter

The tx reporter is responsible for reporting and recovering of the following three error scenarios:

* tx timeout
  :   Report on kernel tx timeout detection.
      Recover by searching lost interrupts.
* tx error completion
  :   Report on error tx completion.
      Recover by flushing the tx queue and reset it.
* tx PTP port timestamping CQ unhealthy
  :   Report too many CQEs never delivered on port ts CQ.
      Recover by flushing and re-creating all PTP channels.

tx reporter also support on demand diagnose callback, on which it provides
real time information of its send queues status.

User commands examples:

* Diagnose send queues status:

  ```
  $ devlink health diagnose pci/0000:82:00.0 reporter tx
  ```

Note

This command has valid output only when interface is up, otherwise the command has empty output.

* Show number of tx errors indicated, number of recover flows ended successfully,
  is autorecover enabled and graceful period from last recover:

  ```
  $ devlink health show pci/0000:82:00.0 reporter tx
  ```

### rx reporter

The rx reporter is responsible for reporting and recovering of the following two error scenarios:

* rx queues’ initialization (population) timeout
  :   Population of rx queues’ descriptors on ring initialization is done
      in napi context via triggering an irq. In case of a failure to get
      the minimum amount of descriptors, a timeout would occur, and
      descriptors could be recovered by polling the EQ (Event Queue).
* rx completions with errors (reported by HW on interrupt context)
  :   Report on rx completion error.
      Recover (if needed) by flushing the related queue and reset it.

rx reporter also supports on demand diagnose callback, on which it
provides real time information of its receive queues’ status.

* Diagnose rx queues’ status and corresponding completion queue:

  ```
  $ devlink health diagnose pci/0000:82:00.0 reporter rx
  ```

Note

This command has valid output only when interface is up. Otherwise, the command has empty output.

* Show number of rx errors indicated, number of recover flows ended successfully,
  is autorecover enabled, and graceful period from last recover:

  ```
  $ devlink health show pci/0000:82:00.0 reporter rx
  ```

### fw reporter

The fw reporter implements diagnose and dump callbacks.
It follows symptoms of fw error such as fw syndrome by triggering
fw core dump and storing it into the dump buffer.
The fw reporter diagnose command can be triggered any time by the user to check
current fw status.

User commands examples:

* Check fw heath status:

  ```
  $ devlink health diagnose pci/0000:82:00.0 reporter fw
  ```
* Read FW core dump if already stored or trigger new one:

  ```
  $ devlink health dump show pci/0000:82:00.0 reporter fw
  ```

Note

This command can run only on the PF which has fw tracer ownership,
running it on other PF or any VF will return “Operation not permitted”.

### fw fatal reporter

The fw fatal reporter implements dump and recover callbacks.
It follows fatal errors indications by CR-space dump and recover flow.
The CR-space dump uses vsc interface which is valid even if the FW command
interface is not functional, which is the case in most FW fatal errors.
The recover function runs recover flow which reloads the driver and triggers fw
reset if needed.
On firmware error, the health buffer is dumped into the dmesg. The log
level is derived from the error’s severity (given in health buffer).

User commands examples:

* Run fw recover flow manually:

  ```
  $ devlink health recover pci/0000:82:00.0 reporter fw_fatal
  ```
* Read FW CR-space dump if already stored or trigger new one:

  ```
  $ devlink health dump show pci/0000:82:00.1 reporter fw_fatal
  ```

Note

This command can run only on PF.

### vnic reporter

The vnic reporter implements only the diagnose callback.
It is responsible for querying the vnic diagnostic counters from fw and displaying
them in realtime.

Description of the vnic counters:

* total\_error\_queues
  :   number of queues in an error state due to
      an async error or errored command.
* send\_queue\_priority\_update\_flow
  :   number of QP/SQ priority/SL update events.
* cq\_overrun
  :   number of times CQ entered an error state due to an overflow.
* async\_eq\_overrun
  :   number of times an EQ mapped to async events was overrun.
* comp\_eq\_overrun
  :   number of times an EQ mapped to completion events was
      overrun.
* quota\_exceeded\_command
  :   number of commands issued and failed due to quota exceeded.
* invalid\_command
  :   number of commands issued and failed dues to any reason other than quota
      exceeded.
* nic\_receive\_steering\_discard
  :   number of packets that completed RX flow
      steering but were discarded due to a mismatch in flow table.
* generated\_pkt\_steering\_fail
  :   number of packets generated by the VNIC experiencing unexpected steering
      failure (at any point in steering flow).
* handled\_pkt\_steering\_fail
  :   number of packets handled by the VNIC experiencing unexpected steering
      failure (at any point in steering flow owned by the VNIC, including the FDB
      for the eswitch owner).
* icm\_consumption
  :   amount of Interconnect Host Memory (ICM) consumed by the vnic in
      granularity of 4KB. ICM is host memory allocated by SW upon HCA request
      and is used for storing data structures that control HCA operation.
* bar\_uar\_access
  :   number of WRITE or READ access operations to the UAR on the PCIe BAR.
* odp\_local\_triggered\_page\_fault
  :   number of locally-triggered page-faults due to ODP.
* odp\_remote\_triggered\_page\_fault
  :   number of remotly-triggered page-faults due to ODP.

User commands examples:

* Diagnose PF/VF vnic counters:

  ```
  $ devlink health diagnose pci/0000:82:00.1 reporter vnic
  ```
* Diagnose representor vnic counters (performed by supplying devlink port of the
  representor, which can be obtained via devlink port command):

  ```
  $ devlink health diagnose pci/0000:82:00.1/65537 reporter vnic
  ```

Note

This command can run over all interfaces such as PF/VF and representor ports.
