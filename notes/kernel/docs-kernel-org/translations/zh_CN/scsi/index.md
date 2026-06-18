# SCSI子系统

> 출처(원문): https://docs.kernel.org/translations/zh_CN/scsi/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [SCSI Subsystem](../../../scsi/index.html)

翻译:
:   郝栋栋 doubled <[doubled@leap-io-kernel.com](mailto:doubled%40leap-io-kernel.com)>

校译:

# SCSI子系统

## 简介

* [SCSI子系统文档](scsi.html)

## SCSI驱动接口

* [SCSI中间层 — 底层驱动接口](scsi_mid_low_api.html)
* [SCSI 中间层错误处理](scsi_eh.html)

## SCSI驱动参数

* [SCSI内核参数](scsi-parameters.html)
* [链路电源管理策略](link_power_management_policy.html)

## SCSI主机适配器驱动

* [SAS 层](libsas.html)
* [Linux SCSI磁盘驱动（sd）参数](sd-parameters.html)
* [Western Digital WD7193, WD7197 和 WD7296 SCSI 卡驱动](wd719x.html)

Todolist:

* 53c700
* aacraid
* advansys
* aha152x
* aic79xx
* aic7xxx
* arcmsr\_spec
* bfa
* bnx2fc
* BusLogic
* cxgb3i
* dc395x
* dpti
* FlashPoint
* g\_NCR5380
* hpsa
* hptiop
* lpfc
* megaraid
* ncr53c8xx
* NinjaSCSI
* ppa
* qlogicfas
* scsi-changer
* scsi\_fc\_transport
* scsi-generic
* smartpqi
* st
* sym53c500\_cs
* sym53c8xx\_2
* tcm\_qla2xxx
* ufs
* scsi\_transport\_srp/figures
