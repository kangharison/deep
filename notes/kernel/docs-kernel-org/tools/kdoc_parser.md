# Kernel-doc parser stage

> 출처(원문): https://docs.kernel.org/tools/kdoc_parser.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel-doc parser stage

## C replacement rules used by the parser

*class* lib.python.kdoc.xforms\_lists.CTransforms
:   Bases: `object`

    Data class containing a long set of transformations to turn
    structure member prefixes, and macro invocations and variables
    into something we can parse and generate kdoc for.

    apply(*xforms\_type*, *source*)
    :   Apply a set of transforms to a block of source.

        As tokenizer is used here, this function also remove comments
        at the end.

    function\_xforms *= [(CMatch("^static\b"), ''), (CMatch("^extern\b"), ''), (CMatch("^asmlinkage\b"), ''), (CMatch("^inline\b"), ''), (CMatch("^\_\_inline\_\_\b"), ''), (CMatch("^\_\_inline\b"), ''), (CMatch("^\_\_always\_inline\b"), ''), (CMatch("^noinline\b"), ''), (CMatch("^\_\_FORTIFY\_INLINE\b"), ''), (CMatch("^\_\_init\b"), ''), (CMatch("^\_\_init\_or\_module\b"), ''), (CMatch("^\_\_exit\b"), ''), (CMatch("^\_\_deprecated\b"), ''), (CMatch("^\_\_flatten\b"), ''), (CMatch("^\_\_meminit\b"), ''), (CMatch("^\_\_must\_check\b"), ''), (CMatch("^\_\_weak\b"), ''), (CMatch("^\_\_sched\b"), ''), (CMatch("^\_\_always\_unused\b"), ''), (CMatch("^\_\_printf\b"), ''), (CMatch("^\_\_(?:re)?alloc\_size\b"), ''), (CMatch("^\_\_diagnose\_as\b"), ''), (CMatch("^DECL\_BUCKET\_PARAMS\b"), '\\1, \\2'), (CMatch("^\_\_no\_context\_analysis\b"), ''), (CMatch("^\_\_attribute\_const\_\_\b"), ''), (CMatch("^\_\_attribute\_\_\b"), ''), (KernRe("\_noprof" ), '')]*
    :   Transforms for function prototypes.

    struct\_xforms *= [(CMatch("^\_\_attribute\_\_\b"), ''), (CMatch("^\_\_aligned\b"), ''), (CMatch("^\_\_counted\_by\b"), ''), (CMatch("^\_\_counted\_by\_(le|be)\b"), ''), (CMatch("^\_\_guarded\_by\b"), ''), (CMatch("^\_\_pt\_guarded\_by\b"), ''), (CMatch("^\_\_packed\b"), ''), (CMatch("^CRYPTO\_MINALIGN\_ATTR\b"), ''), (CMatch("^\_\_private\b"), ''), (CMatch("^\_\_rcu\b"), ''), (CMatch("^\_\_\_\_cacheline\_aligned\_in\_smp\b"), ''), (CMatch("^\_\_\_\_cacheline\_aligned\b"), ''), (CMatch("^\_\_cacheline\_group\_(?:begin|end)\b"), ''), (CMatch("^\_\_ETHTOOL\_DECLARE\_LINK\_MODE\_MASK\b"), 'DECLARE\_BITMAP(\\1, \_\_ETHTOOL\_LINK\_MODE\_MASK\_NBITS)'), (CMatch("^DECLARE\_PHY\_INTERFACE\_MASK\b"), 'DECLARE\_BITMAP(\\1, PHY\_INTERFACE\_MODE\_MAX)'), (CMatch("^DECLARE\_BITMAP\b"), 'unsigned long \\1[BITS\_TO\_LONGS(\\2)]'), (CMatch("^DECLARE\_HASHTABLE\b"), 'unsigned long \\1[1 << ((\\2) - 1)]'), (CMatch("^DECLARE\_KFIFO\b"), '\\2 \*\\1'), (CMatch("^DECLARE\_KFIFO\_PTR\b"), '\\2 \*\\1'), (CMatch("^(?:\_\_)?DECLARE\_FLEX\_ARRAY\b"), '\\1 \\2[]'), (CMatch("^DEFINE\_DMA\_UNMAP\_ADDR\b"), 'dma\_addr\_t \\1'), (CMatch("^DEFINE\_DMA\_UNMAP\_LEN\b"), '\_\_u32 \\1'), (CMatch("^VIRTIO\_DECLARE\_FEATURES\b"), 'union { u64 \\1; u64 \\1\_array[VIRTIO\_FEATURES\_U64S]; }'), (CMatch("^\_\_cond\_acquires\b"), ''), (CMatch("^\_\_cond\_releases\b"), ''), (CMatch("^\_\_acquires\b"), ''), (CMatch("^\_\_releases\b"), ''), (CMatch("^\_\_must\_hold\b"), ''), (CMatch("^\_\_must\_not\_hold\b"), ''), (CMatch("^\_\_must\_hold\_shared\b"), ''), (CMatch("^\_\_cond\_acquires\_shared\b"), ''), (CMatch("^\_\_acquires\_shared\b"), ''), (CMatch("^\_\_releases\_shared\b"), ''), (CMatch("^\_\_attribute\_\_\b"), ''), (CMatch("^struct\_group\b"), 'struct { \\2+ };'), (CMatch("^struct\_group\_attr\b"), 'struct { \\3+ };'), (CMatch("^struct\_group\_tagged\b"), 'struct { \\3+ };'), (CMatch("^\_\_struct\_group\b"), 'struct { \\4+ };')]*
    :   Transforms for structs and unions.

    var\_xforms *= [(CMatch("^\_\_read\_mostly\b"), ''), (CMatch("^\_\_ro\_after\_init\b"), ''), (CMatch("^\_\_guarded\_by\b"), ''), (CMatch("^\_\_pt\_guarded\_by\b"), ''), (CMatch("^LIST\_HEAD\b"), 'struct list\_head \\1'), (KernRe("(?://.\*)$" ), ''), (KernRe("(?:/\\*.\*\\*/)" ), ''), (KernRe(";$" ), '')]*
    :   Transforms for variable prototypes.

    xforms *= {'func': [(CMatch("^static\b"), ''), (CMatch("^extern\b"), ''), (CMatch("^asmlinkage\b"), ''), (CMatch("^inline\b"), ''), (CMatch("^\_\_inline\_\_\b"), ''), (CMatch("^\_\_inline\b"), ''), (CMatch("^\_\_always\_inline\b"), ''), (CMatch("^noinline\b"), ''), (CMatch("^\_\_FORTIFY\_INLINE\b"), ''), (CMatch("^\_\_init\b"), ''), (CMatch("^\_\_init\_or\_module\b"), ''), (CMatch("^\_\_exit\b"), ''), (CMatch("^\_\_deprecated\b"), ''), (CMatch("^\_\_flatten\b"), ''), (CMatch("^\_\_meminit\b"), ''), (CMatch("^\_\_must\_check\b"), ''), (CMatch("^\_\_weak\b"), ''), (CMatch("^\_\_sched\b"), ''), (CMatch("^\_\_always\_unused\b"), ''), (CMatch("^\_\_printf\b"), ''), (CMatch("^\_\_(?:re)?alloc\_size\b"), ''), (CMatch("^\_\_diagnose\_as\b"), ''), (CMatch("^DECL\_BUCKET\_PARAMS\b"), '\\1, \\2'), (CMatch("^\_\_no\_context\_analysis\b"), ''), (CMatch("^\_\_attribute\_const\_\_\b"), ''), (CMatch("^\_\_attribute\_\_\b"), ''), (KernRe("\_noprof" ), '')], 'struct': [(CMatch("^\_\_attribute\_\_\b"), ''), (CMatch("^\_\_aligned\b"), ''), (CMatch("^\_\_counted\_by\b"), ''), (CMatch("^\_\_counted\_by\_(le|be)\b"), ''), (CMatch("^\_\_guarded\_by\b"), ''), (CMatch("^\_\_pt\_guarded\_by\b"), ''), (CMatch("^\_\_packed\b"), ''), (CMatch("^CRYPTO\_MINALIGN\_ATTR\b"), ''), (CMatch("^\_\_private\b"), ''), (CMatch("^\_\_rcu\b"), ''), (CMatch("^\_\_\_\_cacheline\_aligned\_in\_smp\b"), ''), (CMatch("^\_\_\_\_cacheline\_aligned\b"), ''), (CMatch("^\_\_cacheline\_group\_(?:begin|end)\b"), ''), (CMatch("^\_\_ETHTOOL\_DECLARE\_LINK\_MODE\_MASK\b"), 'DECLARE\_BITMAP(\\1, \_\_ETHTOOL\_LINK\_MODE\_MASK\_NBITS)'), (CMatch("^DECLARE\_PHY\_INTERFACE\_MASK\b"), 'DECLARE\_BITMAP(\\1, PHY\_INTERFACE\_MODE\_MAX)'), (CMatch("^DECLARE\_BITMAP\b"), 'unsigned long \\1[BITS\_TO\_LONGS(\\2)]'), (CMatch("^DECLARE\_HASHTABLE\b"), 'unsigned long \\1[1 << ((\\2) - 1)]'), (CMatch("^DECLARE\_KFIFO\b"), '\\2 \*\\1'), (CMatch("^DECLARE\_KFIFO\_PTR\b"), '\\2 \*\\1'), (CMatch("^(?:\_\_)?DECLARE\_FLEX\_ARRAY\b"), '\\1 \\2[]'), (CMatch("^DEFINE\_DMA\_UNMAP\_ADDR\b"), 'dma\_addr\_t \\1'), (CMatch("^DEFINE\_DMA\_UNMAP\_LEN\b"), '\_\_u32 \\1'), (CMatch("^VIRTIO\_DECLARE\_FEATURES\b"), 'union { u64 \\1; u64 \\1\_array[VIRTIO\_FEATURES\_U64S]; }'), (CMatch("^\_\_cond\_acquires\b"), ''), (CMatch("^\_\_cond\_releases\b"), ''), (CMatch("^\_\_acquires\b"), ''), (CMatch("^\_\_releases\b"), ''), (CMatch("^\_\_must\_hold\b"), ''), (CMatch("^\_\_must\_not\_hold\b"), ''), (CMatch("^\_\_must\_hold\_shared\b"), ''), (CMatch("^\_\_cond\_acquires\_shared\b"), ''), (CMatch("^\_\_acquires\_shared\b"), ''), (CMatch("^\_\_releases\_shared\b"), ''), (CMatch("^\_\_attribute\_\_\b"), ''), (CMatch("^struct\_group\b"), 'struct { \\2+ };'), (CMatch("^struct\_group\_attr\b"), 'struct { \\3+ };'), (CMatch("^struct\_group\_tagged\b"), 'struct { \\3+ };'), (CMatch("^\_\_struct\_group\b"), 'struct { \\4+ };')], 'var': [(CMatch("^\_\_read\_mostly\b"), ''), (CMatch("^\_\_ro\_after\_init\b"), ''), (CMatch("^\_\_guarded\_by\b"), ''), (CMatch("^\_\_pt\_guarded\_by\b"), ''), (CMatch("^LIST\_HEAD\b"), 'struct list\_head \\1'), (KernRe("(?://.\*)$" ), ''), (KernRe("(?:/\\*.\*\\*/)" ), ''), (KernRe(";$" ), '')]}*
    :   Transforms main dictionary used at `apply_transforms()`.

## File handler classes

Classes for navigating through the files that kernel-doc needs to handle
to generate documentation.

*class* lib.python.kdoc.kdoc\_files.GlobSourceFiles(*srctree=None*, *valid\_extensions=None*)
:   Bases: `object`

    Parse C source code file names and directories via an Interactor.

    parse\_files(*file\_list*, *file\_not\_found\_cb*)
    :   Define an iterator to parse all source files from file\_list,
        handling directories if any.

*class* lib.python.kdoc.kdoc\_files.KdocConfig(*verbose=False*, *werror=False*, *wreturn=False*, *wshort\_desc=False*, *wcontents\_before\_sections=False*, *logger=None*)
:   Bases: `object`

    Stores all configuration attributes that kdoc\_parser and kdoc\_output
    needs.

*class* lib.python.kdoc.kdoc\_files.KernelFiles(*verbose=False*, *out\_style=None*, *xforms=None*, *werror=False*, *wreturn=False*, *wshort\_desc=False*, *wcontents\_before\_sections=False*, *yaml\_file=None*, *yaml\_content=None*, *logger=None*)
:   Bases: `object`

    Parse kernel-doc tags on multiple kernel source files.

    This is the main entry point to run kernel-doc. This class is initialized
    using a series of optional arguments:

    `verbose`
    :   If True, enables kernel-doc verbosity. Default: False.

    `out_style`
    :   Class to be used to format output. If None (default),
        only report errors.

    `xforms`
    :   Transforms to be applied to C prototypes and data structs.
        If not specified, defaults to xforms = `CFunction()`

    `werror`
    :   If True, treat warnings as errors, retuning an error code on warnings.

        Default: False.

    `wreturn`
    :   If True, warns about the lack of a return markup on functions.

        Default: False.

    `wshort_desc`
    :   If True, warns if initial short description is missing.

        Default: False.

    `wcontents_before_sections`
    :   If True, warn if there are contents before sections (deprecated).
        This option is kept just for backward-compatibility, but it does
        nothing, neither here nor at the original Perl script.

        Default: False.

    `logger`
    :   Optional logger class instance.

        If not specified, defaults to use: `logging.getLogger("kernel-doc")`

    `yaml_file`
    :   If defined, stores the output inside a YAML file.

    `yaml_content`
    :   Defines what will be inside the YAML file.

    Note:
    :   There are two type of parsers defined here:

        * self.`parse_file()`: parses both kernel-doc markups and
          `EXPORT_SYMBOL*` macros;
        * self.`process_export_file()`: parses only `EXPORT_SYMBOL*` macros.

    error(*msg*)
    :   Ancillary routine to output an error and increment error count.

    file\_not\_found\_cb(*fname*)
    :   Callback to warn if a file was not found.

    msg(*enable\_lineno=False*, *export=False*, *internal=False*, *symbol=None*, *nosymbol=None*, *no\_doc\_sections=False*, *filenames=None*, *export\_file=None*)
    :   Interacts over the kernel-doc results and output messages,
        returning kernel-doc markups on each interaction.

    out\_msg(*fname*, *name*, *arg*)
    :   Return output messages from a file name using the output style
        filtering.

        If output type was not handled by the styler, return None.

    parse(*file\_list*, *export\_file=None*)
    :   Parse all files.

    parse\_file(*fname*)
    :   Parse a single Kernel source.

    process\_export\_file(*fname*)
    :   Parses `EXPORT_SYMBOL*` macros from a single Kernel source file.

    warning(*msg*)
    :   Ancillary routine to output a warning and increment error count.

## Parsed item data class

Data class to store a kernel-doc Item.

*class* lib.python.kdoc.kdoc\_item.KdocItem(*name*, *fname*, *type*, *start\_line*, *\*\*other\_stuff*)
:   Bases: `object`

    A class that will, eventually, encapsulate all of the parsed data that we
    then pass into the output modules.

    *classmethod* from\_dict(*d*)
    :   Create a KdocItem from a plain dict.

    get(*key*, *default=None*)
    :   Get a value from optional keys.

    set\_params(*names*, *descs*, *types*, *starts*)
    :   Set parameter list: names, descriptions, types and start lines.

    set\_sections(*sections*, *start\_lines*)
    :   Set sections and start lines.

## Parser classes and methods

Classes and functions related to reading a C language source or header FILE
and extract embedded documentation comments from it.

*class* lib.python.kdoc.kdoc\_parser.KernelDoc(*config*, *fname*, *xforms*, *store\_src=False*)
:   Bases: `object`

    Read a C language source or header FILE and extract embedded
    documentation comments.

    check\_return\_section(*ln*, *declaration\_name*, *return\_type*)
    :   If the function doesn’t return void, warns about the lack of a
        return description.

    check\_sections(*ln*, *decl\_name*, *decl\_type*)
    :   Check for errors inside sections, emitting warnings if not found
        parameters are described.

    create\_parameter\_list(*ln*, *decl\_type*, *args*, *splitter*, *declaration\_name*)
    :   Creates a list of parameters, storing them at self.entry.

    dump\_declaration(*ln*, *prototype*, *source*)
    :   Store a data declaration inside self.entries array.

    dump\_enum(*ln*, *proto*, *source*)
    :   Store an `enum` inside self.entries array.

    dump\_function(*ln*, *prototype*, *source*)
    :   Store a function or function macro inside self.entries array.

    dump\_section(*start\_new=True*)
    :   Dump section contents to arrays/hashes intended for that purpose.

    dump\_struct(*ln*, *proto*, *source*)
    :   Store an entry for a `struct` or `union`

    dump\_typedef(*ln*, *proto*, *source*)
    :   Store a `typedef` inside self.entries array.

    dump\_var(*ln*, *proto*, *source*)
    :   Store variables that are part of kAPI.

    emit\_msg(*ln*, *msg*, *\**, *warning=True*)
    :   Emit a message

    emit\_unused\_warnings()
    :   When the parser fails to produce a valid entry, it places some
        warnings under entry.warnings that will be discarded when resetting
        the state.

        Ensure that those warnings are not lost.

        Note

        Because we are calling config.`warning()` here, those
        warnings are not filtered by the -W parameters: they will all
        be produced even when -Wreturn, -Wshort-desc, and/or
        -Wcontents-before-sections are used.

        Allowing those warnings to be filtered is complex, because it
        would require storing them in a buffer and then filtering them
        during the output step of the code, depending on the
        selected symbols.

    format\_struct\_decl(*declaration*)
    :   Format the `struct` declaration into a standard form for inclusion
        in the resulting docs.

    is\_comment\_end(*ln*, *line*)
    :   Helper function to detect (and effect) the end of a kerneldoc comment.

    is\_new\_section(*ln*, *line*)
    :   Helper function to determine if a new section is being started.

    output\_declaration(*dtype*, *name*, *\*\*args*)
    :   Store the entry into an entry array.

        The actual output and output filters will be handled elsewhere.

    parse\_export()
    :   Parses `EXPORT_SYMBOL*` macros from a single Kernel source file.

    parse\_kdoc()
    :   Open and process each line of a C source file.
        The parsing is controlled via a state machine, and the line is passed
        to a different process function depending on the state. The process
        function may update the state as needed.

        Besides parsing kernel-doc tags, it also parses export symbols.

    process\_body(*ln*, *line*, *source*)
    :   STATE\_BODY: the bulk of a kerneldoc comment.

    process\_decl(*ln*, *line*, *source*)
    :   STATE\_DECLARATION: We’ve seen the beginning of a declaration.

    process\_docblock(*ln*, *line*, *source*)
    :   STATE\_DOCBLOCK: within a `DOC:` block.

    *static* process\_export(*function\_set*, *line*)
    :   process `EXPORT_SYMBOL*` tags

        This method doesn’t use any variable from the class, so declare it
        with a staticmethod decorator.

    process\_inline\_name(*ln*, *line*, *source*)
    :   STATE\_INLINE\_NAME: beginning of docbook comments within a prototype.

    process\_inline\_text(*ln*, *line*, *source*)
    :   STATE\_INLINE\_TEXT: docbook comments within a prototype.

    process\_name(*ln*, *line*, *source*)
    :   STATE\_NAME: Looking for the “name - description” line

    process\_normal(*ln*, *line*, *source*)
    :   STATE\_NORMAL: looking for the `/**` to begin everything.

    process\_proto(*ln*, *line*, *source*)
    :   STATE\_PROTO: reading a function/whatever prototype.

    process\_proto\_function(*ln*, *line*, *source*)
    :   Ancillary routine to process a function prototype.

    process\_proto\_type(*ln*, *line*, *source*)
    :   Ancillary routine to process a type.

    process\_special(*ln*, *line*, *source*)
    :   STATE\_SPECIAL\_SECTION: a section ending with a blank line.

    push\_parameter(*ln*, *decl\_type*, *param*, *dtype*, *org\_arg*, *declaration\_name*)
    :   Store parameters and their descriptions at self.entry.

    reset\_state(*ln*)
    :   Ancillary routine to create a new entry. It initializes all
        variables used by the state machine.

    rewrite\_struct\_members(*members*)
    :   Process `struct`/`union` members from the most deeply nested
        outward.

        Rewrite the members of a `struct` or `union` for easier formatting
        later on. Among other things, this function will turn a member like:

        ```
        struct { inner_members; } foo;
        ```

        into:

        ```
        struct foo; inner_members;
        ```

    section\_context *= 'Context'*
    :   Name of context section.

    section\_return *= 'Return'*
    :   Name of return section.

    split\_struct\_proto(*proto*)
    :   Split apart a structure prototype; returns (struct|union, name,
        members) or `None`.

    state\_actions *= {0: <function KernelDoc.process\_normal>, 1: <function KernelDoc.process\_name>, 2: <function KernelDoc.process\_decl>, 3: <function KernelDoc.process\_body>, 4: <function KernelDoc.process\_special>, 5: <function KernelDoc.process\_proto>, 6: <function KernelDoc.process\_docblock>, 7: <function KernelDoc.process\_inline\_name>, 8: <function KernelDoc.process\_inline\_text>}*
    :   The state/action table telling us which function to invoke in each state.

    syscall\_munge(*ln*, *proto*)
    :   Handle syscall definitions.

    tracepoint\_munge(*ln*, *proto*)
    :   Handle tracepoint definitions.

    undescribed *= '-- undescribed --'*
    :   String to write when a parameter is not described.

*class* lib.python.kdoc.kdoc\_parser.KernelEntry(*config*, *fname*, *ln*)
:   Bases: `object`

    Encapsulates a Kernel documentation entry.

    add\_text(*text*)
    :   Add a new text to the entry contents list.

    begin\_section(*line\_no*, *title='Description'*, *dump=False*)
    :   Begin a new section.

    contents()
    :   Returns a string with all content texts that were added.

    dump\_section(*start\_new=True*)
    :   Dumps section contents to arrays/hashes intended for that purpose.

    emit\_msg(*ln*, *msg*, *\**, *warning=True*)
    :   Emit a message.

lib.python.kdoc.kdoc\_parser.SECTION\_DEFAULT *= 'Description'*
:   Default section.

*class* lib.python.kdoc.kdoc\_parser.state
:   Bases: `object`

    States used by the parser’s state machine.

    BODY *= 3*
    :   The body of the comment.

    DECLARATION *= 2*
    :   We have seen a declaration which might not be done.

    DOCBLOCK *= 6*
    :   Documentation block.

    INLINE\_NAME *= 7*
    :   Gathering doc outside main block.

    INLINE\_TEXT *= 8*
    :   Reading the body of inline docs.

    NAME *= 1*
    :   Looking for function name.

    NORMAL *= 0*
    :   Normal code.

    PROTO *= 5*
    :   Scanning prototype.

    SPECIAL\_SECTION *= 4*
    :   Doc section ending with a blank line.

    name *= ['NORMAL', 'NAME', 'DECLARATION', 'BODY', 'SPECIAL\_SECTION', 'PROTO', 'DOCBLOCK', 'INLINE\_NAME', 'INLINE\_TEXT']*
    :   Names for each parser state.

lib.python.kdoc.kdoc\_parser.trim\_private\_members(*text*)
:   Remove `struct`/`enum` members that have been marked “private”.

lib.python.kdoc.kdoc\_parser.trim\_whitespace(*s*)
:   A little helper to get rid of excess white space.
