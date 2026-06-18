# 6.1.1.4.frontend events

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dvb-frontend-event.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

type dvb\_frontend\_event

# 6.1.1.4. frontend events

```
struct dvb_frontend_event {
    fe_status_t status;
    struct dvb_frontend_parameters parameters;
};
```
