# 程序設計語言

> 출처(원문): https://docs.kernel.org/translations/zh_TW/process/programming-language.html
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
:   [Documentation/process/programming-language.rst](../../../process/programming-language.html#programming-language)

Translator:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>
    Hu Haowen <[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>

# 程序設計語言

內核是用C語言 [c-language](#tw-c-language) 編寫的。更準確地說，內核通常是用 [gcc](#tw-gcc)
在 `-std=gnu11` [gcc-c-dialect-options](#tw-gcc-c-dialect-options) 下編譯的：ISO C11的 GNU 方言

這種方言包含對語言 [gnu-extensions](#tw-gnu-extensions) 的許多擴展，當然，它們許多都在內核中使用。

對於一些體系結構，有一些使用 [clang](#tw-clang) 和 [icc](#tw-icc) 編譯內核
的支持，儘管在編寫此文檔時還沒有完成，仍需要第三方補丁。

## 屬性

在整個內核中使用的一個常見擴展是屬性（attributes） [gcc-attribute-syntax](#tw-gcc-attribute-syntax)
屬性允許將實現定義的語義引入語言實體（如變量、函數或類型），而無需對語言進行
重大的語法更改（例如添加新關鍵字） [n2049](#tw-n2049)

在某些情況下，屬性是可選的（即不支持這些屬性的編譯器仍然應該生成正確的代碼，
即使其速度較慢或執行的編譯時檢查/診斷次數不夠）

內核定義了僞關鍵字（例如， `pure` ），而不是直接使用GNU屬性語法（例如,
`__attribute__((__pure__))` ），以檢測可以使用哪些關鍵字和/或縮短代碼, 具體
請參閱 `include/linux/compiler_attributes.h`

c-language
:   <http://www.open-std.org/jtc1/sc22/wg14/www/standards>

gcc
:   <https://gcc.gnu.org>

clang
:   <https://clang.llvm.org>

icc
:   <https://software.intel.com/en-us/c-compilers>

c-dialect-options
:   <https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html>

gnu-extensions
:   <https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html>

gcc-attribute-syntax
:   <https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html>

n2049
:   <http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2049.pdf>
