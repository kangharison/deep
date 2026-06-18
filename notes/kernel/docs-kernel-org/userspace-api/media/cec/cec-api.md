# Part V - Consumer Electronics Control API

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Part V - Consumer Electronics Control API

This part describes the CEC: Consumer Electronics Control

Table of Contents

* [1. Introduction](cec-intro.html)
* [2. Function Reference](cec-funcs.html)
  + [2.1. cec open()](cec-func-open.html)
    - [2.1.1. Name](cec-func-open.html#name)
    - [2.1.2. Synopsis](cec-func-open.html#synopsis)
    - [2.1.3. Arguments](cec-func-open.html#arguments)
    - [2.1.4. Description](cec-func-open.html#description)
    - [2.1.5. Return Value](cec-func-open.html#return-value)
  + [2.2. cec close()](cec-func-close.html)
    - [2.2.1. Name](cec-func-close.html#name)
    - [2.2.2. Synopsis](cec-func-close.html#synopsis)
    - [2.2.3. Arguments](cec-func-close.html#arguments)
    - [2.2.4. Description](cec-func-close.html#description)
    - [2.2.5. Return Value](cec-func-close.html#return-value)
  + [2.3. cec ioctl()](cec-func-ioctl.html)
    - [2.3.1. Name](cec-func-ioctl.html#name)
    - [2.3.2. Synopsis](cec-func-ioctl.html#synopsis)
    - [2.3.3. Arguments](cec-func-ioctl.html#arguments)
    - [2.3.4. Description](cec-func-ioctl.html#description)
    - [2.3.5. Return Value](cec-func-ioctl.html#return-value)
  + [2.4. cec poll()](cec-func-poll.html)
    - [2.4.1. Name](cec-func-poll.html#name)
    - [2.4.2. Synopsis](cec-func-poll.html#synopsis)
    - [2.4.3. Arguments](cec-func-poll.html#arguments)
    - [2.4.4. Description](cec-func-poll.html#description)
    - [2.4.5. Return Value](cec-func-poll.html#return-value)
  + [2.5. ioctl CEC\_ADAP\_G\_CAPS](cec-ioc-adap-g-caps.html)
    - [2.5.1. Name](cec-ioc-adap-g-caps.html#name)
    - [2.5.2. Synopsis](cec-ioc-adap-g-caps.html#synopsis)
    - [2.5.3. Arguments](cec-ioc-adap-g-caps.html#arguments)
    - [2.5.4. Description](cec-ioc-adap-g-caps.html#description)
    - [2.5.5. Return Value](cec-ioc-adap-g-caps.html#return-value)
  + [2.6. ioctls CEC\_ADAP\_G\_LOG\_ADDRS and CEC\_ADAP\_S\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html)
    - [2.6.1. Name](cec-ioc-adap-g-log-addrs.html#name)
    - [2.6.2. Synopsis](cec-ioc-adap-g-log-addrs.html#synopsis)
    - [2.6.3. Arguments](cec-ioc-adap-g-log-addrs.html#arguments)
    - [2.6.4. Description](cec-ioc-adap-g-log-addrs.html#description)
    - [2.6.5. Return Value](cec-ioc-adap-g-log-addrs.html#return-value)
  + [2.7. ioctls CEC\_ADAP\_G\_PHYS\_ADDR and CEC\_ADAP\_S\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html)
    - [2.7.1. Name](cec-ioc-adap-g-phys-addr.html#name)
    - [2.7.2. Synopsis](cec-ioc-adap-g-phys-addr.html#synopsis)
    - [2.7.3. Arguments](cec-ioc-adap-g-phys-addr.html#arguments)
    - [2.7.4. Description](cec-ioc-adap-g-phys-addr.html#description)
    - [2.7.5. Return Value](cec-ioc-adap-g-phys-addr.html#return-value)
  + [2.8. ioctl CEC\_ADAP\_G\_CONNECTOR\_INFO](cec-ioc-adap-g-conn-info.html)
    - [2.8.1. Name](cec-ioc-adap-g-conn-info.html#name)
    - [2.8.2. Synopsis](cec-ioc-adap-g-conn-info.html#synopsis)
    - [2.8.3. Arguments](cec-ioc-adap-g-conn-info.html#arguments)
    - [2.8.4. Description](cec-ioc-adap-g-conn-info.html#description)
  + [2.9. ioctl CEC\_DQEVENT](cec-ioc-dqevent.html)
    - [2.9.1. Name](cec-ioc-dqevent.html#name)
    - [2.9.2. Synopsis](cec-ioc-dqevent.html#synopsis)
    - [2.9.3. Arguments](cec-ioc-dqevent.html#arguments)
    - [2.9.4. Description](cec-ioc-dqevent.html#description)
    - [2.9.5. Return Value](cec-ioc-dqevent.html#return-value)
  + [2.10. ioctls CEC\_G\_MODE and CEC\_S\_MODE](cec-ioc-g-mode.html)
    - [2.10.1. Synopsis](cec-ioc-g-mode.html#synopsis)
    - [2.10.2. Arguments](cec-ioc-g-mode.html#arguments)
    - [2.10.3. Description](cec-ioc-g-mode.html#description)
    - [2.10.4. Return Value](cec-ioc-g-mode.html#return-value)
  + [2.11. ioctls CEC\_RECEIVE and CEC\_TRANSMIT](cec-ioc-receive.html)
    - [2.11.1. Name](cec-ioc-receive.html#name)
    - [2.11.2. Synopsis](cec-ioc-receive.html#synopsis)
    - [2.11.3. Arguments](cec-ioc-receive.html#arguments)
    - [2.11.4. Description](cec-ioc-receive.html#description)
    - [2.11.5. Return Value](cec-ioc-receive.html#return-value)
* [3. CEC Pin Framework Error Injection](cec-pin-error-inj.html)
  + [3.1. Basic Syntax](cec-pin-error-inj.html#basic-syntax)
  + [3.2. Clear Error Injections](cec-pin-error-inj.html#clear-error-injections)
  + [3.3. Receive Messages](cec-pin-error-inj.html#receive-messages)
  + [3.4. Transmit Messages](cec-pin-error-inj.html#transmit-messages)
  + [3.5. Custom Pulses](cec-pin-error-inj.html#custom-pulses)
  + [3.6. Glitch Pulses](cec-pin-error-inj.html#glitch-pulses)
* [4. CEC uAPI Symbols](cec-header.html)
  + [4.1. IOCTL Commands](cec-header.html#ioctl-commands)
  + [4.2. Macros and Definitions](cec-header.html#macros-and-definitions)
  + [4.3. Structures](cec-header.html#structures)

## Revision and Copyright

Authors:

* Verkuil, Hans <[hverkuil@kernel.org](mailto:hverkuil%40kernel.org)>

> * Initial version.

**Copyright** © 2016 : Hans Verkuil

## Revision History

revision:
:   1.0.0 / 2016-03-17 (*hv*)

Initial revision
