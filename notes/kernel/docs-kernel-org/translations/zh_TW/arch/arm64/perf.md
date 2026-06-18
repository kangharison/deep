# Perf 事件屬性

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/arm64/perf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

此文件的目的是爲讓中文讀者更容易閱讀和理解，而不是作爲一個分支。因此，
如果您對此文件有任何意見或改動，請先嘗試更新原始英文文件。如果要更改或
修正某處翻譯文件，請將意見或補丁發送給維護者（聯繫方式見下）。

Note

如果您發現本文檔與原始文件有任何不同或者有翻譯問題，請聯繫該文件的譯者，
或者發送電子郵件給胡皓文以獲取幫助：<[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>。

Original:
:   [Documentation/arch/arm64/perf.rst](../../../../arch/arm64/perf.html#perf-index)

Translator: Bailu Lin <[bailu.lin@vivo.com](mailto:bailu.lin%40vivo.com)>
:   Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# Perf 事件屬性

作者:
:   Andrew Murray <[andrew.murray@arm.com](mailto:andrew.murray%40arm.com)>

日期:
:   2019-03-06

## exclude\_user

該屬性排除用戶空間。

用戶空間始終運行在 EL0，因此該屬性將排除 EL0。

## exclude\_kernel

該屬性排除內核空間。

打開 VHE 時內核運行在 EL2，不打開 VHE 時內核運行在 EL1。客戶機
內核總是運行在 EL1。

對於宿主機，該屬性排除 EL1 和 VHE 上的 EL2。

對於客戶機，該屬性排除 EL1。請注意客戶機從來不會運行在 EL2。

## exclude\_hv

該屬性排除虛擬機監控器。

對於 VHE 宿主機該屬性將被忽略，此時我們認爲宿主機內核是虛擬機監
控器。

對於 non-VHE 宿主機該屬性將排除 EL2，因爲虛擬機監控器運行在 EL2
的任何代碼主要用於客戶機和宿主機的切換。

對於客戶機該屬性無效。請注意客戶機從來不會運行在 EL2。

## exclude\_host / exclude\_guest

這些屬性分別排除了 KVM 宿主機和客戶機。

KVM 宿主機可能運行在 EL0（用戶空間），EL1（non-VHE 內核）和
EL2（VHE 內核 或 non-VHE 虛擬機監控器）。

KVM 客戶機可能運行在 EL0（用戶空間）和 EL1（內核）。

由於宿主機和客戶機之間重疊的異常級別，我們不能僅僅依靠 PMU 的硬件異
常過濾機制-因此我們必須啓用/禁用對於客戶機進入和退出的計數。而這在
VHE 和 non-VHE 系統上表現不同。

對於 non-VHE 系統的 exclude\_host 屬性排除 EL2 - 在進入和退出客戶
機時，我們會根據 exclude\_host 和 exclude\_guest 屬性在適當的情況下
禁用/啓用該事件。

對於 VHE 系統的 exclude\_guest 屬性排除 EL1，而對其中的 exclude\_host
屬性同時排除 EL0，EL2。在進入和退出客戶機時，我們會適當地根據
exclude\_host 和 exclude\_guest 屬性包括/排除 EL0。

以上聲明也適用於在 not-VHE 客戶機使用這些屬性時，但是請注意客戶機從
來不會運行在 EL2。

## 準確性

在 non-VHE 宿主機上，我們在 EL2 進入/退出宿主機/客戶機的切換時啓用/
關閉計數器 -但是在啓用/禁用計數器和進入/退出客戶機之間存在一段延時。
對於 exclude\_host， 我們可以通過過濾 EL2 消除在客戶機進入/退出邊界
上用於計數客戶機事件的宿主機事件計數器。但是當使用 !exclude\_hv 時，
在客戶機進入/退出有一個小的停電窗口無法捕獲到宿主機的事件。

在 VHE 系統沒有停電窗口。
