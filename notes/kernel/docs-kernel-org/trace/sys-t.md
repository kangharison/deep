# MIPI SyS-T over STP

> 출처(원문): https://docs.kernel.org/trace/sys-t.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# MIPI SyS-T over STP

The MIPI SyS-T protocol driver can be used with STM class devices to
generate standardized trace stream. Aside from being a standard, it
provides better trace source identification and timestamp correlation.

In order to use the MIPI SyS-T protocol driver with your STM device,
first, you’ll need CONFIG\_STM\_PROTO\_SYS\_T.

Now, you can select which protocol driver you want to use when you create
a policy for your STM device, by specifying it in the policy name:

# mkdir /config/stp-policy/dummy\_stm.0:p\_sys-t.my-policy/

In other words, the policy name format is extended like this:

> <device\_name>:<protocol\_name>.<policy\_name>

With Intel TH, therefore it can look like “0-sth:p\_sys-t.my-policy”.

If the protocol name is omitted, the STM class will chose whichever
protocol driver was loaded first.

You can also double check that everything is working as expected by

# cat /config/stp-policy/dummy\_stm.0:p\_sys-t.my-policy/protocol
p\_sys-t

Now, with the MIPI SyS-T protocol driver, each policy node in the
configfs gets a few additional attributes, which determine per-source
parameters specific to the protocol:

# mkdir /config/stp-policy/dummy\_stm.0:p\_sys-t.my-policy/default
# ls /config/stp-policy/dummy\_stm.0:p\_sys-t.my-policy/default
channels
clocksync\_interval
do\_len
masters
ts\_interval
uuid

The most important one here is the “uuid”, which determines the UUID
that will be used to tag all data coming from this source. It is
automatically generated when a new node is created, but it is likely
that you would want to change it.

do\_len switches on/off the additional “payload length” field in the
MIPI SyS-T message header. It is off by default as the STP already
marks message boundaries.

ts\_interval and clocksync\_interval determine how much time in milliseconds
can pass before we need to include a protocol (not transport, aka STP)
timestamp in a message header or send a CLOCKSYNC packet, respectively.

See [ABI file testing/configfs-stp-policy-p\_sys-t](../admin-guide/abi-testing-files.html#abi-file-testing-configfs-stp-policy-p-sys-t) for more
details.

* [1] <https://www.mipi.org/specifications/sys-t>
