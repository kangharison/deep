# Linux安全模块开发

> 출처(원문): https://docs.kernel.org/translations/zh_CN/security/lsm-development.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Linux Security Module Development](../../../security/lsm-development.html)

翻译:
:   赵硕 Shuo Zhao <[zhaoshuo@cqsoftware.com.cn](mailto:zhaoshuo%40cqsoftware.com.cn)>

# Linux安全模块开发

基于https:[//lore.kernel.org/r/20071026073721.618b4778@laptopd505.fenrus.org](mailto://lore.kernel.org/r/20071026073721.618b4778%40laptopd505.fenrus.org)，
当一种新的LSM的意图（它试图防范什么，以及在哪些情况下人们会期望使用它）在
`Documentation/admin-guide/LSM/` 中适当记录下来后，就会被接受进入内核。
这使得LSM的代码可以很轻松的与其目标进行对比，从而让最终用户和发行版可以更
明智地决定那些LSM适合他们的需求。

有关可用的 LSM 钩子接口的详细文档，请参阅 `security/security.c` 及相关结构。
