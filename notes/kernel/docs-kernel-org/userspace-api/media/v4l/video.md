# 1.4.Video Inputs and Outputs

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/video.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.4. Video Inputs and Outputs

Video inputs and outputs are physical connectors of a device. These can
be for example: RF connectors (antenna/cable), CVBS a.k.a. Composite
Video, S-Video and RGB connectors. Camera sensors are also considered to
be a video input. Video and VBI capture devices have inputs. Video and
VBI output devices have outputs, at least one each. Radio devices have
no video inputs or outputs.

To learn about the number and attributes of the available inputs and
outputs applications can enumerate them with the
[ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) and
[ioctl VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput) ioctl, respectively. The
struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input") returned by the
[ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) ioctl also contains signal
status information applicable when the current video input is queried.

The [VIDIOC\_G\_INPUT](vidioc-g-input.html#vidioc-g-input) and
[VIDIOC\_G\_OUTPUT](vidioc-g-output.html#vidioc-g-output) ioctls return the index of
the current video input or output. To select a different input or output
applications call the [VIDIOC\_S\_INPUT](vidioc-g-input.html#vidioc-g-input) and
[VIDIOC\_S\_OUTPUT](vidioc-g-output.html#vidioc-g-output) ioctls. Drivers must
implement all the input ioctls when the device has one or more inputs,
all the output ioctls when the device has one or more outputs.

## 1.4.1. Example: Information about the current video input

```
struct v4l2_input input;
int index;

if (-1 == ioctl(fd, VIDIOC_G_INPUT, &index)) {
    perror("VIDIOC_G_INPUT");
    exit(EXIT_FAILURE);
}

memset(&input, 0, sizeof(input));
input.index = index;

if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
    perror("VIDIOC_ENUMINPUT");
    exit(EXIT_FAILURE);
}

printf("Current input: %s\\n", input.name);
```

## 1.4.2. Example: Switching to the first video input

```
int index;

index = 0;

if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index)) {
    perror("VIDIOC_S_INPUT");
    exit(EXIT_FAILURE);
}
```
