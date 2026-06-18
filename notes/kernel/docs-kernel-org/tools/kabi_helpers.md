# Ancillary classes

> 출처(원문): https://docs.kernel.org/tools/kabi_helpers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ancillary classes

Helper classes for ABI parser

*class* lib.python.abi.helpers.AbiDebug
:   Bases: `object`

    Debug levels

    WHAT\_PARSING *= 1*
    :   Enable debug parsing logic.

    WHAT\_OPEN *= 2*
    :   Enable debug messages on file open.

    DUMP\_ABI\_STRUCTS *= 4*
    :   Enable debug for ABI parse data.

    UNDEFINED *= 8*
    :   Enable extra undefined symbol data.

    REGEX *= 16*
    :   Enable debug for what to regex conversion.

    SUBGROUP\_MAP *= 32*
    :   Enable debug for symbol regex subgroups

    SUBGROUP\_DICT *= 64*
    :   Enable debug for sysfs graph tree variable.

    SUBGROUP\_SIZE *= 128*
    :   Enable debug of search groups.

    GRAPH *= 256*
    :   Display ref tree graph for undefined symbols.

lib.python.abi.helpers.DEBUG\_HELP *= "\n1   - enable debug parsing logic\n2   - enable debug messages on file open\n4   - enable debug for ABI parse data\n8   - enable extra debug information to identify troubles\n      with ABI symbols found at the local machine that\n      weren't found on ABI documentation (used only for\n      undefined subcommand)\n16  - enable debug for what to regex conversion\n32  - enable debug for symbol regex subgroups\n64  - enable debug for sysfs graph tree variable\n128 - enable debug of search groups\n256 - enable displaying refrence tree graphs for undefined symbols.\n"*
:   Helper messages for each debug variable
