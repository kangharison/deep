# Subsystem and maintainer tree specific development process notes

> 출처(원문): https://docs.kernel.org/process/maintainer-handbooks.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Subsystem and maintainer tree specific development process notes

The purpose of this document is to provide subsystem specific information
which is supplementary to the general development process handbook
[Documentation/process](development-process.html#development-process-main).

Contents:

* [1. Networking subsystem (netdev)](maintainer-netdev.html)
  + [1.1. tl;dr](maintainer-netdev.html#tl-dr)
  + [1.2. netdev](maintainer-netdev.html#netdev)
  + [1.3. Development cycle](maintainer-netdev.html#development-cycle)
  + [1.4. git trees and patch flow](maintainer-netdev.html#git-trees-and-patch-flow)
  + [1.5. netdev patch review](maintainer-netdev.html#netdev-patch-review)
  + [1.6. Preparing changes](maintainer-netdev.html#preparing-changes)
  + [1.7. Limit patches outstanding on mailing list](maintainer-netdev.html#limit-patches-outstanding-on-mailing-list)
  + [1.8. Testing](maintainer-netdev.html#testing)
  + [1.9. Supported status for drivers](maintainer-netdev.html#supported-status-for-drivers)
  + [1.10. Reviewer guidance](maintainer-netdev.html#reviewer-guidance)
  + [1.11. Testimonials / feedback](maintainer-netdev.html#testimonials-feedback)
* [2. SoC Subsystem](maintainer-soc.html)
  + [2.1. Overview](maintainer-soc.html#overview)
  + [2.2. Maintainers](maintainer-soc.html#maintainers)
  + [2.3. Information for (new) Submaintainers](maintainer-soc.html#information-for-new-submaintainers)
* [3. SoC Platforms with DTS Compliance Requirements](maintainer-soc-clean-dts.html)
  + [3.1. Overview](maintainer-soc-clean-dts.html#overview)
  + [3.2. Strict DTS DT Schema and dtc Compliance](maintainer-soc-clean-dts.html#strict-dts-dt-schema-and-dtc-compliance)
* [4. The tip tree handbook](maintainer-tip.html)
  + [4.1. What is the tip tree?](maintainer-tip.html#what-is-the-tip-tree)
  + [4.2. Patch submission notes](maintainer-tip.html#patch-submission-notes)
  + [4.3. Coding style notes](maintainer-tip.html#coding-style-notes)
  + [4.4. Commit notifications](maintainer-tip.html#commit-notifications)
* [5. KVM x86](maintainer-kvm-x86.html)
  + [5.1. Foreword](maintainer-kvm-x86.html#foreword)
  + [5.2. TL;DR](maintainer-kvm-x86.html#tl-dr)
  + [5.3. Trees](maintainer-kvm-x86.html#trees)
  + [5.4. Development](maintainer-kvm-x86.html#development)
  + [5.5. Testing](maintainer-kvm-x86.html#testing)
  + [5.6. Posting](maintainer-kvm-x86.html#posting)
  + [5.7. Notifications](maintainer-kvm-x86.html#notifications)
  + [5.8. Vulnerabilities](maintainer-kvm-x86.html#vulnerabilities)
