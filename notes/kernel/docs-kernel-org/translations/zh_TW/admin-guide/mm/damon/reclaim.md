# 基於DAMON的回收

> 출처(원문): https://docs.kernel.org/translations/zh_TW/admin-guide/mm/damon/reclaim.html
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
:   [DAMON-based Reclamation](../../../../../admin-guide/mm/damon/reclaim.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校譯:

# 基於DAMON的回收

基於DAMON的回收（DAMON\_RECLAIM）是一個靜態的內核模塊，旨在用於輕度內存壓力下的主動和輕
量級的回收。它的目的不是取代基於LRU列表的頁面回收，而是有選擇地用於不同程度的內存壓力和要
求。

## 哪些地方需要主動回收？

在一般的內存超量使用（over-committed systems，虛擬化相關術語）的系統上，主動回收冷頁
有助於節省內存和減少延遲高峯，這些延遲是由直接回收進程或kswapd的CPU消耗引起的，同時只產
生最小的性能下降 [[1]](#id8) [[2]](#id9) 。

基於空閒頁報告 [[3]](#id10) 的內存過度承諾的虛擬化系統就是很好的例子。在這樣的系統中，客戶機
向主機報告他們的空閒內存，而主機則將報告的內存重新分配給其他客戶。因此，系統的內存得到了充
分的利用。然而，客戶可能不那麼節省內存，主要是因爲一些內核子系統和用戶空間應用程序被設計爲
使用盡可能多的內存。然後，客戶機可能只向主機報告少量的內存是空閒的，導致系統的內存利用率下降。
在客戶中運行主動回收可以緩解這個問題。

## 它是如何工作的？

DAMON\_RECLAIM找到在特定時間內沒有被訪問的內存區域並分頁。爲了避免它在分頁操作中消耗過多
的CPU，可以配置一個速度限制。在這個速度限制下，它首先分頁出那些沒有被訪問過的內存區域。系
統管理員還可以配置在什麼情況下這個方案應該自動激活和停用三個內存壓力水位。

## 接口: 模塊參數

要使用這個功能，你首先要確保你的系統運行在一個以 `CONFIG_DAMON_RECLAIM=y` 構建的內
核上。

爲了讓系統管理員啓用或禁用它，併爲給定的系統進行調整，DAMON\_RECLAIM利用了模塊參數。也就
是說，你可以把 `damon_reclaim.<parameter>=<value>` 放在內核啓動命令行上，或者把
適當的值寫入 `/sys/module/damon_reclaim/parameters/<parameter>` 文件。

下面是每個參數的描述。

### enabled

啓用或禁用DAMON\_RECLAIM。

你可以通過把這個參數的值設置爲 `Y` 來啓用DAMON\_RCLAIM，把它設置爲 `N` 可以禁用
DAMON\_RECLAIM。注意，由於基於水位的激活條件，DAMON\_RECLAIM不能進行真正的監測和回收。
這一點請參考下面關於水位參數的描述。

### min\_age

識別冷內存區域的時間閾值，單位是微秒。

如果一個內存區域在這個時間或更長的時間內沒有被訪問，DAMON\_RECLAIM會將該區域識別爲冷的，
並回收它。

默認爲120秒。

### quota\_ms

回收的時間限制，以毫秒爲單位。

DAMON\_RECLAIM 試圖在一個時間窗口（quota\_reset\_interval\_ms）內只使用到這個時間，以
嘗試回收冷頁。這可以用來限制DAMON\_RECLAIM的CPU消耗。如果該值爲零，則該限制被禁用。

默認爲10ms。

### quota\_sz

回收的內存大小限制，單位爲字節。

DAMON\_RECLAIM 收取在一個時間窗口（quota\_reset\_interval\_ms）內試圖回收的內存量，並
使其不超過這個限制。這可以用來限制CPU和IO的消耗。如果該值爲零，則限制被禁用。

默認情況下是128 MiB。

### quota\_reset\_interval\_ms

時間/大小配額收取重置間隔，單位爲毫秒。

時間（quota\_ms）和大小（quota\_sz）的配額的目標重置間隔。也就是說，DAMON\_RECLAIM在
嘗試回收‘不’超過quota\_ms毫秒或quota\_sz字節的內存。

默認爲1秒。

### wmarks\_interval

當DAMON\_RECLAIM被啓用但由於其水位規則而不活躍時，在檢查水位之前的最小等待時間。

### wmarks\_high

高水位的可用內存率（每千字節）。

如果系統的可用內存（以每千字節爲單位）高於這個數值，DAMON\_RECLAIM就會變得不活躍，所以
它什麼也不做，只是定期檢查水位。

### wmarks\_mid

中間水位的可用內存率（每千字節）。

如果系統的空閒內存（以每千字節爲單位）在這個和低水位線之間，DAMON\_RECLAIM就會被激活，
因此開始監測和回收。

### wmarks\_low

低水位的可用內存率（每千字節）。

如果系統的空閒內存（以每千字節爲單位）低於這個數值，DAMON\_RECLAIM就會變得不活躍，所以
它除了定期檢查水位外什麼都不做。在這種情況下，系統會退回到基於LRU列表的頁面粒度回收邏輯。

### sample\_interval

監測的採樣間隔，單位是微秒。

DAMON用於監測冷內存的採樣間隔。更多細節請參考DAMON文檔 ([詳細用法](usage.html)) 。

### aggr\_interval

監測的聚集間隔，單位是微秒。

DAMON對冷內存監測的聚集間隔。更多細節請參考DAMON文檔 ([詳細用法](usage.html))。

### min\_nr\_regions

監測區域的最小數量。

DAMON用於冷內存監測的最小監測區域數。這可以用來設置監測質量的下限。但是，設
置的太高可能會導致監測開銷的增加。更多細節請參考DAMON文檔 ([詳細用法](usage.html)) 。

### max\_nr\_regions

監測區域的最大數量。

DAMON用於冷內存監測的最大監測區域數。這可以用來設置監測開銷的上限值。但是，
設置得太低可能會導致監測質量不好。更多細節請參考DAMON文檔 ([詳細用法](usage.html)) 。

### monitor\_region\_start

目標內存區域的物理地址起點。

DAMON\_RECLAIM將對其進行工作的內存區域的起始物理地址。也就是說，DAMON\_RECLAIM
將在這個區域中找到冷的內存區域並進行回收。默認情況下，該區域使用最大系統內存區。

### monitor\_region\_end

目標內存區域的結束物理地址。

DAMON\_RECLAIM將對其進行工作的內存區域的末端物理地址。也就是說，DAMON\_RECLAIM將
在這個區域內找到冷的內存區域並進行回收。默認情況下，該區域使用最大系統內存區。

### kdamond\_pid

DAMON線程的PID。

如果DAMON\_RECLAIM被啓用，這將成爲工作線程的PID。否則，爲-1。

### nr\_reclaim\_tried\_regions

試圖通過DAMON\_RECLAIM回收的內存區域的數量。

### bytes\_reclaim\_tried\_regions

試圖通過DAMON\_RECLAIM回收的內存區域的總字節數。

### nr\_reclaimed\_regions

通過DAMON\_RECLAIM成功回收的內存區域的數量。

### bytes\_reclaimed\_regions

通過DAMON\_RECLAIM成功回收的內存區域的總字節數。

### nr\_quota\_exceeds

超過時間/空間配額限制的次數。

## 例子

下面的運行示例命令使DAMON\_RECLAIM找到30秒或更長時間沒有訪問的內存區域並“回收”？
爲了避免DAMON\_RECLAIM在分頁操作中消耗過多的CPU時間，回收被限制在每秒1GiB以內。
它還要求DAMON\_RECLAIM在系統的可用內存率超過50%時不做任何事情，但如果它低於40%時
就開始真正的工作。如果DAMON\_RECLAIM沒有取得進展，因此空閒內存率低於20%，它會要求
DAMON\_RECLAIM再次什麼都不做，這樣我們就可以退回到基於LRU列表的頁面粒度回收了:

```
# cd /sys/module/damon_reclaim/parameters
# echo 30000000 > min_age
# echo $((1 * 1024 * 1024 * 1024)) > quota_sz
# echo 1000 > quota_reset_interval_ms
# echo 500 > wmarks_high
# echo 400 > wmarks_mid
# echo 200 > wmarks_low
# echo Y > enabled
```

[[1](#id2)]

<https://research.google/pubs/pub48551/>


[[2](#id3)]

<https://lwn.net/Articles/787611/>


[[3](#id4)]

<https://www.kernel.org/doc/html/latest/mm/free_page_reporting.html>
