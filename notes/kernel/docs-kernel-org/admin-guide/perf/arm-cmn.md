# Arm Coherent Mesh Network PMU

> 출처(원문): https://docs.kernel.org/admin-guide/perf/arm-cmn.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Arm Coherent Mesh Network PMU

CMN-600 is a configurable mesh interconnect consisting of a rectangular
grid of crosspoints (XPs), with each crosspoint supporting up to two
device ports to which various AMBA CHI agents are attached.

CMN implements a distributed PMU design as part of its debug and trace
functionality. This consists of a local monitor (DTM) at every XP, which
counts up to 4 event signals from the connected device nodes and/or the
XP itself. Overflow from these local counters is accumulated in up to 8
global counters implemented by the main controller (DTC), which provides
overall PMU control and interrupts for global counter overflow.

## PMU events

The PMU driver registers a single PMU device for the whole interconnect,
see /sys/bus/event\_source/devices/arm\_cmn\_0. Multi-chip systems may link
more than one CMN together via external CCIX links - in this situation,
each mesh counts its own events entirely independently, and additional
PMU devices will be named arm\_cmn\_{1..n}.

Most events are specified in a format based directly on the TRM
definitions - “type” selects the respective node type, and “eventid” the
event number. Some events require an additional occupancy ID, which is
specified by “occupid”.

* Since RN-D nodes do not have any distinct events from RN-I nodes, they
  are treated as the same type (0xa), and the common event templates are
  named “rnid\_\*”.
* The cycle counter is treated as a synthetic event belonging to the DTC
  node (“type” == 0x3, “eventid” is ignored).
* XP events also encode the port and channel in the “eventid” field, to
  match the underlying pmu\_event0\_id encoding for the pmu\_event\_sel
  register. The event templates are named with prefixes to cover all
  permutations.

By default each event provides an aggregate count over all nodes of the
given type. To target a specific node, “bynodeid” must be set to 1 and
“nodeid” to the appropriate value derived from the CMN configuration
(as defined in the “Node ID Mapping” section of the TRM).

## Watchpoints

The PMU can also count watchpoint events to monitor specific flit
traffic. Watchpoints are treated as a synthetic event type, and like PMU
events can be global or targeted with a particular XP’s “nodeid” value.
Since the watchpoint direction is otherwise implicit in the underlying
register selection, separate events are provided for flit uploads and
downloads.

The flit match value and mask are passed in config1 and config2 (“val”
and “mask” respectively). “wp\_dev\_sel”, “wp\_chn\_sel”, “wp\_grp” and
“wp\_exclusive” are specified per the TRM definitions for dtm\_wp\_config0.
Where a watchpoint needs to match fields from both match groups on the
REQ or SNP channel, it can be specified as two events - one for each
group - with the same nonzero “combine” value. The count for such a
pair of combined events will be attributed to the primary match.
Watchpoint events with a “combine” value of 0 are considered independent
and will count individually.
