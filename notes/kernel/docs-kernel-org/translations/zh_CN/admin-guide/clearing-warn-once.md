# 清除 WARN_ONCE

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/clearing-warn-once.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 清除 WARN\_ONCE

WARN\_ONCE / WARN\_ON\_ONCE / printk\_once 仅仅打印一次消息.

echo 1 > /sys/kernel/debug/clear\_warn\_once

可以清除这种状态并且再次允许打印一次告警信息，这对于运行测试集后重现问题
很有用。
