# 可信平台模块文档

> 출처(원문): https://docs.kernel.org/translations/zh_CN/security/tpm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Trusted Platform Module documentation](../../../../security/tpm/index.html)

翻译:
:   赵硕 Shuo Zhao <[zhaoshuo@cqsoftware.com.cn](mailto:zhaoshuo%40cqsoftware.com.cn)>

# 可信平台模块文档

* [TPM事件日志](tpm_event_log.html)
  + [介绍](tpm_event_log.html#id1)
  + [UEFI事件日志](tpm_event_log.html#uefi)
  + [参考文献](tpm_event_log.html#id2)
* [TPM安全](tpm-security.html)
  + [介绍](tpm-security.html#id1)
  + [总线上的窥探和篡改攻击](tpm-security.html#id2)
  + [测量（PCR）完整性](tpm-security.html#pcr)
  + [秘密保护](tpm-security.html#id3)
  + [与TPM建立初始信任](tpm-security.html#id4)
  + [信任堆叠](tpm-security.html#id5)
  + [会话属性](tpm-security.html#id6)
  + [保护类型](tpm-security.html#id7)
* [空主密钥认证在用户空间的实现](tpm-security.html#id8)
* [TPM FIFO接口驱动](tpm_tis.html)
* [参考文献](tpm_tis.html#id1)
* [Linux容器的虚拟TPM代理驱动](tpm_vtpm_proxy.html)
  + [介绍](tpm_vtpm_proxy.html#id1)
  + [设计](tpm_vtpm_proxy.html#id2)
  + [UAPI](tpm_vtpm_proxy.html#uapi)
* [Xen的虚拟TPM接口](xen-tpmfront.html)
  + [介绍](xen-tpmfront.html#id1)
  + [设计概述](xen-tpmfront.html#id2)
  + [与Xen的集成](xen-tpmfront.html#xen)
* [固件TPM驱动](tpm_ftpm_tee.html)
  + [介绍](tpm_ftpm_tee.html#id1)
  + [设计](tpm_ftpm_tee.html#id2)
