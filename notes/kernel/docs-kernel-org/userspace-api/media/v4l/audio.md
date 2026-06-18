# 1.5.Audio Inputs and Outputs

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/audio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.5. Audio Inputs and Outputs

Audio inputs and outputs are physical connectors of a device. Video
capture devices have inputs, output devices have outputs, zero or more
each. Radio devices have no audio inputs or outputs. They have exactly
one tuner which in fact *is* an audio source, but this API associates
tuners with video inputs or outputs only, and radio devices have none of
these. [[1]](#f1) A connector on a TV card to loop back the received audio
signal to a sound card is not considered an audio output.

Audio and video inputs and outputs are associated. Selecting a video
source also selects an audio source. This is most evident when the video
and audio source is a tuner. Further audio connectors can combine with
more than one video input or output. Assumed two composite video inputs
and two audio inputs exist, there may be up to four valid combinations.
The relation of video and audio connectors is defined in the
`audioset` field of the respective struct
[`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input") or struct
[`v4l2_output`](vidioc-enumoutput.html#c.V4L.v4l2_output "v4l2_output"), where each bit represents the index
number, starting at zero, of one audio input or output.

To learn about the number and attributes of the available inputs and
outputs applications can enumerate them with the
[ioctl VIDIOC\_ENUMAUDIO](vidioc-enumaudio.html#vidioc-enumaudio) and
[VIDIOC\_ENUMAUDOUT](vidioc-enumaudioout.html#vidioc-enumaudout) ioctl, respectively.
The struct [`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio") returned by the
[ioctl VIDIOC\_ENUMAUDIO](vidioc-enumaudio.html#vidioc-enumaudio) ioctl also contains signal
status information applicable when the current audio input is queried.

The [VIDIOC\_G\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) and
[VIDIOC\_G\_AUDOUT](vidioc-g-audioout.html#vidioc-g-audout) ioctls report the current
audio input and output, respectively.

Note

Note that, unlike [VIDIOC\_G\_INPUT](vidioc-g-input.html#vidioc-g-input) and
[VIDIOC\_G\_OUTPUT](vidioc-g-output.html#vidioc-g-output) these ioctls return a
structure as [ioctl VIDIOC\_ENUMAUDIO](vidioc-enumaudio.html#vidioc-enumaudio) and
[VIDIOC\_ENUMAUDOUT](vidioc-enumaudioout.html#vidioc-enumaudout) do, not just an index.

To select an audio input and change its properties applications call the
[VIDIOC\_S\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) ioctl. To select an audio
output (which presently has no changeable properties) applications call
the [VIDIOC\_S\_AUDOUT](vidioc-g-audioout.html#vidioc-g-audout) ioctl.

Drivers must implement all audio input ioctls when the device has
multiple selectable audio inputs, all audio output ioctls when the
device has multiple selectable audio outputs. When the device has any
audio inputs or outputs the driver must set the `V4L2_CAP_AUDIO` flag
in the struct [`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by
the [ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl.

## 1.5.1. Example: Information about the current audio input

```
struct v4l2_audio audio;

memset(&audio, 0, sizeof(audio));

if (-1 == ioctl(fd, VIDIOC_G_AUDIO, &audio)) {
    perror("VIDIOC_G_AUDIO");
    exit(EXIT_FAILURE);
}

printf("Current input: %s\\n", audio.name);
```

## 1.5.2. Example: Switching to the first audio input

```
struct v4l2_audio audio;

memset(&audio, 0, sizeof(audio)); /* clear audio.mode, audio.reserved */

audio.index = 0;

if (-1 == ioctl(fd, VIDIOC_S_AUDIO, &audio)) {
    perror("VIDIOC_S_AUDIO");
    exit(EXIT_FAILURE);
}
```

[[1](#id1)]

Actually struct [`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio") ought to have a
`tuner` field like struct [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "v4l2_input"), not
only making the API more consistent but also permitting radio devices
with multiple tuners.
