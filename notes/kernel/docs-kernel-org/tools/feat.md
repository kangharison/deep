# Documentation features parser module

> 출처(원문): https://docs.kernel.org/tools/feat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Documentation features parser module

Library to parse the Linux Feature files and produce a ReST book.

*class* lib.python.feat.parse\_features.ParseFeature(*prefix*, *debug=0*, *enable\_fname=False*)
:   Bases: `object`

    Parses Documentation/features, allowing to generate ReST documentation
    from it.

    emit(*msg=''*, *end='\n'*)
    :   Helper function to append a new message for feature output.

    h\_arch *= 'Architecture'*
    :   architecture header string.

    h\_description *= 'Description'*
    :   description header string.

    h\_kconfig *= 'Kconfig'*
    :   Kernel config header string.

    h\_name *= 'Feature'*
    :   feature header string.

    h\_status *= 'Status'*
    :   status header string.

    h\_subsys *= 'Subsystem'*
    :   subsystem header string.

    list\_arch\_features(*arch*, *feat*)
    :   Print a matrix of kernel feature support for the chosen architecture.

    matrix\_lines(*desc\_size*, *max\_size\_status*, *header*)
    :   Helper function to split element tables at the output matrix.

    output\_arch\_table(*arch*, *feat=None*)
    :   Output feature(s) for a given architecture.

    output\_feature(*feat*)
    :   Output a feature on all architectures.

    output\_matrix()
    :   Generates a set of tables, groped by subsystem, containing
        what’s the feature state on each architecture.

    parse()
    :   Parses all arch-support.txt feature files inside self.prefix.

    parse\_error(*fname*, *ln*, *msg*, *data=None*)
    :   Displays an error message, printing file name and line.

    parse\_feat\_file(*fname*)
    :   Parses a single arch-support.txt feature file.

    status\_map *= {'N/A': 2, 'TODO': 1, 'ok': 0}*
    :   Sort order for status. Others will be mapped at the end.
