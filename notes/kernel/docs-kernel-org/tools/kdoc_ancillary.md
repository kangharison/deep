# Ancillary classes

> 출처(원문): https://docs.kernel.org/tools/kdoc_ancillary.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ancillary classes

## Argparse formatter class

Ancillary argparse HelpFormatter class that works on a similar way as
argparse.RawDescriptionHelpFormatter, e.g. description maintains line
breaks, but it also implement transformations to the help text. The
actual transformations ar given by `enrich_text()`, if the output is tty.

Currently, the follow transformations are done:

> * Positional arguments are shown in upper cases;
> * if output is TTY, `var` and positional arguments are shown prepended
>   by an ANSI SGR code. This is usually translated to bold. On some
>   terminals, like, konsole, this is translated into a colored bold text.

*class* lib.python.kdoc.enrich\_formatter.EnrichFormatter(*\*args*, *\*\*kwargs*)
:   Bases: `HelpFormatter`

    Better format the output, making easier to identify the positional args
    and how they’re used at the \_\_doc\_\_ description.

    enrich\_text(*text*)
    :   Handle ReST markups (currently, only ``text`` markups).

## Regular expression class handler

Regular expression ancillary classes.

Those help caching regular expressions and do matching for kernel-doc.

*class* lib.python.kdoc.kdoc\_re.KernRe(*string*, *cache=True*, *flags=0*)
:   Bases: `object`

    Helper class to simplify regex declaration and usage.

    It calls re.compile for a given pattern. It also allows adding
    regular expressions and define sub at class init time.

    Regular expressions can be cached via an argument, helping to speedup
    searches.

    findall(*string*)
    :   Alias to re.findall.

    finditer(*string*)
    :   Alias to re.finditer.

    group(*num*)
    :   Returns the group results of the last match.

    groups()
    :   Returns the group results of the last match

    match(*string*)
    :   Handles a re.match storing its results.

    search(*string*)
    :   Handles a re.search storing its results.

    split(*string*)
    :   Alias to re.split.

    sub(*sub*, *string*, *count=0*)
    :   Alias to re.sub.

## C tokenizer

Regular expression ancillary classes.

Those help caching regular expressions and do matching for kernel-doc.

Please notice that the code here may rise exceptions to indicate bad
usage inside kdoc to indicate problems at the replace pattern.

Other errors are logged via log instance.

*class* lib.python.kdoc.c\_lex.CMatch(*regex*, *delim='('*)
:   Bases: `object`

    Finding nested delimiters is hard with regular expressions. It is
    even harder on Python with its normal re module, as there are several
    advanced regular expressions that are missing.

    This is the case of this pattern:

    ```
    '\bSTRUCT_GROUP(\(((?:(?>[^)(]+)|(?1))*)\))[^;]*;'
    ```

    which is used to properly match open/close parentheses of the
    string search `STRUCT_GROUP()`,

    Add a class that counts pairs of delimiters, using it to match and
    replace nested expressions.

    The original approach was suggested by:

    > <https://stackoverflow.com/questions/5454322/python-how-to-match-nested-parentheses-with-regex>

    Although I re-implemented it to make it more generic and match 3 types
    of delimiters. The logic checks if delimiters are paired. If not, it
    will ignore the search string.

    search(*source*)
    :   This is similar to re.search:

        It matches a regex that it is followed by a delimiter,
        returning occurrences only if all delimiters are paired.

    sub(*sub\_str*, *source*, *count=0*)
    :   This is similar to re.sub:

        It matches a regex that it is followed by a delimiter,
        replacing occurrences only if all delimiters are paired.

        if the sub argument contains:

        ```
        r''
        ```

        it will work just like re: it places there the matched paired data
        with the delimiter stripped.

        If count is different than zero, it will replace at most count
        items.

*class* lib.python.kdoc.c\_lex.CToken(*kind*, *value=None*, *pos=0*, *brace\_level=0*, *paren\_level=0*, *bracket\_level=0*)
:   Bases: `object`

    Data class to define a C token.

    BACKREF *= 17*
    :   Not a valid C sequence, but used at sub regex patterns.

    BEGIN *= 5*
    :   A begin character: `{` / `[` / `(`.

    CHAR *= 2*
    :   A character, including apostophes.

    COMMENT *= 0*
    :   A standard C or C99 comment, including delimiter.

    CPP *= 7*
    :   A preprocessor macro.

    END *= 6*
    :   A end character: `}` / `]` / `)`.

    ENDSTMT *= 16*
    :   End of an statement (`;`).

    ENUM *= 12*
    :   A `struct` keyword.

    HASH *= 8*
    :   The hash character - useful to handle other macros.

    MISMATCH *= 255*
    :   an error indicator: should never happen in practice.

    NAME *= 14*
    :   A name. Can be an ID or a type.

    NUMBER *= 3*
    :   A number.

    OP *= 9*
    :   A C operator (add, subtract, ...).

    PUNC *= 4*
    :   A puntuation mark: / `,` / `.`.

    SPACE *= 15*
    :   Any space characters, including new lines

    STRING *= 1*
    :   A string, including quotation marks.

    STRUCT *= 10*
    :   A `struct` keyword.

    TYPEDEF *= 13*
    :   A `typedef` keyword.

    UNION *= 11*
    :   An `union` keyword.

    *static* from\_name(*name*)
    :   Convert a string into a CToken `enum value`

    *static* to\_name(*val*)
    :   Convert from an integer value from CToken `enum into` a string

*class* lib.python.kdoc.c\_lex.CTokenArgs(*sub\_str*)
:   Bases: `object`

    Ancillary class to help using backrefs from sub matches.

    If the highest backref contain a “+” at the last element,
    the logic will be greedy, picking all other delims.

    This is needed to parse struct\_group macros with end with `MEMBERS...`.

    groups(*new\_tokenizer*)
    :   Create replacement arguments for backrefs like:

        `\0`, `\1`, `\2`, ... `\{number}`

        It also accepts a `+` character to the highest backref, like
        `\4+`. When used, the backref will be greedy, picking all other
        arguments afterwards.

        The logic is smart enough to only go up to the maximum required
        argument, even if there are more.

        If there is a backref for an argument above the limit, it will
        raise an exception. Please notice that, on C, square brackets
        don’t have any separator on it. Trying to use `\1`..``n`` for
        brackets also raise an exception.

    tokens(*new\_tokenizer*)

*class* lib.python.kdoc.c\_lex.CTokenizer(*source=None*)
:   Bases: `object`

    Scan C statements and definitions and produce tokens.

    When converted to string, it drops comments and handle public/private
    values, respecting depth.

lib.python.kdoc.c\_lex.RE\_CONT *= KernRe("\\\n" )*
:   Handle C continuation lines.

lib.python.kdoc.c\_lex.RE\_SCANNER *= KernRe("(?P<COMMENT>//[^\n]\*|/\\*[\s\S]\*?\\*/)|(?P<STRING>"(?:\\.|[^"\" "\])\*")|(?P<CHAR>'(?:\\.|[^'\\])')|(?P<NUMBER>0[xX][\da-fA-F]" "+[uUlL]\*|0[0-7]+[uUlL]\*|\d+(?:\.\d\*)?(?:[eE][+-]?\d+)?[fFlL]" "\*)|(?P<ENDSTMT>(?:\s+;|;))|(?P<PUNC>[,\.])|(?P<BEGIN>[\[\(\{" "])|(?P<END>[\]\)\}])|(?P<CPP>#\s\*(?:define|include|ifdef|ifn" "def|if|else|elif|endif|undef|pragma)\b)|(?P<HASH>#)|(?P<OP>\" "+\+|\-\-|\->|==|\!=|<=|>=|&&|\|\||<<|>>|\+=|\-=|\\*=|/=|%=|&=" "|\|=|\^=|[=\+\-\\*/%<>&\|\^~!\?\:])|(?P<STRUCT>\bstruct\b)|(?" "P<UNION>\bunion\b)|(?P<ENUM>\benum\b)|(?P<TYPEDEF>\btypedef\" "b)|(?P<NAME>[A-Za-z\_]\w\*)|(?P<SPACE>\s+)|(?P<BACKREF>\\\d+)|" "(?P<MISMATCH>.)" )*
:   tokenizer regex. Will be filled at the first CTokenizer usage.

lib.python.kdoc.c\_lex.RE\_SCANNER\_LIST *= [(0, '//[^\\n]\*|/\\\*[\\s\\S]\*?\\\*/'), (1, '"(?:\\\\.|[^"\\\\])\*"'), (2, "'(?:\\\\.|[^'\\\\])'"), (3, '0[xX][\\da-fA-F]+[uUlL]\*|0[0-7]+[uUlL]\*|\\d+(?:\\.\\d\*)?(?:[eE][+-]?\\d+)?[fFlL]\*'), (16, '(?:\\s+;|;)'), (4, '[,\\.]'), (5, '[\\[\\(\\{]'), (6, '[\\]\\)\\}]'), (7, '#\\s\*(?:define|include|ifdef|ifndef|if|else|elif|endif|undef|pragma)\\b'), (8, '#'), (9, '\\+\\+|\\-\\-|\\->|==|\\!=|<=|>=|&&|\\|\\||<<|>>|\\+=|\\-=|\\\*=|/=|%=|&=|\\|=|\\^=|[=\\+\\-\\\*/%<>&\\|\\^~!\\?\\:]'), (10, '\\bstruct\\b'), (11, '\\bunion\\b'), (12, '\\benum\\b'), (13, '\\btypedef\\b'), (14, '[A-Za-z\_]\\w\*'), (15, '\\s+'), (17, '\\\\\\d+'), (255, '.')]*
:   Regexes to parse C code, transforming it into tokens.

lib.python.kdoc.c\_lex.fill\_re\_scanner(*token\_list*)
:   Ancillary routine to convert RE\_SCANNER\_LIST into a finditer regex

lib.python.kdoc.c\_lex.tokenizer\_set\_log(*logger*, *prefix=''*)
:   Replace the module‑level logger with a LoggerAdapter that
    prepends *prefix* to every message.

## Chinese, Japanese and Korean variable fonts handler

### Detect problematic Noto CJK variable fonts

For `make pdfdocs`, reports of build errors of translations.pdf started
arriving early 2024 [[1]](#id6) [[2]](#id7). It turned out that Fedora and openSUSE
tumbleweed have started deploying variable-font [[3]](#id8) format of “Noto CJK”
fonts [[4]](#id9) [[5]](#id10). For PDF, a LaTeX package named xeCJK is used for CJK
(Chinese, Japanese, Korean) pages. xeCJK requires XeLaTeX/XeTeX, which
does not (and likely never will) understand variable fonts for historical
reasons.

The build error happens even when both of variable- and non-variable-format
fonts are found on the build system. To make matters worse, Fedora enlists
variable “Noto CJK” fonts in the requirements of langpacks-ja, -ko, -zh\_CN,
-zh\_TW, etc. Hence developers who have interest in CJK pages are more
likely to encounter the build errors.

This script is invoked from the error path of “make pdfdocs” and emits
suggestions if variable-font files of “Noto CJK” fonts are in the list of
fonts accessible from XeTeX.

[[1](#id1)]

<https://lore.kernel.org/r/8734tqsrt7.fsf@meer.lwn.net/>


[[2](#id2)]

<https://lore.kernel.org/r/1708585803.600323099@f111.i.mail.ru/>


[[3](#id3)]

<https://en.wikipedia.org/wiki/Variable_font>


[[4](#id4)]

<https://fedoraproject.org/wiki/Changes/Noto_CJK_Variable_Fonts>


[[5](#id5)]

<https://build.opensuse.org/request/show/1157217>

#### Workarounds for building translations.pdf

* Denylist “variable font” Noto CJK fonts.

  + Create $HOME/deny-vf/fontconfig/fonts.conf from template below, with
    tweaks if necessary. Remove leading “”.
  + Path of fontconfig/fonts.conf can be overridden by setting an env
    variable FONTS\_CONF\_DENY\_VF.

    - Template:

      ```
      <?xml version="1.0"?>
      <!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
      <fontconfig>
      <!--
      Ignore variable-font glob (not to break xetex)
      -->
          <selectfont>
              <rejectfont>
                  <!--
                      for Fedora
                  -->
                  <glob>/usr/share/fonts/google-noto-*-cjk-vf-fonts</glob>
                  <!--
                      for openSUSE tumbleweed
                  -->
                  <glob>/usr/share/fonts/truetype/Noto*CJK*-VF.otf</glob>
              </rejectfont>
          </selectfont>
      </fontconfig>
      ```

    The denylisting is activated for “make pdfdocs”.
* For skipping CJK pages in PDF

  + Uninstall texlive-xecjk.
    Denylisting is not needed in this case.
* For printing CJK pages in PDF

  + Need non-variable “Noto CJK” fonts.

    - Fedora

      * google-noto-sans-cjk-fonts
      * google-noto-serif-cjk-fonts
    - openSUSE tumbleweed

      * Non-variable “Noto CJK” fonts are not available as distro packages
        as of April, 2024. Fetch a set of font files from upstream Noto
        CJK Font released at:

        > <https://github.com/notofonts/noto-cjk/tree/main/Sans#super-otc>

        and at:

        > <https://github.com/notofonts/noto-cjk/tree/main/Serif#super-otc>

        then uncompress and deploy them.
      * Remember to update fontconfig cache by running fc-cache.

Caution

Uninstalling “variable font” packages can be dangerous.
They might be depended upon by other packages important for your work.
Denylisting should be less invasive, as it is effective only while
XeLaTeX runs in “make pdfdocs”.

*class* lib.python.kdoc.latex\_fonts.LatexFontChecker(*deny\_vf=None*)
:   Bases: `object`

    Detect problems with CJK variable fonts that affect PDF builds for
    translations.

    check()
    :   Check for problems with CJK fonts.

    description()
    :   Returns module description.

    get\_noto\_cjk\_vf\_fonts()
    :   Get Noto CJK fonts.

## Kernel C file include logic

Parse a source file or header, creating ReStructured Text cross references.

It accepts an optional file to change the default symbol reference or to
suppress symbols from the output.

It is capable of identifying `define`, function, `struct`, `typedef`,
`enum` and `enum` symbols and create cross-references for all of them.
It is also capable of distinguish #define used for specifying a Linux
ioctl.

The optional rules file contains a set of rules like:

```
ignore ioctl VIDIOC_ENUM_FMT
replace ioctl VIDIOC_DQBUF vidioc_qbuf
replace define V4L2_EVENT_MD_FL_HAVE_FRAME_SEQ :c:type:`v4l2_event_motion_det`
```

*class* lib.python.kdoc.parse\_data\_structs.ParseDataStructs(*debug: bool = False*)
:   Bases: `object`

    Creates an enriched version of a Kernel header file with cross-links
    to each C data structure type.

    It is meant to allow having a more comprehensive documentation, where
    uAPI headers will create cross-reference links to the code.

    It is capable of identifying `define`, function, `struct`, `typedef`,
    `enum` and `enum` symbols and create cross-references for all of them.
    It is also capable of distinguish #define used for specifying a Linux
    ioctl.

    By default, it create rules for all symbols and defines, but it also
    allows parsing an exception file. Such file contains a set of rules
    using the syntax below:

    1. Ignore rules:

       ```
       ignore <type> <symbol>`
       ```

    Removes the symbol from reference generation.

    2. Replace rules:

       ```
       replace <type> <old_symbol> <new_reference>
       ```

       Replaces how old\_symbol with a new reference. The new\_reference can be:

       > * A simple symbol name;
       > * A full Sphinx reference.
    3. Namespace rules:

       ```
       namespace <namespace>
       ```

       Sets C namespace to be used during cross-reference generation. Can
       be overridden by replace rules.

    On ignore and replace rules, `<type>` can be:
    :   * `ioctl`: for defines that end with `_IO*`, e.g. ioctl definitions
        * `define`: for other defines
        * `symbol`: for symbols defined within enums;
        * `typedef`: for typedefs;
        * `enum`: for the name of a non-anonymous enum;
        * `struct`: for structs.

    Examples:

    ```
    ignore define __LINUX_MEDIA_H
    ignore ioctl VIDIOC_ENUM_FMT
    replace ioctl VIDIOC_DQBUF vidioc_qbuf
    replace define V4L2_EVENT_MD_FL_HAVE_FRAME_SEQ :c:type:`v4l2_event_motion_det`

    namespace MC
    ```

    DEF\_SYMBOL\_TYPES *= {'define': {'description': 'Macros and Definitions', 'prefix': '\\ ', 'ref\_type': ':ref', 'suffix': '\\ '}, 'enum': {'description': 'Enumerations', 'prefix': '\\ ', 'ref\_type': ':c:type', 'suffix': '\\ '}, 'ioctl': {'description': 'IOCTL Commands', 'prefix': '\\ ', 'ref\_type': ':ref', 'suffix': '\\ '}, 'struct': {'description': 'Structures', 'prefix': '\\ ', 'ref\_type': ':c:type', 'suffix': '\\ '}, 'symbol': {'description': 'Enumeration values', 'prefix': '\\ ', 'ref\_type': ':ref', 'suffix': '\\ '}, 'typedef': {'description': 'Type Definitions', 'prefix': '\\ ', 'ref\_type': ':c:type', 'suffix': '\\ '}}*
    :   Dictionary containing C type identifiers to be transformed.

    RE\_ENUMS *= [re.compile('^\\s\*enum\\s+([\\w\_]+)\\s\*\\{'), re.compile('^\\s\*enum\\s+([\\w\_]+)\\s\*$'), re.compile('^\\s\*typedef\\s\*enum\\s+([\\w\_]+)\\s\*\\{'), re.compile('^\\s\*typedef\\s\*enum\\s+([\\w\_]+)\\s\*$')]*
    :   Parser regex with multiple ways to capture enums.

    RE\_STRUCTS *= [re.compile('^\\s\*struct\\s+([\_\\w][\\w\\d\_]+)\\s\*\\{'), re.compile('^\\s\*struct\\s+([\_\\w][\\w\\d\_]+)$'), re.compile('^\\s\*typedef\\s\*struct\\s+([\_\\w][\\w\\d\_]+)\\s\*\\{'), re.compile('^\\s\*typedef\\s\*struct\\s+([\_\\w][\\w\\d\_]+)$')]*
    :   Parser regex with multiple ways to capture structs.

    apply\_exceptions()
    :   Process exceptions file with rules to ignore or replace references.

    debug\_print()
    :   Print debug information containing the replacement rules per symbol.
        To make easier to check, group them per type.

    gen\_output()
    :   Write the formatted output to a file.

    gen\_toc()
    :   Create a list of symbols to be part of a TOC contents table.

    parse\_file(*file\_in: str*, *exceptions: Optional[str] = None*)
    :   Read a C source file and get identifiers.

    read\_exceptions(*fname: str*)
    :   Read an optional exceptions file, used to override defaults.

    store\_line(*line*)
    :   Store a line at self.data, properly indented.

    store\_type(*ln*, *symbol\_type: str*, *symbol: str*, *ref\_name: Optional[str] = None*, *replace\_underscores: bool = True*)
    :   Store a new symbol at self.symbols under symbol\_type.

        By default, underscores are replaced by `-`.

    write\_output(*file\_in: str*, *file\_out: str*, *toc: bool*)
    :   Write a ReST output file.

## Python version ancillary methods

Handle Python version check logic.

Not all Python versions are supported by scripts. Yet, on some cases,
like during documentation build, a newer version of python could be
available.

This class allows checking if the minimal requirements are followed.

Better than that, PythonVersion.`check_python()` not only checks the minimal
requirements, but it automatically switches to a the newest available
Python version if present.

*class* lib.python.kdoc.python\_version.PythonVersion(*version*)
:   Bases: `object`

    Ancillary methods that checks for missing dependencies for different
    types of types, like binaries, python modules, rpm deps, etc.

    *static* check\_python(*min\_version*, *show\_alternatives=False*, *bail\_out=False*, *success\_on\_error=False*)
    :   Check if the current python binary satisfies our minimal requirement
        for Sphinx build. If not, re-run with a newer version if found.

    *static* cmd\_print(*cmd*, *max\_len=80*)
    :   Outputs a command line, repecting maximum width.

    *static* find\_python(*min\_version*)
    :   Detect if are out there any python 3.xy version newer than the
        current one.

        Note: this routine is limited to up to 2 digits for python3. We
        may need to update it one day, hopefully on a distant future.

    *static* get\_python\_version(*cmd*)
    :   Get python version from a Python binary. As we need to detect if
        are out there newer python binaries, we can’t rely on sys.release here.

    *static* parse\_version(*version*)
    :   Convert a major.minor.patch version into a tuple.

    *static* ver\_str(*version*)
    :   Returns a version tuple as major.minor.patch.

## Write output on YAML file

*class* lib.python.kdoc.kdoc\_yaml\_file.KDocTestFile(*config*, *yaml\_file*, *yaml\_content*)
:   Bases: `object`

    Handles the logic needed to store kernel‑doc output inside a YAML file.
    :   Useful for unit tests and regression tests.

    *static* get\_kdoc\_item(*arg*, *start\_line=1*)

    output\_symbols(*fname*, *symbols*)
    :   Store source, symbols and output strings at self.tests.

    set\_filter(*export*, *internal*, *symbol*, *nosymbol*, *function\_table*, *enable\_lineno*, *no\_doc\_sections*)
    :   Set filters at the output classes.

    write()
    :   Output the content of self.tests to self.test\_file.
