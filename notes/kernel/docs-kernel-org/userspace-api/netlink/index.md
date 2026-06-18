# Netlink Handbook

> 출처(원문): https://docs.kernel.org/userspace-api/netlink/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Netlink Handbook

Netlink documentation for users.

* [Introduction to Netlink](intro.html)
  + [Opening a socket](intro.html#opening-a-socket)
  + [Generic Netlink](intro.html#generic-netlink)
  + [Advanced topics](intro.html#advanced-topics)
  + [Classic Netlink](intro.html#classic-netlink)
  + [uAPI reference](intro.html#uapi-reference)
* [Using Netlink protocol specifications](intro-specs.html)
  + [Simple CLI](intro-specs.html#simple-cli)
  + [Generating kernel code](intro-specs.html#generating-kernel-code)
  + [YNL lib](intro-specs.html#ynl-lib)
* [Netlink protocol specifications (in YAML)](specs.html)
  + [Compatibility levels](specs.html#compatibility-levels)
  + [Schema structure](specs.html#schema-structure)
  + [genetlink](specs.html#genetlink)
  + [Attribute types](specs.html#attribute-types)
* [Netlink spec C code generation](c-code-gen.html)
  + [Globals](c-code-gen.html#globals)
  + [Definitions](c-code-gen.html#definitions)
  + [Attributes](c-code-gen.html#attributes)
  + [Operations](c-code-gen.html#operations)
  + [Multicast groups](c-code-gen.html#multicast-groups)
  + [Code generation](c-code-gen.html#code-generation)
* [Netlink specification support for legacy Generic Netlink families](genetlink-legacy.html)
  + [Specification](genetlink-legacy.html#specification)
  + [Operations](genetlink-legacy.html#operations)
  + [Other quirks](genetlink-legacy.html#other-quirks)
* [Netlink specification support for raw Netlink families](netlink-raw.html)
  + [Specification](netlink-raw.html#specification)

See also:
:   * [Documentation/core-api/netlink.rst](../../core-api/netlink.html#kernel-netlink)
    * [Documentation/netlink/specs/index.rst](../../netlink/specs/index.html#specs)
