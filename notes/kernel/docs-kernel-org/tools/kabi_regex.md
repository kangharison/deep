# ABI regex search symbol class

> 출처(원문): https://docs.kernel.org/tools/kabi_regex.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ABI regex search symbol class

Convert ABI what into regular expressions

*class* lib.python.abi.abi\_regex.AbiRegex(*\*args*, *\*\*kwargs*)
:   Bases: `AbiParser`

    Extends AbiParser to search ABI nodes with regular expressions.

    There some optimizations here to allow a quick symbol search:
    instead of trying to place all symbols altogether an doing linear
    search which is very time consuming, create a tree with one depth,
    grouping similar symbols altogether.

    Yet, sometimes a full search will be needed, so we have a special branch
    on such group tree where other symbols are placed.

    escape\_symbols *= '([\\x21-\\x29\\x2b-\\x2d\\x3a-\\x40\\x5c\\x60\\x7b-\\x7e])'*
    :   Escape only ASCII visible characters.

    get\_regexes(*what*)
    :   Given an ABI devnode, return a list of all regular expressions that
        may match it, based on the sub-groups created by `regex_append()`.

    leave\_others *= 'others'*
    :   Special group for other nodes.

    parse\_abi(*\*args*, *\*\*kwargs*)
    :   Parse documentation ABI.

    re\_has\_num *= re.compile('\\\\d')*
    :   Regex to check if the symbol name has a number on it.

    re\_symbol\_name *= re.compile('(\\w|\\\\[\\.\\-\\:])+$')*
    :   Symbol name after escape\_chars that are considered a devnode basename.

    re\_whats *= [(re.compile('\\\\'), ''), (re.compile('\\.'), 'ö'), (re.compile('\\[0\\-9\\]\\+'), 'ÿ'), (re.compile('\\[0\\-\\d+\\]'), 'ÿ'), (re.compile('\\[0:\\d+\\]'), 'ÿ'), (re.compile('\\[(\\d+)\\]'), 'ô\\\\d+õ'), (re.compile('\\[(\\d)\\-(\\d)\\]'), 'ô\x01-\x02õ'), (re.compile('[\\{\\<\\[]([\\w\_]+)(?:[,|]+([\\w\_]+)){1,}[\\}\\>\\]]'), '(\\1|\\2)'), (re.compile('([^\\/])\\\*'), '\\1\\\\w÷'), (re.compile('/\\\*/'), '/.\*/'), (re.compile('/\\xf6\\xf6\\xf6'), '/.\*'), (re.compile('\\<[^\\>]+\\>'), '\\\\w÷'), (re.compile('\\{[^\\}]+\\}'), '\\\\w÷'), (re.compile('\\[[^\\]]+\\]'), '\\\\w÷'), (re.compile('XX+'), '\\\\w÷'), (re.compile('([^A-Z])[XYZ]([^A-Z])'), '\\1\\\\w÷\\2'), (re.compile('([^A-Z])[XYZ]$'), '\\1\\\\w÷'), (re.compile('\_[AB]\_'), '\_\\\\w÷\_'), (re.compile('\\xf4'), '['), (re.compile('\\xf5'), ']'), (re.compile('\\s+'), ' '), (re.compile('\\s\*\\=.\*$'), ''), (re.compile('([\\x21-\\x29\\x2b-\\x2d\\x3a-\\x40\\x5c\\x60\\x7b-\\x7e])'), '\\\\\\1'), (re.compile('\\\\\\\\'), '\\\\'), (re.compile('\\\\([\\[\\]\\(\\)\\|])'), '\\1'), (re.compile('(\\d+)\\\\(-\\d+)'), '\\1\\2'), (re.compile('\\xff'), '\\\\d+'), (re.compile('sqrt(.\*)'), 'sqrt(.\*)'), (re.compile('(?:\\.\\\*){2,}'), ''), (re.compile('\\xf6'), '\\.'), (re.compile('\\xf7'), '+')]*

    regex\_append(*what*, *new*)
    :   Get a search group for a subset of regular expressions.

        As ABI may have thousands of symbols, using a for to search all
        regular expressions is at least O(n^2). When there are wildcards,
        the complexity increases substantially, eventually becoming exponential.

        To avoid spending too much time on them, use a logic to split
        them into groups. The smaller the group, the better, as it would
        mean that searches will be confined to a small number of regular
        expressions.

        The conversion to a regex subset is tricky, as we need something
        that can be easily obtained from the sysfs symbol and from the
        regular expression. So, we need to discard nodes that have
        wildcards.

        If it can’t obtain a subgroup, place the regular expression inside
        a special group (self.leave\_others).

    skip\_names *= {'devices', 'hwmon'}*
    :   List of popular group names to be skipped to minimize regex group size
        Use AbiDebug.SUBGROUP\_SIZE to detect those.
