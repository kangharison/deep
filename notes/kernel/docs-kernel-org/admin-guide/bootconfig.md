# Boot Configuration

> 출처(원문): https://docs.kernel.org/admin-guide/bootconfig.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Boot Configuration

Author:
:   Masami Hiramatsu <[mhiramat@kernel.org](mailto:mhiramat%40kernel.org)>

## Overview

The boot configuration expands the current kernel command line to support
additional key-value data when booting the kernel in an efficient way.
This allows administrators to pass a structured-Key config file.

## Config File Syntax

The boot config syntax is a simple structured key-value. Each key consists
of dot-connected-words, and key and value are connected by `=`. The value
string has to be terminated by the following delimiters described below.

Each key word must contain only alphabets, numbers, dash (`-`) or underscore
(`_`). And each value only contains printable characters or spaces except
for delimiters such as semi-colon (`;`), new-line (`\n`), comma (`,`),
hash (`#`) and closing brace (`}`).

If the `=` is followed by whitespace up to one of these delimiters, the
key is assigned an empty value.

For arrays, the array values are comma (`,`) separated, and comments and
line breaks with newline (`\n`) are allowed between array values for
readability. Thus the first entry of the array must be on the same line as
the key.:

```
KEY[.WORD[...]] = VALUE[, VALUE2[...]][;]
```

Unlike the kernel command line syntax, white spaces (including tabs) are
ignored around the comma and `=`.

If you want to use those delimiters in a value, you can use either double-
quotes (`"VALUE"`) or single-quotes (`'VALUE'`) to quote it. Note that
you can not escape these quotes.

There can be a key which doesn’t have value or has an empty value. Those keys
are used for checking if the key exists or not (like a boolean).

### Key-Value Syntax

The boot config file syntax allows user to merge partially same word keys
by brace. For example:

```
foo.bar.baz = value1
foo.bar.qux.quux = value2
```

These can be written also in:

```
foo.bar {
   baz = value1
   qux.quux = value2
}
```

Or more shorter, written as following:

```
foo.bar { baz = value1; qux.quux = value2 }
```

In both styles, same key words are automatically merged when parsing it
at boot time. So you can append similar trees or key-values.

### Same-key Values

It is prohibited that two or more values or arrays share a same-key.
For example,:

```
foo = bar, baz
foo = qux  # !ERROR! we can not re-define same key
```

If you want to update the value, you must use the override operator
`:=` explicitly. For example:

```
foo = bar, baz
foo := qux
```

then, the `qux` is assigned to `foo` key. This is useful for
overriding the default value by adding (partial) custom bootconfigs
without parsing the default bootconfig.

If you want to append the value to existing key as an array member,
you can use `+=` operator. For example:

```
foo = bar, baz
foo += qux
```

In this case, the key `foo` has `bar`, `baz` and `qux`.

Moreover, sub-keys and a value can coexist under a parent key.
For example, following config is allowed.:

```
foo = value1
foo.bar = value2
foo := value3 # This will update foo's value.
```

Note, since there is no syntax to put a raw value directly under a
structured key, you have to define it outside of the brace. For example:

```
foo {
    bar = value1
    bar {
        baz = value2
        qux = value3
    }
}
```

Also, the order of the value node under a key is fixed. If there
are a value and subkeys, the value is always the first child node
of the key. Thus if user specifies subkeys first, e.g.:

```
foo.bar = value1
foo = value2
```

In the program (and /proc/bootconfig), it will be shown as below:

```
foo = value2
foo.bar = value1
```

### Comments

The config syntax accepts shell-script style comments. The comments starting
with hash (“#”) until newline (”n”) will be ignored.

```
# comment line
foo = value # value is set to foo.
bar = 1, # 1st element
      2, # 2nd element
      3  # 3rd element
```

This is parsed as below:

```
foo = value
bar = 1, 2, 3
```

Note that you can NOT put a comment or a newline between value and delimiter
(`,` or `;`). This means following config has a syntax error

```
key = 1 # comment
      ,2
```

## /proc/bootconfig

/proc/bootconfig is a user-space interface of the boot config.
Unlike /proc/cmdline, this file shows the key-value style list.
Each key-value pair is shown in each line with following style:

```
KEY[.WORDS...] = "[VALUE]"[,"VALUE2"...]
```

## Boot Kernel With a Boot Config

There are two options to boot the kernel with bootconfig: attaching the
bootconfig to the initrd image or embedding it in the kernel itself.

### Attaching a Boot Config to Initrd

Since the boot configuration file is loaded with initrd by default,
it will be added to the end of the initrd (initramfs) image file with
padding, size, checksum and 12-byte magic word as below.

[initrd][bootconfig][padding][size(le32)][checksum(le32)][#BOOTCONFIGn]

The size and checksum fields are unsigned 32bit little endian value.

When the boot configuration is added to the initrd image, the total
file size is aligned to 4 bytes. To fill the gap, null characters
(`\0`) will be added. Thus the `size` is the length of the bootconfig
file + padding bytes.

The Linux kernel decodes the last part of the initrd image in memory to
get the boot configuration data.
Because of this “piggyback” method, there is no need to change or
update the boot loader and the kernel image itself as long as the boot
loader passes the correct initrd file size. If by any chance, the boot
loader passes a longer size, the kernel fails to find the bootconfig data.

To do this operation, Linux kernel provides `bootconfig` command under
tools/bootconfig, which allows admin to apply or delete the config file
to/from initrd image. You can build it by the following command:

```
# make -C tools/bootconfig
```

To add your boot config file to initrd image, run bootconfig as below
(Old data is removed automatically if exists):

```
# tools/bootconfig/bootconfig -a your-config /boot/initrd.img-X.Y.Z
```

To remove the config from the image, you can use -d option as below:

```
# tools/bootconfig/bootconfig -d /boot/initrd.img-X.Y.Z
```

Then add “bootconfig” on the normal kernel command line to tell the
kernel to look for the bootconfig at the end of the initrd file.
Alternatively, build your kernel with the `CONFIG_BOOT_CONFIG_FORCE`
Kconfig option selected.

### Embedding a Boot Config into Kernel

If you can not use initrd, you can also embed the bootconfig file in the
kernel by Kconfig options. In this case, you need to recompile the kernel
with the following configs:

```
CONFIG_BOOT_CONFIG_EMBED=y
CONFIG_BOOT_CONFIG_EMBED_FILE="/PATH/TO/BOOTCONFIG/FILE"
```

`CONFIG_BOOT_CONFIG_EMBED_FILE` requires an absolute path or a relative
path to the bootconfig file from source tree or object tree.
The kernel will embed it as the default bootconfig.

Just as when attaching the bootconfig to the initrd, you need `bootconfig`
option on the kernel command line to enable the embedded bootconfig, or,
alternatively, build your kernel with the `CONFIG_BOOT_CONFIG_FORCE`
Kconfig option selected.

Note that even if you set this option, you can override the embedded
bootconfig by another bootconfig which attached to the initrd.

## Kernel parameters via Boot Config

In addition to the kernel command line, the boot config can be used for
passing the kernel parameters. All the key-value pairs under `kernel`
key will be passed to kernel cmdline directly. Moreover, the key-value
pairs under `init` will be passed to init process via the cmdline.
The parameters are concatenated with user-given kernel cmdline string
as the following order, so that the command line parameter can override
bootconfig parameters (this depends on how the subsystem handles parameters
but in general, earlier parameter will be overwritten by later one.):

```
[bootconfig params][cmdline params] -- [bootconfig init params][cmdline init params]
```

Here is an example of the bootconfig file for kernel/init parameters.:

```
kernel {
  root = 01234567-89ab-cdef-0123-456789abcd
}
init {
 splash
}
```

This will be copied into the kernel cmdline string as the following:

```
root="01234567-89ab-cdef-0123-456789abcd" -- splash
```

If user gives some other command line like,:

```
ro bootconfig -- quiet
```

The final kernel cmdline will be the following:

```
root="01234567-89ab-cdef-0123-456789abcd" ro bootconfig -- splash quiet
```

## Config File Limitation

Currently the maximum config size is 32KB and the total key-words (not
key-value entries) must be under 1024 nodes.
Note: this is not the number of entries but nodes, an entry must consume
more than 2 nodes (a key-word and a value). So theoretically, it will be
up to 512 key-value pairs. If keys contains 3 words in average, it can
contain 256 key-value pairs. In most cases, the number of config items
will be under 100 entries and smaller than 8KB, so it would be enough.
If the node number exceeds 1024, parser returns an error even if the file
size is smaller than 32KB. (Note that this maximum size is not including
the padding null characters.)
Anyway, since bootconfig command verifies it when appending a boot config
to initrd image, user can notice it before boot.

## Bootconfig APIs

User can query or loop on key-value pairs, also it is possible to find
a root (prefix) key node and find key-values under that node.

If you have a key string, you can query the value directly with the key
using [`xbc_find_value()`](#c.xbc_find_value "xbc_find_value"). If you want to know what keys exist in the boot
config, you can use [`xbc_for_each_key_value()`](#c.xbc_for_each_key_value "xbc_for_each_key_value") to iterate key-value pairs.
Note that you need to use [`xbc_array_for_each_value()`](#c.xbc_array_for_each_value "xbc_array_for_each_value") for accessing
each array’s value, e.g.:

```
vnode = NULL;
xbc_find_value("key.word", &vnode);
if (vnode && xbc_node_is_array(vnode))
   xbc_array_for_each_value(vnode, value) {
     printk("%s ", value);
   }
```

If you want to focus on keys which have a prefix string, you can use
[`xbc_find_node()`](#c.xbc_find_node "xbc_find_node") to find a node by the prefix string, and iterate
keys under the prefix node with [`xbc_node_for_each_key_value()`](#c.xbc_node_for_each_key_value "xbc_node_for_each_key_value").

But the most typical usage is to get the named value under prefix
or get the named array under prefix as below:

```
root = xbc_find_node("key.prefix");
value = xbc_node_find_value(root, "option", &vnode);
...
xbc_node_for_each_array_value(root, "array-option", value, anode) {
   ...
}
```

This accesses a value of “key.prefix.option” and an array of
“key.prefix.array-option”.

Locking is not needed, since after initialization, the config becomes
read-only. All data and keys must be copied if you need to modify it.

## Functions and structures

uint32\_t xbc\_calc\_checksum(const void \*data, uint32\_t size)
:   Calculate checksum of bootconfig

**Parameters**

`const void *data`
:   Bootconfig data.

`uint32_t size`
:   The size of the bootconfig data.

**Description**

Calculate the checksum value of the bootconfig data.
The checksum will be used with the BOOTCONFIG\_MAGIC and the size for
embedding the bootconfig in the initrd image.

bool xbc\_node\_is\_value(struct xbc\_node \*node)
:   Test the node is a value node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Test the **node** is a value node and return true if a value node, false if not.

bool xbc\_node\_is\_key(struct xbc\_node \*node)
:   Test the node is a key node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Test the **node** is a key node and return true if a key node, false if not.

bool xbc\_node\_is\_array(struct xbc\_node \*node)
:   Test the node is an arraied value node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Test the **node** is an arraied value node.

bool xbc\_node\_is\_leaf(struct xbc\_node \*node)
:   Test the node is a leaf key node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Test the **node** is a leaf key node which is a key node and has a value node
or no child. Returns true if it is a leaf node, or false if not.
Note that the leaf node can have subkey nodes in addition to the
value node.

const char \*xbc\_find\_value(const char \*key, struct xbc\_node \*\*vnode)
:   Find a value which matches the key

**Parameters**

`const char *key`
:   Search key

`struct xbc_node **vnode`
:   A container pointer of XBC value node.

**Description**

Search a value whose key matches **key** from whole of XBC tree and return
the value if found. Found value node is stored in **\*vnode**.
Note that this can return 0-length string and store NULL in **\*vnode** for
key-only (non-value) entry.

struct xbc\_node \*xbc\_find\_node(const char \*key)
:   Find a node which matches the key

**Parameters**

`const char *key`
:   Search key

**Description**

Search a (key) node whose key matches **key** from whole of XBC tree and
return the node if found. If not found, returns NULL.

struct xbc\_node \*xbc\_node\_get\_subkey(struct xbc\_node \*node)
:   Return the first subkey node if exists

**Parameters**

`struct xbc_node *node`
:   Parent node

**Description**

Return the first subkey node of the **node**. If the **node** has no child
or only value node, this will return NULL.

xbc\_array\_for\_each\_value

`xbc_array_for_each_value (anode, value)`

> Iterate value nodes on an array

**Parameters**

`anode`
:   An XBC arraied value node

`value`
:   A value

**Description**

Iterate array value nodes and values starts from **anode**. This is expected to
be used with [`xbc_find_value()`](#c.xbc_find_value "xbc_find_value") and [`xbc_node_find_value()`](#c.xbc_node_find_value "xbc_node_find_value"), so that user can
process each array entry node.

xbc\_node\_for\_each\_child

`xbc_node_for_each_child (parent, child)`

> Iterate child nodes

**Parameters**

`parent`
:   An XBC node.

`child`
:   Iterated XBC node.

**Description**

Iterate child nodes of **parent**. Each child nodes are stored to **child**.
The **child** can be mixture of a value node and subkey nodes.

xbc\_node\_for\_each\_subkey

`xbc_node_for_each_subkey (parent, child)`

> Iterate child subkey nodes

**Parameters**

`parent`
:   An XBC node.

`child`
:   Iterated XBC node.

**Description**

Iterate subkey nodes of **parent**. Each child nodes are stored to **child**.
The **child** is only the subkey node.

xbc\_node\_for\_each\_array\_value

`xbc_node_for_each_array_value (node, key, anode, value)`

> Iterate array entries of geven key

**Parameters**

`node`
:   An XBC node.

`key`
:   A key string searched under **node**

`anode`
:   Iterated XBC node of array entry.

`value`
:   Iterated value of array entry.

**Description**

Iterate array entries of given **key** under **node**. Each array entry node
is stored to **anode** and **value**. If the **node** doesn’t have **key** node,
it does nothing.
Note that even if the found key node has only one value (not array)
this executes block once. However, if the found key node has no value
(key-only node), this does nothing. So don’t use this for testing the
key-value pair existence.

xbc\_node\_for\_each\_key\_value

`xbc_node_for_each_key_value (node, knode, value)`

> Iterate key-value pairs under a node

**Parameters**

`node`
:   An XBC node.

`knode`
:   Iterated key node

`value`
:   Iterated value string

**Description**

Iterate key-value pairs under **node**. Each key node and value string are
stored in **knode** and **value** respectively.

xbc\_for\_each\_key\_value

`xbc_for_each_key_value (knode, value)`

> Iterate key-value pairs

**Parameters**

`knode`
:   Iterated key node

`value`
:   Iterated value string

**Description**

Iterate key-value pairs in whole XBC tree. Each key node and value string
are stored in **knode** and **value** respectively.

int xbc\_node\_compose\_key(struct xbc\_node \*node, char \*buf, size\_t size)
:   Compose full key string of the XBC node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

`char *buf`
:   A buffer to store the key.

`size_t size`
:   The size of the **buf**.

**Description**

Compose the full-length key of the **node** into **buf**. Returns the total
length of the key stored in **buf**. Or returns -EINVAL if **node** is NULL,
and -ERANGE if the key depth is deeper than max depth.

int xbc\_get\_info(int \*node\_size, size\_t \*data\_size)
:   Get the information of loaded boot config

**Parameters**

`int *node_size`
:   A pointer to store the number of nodes.

`size_t *data_size`
:   A pointer to store the size of bootconfig data.

**Description**

Get the number of used nodes in **node\_size** if it is not NULL,
and the size of bootconfig data in **data\_size** if it is not NULL.
Return 0 if the boot config is initialized, or return -ENODEV.

struct xbc\_node \*xbc\_root\_node(void)
:   Get the root node of extended boot config

**Parameters**

`void`
:   no arguments

**Description**

Return the address of root node of extended boot config. If the
extended boot config is not initialized, return NULL.

uint16\_t xbc\_node\_index(struct xbc\_node \*node)
:   Get the index of XBC node

**Parameters**

`struct xbc_node *node`
:   A target node of getting index.

**Description**

Return the index number of **node** in XBC node list.

struct xbc\_node \*xbc\_node\_get\_parent(struct xbc\_node \*node)
:   Get the parent XBC node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Return the parent node of **node**. If the node is top node of the tree,
return NULL.

struct xbc\_node \*xbc\_node\_get\_child(struct xbc\_node \*node)
:   Get the child XBC node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Return the first child node of **node**. If the node has no child, return
NULL.

struct xbc\_node \*xbc\_node\_get\_next(struct xbc\_node \*node)
:   Get the next sibling XBC node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Return the NEXT sibling node of **node**. If the node has no next sibling,
return NULL. Note that even if this returns NULL, it doesn’t mean **node**
has no siblings. (You also has to check whether the parent’s child node
is **node** or not.)

const char \*xbc\_node\_get\_data(struct xbc\_node \*node)
:   Get the data of XBC node

**Parameters**

`struct xbc_node *node`
:   An XBC node.

**Description**

Return the data (which is always a null terminated string) of **node**.
If the node has invalid data, warn and return NULL.

struct xbc\_node \*xbc\_node\_find\_subkey(struct xbc\_node \*parent, const char \*key)
:   Find a subkey node which matches given key

**Parameters**

`struct xbc_node *parent`
:   An XBC node.

`const char *key`
:   A key string.

**Description**

Search a key node under **parent** which matches **key**. The **key** can contain
several words jointed with ‘.’. If **parent** is NULL, this searches the
node from whole tree. Return NULL if no node is matched.

const char \*xbc\_node\_find\_value(struct xbc\_node \*parent, const char \*key, struct xbc\_node \*\*vnode)
:   Find a value node which matches given key

**Parameters**

`struct xbc_node *parent`
:   An XBC node.

`const char *key`
:   A key string.

`struct xbc_node **vnode`
:   A container pointer of found XBC node.

**Description**

Search a value node under **parent** whose (parent) key node matches **key**,
store it in **\*vnode**, and returns the value string.
The **key** can contain several words jointed with ‘.’. If **parent** is NULL,
this searches the node from whole tree. Return the value string if a
matched key found, return NULL if no node is matched.
Note that this returns 0-length string and stores NULL in **\*vnode** if the
key has no value. And also it will return the value of the first entry if
the value is an array.

int xbc\_node\_compose\_key\_after(struct xbc\_node \*root, struct xbc\_node \*node, char \*buf, size\_t size)
:   Compose partial key string of the XBC node

**Parameters**

`struct xbc_node *root`
:   Root XBC node

`struct xbc_node *node`
:   Target XBC node.

`char *buf`
:   A buffer to store the key.

`size_t size`
:   The size of the **buf**.

**Description**

Compose the partial key of the **node** into **buf**, which is starting right
after **root** (**root** is not included.) If **root** is NULL, this returns full
key words of **node**.
Returns the total length of the key stored in **buf**. Returns -EINVAL
if **node** is NULL or **root** is not the ancestor of **node** or **root** is **node**,
or returns -ERANGE if the key depth is deeper than max depth.
This is expected to be used with [`xbc_find_node()`](#c.xbc_find_node "xbc_find_node") to list up all (child)
keys under given key.

struct xbc\_node \*xbc\_node\_find\_next\_leaf(struct xbc\_node \*root, struct xbc\_node \*node)
:   Find the next leaf node under given node

**Parameters**

`struct xbc_node *root`
:   An XBC root node

`struct xbc_node *node`
:   An XBC node which starts from.

**Description**

Search the next leaf node (which means the terminal key node) of **node**
under **root** node (including **root** node itself).
Return the next node or NULL if next leaf node is not found.

const char \*xbc\_node\_find\_next\_key\_value(struct xbc\_node \*root, struct xbc\_node \*\*leaf)
:   Find the next key-value pair nodes

**Parameters**

`struct xbc_node *root`
:   An XBC root node

`struct xbc_node **leaf`
:   A container pointer of XBC node which starts from.

**Description**

Search the next leaf node (which means the terminal key node) of **\*leaf**
under **root** node. Returns the value and update **\*leaf** if next leaf node
is found, or NULL if no next leaf node is found.
Note that this returns 0-length string if the key has no value, or
the value of the first entry if the value is an array.

void \_xbc\_exit(bool early)
:   Clean up all parsed bootconfig

**Parameters**

`bool early`
:   Set true if this is called before budy system is initialized.

**Description**

This clears all data structures of parsed bootconfig on memory.
If you need to reuse [`xbc_init()`](#c.xbc_init "xbc_init") with new boot config, you can
use this.

int xbc\_init(const char \*data, size\_t size, const char \*\*emsg, int \*epos)
:   Parse given XBC file and build XBC internal tree

**Parameters**

`const char *data`
:   The boot config text original data

`size_t size`
:   The size of **data**

`const char **emsg`
:   A pointer of const char \* to store the error message

`int *epos`
:   A pointer of int to store the error position

**Description**

This parses the boot config text in **data**. **size** must be smaller
than XBC\_DATA\_MAX.
Return the number of stored nodes (>0) if succeeded, or -errno
if there is any error.
In error cases, **emsg** will be updated with an error message and
**epos** will be updated with the error position which is the byte offset
of **buf**. If the error is not a parser error, **epos** will be -1.
