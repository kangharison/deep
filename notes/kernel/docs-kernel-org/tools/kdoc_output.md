# Kernel-doc output stage

> 출처(원문): https://docs.kernel.org/tools/kdoc_output.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel-doc output stage

## Output handler for man pages and ReST

Classes to implement output filters to print kernel-doc documentation.

The implementation uses a virtual base class `OutputFormat`. It
contains dispatches to virtual methods, and some code to filter
out output messages.

The actual implementation is done on one separate class per each type
of output, e.g. `RestFormat` and `ManFormat` classes.

Currently, there are output classes for ReST and man/troff.

*class* lib.python.kdoc.kdoc\_output.ManFormat(*modulename=None*, *section='9'*, *manual='Kernel API Manual'*)
:   Bases: [`OutputFormat`](#lib.python.kdoc.kdoc_output.OutputFormat "lib.python.kdoc.kdoc_output.OutputFormat")

    Consts and functions used by man pages output.

    This class has one mandatory parameter and some optional ones, which
    are needed to define the title header contents:

    `modulename`
    :   Defines the module name to be used at the troff `.TH` output.

        This argument is optional. If not specified, it will be filled
        with the directory which contains the documented file.

    `section`
    :   Usually a numeric value from 0 to 9, but man pages also accept
        some strings like “p”.

        Defauls to `9`

    `manual`
    :   Defaults to `Kernel API Manual`.

    The above controls the output of teh corresponding fields on troff
    title headers, which will be filled like this:

    ```
    .TH "{name}" {section} "{date}" "{modulename}" "{manual}"
    ```

    where `` name` `` will match the API symbol name, and `date` will be
    either the date where the Kernel was compiled or the current date

    arg\_name(*args*, *name*)
    :   Return the name that will be used for the man page.

        As we may have the same name on different namespaces,
        prepend the data type for all types except functions and typedefs.

        The doc section is special: it uses the modulename.

    blankline *= ''*
    :   Blank line character.

    code\_block(*lines*, *start*)
    :   Ensure that code blocks won’t be messed up at the output.

        By default, troff join lines at the same paragraph. Disable it,
        on code blocks.

    date\_formats *= ['%a %b %d %H:%M:%S %Z %Y', '%a %b %d %H:%M:%S %Y', '%Y-%m-%d', '%b %d %Y', '%B %d %Y', '%m %d %Y']*
    :   Allowed timestamp formats.

    emit\_table(*colspec\_row*, *rows*)

    emit\_th(*name*, *args*)
    :   Emit a title header line.

    grid\_table(*lines*, *start*)
    :   Ancillary function to help handling a grid table inside the text.

    highlights *= ((KernRe("\b``([^\`]+)``\b" ), '\\1'), (KernRe("\%([-\_\*\w]+)" ), '\\1'), (KernRe("(\w+)\(\)" ), '\\\\fB\\1\\\\fP'), (KernRe("\&(enum\s\*([\_\w]+))" ), '\\\\fI\\1\\\\fP'), (KernRe("\&(struct\s\*([\_\w]+))" ), '\\\\fI\\1\\\\fP'), (KernRe("\&(typedef\s\*([\_\w]+))" ), '\\\\fI\\1\\\\fP'), (KernRe("\&(union\s\*([\_\w]+))" ), '\\\\fI\\1\\\\fP'), (KernRe("@(\w\*((\.\w+)|(->\w+))\*(\.\.\.)?)" ), '\\\\fI\\1\\\\fP'), (KernRe("([\!~\\*]?)\@(\w\*((\.\w+)|(->\w+))\*(\.\.\.)?)" ), '\\\\fI\\1\\2\\\\fP'), (KernRe("\&([\_\w]+)(\.|->)([\_\w]+)" ), '\\\\fI\\1\\2\\3\\\\fP'), (KernRe("\&([\_\w]+)" ), '\\\\fI\\1\\\\fP'))*
    :   Highlights to be used in ReST format.

    modulename(*args*)

    msg(*fname*, *name*, *args*)
    :   Handles a single entry from kernel-doc parser.

        Add a tail at the end of man pages output.

    out\_doc(*fname*, *name*, *args*)
    :   Outputs a DOC block.

    out\_enum(*fname*, *name*, *args*)
    :   Outputs an enum.

    out\_function(*fname*, *name*, *args*)
    :   Outputs a function.

    out\_struct(*fname*, *name*, *args*)
    :   Outputs a struct.

    out\_tail(*fname*, *name*, *args*)
    :   Adds a tail for all man pages.

    out\_typedef(*fname*, *name*, *args*)
    :   Outputs a typedef.

    out\_var(*fname*, *name*, *args*)
    :   Outputs a variable.

    output\_highlight(*block*)
    :   Outputs a C symbol that may require being highlighted with
        self.highlights variable using troff syntax.

    set\_symbols(*symbols*)
    :   Get a list of all symbols from kernel\_doc.

        Man pages will uses it to add a SEE ALSO section with other
        symbols at the same file.

    simple\_table(*lines*, *start*)
    :   Ancillary function to help handling a simple table inside the text.

*class* lib.python.kdoc.kdoc\_output.OutputFormat
:   Bases: `object`

    Base class for OutputFormat. If used as-is, it means that only
    warnings will be displayed.

    OUTPUT\_ALL *= 0*
    :   Output all symbols and doc sections.

    OUTPUT\_EXPORTED *= 2*
    :   Output exported symbols.

    OUTPUT\_INCLUDE *= 1*
    :   Output only specified symbols.

    OUTPUT\_INTERNAL *= 3*
    :   Output non-exported symbols.

    blankline *= ''*
    :   Blank line character.

    check\_declaration(*dtype*, *name*, *args*)
    :   Checks if a declaration should be output or not based on the
        filtering criteria.

    check\_doc(*name*, *args*)
    :   Check if DOC should be output.

    highlight\_block(*block*)
    :   Apply the RST highlights to a sub-block of text.

    highlights *= []*
    :   Highlights to be used in ReST format.

    msg(*fname*, *name*, *args*)
    :   Handles a single entry from kernel-doc parser.

    out\_doc(*fname*, *name*, *args*)
    :   Outputs a DOC block.

    out\_enum(*fname*, *name*, *args*)
    :   Outputs an enum.

    out\_function(*fname*, *name*, *args*)
    :   Outputs a function.

    out\_struct(*fname*, *name*, *args*)
    :   Outputs a struct.

    out\_typedef(*fname*, *name*, *args*)
    :   Outputs a typedef.

    out\_var(*fname*, *name*, *args*)
    :   Outputs a variable.

    out\_warnings(*args*)
    :   Output warnings for identifiers that will be displayed.

    output\_symbols(*fname*, *symbols*)
    :   Handles a set of KdocItem symbols.

    set\_config(*config*)
    :   Setup global config variables used by both parser and output.

    set\_filter(*export*, *internal*, *symbol*, *nosymbol*, *function\_table*, *enable\_lineno*, *no\_doc\_sections*)
    :   Initialize filter variables according to the requested mode.

        Only one choice is valid between export, internal and symbol.

        The nosymbol filter can be used on all modes.

    set\_symbols(*symbols*)
    :   Get a list of all symbols from kernel\_doc.

*class* lib.python.kdoc.kdoc\_output.RestFormat
:   Bases: [`OutputFormat`](#lib.python.kdoc.kdoc_output.OutputFormat "lib.python.kdoc.kdoc_output.OutputFormat")

    Consts and functions used by ReST output.

    blankline *= '\n'*
    :   Blank line character.

    highlights *= [(KernRe("\b``([^\`]+)``\b" ), '``\\1``'), (KernRe("\%([-\_\*\w]+)" ), '``\\1``'), (KernRe("\&([\_\w]+)(\.|->)([\_\w]+)\(\)" ), ':c:type:`\\1\\2\\3\\\\(\\\\) <\\1>`'), (KernRe("\&([\_\w]+)(\.|->)([\_\w]+)" ), ':c:type:`\\1\\2\\3 <\\1>`'), (KernRe("\@(\w+)\(\)" ), '\*\*\\1\\\\(\\\\)\*\*'), (KernRe("\@(\w+->\S+)\(\)" ), '\*\*\\1\\\\(\\\\)\*\*'), (KernRe("(\w+)\(\)" ), '\\1()'), (KernRe("\&(enum\s\*([\_\w]+))" ), ':c:type:`\\1 <\\2>`'), (KernRe("\&(struct\s\*([\_\w]+))" ), ':c:type:`\\1 <\\2>`'), (KernRe("\&(typedef\s\*([\_\w]+))" ), ':c:type:`\\1 <\\2>`'), (KernRe("\&(union\s\*([\_\w]+))" ), ':c:type:`\\1 <\\2>`'), (KernRe("\&([\_\w]+)" ), ':c:type:`\\1`'), (KernRe("([\!~\\*]?)\@(\w\*((\.\w+)|(->\w+))\*(\.\.\.)?)" ), '\*\*\\1\\2\*\*')]*
    :   Highlights to be used in ReST format

    out\_doc(*fname*, *name*, *args*)
    :   Outputs a DOC block.

    out\_enum(*fname*, *name*, *args*)
    :   Outputs an enum.

    out\_function(*fname*, *name*, *args*)
    :   Outputs a function.

    out\_section(*args*, *out\_docblock=False*)
    :   Outputs a block section.

        This could use some work; it’s used to output the DOC: sections, and
        starts by putting out the name of the doc section itself, but that
        tends to duplicate a header already in the template file.

    out\_struct(*fname*, *name*, *args*)
    :   Outputs a struct.

    out\_typedef(*fname*, *name*, *args*)
    :   Outputs a typedef.

    out\_var(*fname*, *name*, *args*)
    :   Outputs a variable.

    output\_highlight(*args*)
    :   Outputs a C symbol that may require being converted to ReST using
        the self.highlights variable.

    print\_lineno(*ln*)
    :   Outputs a line number.

    sphinx\_cblock *= KernRe("^\.\.\ +code-block::" )*
    :   Sphinx code block regex.

    sphinx\_literal *= KernRe("^[^.].\*::$" )*
    :   Sphinx literal block regex.
