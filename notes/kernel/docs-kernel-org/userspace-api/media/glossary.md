# Glossary

> 출처(원문): https://docs.kernel.org/userspace-api/media/glossary.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Glossary

Note

The goal of this section is to standardize the terms used within the media
userspace API documentation. This is Work In Progress.

Bridge Driver
:   A [Device Driver](#term-Device-Driver) that implements the main logic to talk with
    media hardware.

CEC API
:   **Consumer Electronics Control API**

    An API designed to receive and transmit data via an HDMI
    CEC interface.

    See [Part V - Consumer Electronics Control API](cec/cec-api.html#cec).

Data Unit
:   Unit of data transported by a bus. On parallel buses, the data unit
    consists of one or more related samples while on serial buses the data
    unit is logical. If the data unit is image data, it may also be called a
    pixel.

Device Driver
:   Part of the Linux Kernel that implements support for a hardware
    component.

Device Node
:   A character device node in the file system used to control and
    transfer data in and out of a Kernel driver.

Digital TV API
:   **Previously known as DVB API**

    An API designed to control a subset of the [Media Hardware](#term-Media-Hardware)
    that implements digital TV (e. g. DVB, ATSC, ISDB, etc).

    See [Part II - Digital TV API](dvb/dvbapi.html#dvbapi).

DSP
:   **Digital Signal Processor**

    A specialized [Microprocessor](#term-Microprocessor), with its architecture
    optimized for the operational needs of digital signal processing.

FPGA
:   **Field-programmable Gate Array**

    An [IC](#term-IC) circuit designed to be configured by a customer or
    a designer after manufacturing.

    See <https://en.wikipedia.org/wiki/Field-programmable_gate_array>.

Hardware Component
:   A subset of the [Media Hardware](#term-Media-Hardware). For example an [I²C](#term-I2C) or
    [SPI](#term-SPI) device, or an [IP Block](#term-IP-Block) inside an
    [SoC](#term-SoC) or [FPGA](#term-FPGA).

Hardware Peripheral
:   A group of [hardware components](#term-Hardware-Component) that
    together make a larger user-facing functional peripheral. For
    instance, the [SoC](#term-SoC) [ISP](#term-ISP) [IP Block](#term-IP-Block)
    and the external camera sensors together make a camera hardware
    peripheral.

    Also known as [Peripheral](#term-Peripheral).

I²C
:   **Inter-Integrated Circuit**

    A multi-master, multi-slave, packet switched, single-ended,
    serial computer bus used to control some hardware components
    like sub-device hardware components.

    See <http://www.nxp.com/docs/en/user-guide/UM10204.pdf>.

IC
:   **Integrated circuit**

    A set of electronic circuits on one small flat piece of
    semiconductor material, normally silicon.

    Also known as chip.

IP Block
:   **Intellectual property core**

    In electronic design a semiconductor intellectual property core,
    is a reusable unit of logic, cell, or integrated circuit layout
    design that is the intellectual property of one party.
    IP Blocks may be licensed to another party or can be owned
    and used by a single party alone.

    See <https://en.wikipedia.org/wiki/Semiconductor_intellectual_property_core>).

ISP
:   **Image Signal Processor**

    A specialized processor that implements a set of algorithms for
    processing image data. ISPs may implement algorithms for lens
    shading correction, demosaicing, scaling and pixel format conversion
    as well as produce statistics for the use of the control
    algorithms (e.g. automatic exposure, white balance and focus).

Media API
:   A set of userspace APIs used to control the media hardware. It is
    composed by:

    > * [CEC API](#term-CEC-API);
    > * [Digital TV API](#term-Digital-TV-API);
    > * [MC API](#term-MC-API);
    > * [RC API](#term-RC-API); and
    > * [V4L2 API](#term-V4L2-API).

    See [Linux Media Infrastructure userspace API](index.html).

MC API
:   **Media Controller API**

    An API designed to expose and control the relationships between
    multimedia devices and sub-devices.

    See [Part IV - Media Controller API](mediactl/media-controller.html#media-controller).

MC-centric
:   [V4L2 Hardware](#term-V4L2-Hardware) device driver that requires [MC API](#term-MC-API).

    Such drivers have `V4L2_CAP_IO_MC` device\_caps field set
    (see [ioctl VIDIOC\_QUERYCAP](v4l/vidioc-querycap.html#vidioc-querycap)).

    See [Controlling a hardware peripheral via V4L2](v4l/open.html#v4l2-hardware-control) for more details.

Media Hardware
:   Subset of the hardware that is supported by the Linux Media API.

    This includes audio and video capture and playback hardware,
    digital and analog TV, camera sensors, ISPs, remote controllers,
    codecs, HDMI Consumer Electronics Control, HDMI capture, etc.

Microprocessor
:   Electronic circuitry that carries out the instructions of a
    computer program by performing the basic arithmetic, logical,
    control and input/output (I/O) operations specified by the
    instructions on a single integrated circuit.

Peripheral
:   The same as [Hardware Peripheral](#term-Hardware-Peripheral).

RC API
:   **Remote Controller API**

    An API designed to receive and transmit data from remote
    controllers.

    See [Part III - Remote Controller API](rc/remote_controllers.html#remote-controllers).

SMBus
:   A subset of I²C, which defines a stricter usage of the bus.

SPI
:   **Serial Peripheral Interface Bus**

    Synchronous serial communication interface specification used for
    short distance communication, primarily in embedded systems.

SoC
:   **System on a Chip**

    An integrated circuit that integrates all components of a computer
    or other electronic systems.

Stream
:   A distinct flow of data (image data or metadata) from an initial source
    to a final sink. The initial source may be e.g. an image sensor and the
    final sink e.g. a memory buffer.

V4L2 API
:   **V4L2 userspace API**

    The userspace API defined in [Part I - Video for Linux API](v4l/v4l2.html#v4l2spec), which is used to
    control a V4L2 hardware.

V4L2 Device Node
:   A [Device Node](#term-Device-Node) that is associated to a V4L driver.

    The V4L2 device node naming is specified at [V4L2 Device Node Naming](v4l/open.html#v4l2-device-naming).

V4L2 Hardware
:   Part of the media hardware which is supported by the [V4L2 API](#term-V4L2-API).

V4L2 Sub-device
:   V4L2 hardware components that aren’t controlled by a
    [Bridge Driver](#term-Bridge-Driver). See [Sub-device Interface](v4l/dev-subdev.html#subdev).

Video-node-centric
:   V4L2 device driver that doesn’t require a media controller to be used.

    Such drivers have the `V4L2_CAP_IO_MC` device\_caps field unset
    (see [ioctl VIDIOC\_QUERYCAP](v4l/vidioc-querycap.html#vidioc-querycap)).

V4L2 Sub-device API
:   Part of the [V4L2 API](#term-V4L2-API) which control
    [V4L2 sub-devices](#term-V4L2-Sub-device), like sensors,
    HDMI receivers, scalers, deinterlacers.

    See [Controlling a hardware peripheral via V4L2](v4l/open.html#v4l2-hardware-control) for more details.
