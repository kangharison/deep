# 1.Common API Elements

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/common.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. Common API Elements

Programming a V4L2 device consists of these steps:

* Opening the device
* Changing device properties, selecting a video and audio input, video
  standard, picture brightness a. o.
* Negotiating a data format
* Negotiating an input/output method
* The actual input/output loop
* Closing the device

In practice most steps are optional and can be executed out of order. It
depends on the V4L2 device type, you can read about the details in
[Interfaces](devices.html#devices). In this chapter we will discuss the basic concepts
applicable to all devices.

* [1.1. Opening and Closing Devices](open.html)
* [1.2. Querying Capabilities](querycap.html)
* [1.3. Application Priority](app-pri.html)
* [1.4. Video Inputs and Outputs](video.html)
* [1.5. Audio Inputs and Outputs](audio.html)
* [1.6. Tuners and Modulators](tuner.html)
* [1.7. Video Standards](standard.html)
* [1.8. Digital Video (DV) Timings](dv-timings.html)
* [1.9. User Controls](control.html)
* [1.10. Extended Controls API](extended-controls.html)
* [1.11. Camera Control Reference](ext-ctrls-camera.html)
* [1.12. Flash Control Reference](ext-ctrls-flash.html)
* [1.13. Image Source Control Reference](ext-ctrls-image-source.html)
* [1.14. Image Process Control Reference](ext-ctrls-image-process.html)
* [1.15. Codec Control Reference](ext-ctrls-codec.html)
* [1.16. Stateless Codec Control Reference](ext-ctrls-codec-stateless.html)
* [1.17. JPEG Control Reference](ext-ctrls-jpeg.html)
* [1.18. Digital Video Control Reference](ext-ctrls-dv.html)
* [1.19. RF Tuner Control Reference](ext-ctrls-rf-tuner.html)
* [1.20. FM Transmitter Control Reference](ext-ctrls-fm-tx.html)
* [1.21. FM Receiver Control Reference](ext-ctrls-fm-rx.html)
* [1.22. Detect Control Reference](ext-ctrls-detect.html)
* [1.23. Colorimetry Control Reference](ext-ctrls-colorimetry.html)
* [1.24. Guidelines for Video4Linux pixel format 4CCs](fourcc.html)
* [1.25. Data Formats](format.html)
* [1.26. Single- and multi-planar APIs](planar-apis.html)
* [1.27. Cropping, composing and scaling -- the SELECTION API](selection-api.html)
* [1.28. Image Cropping, Insertion and Scaling -- the CROP API](crop.html)
* [1.29. Streaming Parameters](streaming-par.html)
