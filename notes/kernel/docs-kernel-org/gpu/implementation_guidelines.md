# Misc DRM driver uAPI- and feature implementation guidelines

> 출처(원문): https://docs.kernel.org/gpu/implementation_guidelines.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Misc DRM driver uAPI- and feature implementation guidelines

* [Asynchronous VM\_BIND](drm-vm-bind-async.html)
  + [Nomenclature:](drm-vm-bind-async.html#nomenclature)
  + [Synchronous / Asynchronous VM\_BIND operation](drm-vm-bind-async.html#synchronous-asynchronous-vm-bind-operation)
    - [Synchronous VM\_BIND](drm-vm-bind-async.html#synchronous-vm-bind)
    - [Asynchronous VM\_BIND](drm-vm-bind-async.html#id1)
  + [Multi-operation VM\_BIND IOCTL error handling and interrupts](drm-vm-bind-async.html#multi-operation-vm-bind-ioctl-error-handling-and-interrupts)
  + [Example: The Xe VM\_BIND uAPI](drm-vm-bind-async.html#example-the-xe-vm-bind-uapi)
* [VM\_BIND locking](drm-vm-bind-locking.html)
  + [The DRM GPUVM set of helpers](drm-vm-bind-locking.html#the-drm-gpuvm-set-of-helpers)
  + [Nomenclature](drm-vm-bind-locking.html#nomenclature)
  + [Locks and locking order](drm-vm-bind-locking.html#locks-and-locking-order)
  + [Protection and lifetime of gpu\_vm\_bos and gpu\_vmas](drm-vm-bind-locking.html#protection-and-lifetime-of-gpu-vm-bos-and-gpu-vmas)
  + [Revalidation and eviction of local objects](drm-vm-bind-locking.html#revalidation-and-eviction-of-local-objects)
    - [Revalidation](drm-vm-bind-locking.html#revalidation)
    - [Eviction](drm-vm-bind-locking.html#eviction)
  + [Locking with external buffer objects](drm-vm-bind-locking.html#locking-with-external-buffer-objects)
  + [Accessing the gpu\_vm’s lists without the dma\_resv lock held](drm-vm-bind-locking.html#accessing-the-gpu-vm-s-lists-without-the-dma-resv-lock-held)
  + [userptr gpu\_vmas](drm-vm-bind-locking.html#userptr-gpu-vmas)
    - [Efficient userptr gpu\_vma exec\_function iteration](drm-vm-bind-locking.html#efficient-userptr-gpu-vma-exec-function-iteration)
  + [Locking at bind and unbind time](drm-vm-bind-locking.html#locking-at-bind-and-unbind-time)
  + [Locking for recoverable page-fault page-table updates](drm-vm-bind-locking.html#locking-for-recoverable-page-fault-page-table-updates)
