# AMD-TEE (AMD’s Trusted Execution Environment)

> 출처(원문): https://docs.kernel.org/tee/amd-tee.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# AMD-TEE (AMD’s Trusted Execution Environment)

The AMD-TEE driver handles the communication with AMD’s TEE environment. The
TEE environment is provided by AMD Secure Processor.

The AMD Secure Processor (formerly called Platform Security Processor or PSP)
is a dedicated processor that features ARM TrustZone technology, along with a
software-based Trusted Execution Environment (TEE) designed to enable
third-party Trusted Applications. This feature is currently enabled only for
APUs.

The following picture shows a high level overview of AMD-TEE:

```
                                            |
   x86                                      |
                                            |
User space            (Kernel space)        |    AMD Secure Processor (PSP)
~~~~~~~~~~            ~~~~~~~~~~~~~~        |    ~~~~~~~~~~~~~~~~~~~~~~~~~~
                                            |
+--------+                                  |       +-------------+
| Client |                                  |       | Trusted     |
+--------+                                  |       | Application |
    /\                                      |       +-------------+
    ||                                      |             /\
    ||                                      |             ||
    ||                                      |             \/
    ||                                      |         +----------+
    ||                                      |         |   TEE    |
    ||                                      |         | Internal |
    \/                                      |         |   API    |
+---------+           +-----------+---------+         +----------+
| TEE     |           | TEE       | AMD-TEE |         | AMD-TEE  |
| Client  |           | subsystem | driver  |         | Trusted  |
| API     |           |           |         |         |   OS     |
+---------+-----------+----+------+---------+---------+----------+
|   Generic TEE API        |      | ASP     |      Mailbox       |
|   IOCTL (TEE_IOC_*)      |      | driver  | Register Protocol  |
+--------------------------+      +---------+--------------------+
```

At the lowest level (in x86), the AMD Secure Processor (ASP) driver uses the
CPU to PSP mailbox register to submit commands to the PSP. The format of the
command buffer is opaque to the ASP driver. It’s role is to submit commands to
the secure processor and return results to AMD-TEE driver. The interface
between AMD-TEE driver and AMD Secure Processor driver can be found in [1].

The AMD-TEE driver packages the command buffer payload for processing in TEE.
The command buffer format for the different TEE commands can be found in [2].

The TEE commands supported by AMD-TEE Trusted OS are:

* TEE\_CMD\_ID\_LOAD\_TA - loads a Trusted Application (TA) binary into
  :   TEE environment.
* TEE\_CMD\_ID\_UNLOAD\_TA - unloads TA binary from TEE environment.
* TEE\_CMD\_ID\_OPEN\_SESSION - opens a session with a loaded TA.
* TEE\_CMD\_ID\_CLOSE\_SESSION - closes session with loaded TA
* TEE\_CMD\_ID\_INVOKE\_CMD - invokes a command with loaded TA
* TEE\_CMD\_ID\_MAP\_SHARED\_MEM - maps shared memory
* TEE\_CMD\_ID\_UNMAP\_SHARED\_MEM - unmaps shared memory

AMD-TEE Trusted OS is the firmware running on AMD Secure Processor.

The AMD-TEE driver registers itself with TEE subsystem and implements the
following driver function callbacks:

* get\_version - returns the driver implementation id and capability.
* open - sets up the driver context data structure.
* release - frees up driver resources.
* open\_session - loads the TA binary and opens session with loaded TA.
* close\_session - closes session with loaded TA and unloads it.
* invoke\_func - invokes a command with loaded TA.

cancel\_req driver callback is not supported by AMD-TEE.

The GlobalPlatform TEE Client API [3] can be used by the user space (client) to
talk to AMD’s TEE. AMD’s TEE provides a secure environment for loading, opening
a session, invoking commands and closing session with TA.

## References

[1] include/linux/psp-tee.h

[2] drivers/tee/amdtee/amdtee\_if.h

[3] <http://www.globalplatform.org/specificationsdevice.asp> look for
:   “TEE Client API Specification v1.0” and click download.
