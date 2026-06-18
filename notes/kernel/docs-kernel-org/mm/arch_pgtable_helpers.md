# Architecture Page Table Helpers

> 출처(원문): https://docs.kernel.org/mm/arch_pgtable_helpers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Architecture Page Table Helpers

Generic MM expects architectures (with MMU) to provide helpers to create, access
and modify page table entries at various level for different memory functions.
These page table helpers need to conform to a common semantics across platforms.
Following tables describe the expected semantics which can also be tested during
boot via CONFIG\_DEBUG\_VM\_PGTABLE option. All future changes in here or the debug
test need to be in sync.

## PTE Page Table Helpers

|  |  |
| --- | --- |
| pte\_same | Tests whether both PTE entries are the same |
| pte\_present | Tests a valid mapped PTE |
| pte\_young | Tests a young PTE |
| pte\_dirty | Tests a dirty PTE |
| pte\_write | Tests a writable PTE |
| pte\_special | Tests a special PTE |
| pte\_protnone | Tests a PROT\_NONE PTE |
| pte\_soft\_dirty | Tests a soft dirty PTE |
| pte\_swp\_soft\_dirty | Tests a soft dirty swapped PTE |
| pte\_mkyoung | Creates a young PTE |
| pte\_mkold | Creates an old PTE |
| pte\_mkdirty | Creates a dirty PTE |
| pte\_mkclean | Creates a clean PTE |
| pte\_mkwrite | Creates a writable PTE of the type specified by the VMA. |
| pte\_mkwrite\_novma | Creates a writable PTE, of the conventional type of writable. |
| pte\_wrprotect | Creates a write protected PTE |
| pte\_mkspecial | Creates a special PTE |
| pte\_mksoft\_dirty | Creates a soft dirty PTE |
| pte\_clear\_soft\_dirty | Clears a soft dirty PTE |
| pte\_swp\_mksoft\_dirty | Creates a soft dirty swapped PTE |
| pte\_swp\_clear\_soft\_dirty | Clears a soft dirty swapped PTE |
| pte\_mknotpresent | Invalidates a mapped PTE |
| ptep\_clear | Clears a PTE |
| ptep\_get\_and\_clear | Clears and returns PTE |
| ptep\_get\_and\_clear\_full | Clears and returns PTE (batched PTE unmap) |
| ptep\_test\_and\_clear\_young | Clears young from a PTE |
| ptep\_set\_wrprotect | Converts into a write protected PTE |
| ptep\_set\_access\_flags | Converts into a more permissive PTE |

## PMD Page Table Helpers

|  |  |
| --- | --- |
| pmd\_same | Tests whether both PMD entries are the same |
| pmd\_bad | Tests a non-table mapped PMD |
| pmd\_leaf | Tests a leaf mapped PMD |
| pmd\_trans\_huge | Tests a Transparent Huge Page (THP) at PMD |
| pmd\_present | Tests whether `pmd_page()` points to valid memory |
| pmd\_young | Tests a young PMD |
| pmd\_dirty | Tests a dirty PMD |
| pmd\_write | Tests a writable PMD |
| pmd\_special | Tests a special PMD |
| pmd\_protnone | Tests a PROT\_NONE PMD |
| pmd\_soft\_dirty | Tests a soft dirty PMD |
| pmd\_swp\_soft\_dirty | Tests a soft dirty swapped PMD |
| pmd\_mkyoung | Creates a young PMD |
| pmd\_mkold | Creates an old PMD |
| pmd\_mkdirty | Creates a dirty PMD |
| pmd\_mkclean | Creates a clean PMD |
| pmd\_mkwrite | Creates a writable PMD of the type specified by the VMA. |
| pmd\_mkwrite\_novma | Creates a writable PMD, of the conventional type of writable. |
| pmd\_wrprotect | Creates a write protected PMD |
| pmd\_mkspecial | Creates a special PMD |
| pmd\_mksoft\_dirty | Creates a soft dirty PMD |
| pmd\_clear\_soft\_dirty | Clears a soft dirty PMD |
| pmd\_swp\_mksoft\_dirty | Creates a soft dirty swapped PMD |
| pmd\_swp\_clear\_soft\_dirty | Clears a soft dirty swapped PMD |
| pmd\_mkinvalid | Invalidates a present PMD; do not call for non-present PMD [1] |
| pmd\_set\_huge | Creates a PMD huge mapping |
| pmd\_clear\_huge | Clears a PMD huge mapping |
| pmdp\_get\_and\_clear | Clears a PMD |
| pmdp\_get\_and\_clear\_full | Clears a PMD |
| pmdp\_test\_and\_clear\_young | Clears young from a PMD |
| pmdp\_set\_wrprotect | Converts into a write protected PMD |
| pmdp\_set\_access\_flags | Converts into a more permissive PMD |

## PUD Page Table Helpers

|  |  |
| --- | --- |
| pud\_same | Tests whether both PUD entries are the same |
| pud\_bad | Tests a non-table mapped PUD |
| pud\_leaf | Tests a leaf mapped PUD |
| pud\_trans\_huge | Tests a Transparent Huge Page (THP) at PUD |
| pud\_present | Tests a valid mapped PUD |
| pud\_young | Tests a young PUD |
| pud\_dirty | Tests a dirty PUD |
| pud\_write | Tests a writable PUD |
| pud\_mkyoung | Creates a young PUD |
| pud\_mkold | Creates an old PUD |
| pud\_mkdirty | Creates a dirty PUD |
| pud\_mkclean | Creates a clean PUD |
| pud\_mkwrite | Creates a writable PUD |
| pud\_wrprotect | Creates a write protected PUD |
| pud\_mkinvalid | Invalidates a present PUD; do not call for non-present PUD [1] |
| pud\_set\_huge | Creates a PUD huge mapping |
| pud\_clear\_huge | Clears a PUD huge mapping |
| pudp\_get\_and\_clear | Clears a PUD |
| pudp\_get\_and\_clear\_full | Clears a PUD |
| pudp\_test\_and\_clear\_young | Clears young from a PUD |
| pudp\_set\_wrprotect | Converts into a write protected PUD |
| pudp\_set\_access\_flags | Converts into a more permissive PUD |

## HugeTLB Page Table Helpers

|  |  |
| --- | --- |
| pte\_huge | Tests a HugeTLB |
| arch\_make\_huge\_pte | Creates a HugeTLB |
| huge\_pte\_dirty | Tests a dirty HugeTLB |
| huge\_pte\_write | Tests a writable HugeTLB |
| huge\_pte\_mkdirty | Creates a dirty HugeTLB |
| huge\_pte\_mkwrite | Creates a writable HugeTLB |
| huge\_pte\_wrprotect | Creates a write protected HugeTLB |
| huge\_ptep\_get\_and\_clear | Clears a HugeTLB |
| huge\_ptep\_set\_wrprotect | Converts into a write protected HugeTLB |
| huge\_ptep\_set\_access\_flags | Converts into a more permissive HugeTLB | |

## SWAP Page Table Helpers

|  |  |  |
| --- | --- | --- |
| \_\_pte\_to\_swp\_entry | Creates a swp\_entry\_t (arch) from a swap PTE | |
| \_\_swp\_entry\_to\_pte | Creates a swap PTE from a swp\_entry\_t (arch) | |
| \_\_pmd\_to\_swp\_entry | Creates a swp\_entry\_t (arch) from a swap PMD | |
| \_\_swp\_entry\_to\_pmd | Creates a swap PMD from a swp\_entry\_t (arch) | |
| is\_migration\_entry | Tests a migration (read or write) swapped entry | | |
| is\_writable\_migration\_entry | | Tests a write migration swapped entry |
| make\_readable\_migration\_entry | | Creates a read migration swapped entry |
| make\_writable\_migration\_entry | | Creates a write migration swapped entry |

[1] <https://lore.kernel.org/linux-mm/20181017020930.GN30832@redhat.com/>
