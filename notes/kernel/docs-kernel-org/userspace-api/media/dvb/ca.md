# 4.Digital TV CA Device

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4. Digital TV CA Device

The Digital TV CA device controls the conditional access hardware. It
can be accessed through `/dev/dvb/adapter?/ca?`. Data types and ioctl
definitions can be accessed by including `linux/dvb/ca.h` in your
application.

Note

There are three ioctls at this API that aren’t documented:
[CA\_GET\_MSG](ca-get-msg.html#ca-get-msg), [CA\_SEND\_MSG](ca-send-msg.html#ca-send-msg) and [CA\_SET\_DESCR](ca-set-descr.html#ca-set-descr).
Documentation for them are welcome.

* [4.1. CA Data Types](ca_data_types.html)
* [4.2. CA Function Calls](ca_function_calls.html)
* [4.3. The High level CI API](ca_high_level.html)
