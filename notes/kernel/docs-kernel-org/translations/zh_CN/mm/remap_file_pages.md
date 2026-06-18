# remap_file_pages()系统调用

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/remap_file_pages.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# remap\_file\_pages()系统调用

`remap_file_pages()`系统调用被用来创建一个非线性映射，也就是说，在这个映射中，
文件的页面被无序映射到内存中。使用`remap_file_pages()`比重复调用mmap(2)的好
处是，前者不需要内核创建额外的VMA（虚拟内存区）数据结构。

支持非线性映射需要在内核虚拟内存子系统中编写大量的non-trivial的代码，包括热
路径。另外，为了使非线性映射工作，内核需要一种方法来区分正常的页表项和带有文件
偏移的项（pte\_file）。内核为达到这个目的在PTE中保留了标志。PTE标志是稀缺资
源，特别是在某些CPU架构上。如果能腾出这个标志用于其他用途就更好了。

幸运的是，在生活中并没有很多`remap_file_pages()`的用户。只知道有一个企业的RDBMS
实现在32位系统上使用这个系统调用来映射比32位虚拟地址空间线性尺寸更大的文件。
由于64位系统的广泛使用，这种使用情况已经不重要了。

syscall被废弃了，现在用一个模拟来代替它。仿真会创建新的VMA，而不是非线性映射。
对于`remap_file_pages()`的少数用户来说，它的工作速度会变慢，但ABI被保留了。

仿真的一个副作用（除了性能之外）是，由于额外的VMA，用户可以更容易达到
vm.max\_map\_count的限制。关于限制的更多细节，请参见DEFAULT\_MAX\_MAP\_COUNT
的注释。
