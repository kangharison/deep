# Clearing WARN_ONCE

> 출처(원문): https://docs.kernel.org/admin-guide/clearing-warn-once.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Clearing WARN\_ONCE

WARN\_ONCE / WARN\_ON\_ONCE / printk\_once only emit a message once.

echo 1 > /sys/kernel/debug/clear\_warn\_once

clears the state and allows the warnings to print once again.
This can be useful after test suite runs to reproduce problems.
