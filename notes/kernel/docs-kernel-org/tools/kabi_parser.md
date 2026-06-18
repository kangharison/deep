# Kernel ABI documentation parser class

> 출처(원문): https://docs.kernel.org/tools/kabi_parser.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel ABI documentation parser class

Parse ABI documentation and produce results from it.

*class* lib.python.abi.abi\_parser.AbiParser(*directory*, *logger=None*, *enable\_lineno=False*, *show\_warnings=True*, *debug=0*)
:   Bases: `object`

    Main class to parse ABI files.

    TAGS *= '(what|where|date|kernelversion|contact|description|users)'*
    :   Valid tags at Documentation/ABI.

    XREF *= '(?:^|\\s|\\()(\\/(?:sys|config|proc|dev|kvd)\\/[^,.:;\\)\\s]+)(?:[,.:;\\)\\s]|\\Z)'*
    :   ABI elements that will auto-generate cross-references.

    add\_symbol(*what*, *fname*, *ln=None*, *xref=None*)
    :   Create a reference table describing where each ‘what’ is located.

    check\_issues()
    :   Warn about duplicated ABI entries.

    desc\_rst(*desc*)
    :   Enrich ReST output by creating cross-references.

    desc\_txt(*desc*)
    :   Print description as found inside ABI files.

    doc(*output\_in\_txt=False*, *show\_symbols=True*, *show\_file=True*, *filter\_path=None*)
    :   Print ABI at stdout.

    parse\_abi(*root=None*)
    :   Parse documentation ABI.

    parse\_file(*fname*, *path*, *basename*)
    :   Parse a single file.

    parse\_readme(*nametag*, *fname*)
    :   Parse ABI README file.

    search\_symbols(*expr*)
    :   Searches for ABI symbols.

    warn(*fdata*, *msg*, *extra=None*)
    :   Displays a parse error if warning is enabled.

    xref(*fname*)
    :   Converts a Documentation/ABI + basename into a ReST cross-reference.
