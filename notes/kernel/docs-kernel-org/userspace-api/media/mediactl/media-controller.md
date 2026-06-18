# Part IV - Media Controller API

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-controller.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Part IV - Media Controller API

Table of Contents

* [1. Introduction](media-controller-intro.html)
* [2. Media device model](media-controller-model.html)
* [3. Types and flags used to represent the media graph elements](media-types.html)
* [4. Request API](request-api.html)
  + [4.1. General Usage](request-api.html#general-usage)
  + [4.2. Request Allocation](request-api.html#request-allocation)
  + [4.3. Request Preparation](request-api.html#request-preparation)
  + [4.4. Request Submission](request-api.html#request-submission)
  + [4.5. Recycling and Destruction](request-api.html#recycling-and-destruction)
  + [4.6. Example for a Codec Device](request-api.html#example-for-a-codec-device)
  + [4.7. Example for a Simple Capture Device](request-api.html#example-for-a-simple-capture-device)
* [5. Function Reference](media-funcs.html)
  + [5.1. media open()](media-func-open.html)
    - [5.1.1. Name](media-func-open.html#name)
    - [5.1.2. Synopsis](media-func-open.html#synopsis)
    - [5.1.3. Arguments](media-func-open.html#arguments)
    - [5.1.4. Description](media-func-open.html#description)
    - [5.1.5. Return Value](media-func-open.html#return-value)
  + [5.2. media close()](media-func-close.html)
    - [5.2.1. Name](media-func-close.html#name)
    - [5.2.2. Synopsis](media-func-close.html#synopsis)
    - [5.2.3. Arguments](media-func-close.html#arguments)
    - [5.2.4. Description](media-func-close.html#description)
    - [5.2.5. Return Value](media-func-close.html#return-value)
  + [5.3. media ioctl()](media-func-ioctl.html)
    - [5.3.1. Name](media-func-ioctl.html#name)
    - [5.3.2. Synopsis](media-func-ioctl.html#synopsis)
    - [5.3.3. Arguments](media-func-ioctl.html#arguments)
    - [5.3.4. Description](media-func-ioctl.html#description)
    - [5.3.5. Return Value](media-func-ioctl.html#return-value)
  + [5.4. ioctl MEDIA\_IOC\_DEVICE\_INFO](media-ioc-device-info.html)
    - [5.4.1. Name](media-ioc-device-info.html#name)
    - [5.4.2. Synopsis](media-ioc-device-info.html#synopsis)
    - [5.4.3. Arguments](media-ioc-device-info.html#arguments)
    - [5.4.4. Description](media-ioc-device-info.html#description)
    - [5.4.5. Return Value](media-ioc-device-info.html#return-value)
  + [5.5. ioctl MEDIA\_IOC\_G\_TOPOLOGY](media-ioc-g-topology.html)
    - [5.5.1. Name](media-ioc-g-topology.html#name)
    - [5.5.2. Synopsis](media-ioc-g-topology.html#synopsis)
    - [5.5.3. Arguments](media-ioc-g-topology.html#arguments)
    - [5.5.4. Description](media-ioc-g-topology.html#description)
    - [5.5.5. Return Value](media-ioc-g-topology.html#return-value)
  + [5.6. ioctl MEDIA\_IOC\_ENUM\_ENTITIES](media-ioc-enum-entities.html)
    - [5.6.1. Name](media-ioc-enum-entities.html#name)
    - [5.6.2. Synopsis](media-ioc-enum-entities.html#synopsis)
    - [5.6.3. Arguments](media-ioc-enum-entities.html#arguments)
    - [5.6.4. Description](media-ioc-enum-entities.html#description)
    - [5.6.5. Return Value](media-ioc-enum-entities.html#return-value)
  + [5.7. ioctl MEDIA\_IOC\_ENUM\_LINKS](media-ioc-enum-links.html)
    - [5.7.1. Name](media-ioc-enum-links.html#name)
    - [5.7.2. Synopsis](media-ioc-enum-links.html#synopsis)
    - [5.7.3. Arguments](media-ioc-enum-links.html#arguments)
    - [5.7.4. Description](media-ioc-enum-links.html#description)
    - [5.7.5. Return Value](media-ioc-enum-links.html#return-value)
  + [5.8. ioctl MEDIA\_IOC\_SETUP\_LINK](media-ioc-setup-link.html)
    - [5.8.1. Name](media-ioc-setup-link.html#name)
    - [5.8.2. Synopsis](media-ioc-setup-link.html#synopsis)
    - [5.8.3. Arguments](media-ioc-setup-link.html#arguments)
    - [5.8.4. Description](media-ioc-setup-link.html#description)
    - [5.8.5. Return Value](media-ioc-setup-link.html#return-value)
  + [5.9. ioctl MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html)
    - [5.9.1. Name](media-ioc-request-alloc.html#name)
    - [5.9.2. Synopsis](media-ioc-request-alloc.html#synopsis)
    - [5.9.3. Arguments](media-ioc-request-alloc.html#arguments)
    - [5.9.4. Description](media-ioc-request-alloc.html#description)
    - [5.9.5. Return Value](media-ioc-request-alloc.html#return-value)
  + [5.10. request close()](request-func-close.html)
    - [5.10.1. Name](request-func-close.html#name)
    - [5.10.2. Synopsis](request-func-close.html#synopsis)
    - [5.10.3. Arguments](request-func-close.html#arguments)
    - [5.10.4. Description](request-func-close.html#description)
    - [5.10.5. Return Value](request-func-close.html#return-value)
  + [5.11. request ioctl()](request-func-ioctl.html)
    - [5.11.1. Name](request-func-ioctl.html#name)
    - [5.11.2. Synopsis](request-func-ioctl.html#synopsis)
    - [5.11.3. Arguments](request-func-ioctl.html#arguments)
    - [5.11.4. Description](request-func-ioctl.html#description)
    - [5.11.5. Return Value](request-func-ioctl.html#return-value)
  + [5.12. request poll()](request-func-poll.html)
    - [5.12.1. Name](request-func-poll.html#name)
    - [5.12.2. Synopsis](request-func-poll.html#synopsis)
    - [5.12.3. Arguments](request-func-poll.html#arguments)
    - [5.12.4. Description](request-func-poll.html#description)
    - [5.12.5. Return Value](request-func-poll.html#return-value)
  + [5.13. ioctl MEDIA\_REQUEST\_IOC\_QUEUE](media-request-ioc-queue.html)
    - [5.13.1. Name](media-request-ioc-queue.html#name)
    - [5.13.2. Synopsis](media-request-ioc-queue.html#synopsis)
    - [5.13.3. Arguments](media-request-ioc-queue.html#arguments)
    - [5.13.4. Description](media-request-ioc-queue.html#description)
    - [5.13.5. Return Value](media-request-ioc-queue.html#return-value)
  + [5.14. ioctl MEDIA\_REQUEST\_IOC\_REINIT](media-request-ioc-reinit.html)
    - [5.14.1. Name](media-request-ioc-reinit.html#name)
    - [5.14.2. Synopsis](media-request-ioc-reinit.html#synopsis)
    - [5.14.3. Arguments](media-request-ioc-reinit.html#arguments)
    - [5.14.4. Description](media-request-ioc-reinit.html#description)
    - [5.14.5. Return Value](media-request-ioc-reinit.html#return-value)
* [6. Media controller uAPI symbols](media-header.html)
  + [6.1. IOCTL Commands](media-header.html#ioctl-commands)
  + [6.2. Macros and Definitions](media-header.html#macros-and-definitions)
  + [6.3. Structures](media-header.html#structures)

## Revision and Copyright

Authors:

* Pinchart, Laurent <[laurent.pinchart@ideasonboard.com](mailto:laurent.pinchart%40ideasonboard.com)>

> * Initial version.

* Carvalho Chehab, Mauro <[mchehab@kernel.org](mailto:mchehab%40kernel.org)>

> * MEDIA\_IOC\_G\_TOPOLOGY documentation and documentation improvements.

**Copyright** © 2010 : Laurent Pinchart

**Copyright** © 2015-2016 : Mauro Carvalho Chehab

## Revision History

revision:
:   1.1.0 / 2015-12-12 (*mcc*)

revision:
:   1.0.0 / 2010-11-10 (*lp*)

Initial revision
