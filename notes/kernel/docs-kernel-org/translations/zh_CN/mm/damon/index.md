# DAMON:数据访问监视器

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/damon/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAMON:数据访问监视器

DAMON是Linux内核的一个数据访问监控框架子系统。DAMON的核心机制使其成为
（该核心机制详见([设计](design.html))）

> * *准确度* （监测输出对DRAM级别的内存管理足够有用；但可能不适合CPU Cache级别），
> * *轻量级* （监控开销低到可以在线应用），以及
> * *可扩展* （无论目标工作负载的大小，开销的上限值都在恒定范围内）。

因此，利用这个框架，内核的内存管理机制可以做出高级决策。会导致高数据访问监控开销的实
验性内存管理优化工作可以再次进行。同时，在用户空间，有一些特殊工作负载的用户可以编写
个性化的应用程序，以便更好地了解和优化他们的工作负载和系统。

* [常见问题](faq.html)
  + [DAMON是否只支持虚拟内存？](faq.html#damon)
  + [我可以简单地监测页面的粒度吗？](faq.html#id2)
* [设计](design.html)
  + [可配置的层](design.html#id2)
  + [特定地址空间基元的参考实现](design.html#id3)
  + [独立于地址空间的核心机制](design.html#id4)
* [API参考](api.html)
  + [结构体](api.html#id1)
  + [函数](api.html#id2)
