# 1.8.Digital Video (DV) Timings

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dv-timings.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.8. Digital Video (DV) Timings

The video standards discussed so far have been dealing with Analog TV
and the corresponding video timings. Today there are many more different
hardware interfaces such as High Definition TV interfaces (HDMI), VGA,
DVI connectors etc., that carry video signals and there is a need to
extend the API to select the video timings for these interfaces. Since
it is not possible to extend the [v4l2\_std\_id](vidioc-enumstd.html#v4l2-std-id)
due to the limited bits available, a new set of ioctls was added to
set/get video timings at the input and output.

These ioctls deal with the detailed digital video timings that define
each video format. This includes parameters such as the active video
width and height, signal polarities, frontporches, backporches, sync
widths etc. The `linux/v4l2-dv-timings.h` header can be used to get
the timings of the formats in the [CEA-861-E](biblio.html#cea861) and [VESA DMT](biblio.html#vesadmt)
standards.

To enumerate and query the attributes of the DV timings supported by a
device applications use the
[ioctl VIDIOC\_ENUM\_DV\_TIMINGS, VIDIOC\_SUBDEV\_ENUM\_DV\_TIMINGS](vidioc-enum-dv-timings.html#vidioc-enum-dv-timings) and
[ioctl VIDIOC\_DV\_TIMINGS\_CAP, VIDIOC\_SUBDEV\_DV\_TIMINGS\_CAP](vidioc-dv-timings-cap.html#vidioc-dv-timings-cap) ioctls. To set
DV timings for the device applications use the
[VIDIOC\_S\_DV\_TIMINGS](vidioc-g-dv-timings.html#vidioc-g-dv-timings) ioctl and to get
current DV timings they use the
[VIDIOC\_G\_DV\_TIMINGS](vidioc-g-dv-timings.html#vidioc-g-dv-timings) ioctl. To detect
the DV timings as seen by the video receiver applications use the
[ioctl VIDIOC\_QUERY\_DV\_TIMINGS](vidioc-query-dv-timings.html#vidioc-query-dv-timings) ioctl.

When the hardware detects a video source change (e.g. the video
signal appears or disappears, or the video resolution changes), then
it will issue a V4L2\_EVENT\_SOURCE\_CHANGE event. Use the
[ioctl VIDIOC\_SUBSCRIBE\_EVENT](vidioc-subscribe-event.html#vidioc-subscribe-event) and the
[ioctl VIDIOC\_DQEVENT](vidioc-dqevent.html#vidioc-dqevent) to check if this event was reported.

If the video signal changed, then the application has to stop
streaming, free all buffers, and call the [ioctl VIDIOC\_QUERY\_DV\_TIMINGS](vidioc-query-dv-timings.html#vidioc-query-dv-timings)
to obtain the new video timings, and if they are valid, it can set
those by calling the [ioctl VIDIOC\_S\_DV\_TIMINGS](vidioc-g-dv-timings.html#vidioc-g-dv-timings).
This will also update the format, so use the [ioctl VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
to obtain the new format. Now the application can allocate new buffers
and start streaming again.

The [ioctl VIDIOC\_QUERY\_DV\_TIMINGS](vidioc-query-dv-timings.html#vidioc-query-dv-timings) will just report what the
hardware detects, it will never change the configuration. If the
currently set timings and the actually detected timings differ, then
typically this will mean that you will not be able to capture any
video. The correct approach is to rely on the V4L2\_EVENT\_SOURCE\_CHANGE
event so you know when something changed.

Applications can make use of the [Input capabilities](vidioc-enuminput.html#input-capabilities) and
[Output capabilities](vidioc-enumoutput.html#output-capabilities) flags to determine whether the digital
video ioctls can be used with the given input or output.
