# System ABI documentation validation class

> 출처(원문): https://docs.kernel.org/tools/kabi_symbols.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# System ABI documentation validation class

Parse ABI documentation and produce results from it.

*class* lib.python.abi.system\_symbols.SystemSymbols(*abi*, *sysfs='/sys'*, *hints=False*)
:   Bases: `object`

    Stores arguments for the class and initialize class vars.

    check\_file(*refs*, *found*)
    :   Check missing ABI symbols for a given sysfs file.

    check\_undefined\_symbols(*max\_workers=None*, *chunk\_size=50*, *found=None*, *dry\_run=None*)
    :   Seach ABI for sysfs symbols missing documentation.

    get\_fileref(*all\_refs*, *chunk\_size*)
    :   Interactor to group refs into chunks.

    graph\_add\_file(*path*, *link=None*)
    :   add a file path to the sysfs graph stored at self.root.

    print\_graph(*root\_prefix=''*, *root=None*, *level=0*)
    :   Prints a reference tree graph using UTF-8 characters.
