# 7.10.ioctl VIDIOC_ENCODER_CMD, VIDIOC_TRY_ENCODER_CMD

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-encoder-cmd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.10. ioctl VIDIOC\_ENCODER\_CMD, VIDIOC\_TRY\_ENCODER\_CMD

## 7.10.1. Name

VIDIOC\_ENCODER\_CMD - VIDIOC\_TRY\_ENCODER\_CMD - Execute an encoder command

## 7.10.2. Synopsis

VIDIOC\_ENCODER\_CMD

`int ioctl(int fd, VIDIOC_ENCODER_CMD, struct v4l2_encoder_cmd *argp)`

VIDIOC\_TRY\_ENCODER\_CMD

`int ioctl(int fd, VIDIOC_TRY_ENCODER_CMD, struct v4l2_encoder_cmd *argp)`

## 7.10.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_encoder_cmd`](#c.V4L.v4l2_encoder_cmd "v4l2_encoder_cmd").

## 7.10.4. Description

These ioctls control an audio/video (usually MPEG-) encoder.
`VIDIOC_ENCODER_CMD` sends a command to the encoder,
`VIDIOC_TRY_ENCODER_CMD` can be used to try a command without actually
executing it.

To send a command applications must initialize all fields of a struct
[`v4l2_encoder_cmd`](#c.V4L.v4l2_encoder_cmd "v4l2_encoder_cmd") and call
`VIDIOC_ENCODER_CMD` or `VIDIOC_TRY_ENCODER_CMD` with a pointer to
this structure.

The `cmd` field must contain the command code. Some commands use the
`flags` field for additional information.

After a STOP command, [`read()`](func-read.html#c.V4L.read "read") calls will read
the remaining data buffered by the driver. When the buffer is empty,
[`read()`](func-read.html#c.V4L.read "read") will return zero and the next [`read()`](func-read.html#c.V4L.read "read")
call will restart the encoder.

A [`read()`](func-read.html#c.V4L.read "read") or [VIDIOC\_STREAMON](vidioc-streamon.html#vidioc-streamon)
call sends an implicit START command to the encoder if it has not been
started yet. Applies to both queues of mem2mem encoders.

A [`close()`](func-close.html#c.V4L.close "close") or [VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon)
call of a streaming file descriptor sends an implicit immediate STOP to
the encoder, and all buffered data is discarded. Applies to both queues of
mem2mem encoders.

These ioctls are optional, not all drivers may support them. They were
introduced in Linux 2.6.21. They are, however, mandatory for stateful mem2mem
encoders (as further documented in [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder)).

type v4l2\_encoder\_cmd

struct v4l2\_encoder\_cmd

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `cmd` | The encoder command, see [Encoder Commands](#encoder-cmds). |
| \_\_u32 | `flags` | Flags to go with the command, see [Encoder Command Flags](#encoder-flags). If no flags are defined for this command, drivers and applications must set this field to zero. |
| \_\_u32 | `data`[8] | Reserved for future extensions. Drivers and applications must set the array to zero. |

Encoder Commands

|  |  |  |
| --- | --- | --- |
| `V4L2_ENC_CMD_START` | 0 | Start the encoder. When the encoder is already running or paused, this command does nothing. No flags are defined for this command.  For a device implementing the [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder), once the drain sequence is initiated with the `V4L2_ENC_CMD_STOP` command, it must be driven to completion before this command can be invoked. Any attempt to invoke the command while the drain sequence is in progress will trigger an `EBUSY` error code. See [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder) for more details. |
| `V4L2_ENC_CMD_STOP` | 1 | Stop the encoder. When the `V4L2_ENC_CMD_STOP_AT_GOP_END` flag is set, encoding will continue until the end of the current *Group Of Pictures*, otherwise encoding will stop immediately. When the encoder is already stopped, this command does nothing.  For a device implementing the [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder), the command will initiate the drain sequence as documented in [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder). No flags or other arguments are accepted in this case. Any attempt to invoke the command again before the sequence completes will trigger an `EBUSY` error code. |
| `V4L2_ENC_CMD_PAUSE` | 2 | Pause the encoder. When the encoder has not been started yet, the driver will return an `EPERM` error code. When the encoder is already paused, this command does nothing. No flags are defined for this command. |
| `V4L2_ENC_CMD_RESUME` | 3 | Resume encoding after a PAUSE command. When the encoder has not been started yet, the driver will return an `EPERM` error code. When the encoder is already running, this command does nothing. No flags are defined for this command. |

Encoder Command Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_ENC_CMD_STOP_AT_GOP_END` | 0x0001 | Stop encoding at the end of the current *Group Of Pictures*, rather than immediately.  Does not apply to [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder). |

## 7.10.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   A drain sequence of a device implementing the [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder) is still in
    progress. It is not allowed to issue another encoder command until it
    completes.

EINVAL
:   The `cmd` field is invalid.

EPERM
:   The application sent a PAUSE or RESUME command when the encoder was
    not running.
