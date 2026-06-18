# Virtual TPM Proxy Driver for Linux Containers

> 출처(원문): https://docs.kernel.org/security/tpm/tpm_vtpm_proxy.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Virtual TPM Proxy Driver for Linux Containers

Authors:

Stefan Berger <[stefanb@linux.vnet.ibm.com](mailto:stefanb%40linux.vnet.ibm.com)>

This document describes the virtual Trusted Platform Module (vTPM)
proxy device driver for Linux containers.

## Introduction

The goal of this work is to provide TPM functionality to each Linux
container. This allows programs to interact with a TPM in a container
the same way they interact with a TPM on the physical system. Each
container gets its own unique, emulated, software TPM.

## Design

To make an emulated software TPM available to each container, the container
management stack needs to create a device pair consisting of a client TPM
character device `/dev/tpmX` (with X=0,1,2...) and a ‘server side’ file
descriptor. The former is moved into the container by creating a character
device with the appropriate major and minor numbers while the file descriptor
is passed to the TPM emulator. Software inside the container can then send
TPM commands using the character device and the emulator will receive the
commands via the file descriptor and use it for sending back responses.

To support this, the virtual TPM proxy driver provides a device `/dev/vtpmx`
that is used to create device pairs using an ioctl. The ioctl takes as
an input flags for configuring the device. The flags for example indicate
whether TPM 1.2 or TPM 2 functionality is supported by the TPM emulator.
The result of the ioctl are the file descriptor for the ‘server side’
as well as the major and minor numbers of the character device that was created.
Besides that the number of the TPM character device is returned. If for
example `/dev/tpm10` was created, the number (`dev_num`) 10 is returned.

Once the device has been created, the driver will immediately try to talk
to the TPM. All commands from the driver can be read from the file descriptor
returned by the ioctl. The commands should be responded to immediately.

## UAPI

enum vtpm\_proxy\_flags
:   flags for the proxy TPM

**Constants**

`VTPM_PROXY_FLAG_TPM2`
:   the proxy TPM uses TPM 2.0 protocol

struct vtpm\_proxy\_new\_dev
:   parameter structure for the `VTPM_PROXY_IOC_NEW_DEV` ioctl

**Definition**:

```
struct vtpm_proxy_new_dev {
    __u32 flags;
    __u32 tpm_num;
    __u32 fd;
    __u32 major;
    __u32 minor;
};
```

**Members**

`flags`
:   flags for the proxy TPM

`tpm_num`
:   index of the TPM device

`fd`
:   the file descriptor used by the proxy TPM

`major`
:   the major number of the TPM device

`minor`
:   the minor number of the TPM device

long vtpmx\_ioc\_new\_dev(struct [file](#c.vtpmx_ioc_new_dev "file") \*file, unsigned int ioctl, unsigned long arg)
:   handler for the `VTPM_PROXY_IOC_NEW_DEV` ioctl

**Parameters**

`struct file *file`
:   /dev/vtpmx

`unsigned int ioctl`
:   the ioctl number

`unsigned long arg`
:   pointer to the `struct vtpmx_proxy_new_dev`

**Description**

Creates an anonymous file that is used by the process acting as a TPM to
communicate with the client processes. The function will also add a new TPM
device through which data is proxied to this TPM acting process. The caller
will be provided with a file descriptor to communicate with the clients and
major and minor numbers for the TPM device.
