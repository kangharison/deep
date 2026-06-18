# Real-time preemption

> 출처(원문): https://docs.kernel.org/core-api/real-time/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Real-time preemption

This documentation is intended for Linux kernel developers and contributors
interested in the inner workings of PREEMPT\_RT. It explains key concepts and
the required changes compared to a non-PREEMPT\_RT configuration.

* [Theory of operation](theory.html)
  + [Preface](theory.html#preface)
  + [Scheduling](theory.html#scheduling)
  + [Sleeping spin locks](theory.html#sleeping-spin-locks)
  + [Priority inheritance](theory.html#priority-inheritance)
  + [Threaded interrupts](theory.html#threaded-interrupts)
  + [Summary](theory.html#summary)
* [How realtime kernels differ](differences.html)
  + [Preface](differences.html#preface)
  + [Locking](differences.html#locking)
  + [Execution context](differences.html#execution-context)
  + [Spin until ready](differences.html#spin-until-ready)
* [Considering hardware](hardware.html)
  + [System memory and cache](hardware.html#system-memory-and-cache)
  + [Hardware buses](hardware.html#hardware-buses)
  + [Virtualization](hardware.html#virtualization)
  + [Networking](hardware.html#networking)
* [Porting an architecture to support PREEMPT\_RT](architecture-porting.html)
  + [Requirements](architecture-porting.html#requirements)
  + [Optional features](architecture-porting.html#optional-features)
