# ABI stable symbols

> 출처(원문): https://docs.kernel.org/admin-guide/abi-stable.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ABI stable symbols

Documents the interfaces that the developer has defined to be stable.

Userspace programs are free to use these interfaces with no
restrictions, and backward compatibility for them will be guaranteed
for at least 2 years.

Most interfaces (like syscalls) are expected to never change and always
be available.

## Symbols under /dev/fw

|  |
| --- |
| **/dev/fw[0-9]+** |

Defined on file [firewire-cdev](abi-stable-files.html#abi-file-stable-firewire-cdev)

The character device files /dev/fw\* are the interface between
firewire-core and IEEE 1394 device drivers implemented in
userspace. The ioctl(2)- and read(2)-based ABI is defined and
documented in <linux/firewire-cdev.h>.

This ABI offers most of the features which firewire-core also
exposes to kernelspace IEEE 1394 drivers.

Each /dev/fw\* is associated with one IEEE 1394 node, which can
be remote or local nodes. Operations on a /dev/fw\* file have
different scope:

> * The 1394 node which is associated with the file:
>
>   > + Asynchronous request transmission
>   > + Get the Configuration ROM
>   > + Query node ID
>   > + Query maximum speed of the path between this node
>   >   and local node
> * The 1394 bus (i.e. “card”) to which the node is attached to:
>
>   > + Isochronous stream transmission and reception
>   > + Asynchronous stream transmission and reception
>   > + Asynchronous broadcast request transmission
>   > + PHY packet transmission and reception
>   > + Allocate, reallocate, deallocate isochronous
>   >   resources (channels, bandwidth) at the bus’s IRM
>   > + Query node IDs of local node, root node, IRM, bus
>   >   manager
>   > + Query cycle time
>   > + Bus reset initiation, bus reset event reception
> * All 1394 buses:
>
>   > + Allocation of IEEE 1212 address ranges on the local
>   >   link layers, reception of inbound requests to such
>   >   an address range, asynchronous response transmission
>   >   to inbound requests
>   > + Addition of descriptors or directories to the local
>   >   nodes’ Configuration ROM

Due to the different scope of operations and in order to let
userland implement different access permission models, some
operations are restricted to /dev/fw\* files that are associated
with a local node:

> * Addition of descriptors or directories to the local
>   nodes’ Configuration ROM
> * PHY packet transmission and reception

A /dev/fw\* file remains associated with one particular node
during its entire life time. Bus topology changes, and hence
node ID changes, are tracked by firewire-core. ABI users do not
need to be aware of topology.

The following file operations are supported:

open(2)
:   Currently the only useful flags are O\_RDWR.

ioctl(2)
:   Initiate various actions. Some take immediate effect, others
    are performed asynchronously while or after the ioctl returns.
    See the inline documentation in <linux/firewire-cdev.h> for
    descriptions of all ioctls.

poll(2), select(2), epoll\_wait(2) etc.
:   Watch for events to become available to be read.

read(2)
:   Receive various events. There are solicited events like
    outbound asynchronous transaction completion or isochronous
    buffer completion, and unsolicited events such as bus resets,
    request reception, or PHY packet reception. Always use a read
    buffer which is large enough to receive the largest event that
    could ever arrive. See <linux/firewire-cdev.h> for descriptions
    of all event types and for which ioctls affect reception of
    events.

mmap(2)
:   Allocate a DMA buffer for isochronous reception or transmission
    and map it into the process address space. The arguments should
    be used as follows: addr = NULL, length = the desired buffer
    size, i.e. number of packets times size of largest packet,
    prot = at least PROT\_READ for reception and at least PROT\_WRITE
    for transmission, flags = MAP\_SHARED, fd = the handle to the
    /dev/fw\*, offset = 0.

Isochronous reception works in packet-per-buffer fashion except
for multichannel reception which works in buffer-fill mode.

munmap(2)
:   Unmap the isochronous I/O buffer from the process address space.

close(2)
:   Besides stopping and freeing I/O contexts that were associated
    with the file descriptor, back out any changes to the local
    nodes’ Configuration ROM. Deallocate isochronous channels and
    bandwidth at the IRM that were marked for kernel-assisted
    re- and deallocation.

Users:
:   libraw1394;
    libdc1394;
    libhinawa;
    tools like linux-firewire-utils, fwhack, ...

## Symbols under /sys/accessibility

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

In /sys/accessibility/speakup is a directory corresponding to
the synthesizer driver currently in use (E.G) soft for the
soft driver. This directory contains files which control the
speech synthesizer itself,
as opposed to controlling the speakup
screen reader. The parameters in this directory have the same
names and functions across all
supported synthesizers. The range
of values for freq, pitch, rate, and vol is the same for all
supported synthesizers, with the given range being internally
mapped by the driver to more or less fit the range of values
supported for a given parameter by the individual synthesizer.
Below is a description of values and parameters for soft
synthesizer, which is currently the most commonly used.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/caps\_start** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This is the string that is sent to the synthesizer to cause it
to start speaking uppercase letters. For the soft synthesizer
and most others, this causes the pitch of the voice to rise
above the currently set pitch.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/caps\_stop** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This is the string sent to the synthesizer to cause it to stop
speaking uppercase letters. In the case of the soft synthesizer
and most others, this returns the pitch of the voice
down to the
currently set pitch.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/delay\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/direct** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Controls if punctuation is spoken by speakup, or by the
synthesizer.

For example, speakup speaks “>” as “greater”, while
the espeak synthesizer used by the soft driver speaks “greater
than”. Zero lets speakup speak the punctuation. One lets the
synthesizer itself speak punctuation.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/flush\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the timeout to wait for the synthesizer flush to
complete. This can be used when the cable gets faulty and flush
notifications are getting lost.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/freq** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the frequency of the speech synthesizer. Range is
0-9.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/full\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/inflection** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the inflection of the synthesizer, i.e. the pitch
range. The range is 0-9.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/jiffy\_delta** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This controls how many jiffys the kernel gives to the
synthesizer. Setting this too high can make a system unstable,
or even crash it.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/pitch** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the pitch of the synthesizer. The range is 0-9.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/punct** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the amount of punctuation spoken by the
synthesizer. The range for the soft driver seems to be 0-2.
TODO: How is this related to speakup’s punc\_level, or
reading\_punc.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/rate** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the rate of the synthesizer. Range is from zero
slowest, to nine fastest.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/tone** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the tone of the speech synthesizer. The range for
the soft driver seems to be 0-2. This seems to make no
difference if using espeak and the espeakup connector.
TODO: does espeakup support different tonalities?

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/trigger\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/voice** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the voice used by the synthesizer if the
synthesizer can speak in more than one voice. The range for the
soft driver is 0-7. Note that while espeak supports multiple
voices, this parameter will not set the voice when the espeakup
connector is used between speakup and espeak.

|  |
| --- |
| **/sys/accessibility/speakup/<synth-name>/vol** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the volume of the speech synthesizer. Range is 0-9,
with zero being the softest, and nine being the loudest.

|  |
| --- |
| **/sys/accessibility/speakup/attrib\_bleep** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Beeps the PC speaker when there is an attribute change such as
foreground or background color when using speakup review
commands. One = on, zero = off.

|  |
| --- |
| **/sys/accessibility/speakup/bell\_pos** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This works much like a typewriter bell. If for example 72 is
echoed to bell\_pos, it will beep the PC speaker when typing on
a line past character 72.

|  |
| --- |
| **/sys/accessibility/speakup/bleep\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This controls the duration of the PC speaker beeps speakup
produces, in milliseconds.

|  |
| --- |
| **/sys/accessibility/speakup/bleeps** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This controls whether one hears beeps through the PC speaker
when using speakup’s review commands. Range: 0-3. 0 = off, 1 = beeps
only, 2 = announcements only, 3 = beeps and announcements (default).

|  |
| --- |
| **/sys/accessibility/speakup/cur\_phonetic** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This allows speakup to speak letters phoneticaly when arrowing through
a word letter by letter. This doesn’t affect the spelling when typing
the characters. When cur\_phonetic=1, speakup will speak characters
phoneticaly when arrowing over a letter. When cur\_phonetic=0, speakup
will speak letters as normally.

|  |
| --- |
| **/sys/accessibility/speakup/cursor\_time** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This controls cursor delay when using arrow keys. When a
connection is very slow, with the default setting, when moving
with the arrows, or backspacing etc. speakup says the incorrect
characters. Set this to a higher value to adjust for the delay
and better synchronisation between cursor position and speech.

|  |
| --- |
| **/sys/accessibility/speakup/delimiters** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Delimit a word from speakup.
TODO: add more info

|  |
| --- |
| **/sys/accessibility/speakup/ex\_num** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/i18n/announcements** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This file contains various general announcements, most of which
cannot be categorized. You will find messages such as “You
killed Speakup”, “I’m alive”, “leaving help”, “parked”,
“unparked”, and others. You will also find the names of the
screen edges and cursor tracking modes here.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/characters** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Through this sys entry, Speakup gives you the ability to change
how Speakup pronounces a given character. You could, for
example, change how some punctuation characters are spoken. You
can even change how Speakup will pronounce certain letters. For
further details see ‘12. Changing the Pronunciation of
Characters’ in Speakup User’s Guide (file spkguide.txt in
source).

|  |
| --- |
| **/sys/accessibility/speakup/i18n/chartab** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO

|  |
| --- |
| **/sys/accessibility/speakup/i18n/colors** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

When you use the “say attributes” function, Speakup says the
name of the foreground and background colors. These names come
from the i18n/colors file.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/ctl\_keys** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Here, you will find names of control keys. These are used with
Speakup’s say\_control feature.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/formatted** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This group of messages contains embedded formatting codes, to
specify the type and width of displayed data. If you change
these, you must preserve all of the formatting codes, and they
must appear in the order used by the default messages.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/function\_names** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Here, you will find a list of names for Speakup functions.
These are used by the help system. For example, suppose that
you have activated help mode, and you pressed
keypad 3. Speakup
says: “keypad 3 is character, say next.”
The message “character, say next” names a Speakup function, and
it comes from this function\_names file.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/key\_names** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Again, key\_names is used by Speakup’s help system. In the
previous example, Speakup said that you pressed “keypad 3.”
This name came from the key\_names file.

|  |
| --- |
| **/sys/accessibility/speakup/i18n/states** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This file contains names for key states.
Again, these are part of the help system. For instance, if you
had pressed speakup + keypad 3, you would hear:
“speakup keypad 3 is go to bottom edge.”

The speakup key is depressed, so the name of the key state is
speakup.

This part of the message comes from the states collection.

|  |
| --- |
| **/sys/accessibility/speakup/key\_echo** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Controls if speakup speaks keys when they are typed. One = on,
zero = off or don’t echo keys.

|  |
| --- |
| **/sys/accessibility/speakup/keymap** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Speakup keymap remaps keys to Speakup functions.
It uses a binary
format. A special program called genmap is needed to compile a
textual keymap into the binary format which is then loaded into
/sys/accessibility/speakup/keymap.

|  |
| --- |
| **/sys/accessibility/speakup/no\_interrupt** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Controls if typing interrupts output from speakup. With
no\_interrupt set to zero, typing on the keyboard will interrupt
speakup if for example
the say screen command is used before the
entire screen is read.

With no\_interrupt set to one, if the say
screen command is used, and one then types on the keyboard,
speakup will continue to say the whole screen regardless until
it finishes.

|  |
| --- |
| **/sys/accessibility/speakup/punc\_all** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This is a list of all the punctuation speakup should speak when
punc\_level is set to four.

|  |
| --- |
| **/sys/accessibility/speakup/punc\_level** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Controls the level of punctuation spoken as the screen is
displayed, not reviewed. Levels range from zero no punctuation,
to four, all punctuation. One corresponds to punc\_some, two
corresponds to punc\_most, and three as well as four both
correspond to punc\_all. Some hardware synthesizers may have
different levels each corresponding to three and four for
punc\_level. Also note that if punc\_level is set to zero, and
key\_echo is set to one, typed punctuation is still spoken as it
is typed.

|  |
| --- |
| **/sys/accessibility/speakup/punc\_most** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This is a list of all the punctuation speakup should speak when
punc\_level is set to two.

|  |
| --- |
| **/sys/accessibility/speakup/punc\_some** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This is a list of all the punctuation speakup should speak when
punc\_level is set to one.

|  |
| --- |
| **/sys/accessibility/speakup/reading\_punc** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Almost the same as punc\_level, the differences being that
reading\_punc controls the level of punctuation when reviewing
the screen with speakup’s screen review commands. The other
difference is that reading\_punc set to three speaks punc\_all,
and reading\_punc set to four speaks all punctuation, including
spaces.

|  |
| --- |
| **/sys/accessibility/speakup/repeats** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

A list of characters speakup repeats. Normally, when there are
more than three characters in a row, speakup
just reads three of
those characters. For example, “......” would be read as dot,
dot, dot. If a . is added to the list of characters in repeats,
“......” would be read as dot, dot, dot, times six.

|  |
| --- |
| **/sys/accessibility/speakup/say\_control** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

If set to one, speakup speaks shift, alt and control when those
keys are pressed. If say\_control is set to zero, shift, ctrl,
and alt are not spoken when they are pressed.

|  |
| --- |
| **/sys/accessibility/speakup/say\_word\_ctl** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/silent** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

TODO:

|  |
| --- |
| **/sys/accessibility/speakup/spell\_delay** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

This controls how fast a word is spelled
when speakup’s say word
review command is pressed twice quickly to speak the current
word being reviewed. Zero just speaks the letters one after
another, while values one through four
seem to introduce more of
a pause between the spelling of each letter by speakup.

|  |
| --- |
| **/sys/accessibility/speakup/synth** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Gets or sets the synthesizer driver currently in use. Reading
synth returns the synthesizer driver currently in use. Writing
synth switches to the given synthesizer driver, provided it is
either built into the kernel, or already loaded as a module.

|  |
| --- |
| **/sys/accessibility/speakup/synth\_direct** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Sends whatever is written to synth\_direct
directly to the speech synthesizer in use, bypassing speakup.
This could be used to make the synthesizer speak
a string, or to
send control sequences to the synthesizer to change how the
synthesizer behaves.

|  |
| --- |
| **/sys/accessibility/speakup/version** |

Defined on file [sysfs-driver-speakup](abi-stable-files.html#abi-file-stable-sysfs-driver-speakup)

Reading version returns the version of speakup, and the version
of the synthesizer driver currently in use.

## Symbols under /sys/block

|  |
| --- |
| **/sys/block/<disk>/<partition>/alignment\_offset** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Storage devices may report a physical block size that is
bigger than the logical block size (for instance a drive
with 4KB physical sectors exposing 512-byte logical
blocks to the operating system). This parameter
indicates how many bytes the beginning of the partition
is offset from the disk’s natural alignment.

|  |
| --- |
| **/sys/block/<disk>/<partition>/discard\_alignment** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Devices that support discard functionality may
internally allocate space in units that are bigger than
the exported logical block size. The discard\_alignment
parameter indicates how many bytes the beginning of the
partition is offset from the internal allocation unit’s
natural alignment.

|  |
| --- |
| **/sys/block/<disk>/<partition>/stat** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

The /sys/block/<disk>/<partition>/stat files display the
I/O statistics of partition <partition>. The format is the
same as the format of /sys/block/<disk>/stat.

|  |
| --- |
| **/sys/block/<disk>/alignment\_offset** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Storage devices may report a physical block size that is
bigger than the logical block size (for instance a drive
with 4KB physical sectors exposing 512-byte logical
blocks to the operating system). This parameter
indicates how many bytes the beginning of the device is
offset from the disk’s natural alignment.

|  |
| --- |
| **/sys/block/<disk>/atomic\_write\_boundary\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] A device may need to internally split an atomic write I/O
which straddles a given logical block address boundary. This
parameter specifies the size in bytes of the atomic boundary if
one is reported by the device. This value must be a
power-of-two and at least the size as in
atomic\_write\_unit\_max\_bytes.
Any attempt to merge atomic write I/Os must not result in a
merged I/O which crosses this boundary (if any).

|  |
| --- |
| **/sys/block/<disk>/atomic\_write\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This parameter specifies the maximum atomic write
size reported by the device. This parameter is relevant
for merging of writes, where a merged atomic write
operation must not exceed this number of bytes.
This parameter may be greater than the value in
atomic\_write\_unit\_max\_bytes as
atomic\_write\_unit\_max\_bytes will be rounded down to a
power-of-two and atomic\_write\_unit\_max\_bytes may also be
limited by some other queue limits, such as max\_segments.
This parameter - along with atomic\_write\_unit\_min\_bytes
and atomic\_write\_unit\_max\_bytes - will not be larger than
max\_hw\_sectors\_kb, but may be larger than max\_sectors\_kb.

|  |
| --- |
| **/sys/block/<disk>/atomic\_write\_unit\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This parameter defines the largest block which can be
written atomically with an atomic write operation. This
value must be a multiple of atomic\_write\_unit\_min and must
be a power-of-two. This value will not be larger than
atomic\_write\_max\_bytes.

|  |
| --- |
| **/sys/block/<disk>/atomic\_write\_unit\_min\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This parameter specifies the smallest block which can
be written atomically with an atomic write operation. All
atomic write operations must begin at a
atomic\_write\_unit\_min boundary and must be multiples of
atomic\_write\_unit\_min. This value must be a power-of-two.

|  |
| --- |
| **/sys/block/<disk>/discard\_alignment** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Devices that support discard functionality may
internally allocate space in units that are bigger than
the exported logical block size. The discard\_alignment
parameter indicates how many bytes the beginning of the
device is offset from the internal allocation unit’s
natural alignment.

|  |
| --- |
| **/sys/block/<disk>/diskseq** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

The /sys/block/<disk>/diskseq files reports the disk
sequence number, which is a monotonically increasing
number assigned to every drive.
Some devices, like the loop device, refresh such number
every time the backing file is changed.
The value type is 64 bit unsigned.

|  |
| --- |
| **/sys/block/<disk>/hidden** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] the block device is hidden. it doesn’t produce events, and
can’t be opened from userspace or using blkdev\_get\*.
Used for the underlying components of multipath devices.

|  |
| --- |
| **/sys/block/<disk>/inflight** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Reports the number of I/O requests currently in progress
(pending / in flight) in a device driver. This can be less
than the number of requests queued in the block device queue.
The report contains 2 fields: one for read requests
and one for write requests.
The value type is unsigned int.
Cf. [Block layer statistics in /sys/block/<dev>/stat](../block/stat.html) which contains a single value for
requests in flight.
This is related to /sys/block/<disk>/queue/nr\_requests
and for SCSI device also its queue\_depth.

|  |
| --- |
| **/sys/block/<disk>/integrity/device\_is\_integrity\_capable** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Indicates whether a storage device is capable of storing
integrity metadata. Set if the device is T10 PI-capable.
This flag is set to 1 if the storage media is formatted
with T10 Protection Information. If the storage media is
not formatted with T10 Protection Information, this flag
is set to 0.

|  |
| --- |
| **/sys/block/<disk>/integrity/format** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Metadata format for integrity capable block device.
E.g. T10-DIF-TYPE1-CRC.
This field describes the type of T10 Protection Information
that the block device can send and receive.
If the device can store application integrity metadata but
no T10 Protection Information profile is used, this field
contains “nop”.
If the device does not support integrity metadata, this
field contains “none”.

|  |
| --- |
| **/sys/block/<disk>/integrity/protection\_interval\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Describes the number of data bytes which are protected
by one integrity tuple. Typically the device’s logical
block size.

|  |
| --- |
| **/sys/block/<disk>/integrity/read\_verify** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Indicates whether the block layer should verify the
integrity of read requests serviced by devices that
support sending integrity metadata.

|  |
| --- |
| **/sys/block/<disk>/integrity/tag\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Number of bytes of integrity tag space available per
protection\_interval\_bytes, which is typically
the device’s logical block size.
This field describes the size of the application tag
if the storage device is formatted with T10 Protection
Information and permits use of the application tag.
The tag\_size is reported in bytes and indicates the
space available for adding an opaque tag to each block
(protection\_interval\_bytes).
If the device does not support T10 Protection Information
(even if the device provides application integrity
metadata space), this field is set to 0.

|  |
| --- |
| **/sys/block/<disk>/integrity/write\_generate** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Indicates whether the block layer should automatically
generate checksums for write requests bound for
devices that support receiving integrity metadata.

|  |
| --- |
| **/sys/block/<disk>/partscan** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

The /sys/block/<disk>/partscan files reports if partition
scanning is enabled for the disk. It returns “1” if partition
scanning is enabled, or “0” if not. The value type is a 32-bit
unsigned integer, but only “0” and “1” are valid values.

|  |
| --- |
| **/sys/block/<disk>/queue/add\_random** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This file allows to turn off the disk entropy contribution.
Default value of this file is ‘1’(on).

|  |
| --- |
| **/sys/block/<disk>/queue/async\_depth** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] Controls how many asynchronous requests may be allocated
in the block layer. The value is always capped at nr\_requests.

When no elevator is active (none):

* async\_depth is always equal to nr\_requests.

For bfq scheduler:

* By default, async\_depth is set to 75% of nr\_requests.
  Internal limits are then derived from this value:

  + Sync writes: limited to async\_depth (≈75% of nr\_requests).
  + Async I/O: limited to ~2/3 of async\_depth (≈50% of
    nr\_requests).

  If a bfq\_queue is weight-raised:

  + Sync writes: limited to ~1/2 of async\_depth (≈37% of
    nr\_requests).
  + Async I/O: limited to ~1/4 of async\_depth (≈18% of
    nr\_requests).
* If the user writes a custom value to async\_depth, BFQ will
  recompute these limits proportionally based on the new value.

For Kyber:

* By default async\_depth is set to 75% of nr\_requests.
* If the user writes a custom value to async\_depth, then it
  overrides the default and directly controls the limit for
  writes and async I/O.

For mq-deadline:

* By default async\_depth is set to nr\_requests.
* If the user writes a custom value to async\_depth, then it
  overrides the default and directly controls the limit for
  writes and async I/O.

|  |
| --- |
| **/sys/block/<disk>/queue/chunk\_sectors** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] chunk\_sectors has different meaning depending on the type
of the disk. For a RAID device (dm-raid), chunk\_sectors
indicates the size in 512B sectors of the RAID volume stripe
segment. For a zoned block device, either host-aware or
host-managed, chunk\_sectors indicates the size in 512B sectors
of the zones of the device, with the eventual exception of the
last zone of the device which may be smaller.

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

The presence of this subdirectory of /sys/block/<disk>/queue/
indicates that the device supports inline encryption. This
subdirectory contains files which describe the inline encryption
capabilities of the device. For more information about inline
encryption, refer to [Inline Encryption](../block/inline-encryption.html).

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/hw\_wrapped\_keys** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] The presence of this file indicates that the device
supports hardware-wrapped inline encryption keys, i.e. key blobs
that can only be unwrapped and used by dedicated hardware. For
more information about hardware-wrapped inline encryption keys,
see [Inline Encryption](../block/inline-encryption.html).

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/max\_dun\_bits** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This file shows the maximum length, in bits, of data unit
numbers accepted by the device in inline encryption requests.

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/modes/<mode>** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] For each crypto mode (i.e., encryption/decryption
algorithm) the device supports with inline encryption, a file
will exist at this location. It will contain a hexadecimal
number that is a bitmask of the supported data unit sizes, in
bytes, for that crypto mode.

Currently, the crypto modes that may be supported are:

> * AES-256-XTS
> * AES-128-CBC-ESSIV
> * Adiantum

For example, if a device supports AES-256-XTS inline encryption
with data unit sizes of 512 and 4096 bytes, the file
/sys/block/<disk>/queue/crypto/modes/AES-256-XTS will exist and
will contain “0x1200”.

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/num\_keyslots** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This file shows the number of keyslots the device has for
use with inline encryption.

|  |
| --- |
| **/sys/block/<disk>/queue/crypto/raw\_keys** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] The presence of this file indicates that the device
supports raw inline encryption keys, i.e. keys that are managed
in raw, plaintext form in software.

|  |
| --- |
| **/sys/block/<disk>/queue/dax** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This file indicates whether the device supports Direct
Access (DAX), used by CPU-addressable storage to bypass the
pagecache. It shows ‘1’ if true, ‘0’ if not.

|  |
| --- |
| **/sys/block/<disk>/queue/discard\_granularity** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Devices that support discard functionality may internally
allocate space using units that are bigger than the logical
block size. The discard\_granularity parameter indicates the size
of the internal allocation unit in bytes if reported by the
device. Otherwise the discard\_granularity will be set to match
the device’s physical block size. A discard\_granularity of 0
means that the device does not support discard functionality.

|  |
| --- |
| **/sys/block/<disk>/queue/discard\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] While discard\_max\_hw\_bytes is the hardware limit for the
device, this setting is the software limit. Some devices exhibit
large latencies when large discards are issued, setting this
value lower will make Linux issue smaller discards and
potentially help reduce latencies induced by large discard
operations.

|  |
| --- |
| **/sys/block/<disk>/queue/discard\_max\_hw\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Devices that support discard functionality may have
internal limits on the number of bytes that can be trimmed or
unmapped in a single operation. The discard\_max\_hw\_bytes
parameter is set by the device driver to the maximum number of
bytes that can be discarded in a single operation. Discard
requests issued to the device must not exceed this limit. A
discard\_max\_hw\_bytes value of 0 means that the device does not
support discard functionality.

|  |
| --- |
| **/sys/block/<disk>/queue/discard\_zeroes\_data** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Will always return 0. Don’t rely on any specific behavior
for discards, and don’t read this file.

|  |
| --- |
| **/sys/block/<disk>/queue/dma\_alignment** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

Reports the alignment that user space addresses must have to be
used for raw block device access with O\_DIRECT and other driver
specific passthrough mechanisms.

|  |
| --- |
| **/sys/block/<disk>/queue/fua** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Whether or not the block driver supports the FUA flag for
write requests. FUA stands for Force Unit Access. If the FUA
flag is set that means that write requests must bypass the
volatile cache of the storage device.

|  |
| --- |
| **/sys/block/<disk>/queue/hw\_sector\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This is the hardware sector size of the device, in bytes.

|  |
| --- |
| **/sys/block/<disk>/queue/independent\_access\_ranges/** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] The presence of this sub-directory of the
/sys/block/xxx/queue/ directory indicates that the device is
capable of executing requests targeting different sector ranges
in parallel. For instance, single LUN multi-actuator hard-disks
will have an independent\_access\_ranges directory if the device
correctly advertises the sector ranges of its actuators.

The independent\_access\_ranges directory contains one directory
per access range, with each range described using the sector
(RO) attribute file to indicate the first sector of the range
and the nr\_sectors (RO) attribute file to indicate the total
number of sectors in the range starting from the first sector of
the range. For example, a dual-actuator hard-disk will have the
following independent\_access\_ranges entries.:

```
$ tree /sys/block/<disk>/queue/independent_access_ranges/
/sys/block/<disk>/queue/independent_access_ranges/
|-- 0
|   |-- nr_sectors
|   `-- sector
`-- 1
    |-- nr_sectors
    `-- sector
```

The sector and nr\_sectors attributes use 512B sector unit,
regardless of the actual block size of the device. Independent
access ranges do not overlap and include all sectors within the
device capacity. The access ranges are numbered in increasing
order of the range start sector, that is, the sector attribute
of range 0 always has the value 0.

|  |
| --- |
| **/sys/block/<disk>/queue/io\_poll** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] When read, this file shows whether polling is enabled (1)
or disabled (0). Writing ‘0’ to this file will disable polling
for this device. Writing any non-zero value will enable this
feature.

|  |
| --- |
| **/sys/block/<disk>/queue/io\_poll\_delay** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This was used to control what kind of polling will be
performed. It is now fixed to -1, which is classic polling.
In this mode, the CPU will repeatedly ask for completions
without giving up any time.
<deprecated>

|  |
| --- |
| **/sys/block/<disk>/queue/io\_timeout** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] io\_timeout is the request timeout in milliseconds. If a
request does not complete in this time then the block driver
timeout handler is invoked. That timeout handler can decide to
retry the request, to fail it or to start a device recovery
strategy.

|  |
| --- |
| **/sys/block/<disk>/queue/iostats** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This file is used to control (on/off) the iostats
accounting of the disk.

|  |
| --- |
| **/sys/block/<disk>/queue/iostats\_passthrough** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This file is used to control (on/off) the iostats
accounting of the disk for passthrough commands.

|  |
| --- |
| **/sys/block/<disk>/queue/logical\_block\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This is the smallest unit the storage device can address.
It is typically 512 bytes.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_active\_zones** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] For zoned block devices (zoned attribute indicating
“host-managed” or “host-aware”), the sum of zones belonging to
any of the zone states: EXPLICIT OPEN, IMPLICIT OPEN or CLOSED,
is limited by this value. If this value is 0, there is no limit.

If the host attempts to exceed this limit, the driver should
report this error with BLK\_STS\_ZONE\_ACTIVE\_RESOURCE, which user
space may see as the EOVERFLOW errno.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_discard\_segments** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] The maximum number of DMA scatter/gather entries in a
discard request.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_hw\_sectors\_kb** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This is the maximum number of kilobytes supported in a
single data transfer.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_integrity\_segments** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Maximum number of elements in a DMA scatter/gather list
with integrity data that will be submitted by the block layer
core to the associated block driver.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_open\_zones** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] For zoned block devices (zoned attribute indicating
“host-managed” or “host-aware”), the sum of zones belonging to
any of the zone states: EXPLICIT OPEN or IMPLICIT OPEN, is
limited by this value. If this value is 0, there is no limit.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_sectors\_kb** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This is the maximum number of kilobytes that the block
layer will allow for a filesystem request. Must be smaller than
or equal to the maximum size allowed by the hardware. Write 0
to use default kernel settings.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_segment\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Maximum size in bytes of a single element in a DMA
scatter/gather list.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_segments** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Maximum number of elements in a DMA scatter/gather list
that is submitted to the associated block driver.

|  |
| --- |
| **/sys/block/<disk>/queue/max\_write\_streams** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Maximum number of write streams supported, 0 if not
supported. If supported, valid values are 1 through
max\_write\_streams, inclusive.

|  |
| --- |
| **/sys/block/<disk>/queue/minimum\_io\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Storage devices may report a granularity or preferred
minimum I/O size which is the smallest request the device can
perform without incurring a performance penalty. For disk
drives this is often the physical block size. For RAID arrays
it is often the stripe chunk size. A properly aligned multiple
of minimum\_io\_size is the preferred request size for workloads
where a high number of I/O operations is desired.

|  |
| --- |
| **/sys/block/<disk>/queue/nomerges** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] Standard I/O elevator operations include attempts to merge
contiguous I/Os. For known random I/O loads these attempts will
always fail and result in extra cycles being spent in the
kernel. This allows one to turn off this behavior on one of two
ways: When set to 1, complex merge checks are disabled, but the
simple one-shot merges with the previous I/O request are
enabled. When set to 2, all merge tries are disabled. The
default value is 0 - which enables all types of merge tries.

|  |
| --- |
| **/sys/block/<disk>/queue/nr\_requests** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This controls how many requests may be allocated in the
block layer. Noted this value only represents the quantity for a
single blk\_mq\_tags instance. The actual number for the entire
device depends on the hardware queue count, whether elevator is
enabled, and whether tags are shared.

|  |
| --- |
| **/sys/block/<disk>/queue/nr\_zones** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] nr\_zones indicates the total number of zones of a zoned
block device (“host-aware” or “host-managed” zone model). For
regular block devices, the value is always 0.

|  |
| --- |
| **/sys/block/<disk>/queue/optimal\_io\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Storage devices may report an optimal I/O size, which is
the device’s preferred unit for sustained I/O. This is rarely
reported for disk drives. For RAID arrays it is usually the
stripe width or the internal track size. A properly aligned
multiple of optimal\_io\_size is the preferred request size for
workloads where sustained throughput is desired. If no optimal
I/O size is reported this file contains 0.

|  |
| --- |
| **/sys/block/<disk>/queue/physical\_block\_size** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This is the smallest unit a physical storage device can
write atomically. It is usually the same as the logical block
size but may be bigger. One example is SATA drives with 4KB
sectors that expose a 512-byte logical block size to the
operating system. For stacked block devices the
physical\_block\_size variable contains the maximum
physical\_block\_size of the component devices.

|  |
| --- |
| **/sys/block/<disk>/queue/read\_ahead\_kb** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] Maximum number of kilobytes to read-ahead for filesystems
on this block device.

For MADV\_HUGEPAGE, the readahead size may exceed this setting
since its granularity is based on the hugepage size.

|  |
| --- |
| **/sys/block/<disk>/queue/rotational** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This file is used to stat if the device is of rotational
type or non-rotational type.

|  |
| --- |
| **/sys/block/<disk>/queue/rq\_affinity** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] If this option is ‘1’, the block layer will migrate request
completions to the cpu “group” that originally submitted the
request. For some workloads this provides a significant
reduction in CPU cycles due to caching effects.

For storage configurations that need to maximize distribution of
completion processing setting this option to ‘2’ forces the
completion to run on the requesting cpu (bypassing the “group”
aggregation logic).

|  |
| --- |
| **/sys/block/<disk>/queue/scheduler** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] When read, this file will display the current and available
IO schedulers for this block device. The currently active IO
scheduler will be enclosed in [] brackets. Writing an IO
scheduler name to this file will switch control of this block
device to that new IO scheduler. Note that writing an IO
scheduler name to this file will attempt to load that IO
scheduler module, if it isn’t already present in the system.

|  |
| --- |
| **/sys/block/<disk>/queue/stable\_writes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] This file will contain ‘1’ if memory must not be modified
while it is being used in a write request to this device. When
this is the case and the kernel is performing writeback of a
page, the kernel will wait for writeback to complete before
allowing the page to be modified again, rather than allowing
immediate modification as is normally the case. This
restriction arises when the device accesses the memory multiple
times where the same data must be seen every time -- for
example, once to calculate a checksum and once to actually write
the data. If no such restriction exists, this file will contain
‘0’. This file is writable for testing purposes.

|  |
| --- |
| **/sys/block/<disk>/queue/virt\_boundary\_mask** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This file shows the I/O segment memory alignment mask for
the block device. I/O requests to this device will be split
between segments wherever either the memory address of the end
of the previous segment or the memory address of the beginning
of the current segment is not aligned to virt\_boundary\_mask + 1
bytes.

|  |
| --- |
| **/sys/block/<disk>/queue/wbt\_lat\_usec** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] If the device is registered for writeback throttling, then
this file shows the target minimum read latency. If this latency
is exceeded in a given window of time (see curr\_win\_nsec), then
the writeback throttling will start scaling back writes. Writing
a value of ‘0’ to this file disables the feature. Writing a
value of ‘-1’ to this file resets the value to the default
setting.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_cache** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] When read, this file will display whether the device has
write back caching enabled or not. It will return “write back”
for the former case, and “write through” for the latter. Writing
to this file can change the kernels view of the device, but it
doesn’t alter the device state. This means that it might not be
safe to toggle the setting from “write back” to “write through”,
since that will also eliminate cache flushes issued by the
kernel.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_same\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Some devices support a write same operation in which a
single data block can be written to a range of several
contiguous blocks on storage. This can be used to wipe areas on
disk or to initialize drives in a RAID configuration.
write\_same\_max\_bytes indicates how many bytes can be written in
a single write same command. If write\_same\_max\_bytes is 0, write
same is not supported by the device.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_stream\_granularity** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Granularity of a write stream in bytes. The granularity
of a write stream is the size that should be discarded or
overwritten together to avoid write amplification in the device.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_zeroes\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] Devices that support write zeroes operation in which a
single request can be issued to zero out the range of contiguous
blocks on storage without having any payload in the request.
This can be used to optimize writing zeroes to the devices.
write\_zeroes\_max\_bytes indicates how many bytes can be written
in a single write zeroes command. If write\_zeroes\_max\_bytes is
0, write zeroes is not supported by the device.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_zeroes\_unmap\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] While write\_zeroes\_unmap\_max\_hw\_bytes is the hardware limit
for the device, this setting is the software limit. Since the
unmap write zeroes operation is a best-effort optimization, some
devices may still physically writing zeroes to media. So the
speed of this operation is not guaranteed. Writing a value of
‘0’ to this file disables this operation. Otherwise, this
parameter should be equal to write\_zeroes\_unmap\_max\_hw\_bytes.

|  |
| --- |
| **/sys/block/<disk>/queue/write\_zeroes\_unmap\_max\_hw\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This file indicates whether a device supports zeroing data
in a specified block range without incurring the cost of
physically writing zeroes to the media for each individual
block. If this parameter is set to write\_zeroes\_max\_bytes, the
device implements a zeroing operation which opportunistically
avoids writing zeroes to media while still guaranteeing that
subsequent reads from the specified block range will return
zeroed data. This operation is a best-effort optimization, a
device may fall back to physically writing zeroes to the media
due to other factors such as misalignment or being asked to
clear a block range smaller than the device’s internal
allocation unit. If this parameter is set to 0, the device may
have to write each logical block media during a zeroing
operation.

|  |
| --- |
| **/sys/block/<disk>/queue/zone\_append\_max\_bytes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This is the maximum number of bytes that can be written to
a sequential zone of a zoned block device using a zone append
write operation (REQ\_OP\_ZONE\_APPEND). This value is always 0 for
regular block devices.

|  |
| --- |
| **/sys/block/<disk>/queue/zone\_write\_granularity** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] This indicates the alignment constraint, in bytes, for
write operations in sequential zones of zoned block devices
(devices with a zoned attributed that reports “host-managed” or
“host-aware”). This value is always 0 for regular block devices.

|  |
| --- |
| **/sys/block/<disk>/queue/zoned** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RO] zoned indicates if the device is a zoned block device and
the zone model of the device if it is indeed zoned. The
possible values indicated by zoned are “none” for regular block
devices and “host-aware” or “host-managed” for zoned block
devices. The characteristics of host-aware and host-managed
zoned block devices are described in the ZBC (Zoned Block
Commands) and ZAC (Zoned Device ATA Command Set) standards.
These standards also define the “drive-managed” zone model.
However, since drive-managed zoned block devices do not support
zone commands, they will be treated as regular block devices and
zoned will report “none”.

|  |
| --- |
| **/sys/block/<disk>/queue/zoned\_qd1\_writes** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

[RW] zoned\_qd1\_writes indicates if write operations to a zoned
block device are being handled using a single issuer context (a
kernel thread) operating at a maximum queue depth of 1. This
attribute is visible only for zoned block devices. The default
value for zoned block devices that are not rotational devices
(e.g. ZNS SSDs or zoned UFS devices) is 0. For rotational zoned
block devices (e.g. SMR HDDs) the default value is 1. Since
this default may not be appropriate for some devices, e.g.
remotely connected devices over high latency networks, the user
can disable this feature by setting this attribute to 0.

|  |
| --- |
| **/sys/block/<disk>/stat** |

Defined on file [sysfs-block](abi-stable-files.html#abi-file-stable-sysfs-block)

The /sys/block/<disk>/stat files displays the I/O
statistics of disk <disk>. They contain 11 fields:

|  |  |
| --- | --- |
| 1 | reads completed successfully |
| 2 | reads merged |
| 3 | sectors read |
| 4 | time spent reading (ms) |
| 5 | writes completed |
| 6 | writes merged |
| 7 | sectors written |
| 8 | time spent writing (ms) |
| 9 | I/Os currently in progress |
| 10 | time spent doing I/Os (ms) |
| 11 | weighted time spent doing I/Os (ms) |
| 12 | discards completed |
| 13 | discards merged |
| 14 | sectors discarded |
| 15 | time spent discarding (ms) |
| 16 | flush requests completed |
| 17 | time spent flushing (ms) |

For more details refer [I/O statistics fields](iostats.html)

## Symbols under /sys/bus

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/cdev\_major** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The major number that the character device driver assigned to
this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/cmd\_status** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The last executed device administrative command’s status/error.
Also last configuration error overloaded.
Writing to it will clear the status.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/configurable** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

To indicate if this device is configurable or not.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/dsacaps** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The DSA3 specification introduces three new capability
registers: dsacap[0-2]. User components (e.g., configuration
libraries and workload applications) require this information
to properly utilize the DSA3 features.
This includes SGL capability support, Enabling hardware-specific
optimizations, Configuring memory, etc.
The output format is ‘<dsacap2>,<dsacap1>,<dsacap0>’ where each
DSA cap value is a 64 bit hex value.
This attribute should only be visible on DSA devices of version
3 or later.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/engine<m>.<n>** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The assigned engine under this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/errors** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The error information for this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/event\_log\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The event log size to be configured. Default is 64 entries and
occupies 4k size if the evl entry is 64 bytes. It’s visible
only on platforms that support the capability.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/group<m>.<n>** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The assigned group under this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/iaa\_cap** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

IAA (IAX) capability mask. Exported to user space for application
consumption. This attribute should only be visible on IAA devices
that are version 2 or later.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_batch\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The largest number of work descriptors in a batch.
It’s not visible when the device does not support batch.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_engines** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The maximum number of engines supported by this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_groups** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The maximum number of groups can be created under this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_read\_buffers** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The total number of read buffers supported by this device.
The read buffers represent resources within the DSA
implementation, and these resources are allocated by engines to
support operations. See DSA spec v1.2 9.2.4 Total Read Buffers.
It’s not visible when the device does not support Read Buffer
allocation control.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_transfer\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The number of bytes to be read from the source address to
perform the operation. The maximum transfer size is dependent on
the workqueue the descriptor was submitted to.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_work\_queues** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The maximum work queue number that this device supports.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/max\_work\_queues\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The maximum work queue size supported by this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/numa\_node** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The numa node number for this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/op\_cap** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The operation capability bit mask specify the operation types
supported by the this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/pasid\_enabled** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

To indicate if user PASID (process address space identifier) is
enabled or not for this device.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/read\_buffer\_limit** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The maximum number of read buffers that may be in use at
one time by operations that access low bandwidth memory in the
device. See DSA spec v1.2 9.2.8 GENCFG on Global Read Buffer Limit.
It’s not visible when the device does not support Read Buffer
allocation control.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/state** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The state information of this device. It can be either enabled
or disabled.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/version** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The hardware version number.

|  |
| --- |
| **/sys/bus/dsa/devices/dsa<m>/wq<m>.<n>** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The assigned work queue under this device.

|  |
| --- |
| **/sys/bus/dsa/devices/engine<m>.<n>/group\_id** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The group that this engine belongs to.

|  |
| --- |
| **/sys/bus/dsa/devices/group<m>.<n>/batch\_progress\_limit** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Allows control of the number of batch descriptors that can be
concurrently processed by an engine in the group as a fraction
of the Maximum Batch Descriptors in Progress value specified in
the ENGCAP register. The acceptable values are 0 (default),
1 (1/2 of max value), 2 (1/4 of the max value), and 3 (1/8 of
the max value). It’s visible only on platforms that support
the capability.

|  |
| --- |
| **/sys/bus/dsa/devices/group<m>.<n>/desc\_progress\_limit** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Allows control of the number of work descriptors that can be
concurrently processed by an engine in the group as a fraction
of the Maximum Work Descriptors in Progress value specified in
the ENGCAP register. The acceptable values are 0 (default),
1 (1/2 of max value), 2 (1/4 of the max value), and 3 (1/8 of
the max value). It’s visible only on platforms that support
the capability.

|  |
| --- |
| **/sys/bus/dsa/devices/group<m>.<n>/read\_buffers\_allowed** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Indicates max number of read buffers that may be in use at one time
by all engines in the group. See DSA spec v1.2 9.2.18 GRPCFG Read
Buffers Allowed.
It’s not visible when the device does not support Read Buffer
allocation control.

|  |
| --- |
| **/sys/bus/dsa/devices/group<m>.<n>/read\_buffers\_reserved** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Indicates the number of Read Buffers reserved for the use of
engines in the group. See DSA spec v1.2 9.2.18 GRPCFG Read Buffers
Reserved.
It’s not visible when the device does not support Read Buffer
allocation control.

|  |
| --- |
| **/sys/bus/dsa/devices/group<m>.<n>/use\_read\_buffer\_limit** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Enable the use of global read buffer limit for the group. See DSA
spec v1.2 9.2.18 GRPCFG Use Global Read Buffer Limit.
It’s not visible when the device does not support Read Buffer
allocation control.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/ats\_disable** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Indicate whether ATS disable is turned on for the workqueue.
0 indicates ATS is on, and 1 indicates ATS is off for the workqueue.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/block\_on\_fault** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

To indicate block on fault is allowed or not for the work queue
to support on demand paging.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/cdev\_minor** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The minor number assigned to this work queue by the character
device driver.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/driver\_name** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Name of driver to be bounded to the wq.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/cr\_fault\_failures** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Show the number of Completion Record (CR) faults failures that this
application has caused. The failure counter is incremented when the
driver cannot fault in the address for the CR. Typically this is caused
by a bad address programmed in the submitted descriptor or a malicious
submitter is using bad CR address on purpose.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/cr\_faults** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Show the number of Completion Record (CR) faults this application
has caused.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/pid** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Show the process id of the application that opened the file. This is
helpful information for a monitor daemon that wants to kill the
application that opened the file.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/enqcmds\_retries** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Indicate the number of retires for an enqcmds submission on a sharedwq.
A max value to set attribute is capped at 64.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/group\_id** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The group id that this work queue belongs to.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/max\_batch\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The max batch size for this workqueue. Cannot exceed device
max batch size. Configurable parameter.
It’s not visible when the device does not support batch.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/max\_transfer\_size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The max transfer sized for this workqueue. Cannot exceed device
max transfer size. Configurable parameter.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/mode** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The work queue mode type for this work queue.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/occupancy** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Show the current number of entries in this WQ if WQ Occupancy
Support bit WQ capabilities is 1.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/op\_config** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Shows the operation capability bits displayed in bitmap format
presented by %\*pb [`printk()`](../core-api/printk-basics.html#c.printk "printk") output format specifier.
The attribute can be configured when the WQ is disabled in
order to configure the WQ to accept specific bits that
correlates to the operations allowed. It’s visible only
on platforms that support the capability.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/priority** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The priority value of this work queue, it is a value relative to
other work queue in the same group to control quality of service
for dispatching work from multiple workqueues in the same group.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/prs\_disable** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

Controls whether PRS disable is turned on for the workqueue.
0 indicates PRS is on, and 1 indicates PRS is off for the
workqueue. This option overrides block\_on\_fault attribute
if set. It’s visible only on platforms that support the
capability.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/size** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The work queue size for this work queue.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/state** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The current state of the work queue.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/threshold** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The number of entries in this work queue that may be filled
via a limited portal.

|  |
| --- |
| **/sys/bus/dsa/devices/wq<m>.<n>/type** |

Defined on file [sysfs-driver-dma-idxd](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-idxd)

The type of this work queue, it can be “kernel” type for work
queue usages in the kernel space or “user” type for work queue
usages by applications in user space.

|  |
| --- |
| **/sys/bus/firewire/devices/\*/** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

Attributes common to IEEE 1394 node devices and unit devices.
Read-only. Mutable during the node device’s lifetime.
Immutable during the unit device’s lifetime.
See IEEE 1212 for semantic definitions.

These attributes are only created if the root directory of an
IEEE 1394 node or the unit directory of an IEEE 1394 unit
actually contains according entries.

hardware\_version
:   Hexadecimal string representation of an u24.

hardware\_version\_name
:   Contents of a respective textual descriptor leaf.

model
:   Hexadecimal string representation of an u24.

model\_name
:   Contents of a respective textual descriptor leaf.

specifier\_id
:   Hexadecimal string representation of an u24.
    Mandatory in unit directories according to IEEE 1212.

vendor
:   Hexadecimal string representation of an u24.
    Mandatory in the root directory according to IEEE 1212.

vendor\_name
:   Contents of a respective textual descriptor leaf.

version
:   Hexadecimal string representation of an u24.
    Mandatory in unit directories according to IEEE 1212.

|  |
| --- |
| **/sys/bus/firewire/devices/fw[0-9]+/** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

IEEE 1394 node device attributes.
Read-only. Mutable during the node device’s lifetime.
See IEEE 1212 for semantic definitions.

config\_rom
:   Contents of the Configuration ROM register.
    Binary attribute; an array of host-endian u32.

guid
:   The node’s EUI-64 in the bus information block of
    Configuration ROM.
    Hexadecimal string representation of an u64.

|  |
| --- |
| **/sys/bus/firewire/devices/fw[0-9]+/is\_local** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

IEEE 1394 node device attribute.
Read-only and immutable.
Values: 1: The sysfs entry represents a local node (a controller card).

> 0: The sysfs entry represents a remote node.

|  |
| --- |
| **/sys/bus/firewire/devices/fw[0-9]+/units** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

IEEE 1394 node device attribute.
Read-only. Mutable during the node device’s lifetime.
See IEEE 1212 for semantic definitions.

units
:   Summary of all units present in an IEEE 1394 node.
    Contains space-separated tuples of specifier\_id and
    version of each unit present in the node. Specifier\_id
    and version are hexadecimal string representations of
    u24 of the respective unit directory entries.
    Specifier\_id and version within each tuple are separated
    by a colon.

Users:
:   udev rules to set ownership and access permissions or ACLs of
    /dev/fw[0-9]+ character device files

|  |
| --- |
| **/sys/bus/firewire/devices/fw[0-9]+[.][0-9]+/** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

IEEE 1394 unit device attributes.
Read-only. Immutable during the unit device’s lifetime.
See IEEE 1212 for semantic definitions.

modalias
:   Same as MODALIAS in the uevent at device creation.

rom\_index
:   Offset of the unit directory within the parent device’s
    (node device’s) Configuration ROM, in quadlets.
    Decimal string representation.

|  |
| --- |
| **/sys/bus/firewire/drivers/sbp2/fw\*/host\*/target\*/\*:\*:\*:\*/ieee1394\_id** |
| **formerly** |
| **/sys/bus/ieee1394/drivers/sbp2/fw\*/host\*/target\*/\*:\*:\*:\*/ieee1394\_id** |

Defined on file [sysfs-bus-firewire](abi-stable-files.html#abi-file-stable-sysfs-bus-firewire)

SCSI target port identifier and logical unit identifier of a
logical unit of an SBP-2 target. The identifiers are specified
in SAM-2...SAM-4 annex A. They are persistent and world-wide
unique properties the SBP-2 attached target.

Read-only attribute, immutable during the target’s lifetime.
Format, as exposed by firewire-sbp2 since 2.6.22, May 2007:
Colon-separated hexadecimal string representations of

> u64 EUI-64 : u24 directory\_ID : u16 LUN

without 0x prefixes, without whitespace. The former sbp2 driver
(removed in 2.6.37 after being superseded by firewire-sbp2) used
a somewhat shorter format which was not as close to SAM.

Users:
:   udev rules to create /dev/disk/by-id/ symlinks

|  |
| --- |
| **/sys/bus/fsl-mc/autorescan** |

Defined on file [sysfs-bus-fsl-mc](abi-stable-files.html#abi-file-stable-sysfs-bus-fsl-mc)

Writing a zero value to this attribute will
disable the DPRC IRQs on which automatic rescan
of the fsl-mc bus is performed. A non-zero value
will enable the DPRC IRQs.

Users:
:   Userspace drivers and management tools

|  |
| --- |
| **/sys/bus/fsl-mc/rescan** |

Defined on file [sysfs-bus-fsl-mc](abi-stable-files.html#abi-file-stable-sysfs-bus-fsl-mc)

Writing a non-zero value to this attribute will
force a rescan of fsl-mc bus in the system and
synchronize the objects under fsl-mc bus and the
Management Complex firmware.

Users:
:   Userspace drivers and management tools

|  |
| --- |
| **/sys/bus/mhi/devices/.../oem\_pk\_hash** |

Defined on file [sysfs-bus-mhi](abi-stable-files.html#abi-file-stable-sysfs-bus-mhi)

The file holds the OEM PK Hash value of the endpoint device
obtained using a BHI (Boot Host Interface) register read after
at least one attempt to power up the device has been done. If
read without having the device power on at least once, the file
will read all 0’s.

Users:
:   Any userspace application or clients interested in device info.

|  |
| --- |
| **/sys/bus/mhi/devices/.../serialnumber** |

Defined on file [sysfs-bus-mhi](abi-stable-files.html#abi-file-stable-sysfs-bus-mhi)

The file holds the serial number of the client device obtained
using a BHI (Boot Host Interface) register read after at least
one attempt to power up the device has been done. If read
without having the device power on at least once, the file will
read all 0’s.

Users:
:   Any userspace application or clients interested in device info.

|  |
| --- |
| **/sys/bus/mhi/devices/.../soc\_reset** |

Defined on file [sysfs-bus-mhi](abi-stable-files.html#abi-file-stable-sysfs-bus-mhi)

Initiates a SoC reset on the MHI controller. A SoC reset is
a reset of last resort, and will require a complete re-init.
This can be useful as a method of recovery if the device is
non-responsive, or as a means of loading new firmware as a
system administration task.

|  |
| --- |
| **/sys/bus/mhi/devices/.../trigger\_edl** |

Defined on file [sysfs-bus-mhi](abi-stable-files.html#abi-file-stable-sysfs-bus-mhi)

Writing a non-zero value to this file will force devices to
enter EDL (Emergency Download) mode. This entry only exists for
devices capable of entering the EDL mode using the standard EDL
triggering mechanism defined in the MHI spec v1.2. Once in EDL
mode, the flash programmer image can be downloaded to the
device to enter the flash programmer execution environment.
This can be useful if user wants to use QDL (Qualcomm Download,
which is used to download firmware over EDL) to update firmware.

|  |
| --- |
| **/sys/bus/nvmem/devices/.../force\_ro** |

Defined on file [sysfs-bus-nvmem](abi-stable-files.html#abi-file-stable-sysfs-bus-nvmem)

This read/write attribute allows users to set read-write
devices as read-only and back to read-write from userspace.
This can be used to unlock and relock write-protection of
devices which are generally locked, except during sporadic
programming operation.
Read returns ‘0’ or ‘1’ for read-write or read-only modes
respectively.
Write parses one of ‘YyTt1NnFf0’, or [oO][NnFf] for “on”
and “off”, i.e. what [`kstrtobool()`](../core-api/kernel-api.html#c.kstrtobool "kstrtobool") supports.
Note: This file is only present if CONFIG\_NVMEM\_SYSFS
is enabled.

|  |
| --- |
| **/sys/bus/nvmem/devices/.../nvmem** |

Defined on file [sysfs-bus-nvmem](abi-stable-files.html#abi-file-stable-sysfs-bus-nvmem)

This file allows user to read/write the raw NVMEM contents.
Permissions for write to this file depends on the nvmem
provider configuration.
Note: This file is only present if CONFIG\_NVMEM\_SYSFS
is enabled

ex:

```
hexdump /sys/bus/nvmem/devices/qfprom0/nvmem

0000000 0000 0000 0000 0000 0000 0000 0000 0000
*
00000a0 db10 2240 0000 e000 0c00 0c00 0000 0c00
0000000 0000 0000 0000 0000 0000 0000 0000 0000
...
*
0001000
```

|  |
| --- |
| **/sys/bus/nvmem/devices/.../type** |

Defined on file [sysfs-bus-nvmem](abi-stable-files.html#abi-file-stable-sysfs-bus-nvmem)

This read-only attribute allows user to read the NVMEM
device type. Supported types are “Unknown”, “EEPROM”,
“OTP”, “Battery backed”, “FRAM”.
Note: This file is only present if CONFIG\_NVMEM\_SYSFS
is enabled.

|  |
| --- |
| **/sys/bus/pci/drivers/qaic/XXXX:XX:XX.X/accel/accel<minor\_nr>/dbc<N>\_state** |

Defined on file [sysfs-driver-qaic](abi-stable-files.html#abi-file-stable-sysfs-driver-qaic)

Represents the current state of DMA Bridge channel (DBC). Below are the possible
states:

|  |  |
| --- | --- |
| IDLE (0) | DBC is free and can be activated |
| ASSIGNED (1) | DBC is activated and a workload is running on device |
| BEFORE\_SHUTDOWN (2) | Sub-system associated with this workload has crashed and it will shutdown soon |
| AFTER\_SHUTDOWN (3) | Sub-system associated with this workload has crashed and it has shutdown |
| BEFORE\_POWER\_UP (4) | Sub-system associated with this workload is shutdown and it will be powered up soon |
| AFTER\_POWER\_UP (5) | Sub-system associated with this workload is now powered up |

Users:
:   Any userspace application or clients interested in DBC state.

|  |
| --- |
| **/sys/bus/pci/drivers/qla2xxx/.../devices/\*** |

Defined on file [sysfs-driver-qla2xxx](abi-stable-files.html#abi-file-stable-sysfs-driver-qla2xxx)

qla2xxx-udev.sh currently looks for uevent CHANGE events to
signal a firmware-dump has been generated by the driver and is
ready for retrieval.

Users:
:   qla2xxx-udev.sh. Proposed changes should be mailed to
    [linux-driver@qlogic.com](mailto:linux-driver%40qlogic.com)

|  |
| --- |
| **/sys/bus/platform/drivers/aspeed-vuart/\*/lpc\_address** |

Defined on file [sysfs-driver-aspeed-vuart](abi-stable-files.html#abi-file-stable-sysfs-driver-aspeed-vuart)

Configures which IO port the host side of the UART
will appear on the host <-> BMC LPC bus.

Users:
:   OpenBMC. Proposed changes should be mailed to
    [openbmc@lists.ozlabs.org](mailto:openbmc%40lists.ozlabs.org)

|  |
| --- |
| **/sys/bus/platform/drivers/aspeed-vuart/\*/sirq** |

Defined on file [sysfs-driver-aspeed-vuart](abi-stable-files.html#abi-file-stable-sysfs-driver-aspeed-vuart)

Configures which interrupt number the host side of
the UART will appear on the host <-> BMC LPC bus.

Users:
:   OpenBMC. Proposed changes should be mailed to
    [openbmc@lists.ozlabs.org](mailto:openbmc%40lists.ozlabs.org)

|  |
| --- |
| **/sys/bus/platform/drivers/aspeed-vuart/\*/sirq\_polarity** |

Defined on file [sysfs-driver-aspeed-vuart](abi-stable-files.html#abi-file-stable-sysfs-driver-aspeed-vuart)

Configures the polarity of the serial interrupt to the
host via the BMC LPC bus.
Set to 0 for active-low or 1 for active-high.

Users:
:   OpenBMC. Proposed changes should be mailed to
    [openbmc@lists.ozlabs.org](mailto:openbmc%40lists.ozlabs.org)

|  |
| --- |
| **/sys/bus/usb/device/.../avoid\_reset\_quirk** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Writing 1 to this file tells the kernel that this
device will morph into another mode when it is reset.
Drivers will not use reset for error handling for
such devices.

Users:
:   usb\_modeswitch

|  |
| --- |
| **/sys/bus/usb/device/.../power/active\_duration** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

If CONFIG\_PM is enabled, then this file is present. When read,
it returns the total time (in msec) that the USB device has been
active, i.e. not in a suspended state. This file is read-only.

Tools can use this file and the connected\_duration file to
compute the percentage of time that a device has been active.
For example:

```
echo $((100 * `cat active_duration` / `cat connected_duration`))
```

will give an integer percentage. Note that this does not
account for counter wrap.

Users:
:   PowerTOP <[powertop@lists.01.org](mailto:powertop%40lists.01.org)>
    <https://01.org/powertop/>

|  |
| --- |
| **/sys/bus/usb/device/.../power/connected\_duration** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

If CONFIG\_PM is enabled, then this file is present. When read,
it returns the total time (in msec) that the USB device has been
connected to the machine. This file is read-only.

Users:
:   PowerTOP <[powertop@lists.01.org](mailto:powertop%40lists.01.org)>
    <https://01.org/powertop/>

|  |
| --- |
| **/sys/bus/usb/devices/.../bConfigurationValue** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

bConfigurationValue of the *active* configuration for the
device. Writing 0 or -1 to bConfigurationValue will reset the
active configuration (unconfigure the device). Writing
another value will change the active configuration.

Note that some devices, in violation of the USB spec, have a
configuration with a value equal to 0. Writing 0 to
bConfigurationValue for these devices will install that
configuration, rather then unconfigure the device.

Writing -1 will always unconfigure the device.

Users:
:   libusb

|  |
| --- |
| **/sys/bus/usb/devices/.../busnum** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Bus-number of the USB-bus the device is connected to.

Users:
:   libusb

|  |
| --- |
| **/sys/bus/usb/devices/.../descriptors** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Binary file containing cached descriptors of the device. The
binary data consists of the device descriptor followed by the
descriptors for each configuration of the device.
Note that the wTotalLength of the config descriptors can not
be trusted, as the device may have a smaller config descriptor
than it advertises. The bLength field of each (sub) descriptor
can be trusted, and can be used to seek forward one (sub)
descriptor at a time until the next config descriptor is found.
All descriptors read from this file are in bus-endian format

Users:
:   libusb

|  |
| --- |
| **/sys/bus/usb/devices/.../devnum** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Device address on the USB bus.

Users:
:   libusb

|  |
| --- |
| **/sys/bus/usb/devices/.../power/autosuspend** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Each USB device directory will contain a file named
power/autosuspend. This file holds the time (in seconds)
the device must be idle before it will be autosuspended.
0 means the device will be autosuspended as soon as
possible. Negative values will prevent the device from
being autosuspended at all, and writing a negative value
will resume the device if it is already suspended.

The autosuspend delay for newly-created devices is set to
the value of the usbcore.autosuspend module parameter.

|  |
| --- |
| **/sys/bus/usb/devices/.../power/persist** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

USB device directories can contain a file named power/persist.
The file holds a boolean value (0 or 1) indicating whether or
not the “USB-Persist” facility is enabled for the device. For
hubs this facility is always enabled and their device
directories will not contain this file.

For more information, see [USB device persistence during system suspend](../driver-api/usb/persist.html).

|  |
| --- |
| **/sys/bus/usb/devices/.../speed** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

Speed the device is connected with to the usb-host in
Mbit / second. IE one of 1.5 / 12 / 480 / 5000.

Users:
:   libusb

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<port[.port]>...:<config num>-<interface num>/supports\_autosuspend** |

Defined on file [sysfs-bus-usb](abi-stable-files.html#abi-file-stable-sysfs-bus-usb)

When read, this file returns 1 if the interface driver
for this interface supports autosuspend. It also
returns 1 if no driver has claimed this interface, as an
unclaimed interface will not stop the device from being
autosuspended if all other interface drivers are idle.
The file returns 0 if autosuspend support has not been
added to the driver.

Users:
:   USB PM tool
    git://git.moblin.org/users/sarah/usb-pm-tool/

|  |
| --- |
| **/sys/bus/usb/drivers/usbtmc/\*/interface\_capabilities** |
| **/sys/bus/usb/drivers/usbtmc/\*/device\_capabilities** |

Defined on file [sysfs-driver-usb-usbtmc](abi-stable-files.html#abi-file-stable-sysfs-driver-usb-usbtmc)

These files show the various USB TMC capabilities as described
by the device itself. The full description of the bitfields
can be found in the USB TMC documents from the USB-IF entitled
“Universal Serial Bus Test and Measurement Class Specification
(USBTMC) Revision 1.0” section 4.2.1.8.

The files are read only.

|  |
| --- |
| **/sys/bus/usb/drivers/usbtmc/\*/usb488\_interface\_capabilities** |
| **/sys/bus/usb/drivers/usbtmc/\*/usb488\_device\_capabilities** |

Defined on file [sysfs-driver-usb-usbtmc](abi-stable-files.html#abi-file-stable-sysfs-driver-usb-usbtmc)

These files show the various USB TMC capabilities as described
by the device itself. The full description of the bitfields
can be found in the USB TMC documents from the USB-IF entitled
“Universal Serial Bus Test and Measurement Class, Subclass
USB488 Specification (USBTMC-USB488) Revision 1.0” section
4.2.2.

The files are read only.

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channel\_vp\_mapping** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The mapping of which primary/sub channels are bound to which
Virtual Processors.
Format: <channel’s child\_relid:the bound cpu’s number>

Users:
:   tools/hv/lsvmbus

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Directory for per-channel information
NN is the VMBUS relid associated with the channel.

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/cpu** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

VCPU (sub)channel is affinitized to

Users:
:   tools/hv/lsvmbus and other debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/events** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Number of times we have signaled the host

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/in\_mask** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Host to guest channel interrupt mask

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/interrupts** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Number of times we have taken an interrupt (incoming)

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/intr\_in\_full** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Number of guest to host interrupts caused by the inbound ring
buffer transitioning from full to not full while a packet is
waiting for buffer space to become available

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/intr\_out\_empty** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Number of guest to host interrupts caused by the outbound ring
buffer transitioning from empty to not empty

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/latency** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Channel signaling latency. This file is available only for
performance critical channels (storage, network, etc.) that use
the monitor page mechanism.

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/monitor\_id** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Monitor bit associated with channel. This file is available only
for performance critical channels (storage, network, etc.) that
use the monitor page mechanism.

Users:
:   Debugging tools and userspace drivers

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_full\_first** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Number of write operations that were the first to encounter an
outbound ring buffer full condition

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_full\_total** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Total number of write operations that encountered an outbound
ring buffer full condition

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_mask** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Guest to host channel interrupt mask

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/pending** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Channel interrupt pending state. This file is available only for
performance critical channels (storage, network, etc.) that use
the monitor page mechanism.

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/read\_avail** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Bytes available to read

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/ring** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Binary file created by uio\_hv\_generic for ring buffer

Users:
:   Userspace drivers

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/subchannel\_id** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Subchannel ID associated with VMBUS channel

Users:
:   Debugging tools and userspace drivers

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/channels/<N>/write\_avail** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Bytes available to write

Users:
:   Debugging tools

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/class\_id** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The VMBus interface type GUID of the device

Users:
:   tools/hv/lsvmbus

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/device** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The 16 bit device ID of the device

Users:
:   tools/hv/lsvmbus and user level RDMA libraries

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/device\_id** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The VMBus interface instance GUID of the device

Users:
:   tools/hv/lsvmbus

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/id** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The VMBus child\_relid of the device’s primary channel

Users:
:   tools/hv/lsvmbus

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/numa\_node** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

This NUMA node to which the VMBUS device is
attached, or -1 if the node is unknown.

|  |
| --- |
| **/sys/bus/vmbus/devices/<UUID>/vendor** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

The 16 bit vendor ID of the device

Users:
:   tools/hv/lsvmbus and user level RDMA libraries

|  |
| --- |
| **/sys/bus/vmbus/hibernation** |

Defined on file [sysfs-bus-vmbus](abi-stable-files.html#abi-file-stable-sysfs-bus-vmbus)

Whether the host supports hibernation for the VM.

Users:
:   Daemon that sets up swap partition/file for hibernation.

|  |
| --- |
| **/sys/bus/w1/devices/.../eeprom** |

Defined on file [sysfs-driver-w1\_ds28e04](abi-stable-files.html#abi-file-stable-sysfs-driver-w1-ds28e04)

read/write the contents of the EEPROM memory of the DS28E04-100
see [Kernel driver w1\_ds28e04](../w1/slaves/w1_ds28e04.html) for detailed information

Users:
:   any user space application which wants to communicate with DS28E04-100

|  |
| --- |
| **/sys/bus/w1/devices/.../offset** |

Defined on file [sysfs-driver-w1\_ds2438](abi-stable-files.html#abi-file-stable-sysfs-driver-w1-ds2438)

write the contents to the offset register of the DS2438
see [Kernel driver w1\_ds2438](../w1/slaves/w1_ds2438.html) for detailed information

Users:
:   any user space application which wants to communicate with DS2438

|  |
| --- |
| **/sys/bus/w1/devices/.../page1** |

Defined on file [sysfs-driver-w1\_ds2438](abi-stable-files.html#abi-file-stable-sysfs-driver-w1-ds2438)

read the contents of the page1 of the DS2438
see [Kernel driver w1\_ds2438](../w1/slaves/w1_ds2438.html) for detailed information

Users:
:   any user space application which wants to communicate with DS2438

|  |
| --- |
| **/sys/bus/w1/devices/.../pio** |

Defined on file [sysfs-driver-w1\_ds28e04](abi-stable-files.html#abi-file-stable-sysfs-driver-w1-ds28e04)

read/write the contents of the two PIO’s of the DS28E04-100
see [Kernel driver w1\_ds28e04](../w1/slaves/w1_ds28e04.html) for detailed information

Users:
:   any user space application which wants to communicate with DS28E04-100

|  |
| --- |
| **/sys/bus/w1/devices/.../w1\_master\_timeout\_us** |

Defined on file [sysfs-bus-w1](abi-stable-files.html#abi-file-stable-sysfs-bus-w1)

Bus scanning interval, microseconds component.
Some of 1-Wire devices commonly associated with physical access
control systems are attached/generate presence for as short as
100 ms - hence the tens-to-hundreds milliseconds scan intervals
are required.

see [Introduction to the 1-wire (w1) subsystem](../w1/w1-generic.html) for detailed information.

Users:
:   any user space application which wants to know bus scanning
    interval

|  |
| --- |
| **/sys/bus/w1/devices/.../w1\_seq** |

Defined on file [sysfs-driver-w1\_ds28ea00](abi-stable-files.html#abi-file-stable-sysfs-driver-w1-ds28ea00)

Support for the DS28EA00 chain sequence function
see [Kernel driver w1\_therm](../w1/slaves/w1_therm.html) for detailed information

Users:
:   any user space application which wants to communicate with DS28EA00

|  |
| --- |
| **/sys/bus/wmi/devices/05901221-D566-11D1-B2F0-00A0C9062910[-X]/bmof** |

Defined on file [sysfs-platform-wmi-bmof](abi-stable-files.html#abi-file-stable-sysfs-platform-wmi-bmof)

Binary MOF metadata used to describe the details of available ACPI WMI interfaces.

See [WMI embedded Binary MOF driver](../wmi/devices/wmi-bmof.html) for details.

|  |
| --- |
| **/sys/bus/xen-backend/devices/\*/devtype** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

The type of the device. e.g., one of: ‘vbd’ (block),
‘vif’ (network), or ‘vfb’ (framebuffer).

|  |
| --- |
| **/sys/bus/xen-backend/devices/\*/nodename** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

XenStore node (under /local/domain/NNN/) for this
backend device.

|  |
| --- |
| **/sys/bus/xen-backend/devices/\*/state** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

The state of the device. One of: ‘Unknown’,
‘Initialising’, ‘Initialised’, ‘Connected’, ‘Closing’,
‘Closed’, ‘Reconfiguring’, ‘Reconfigured’.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/mode** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Whether the block device is read-only (‘r’) or
read-write (‘w’).

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/physical\_device** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

The major:minor number (in hexadecimal) of the
physical device providing the storage for this backend
block device.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/f\_req** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of flush requests from the frontend.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/oo\_req** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of requests delayed because the backend was too
busy processing previous requests.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/rd\_req** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of read requests from the frontend.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/rd\_sect** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of sectors read by the frontend.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/wr\_req** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of write requests from the frontend.

|  |
| --- |
| **/sys/bus/xen-backend/devices/vbd-\*/statistics/wr\_sect** |

Defined on file [sysfs-bus-xen-backend](abi-stable-files.html#abi-file-stable-sysfs-bus-xen-backend)

Number of sectors written by the frontend.

## Symbols under /sys/class

|  |
| --- |
| **/sys/class/backlight/<backlight>/actual\_brightness** |

Defined on file [sysfs-class-backlight](abi-stable-files.html#abi-file-stable-sysfs-class-backlight)

Show the actual brightness by querying the hardware. Due
to implementation differences in hardware this may not
match the value in ‘brightness’. For example some hardware
may treat blanking differently or have custom power saving
features. Userspace should generally use the values in
‘brightness’ to make decisions.

Users:
:   HAL

|  |
| --- |
| **/sys/class/backlight/<backlight>/bl\_power** |

Defined on file [sysfs-class-backlight](abi-stable-files.html#abi-file-stable-sysfs-class-backlight)

Control BACKLIGHT power, values are compatible with
FB\_BLANK\_\* from fb.h

> * 0 (FB\_BLANK\_UNBLANK) : power on.
> * 4 (FB\_BLANK\_POWERDOWN) : power off

Users:
:   HAL

|  |
| --- |
| **/sys/class/backlight/<backlight>/brightness** |

Defined on file [sysfs-class-backlight](abi-stable-files.html#abi-file-stable-sysfs-class-backlight)

Control the brightness for this <backlight>. Values
are between 0 and max\_brightness. This file will also
show the brightness level stored in the driver, which
may not be the actual brightness (see actual\_brightness).

Users:
:   HAL

|  |
| --- |
| **/sys/class/backlight/<backlight>/max\_brightness** |

Defined on file [sysfs-class-backlight](abi-stable-files.html#abi-file-stable-sysfs-class-backlight)

Maximum brightness for <backlight>.

Users:
:   HAL

|  |
| --- |
| **/sys/class/backlight/<backlight>/type** |

Defined on file [sysfs-class-backlight](abi-stable-files.html#abi-file-stable-sysfs-class-backlight)

The type of interface controlled by <backlight>.
“firmware”: The driver uses a standard firmware interface
“platform”: The driver uses a platform-specific interface
“raw”: The driver controls hardware registers directly

In the general case, when multiple backlight
interfaces are available for a single device, firmware
control should be preferred to platform control should
be preferred to raw control. Using a firmware
interface reduces the probability of confusion with
the hardware and the OS independently updating the
backlight state. Platform interfaces are mostly a
holdover from pre-standardisation of firmware
interfaces.

|  |
| --- |
| **/sys/class/bluetooth/hci<index>/reset** |

Defined on file [sysfs-class-bluetooth](abi-stable-files.html#abi-file-stable-sysfs-class-bluetooth)

This write-only attribute allows users to trigger the vendor reset
method on the Bluetooth device when arbitrary data is written.
The reset may or may not be done through the device transport
(e.g., UART/USB), and can also be done through an out-of-band
approach such as GPIO.

|  |
| --- |
| **/sys/class/infiniband/<device-name>/hw\_counters/lifespan** |
| **/sys/class/infiniband/<device-name>/ports/<port-num>/hw\_counters/lifespan** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

The optional “hw\_counters” subdirectory can be under either the
parent device or the port subdirectories or both. If present,
there are a list of counters provided by the hardware. They may
match some of the counters in the counters directory, but they
often include many other counters. In addition to the various
counters, there will be a file named “lifespan” that configures
how frequently the core should update the counters when they are
being accessed (counters are not updated if they are not being
accessed). The lifespan is in milliseconds and defaults to 10
unless set to something else by the driver. Users may echo a
value between 0-10000 to the lifespan file to set the length
of time between updates in milliseconds.

|  |
| --- |
| **/sys/class/infiniband/<device>/fw\_ver** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RO) Display firmware version

|  |
| --- |
| **/sys/class/infiniband/<device>/node\_desc** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RW) Update the node description with information such as the
node’s hostname, so that IB network management software can tie
its view to the real world.

|  |
| --- |
| **/sys/class/infiniband/<device>/node\_type** |
| **/sys/class/infiniband/<device>/node\_guid** |
| **/sys/class/infiniband/<device>/sys\_image\_guid** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| node\_type: | (RO) Node type (CA, RNIC, usNIC, usNIC UDP, switch or router) |
| node\_guid: | (RO) Node GUID |
| sys\_image\_guid: | (RO) System image GUID |

|  |
| --- |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/symbol\_error** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_remote\_physical\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_switch\_relay\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/link\_error\_recovery** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_constraint\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_contraint\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/local\_link\_integrity\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/excessive\_buffer\_overrun\_errors** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_data** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_data** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/unicast\_rcv\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/unicast\_xmit\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/multicast\_rcv\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/multicast\_xmit\_packets** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/link\_downed** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_discards** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/VL15\_dropped** |
| **/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_wait** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

**Errors info**:

symbol\_error: (RO) Total number of minor link errors detected on
one or more physical lanes.

port\_rcv\_errors : (RO) Total number of packets containing an
error that were received on the port.

port\_rcv\_remote\_physical\_errors : (RO) Total number of packets
marked with the EBP delimiter received on the port.

port\_rcv\_switch\_relay\_errors : (RO) Total number of packets
received on the port that were discarded because they could not
be forwarded by the switch relay.

link\_error\_recovery: (RO) Total number of times the Port
Training state machine has successfully completed the link error
recovery process.

port\_xmit\_constraint\_errors: (RO) Total number of packets not
transmitted from the switch physical port due to outbound raw
filtering or failing outbound partition or IP version check.

port\_rcv\_constraint\_errors: (RO) Total number of packets
received on the switch physical port that are discarded due to
inbound raw filtering or failing inbound partition or IP version
check.

local\_link\_integrity\_errors: (RO) The number of times that the
count of local physical errors exceeded the threshold specified
by LocalPhyErrors

excessive\_buffer\_overrun\_errors: (RO) This counter, indicates an
input buffer overrun. It indicates possible misconfiguration of
a port, either by the Subnet Manager (SM) or by user
intervention. It can also indicate hardware issues or extremely
poor link signal integrity

**Data info**:

port\_xmit\_data: (RO) Total number of data octets, divided by 4
(lanes), transmitted on all VLs. This is 64 bit counter

port\_rcv\_data: (RO) Total number of data octets, divided by 4
(lanes), received on all VLs. This is 64 bit counter.

port\_xmit\_packets: (RO) Total number of packets transmitted on
all VLs from this port. This may include packets with errors.
This is 64 bit counter.

port\_rcv\_packets: (RO) Total number of packets (this may include
packets containing Errors. This is 64 bit counter.

link\_downed: (RO) Total number of times the Port Training state
machine has failed the link error recovery process and downed
the link.

unicast\_rcv\_packets: (RO) Total number of unicast packets,
including unicast packets containing errors.

unicast\_xmit\_packets: (RO) Total number of unicast packets
transmitted on all VLs from the port. This may include unicast
packets with errors.

multicast\_rcv\_packets: (RO) Total number of multicast packets,
including multicast packets containing errors.

multicast\_xmit\_packets: (RO) Total number of multicast packets
transmitted on all VLs from the port. This may include multicast
packets with errors.

**Misc info**:

port\_xmit\_discards: (RO) Total number of outbound packets
discarded by the port because the port is down or congested.

VL15\_dropped: (RO) Number of incoming VL15 packets dropped due
to resource limitations (e.g., lack of buffers) of the port.

port\_xmit\_wait: (RO) The number of ticks during which the port
had data to transmit but no data was sent during the entire tick
(either because of insufficient credits or because of lack of
arbitration).

Each of these files contains the corresponding value from the
port’s Performance Management PortCounters attribute, as
described in the InfiniBand Architecture Specification.

|  |
| --- |
| **/sys/class/infiniband/<device>/ports/<port-num>/lid** |
| **/sys/class/infiniband/<device>/ports/<port-num>/rate** |
| **/sys/class/infiniband/<device>/ports/<port-num>/lid\_mask\_count** |
| **/sys/class/infiniband/<device>/ports/<port-num>/sm\_sl** |
| **/sys/class/infiniband/<device>/ports/<port-num>/sm\_lid** |
| **/sys/class/infiniband/<device>/ports/<port-num>/state** |
| **/sys/class/infiniband/<device>/ports/<port-num>/phys\_state** |
| **/sys/class/infiniband/<device>/ports/<port-num>/cap\_mask** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| lid: | (RO) Port LID |
| rate: | (RO) Port data rate (active width \* active speed) |
| lid\_mask\_count: | (RO) Port LID mask count |
| sm\_sl: | (RO) Subnet manager SL for port’s subnet |
| sm\_lid: | (RO) Subnet manager LID for port’s subnet |
| state: | (RO) Port state (DOWN, INIT, ARMED, ACTIVE or ACTIVE\_DEFER) |
| phys\_state: | (RO) Port physical state (Sleep, Polling, LinkUp, etc) |
| cap\_mask: | (RO) Port capability mask. 2 bits here are settable- IsCommunicationManagementSupported (set when CM module is loaded) and IsSM (set via open of issmN file). |

|  |
| --- |
| **/sys/class/infiniband/<device>/ports/<port-num>/link\_layer** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RO) Link layer type information (Infiniband or Ethernet type)

|  |
| --- |
| **/sys/class/infiniband/<hca>/ports/<port-number>/gid\_attrs/ndevs/<gid-index>** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

The net-device’s name associated with the GID resides
at index <gid-index>.

|  |
| --- |
| **/sys/class/infiniband/<hca>/ports/<port-number>/gid\_attrs/types/<gid-index>** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

The RoCE type of the associated GID resides at index <gid-index>.
This could either be “IB/RoCE v1” for IB and RoCE v1 based GIDs
or “RoCE v2” for RoCE v2 based GIDs.

|  |
| --- |
| **/sys/class/infiniband/bnxt\_reX/hw\_rev** |
| **/sys/class/infiniband/bnxt\_reX/hca\_type** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |  |
| --- | --- | --- |
| hw\_rev: | (RO) | Hardware revision number |
| hca\_type: | (RO) | Host channel adapter type |

|  |
| --- |
| **/sys/class/infiniband/cxgb4\_X/hw\_rev** |
| **/sys/class/infiniband/cxgb4\_X/hca\_type** |
| **/sys/class/infiniband/cxgb4\_X/board\_id** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| hw\_rev: | (RO) Hardware revision number |
| hca\_type: | (RO) Driver short name. Should normally match the name in its bus driver structure (e.g. pci\_driver::name) |
| board\_id: | (RO) Manufacturing board id. (Vendor + device information) |

sysfs interface for Intel IB driver qib

|  |
| --- |
| **/sys/class/infiniband/hfi1\_X/hw\_rev** |
| **/sys/class/infiniband/hfi1\_X/board\_id** |
| **/sys/class/infiniband/hfi1\_X/nctxts** |
| **/sys/class/infiniband/hfi1\_X/serial** |
| **/sys/class/infiniband/hfi1\_X/chip\_reset** |
| **/sys/class/infiniband/hfi1\_X/boardversion** |
| **/sys/class/infiniband/hfi1\_X/nfreectxts** |
| **/sys/class/infiniband/hfi1\_X/tempsense** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| hw\_rev: | (RO) Hardware revision number |
| board\_id: | (RO) Manufacturing board id |
| nctxts: | (RO) Total contexts available. |
| serial: | (RO) Board serial number |
| chip\_reset: | (WO) Write “reset” to this file to reset the chip if possible. Only allowed if no user contexts are open that use chip resources. |
| boardversion: | (RO) Human readable board info |
| nfreectxts: | (RO) The number of free user ports (contexts) available. |
| tempsense: | (RO) Thermal sense information |

|  |
| --- |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_settings\_bin** |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_table\_bin** |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_prescan** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

Per-port congestion control.

|  |  |
| --- | --- |
| cc\_table\_bin | (RO) CCA tables used by PSM2 Congestion control table size followed by table entries. Binary attribute. |
| cc\_settings\_bin | (RO) Congestion settings: port control, control map and an array of 16 entries for the congestion entries - increase, timer, event log trigger threshold and the minimum injection rate delay. Binary attribute. |
| cc\_prescan | (RW) enable prescanning for faster BECN response. Write “on” to enable and “off” to disable. |

|  |
| --- |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/sc2vl/[0-31]** |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/sl2sc/[0-31]** |
| **/sys/class/infiniband/hfi1\_X/ports/<N>/vl2mtu/[0-15]** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| sc2vl/: | (RO) 32 files (0 - 31) used to translate sl->vl |
| sl2sc/: | (RO) 32 files (0 - 31) used to translate sl->sc |
| vl2mtu/: | (RO) 16 files (0 - 15) used to determine MTU for vl |

|  |
| --- |
| **/sys/class/infiniband/hfi1\_X/sdma\_<N>/cpu\_list** |
| **/sys/class/infiniband/hfi1\_X/sdma\_<N>/vl** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

sdma<N>/ contains one directory per sdma engine (0 - 15)

|  |  |
| --- | --- |
| cpu\_list: | (RW) List of cpus for user-process to sdma engine assignment. |
| vl: | (RO) Displays the virtual lane (vl) the sdma engine maps to. |

This interface gives the user control on the affinity settings
for the device. As an example, to set an sdma engine irq
affinity and thread affinity of a user processes to use the
sdma engine, which is “near” in terms of NUMA configuration, or
physical cpu location, the user will do:

```
echo "3" > /proc/irq/<N>/smp_affinity_list
echo "4-7" > /sys/devices/.../sdma3/cpu_list
cat /sys/devices/.../sdma3/vl
0
echo "8" > /proc/irq/<M>/smp_affinity_list
echo "9-12" > /sys/devices/.../sdma4/cpu_list
cat /sys/devices/.../sdma4/vl
1
```

to make sure that when a process runs on cpus 4,5,6, or 7, and
uses vl=0, then sdma engine 3 is selected by the driver, and
also the interrupt of the sdma engine 3 is steered to cpu 3.
Similarly, when a process runs on cpus 9,10,11, or 12 and sets
vl=1, then engine 4 will be selected and the irq of the sdma
engine 4 is steered to cpu 8. This assumes that in the above N
is the irq number of “sdma3”, and M is irq number of “sdma4” in
the /proc/interrupts file.

sysfs interface for QLogic qedr NIC Driver

|  |
| --- |
| **/sys/class/infiniband/mlx4\_X/hw\_rev** |
| **/sys/class/infiniband/mlx4\_X/hca\_type** |
| **/sys/class/infiniband/mlx4\_X/board\_id** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| hw\_rev: | (RO) Hardware revision number |
| hca\_type: | (RO) Host channel adapter type |
| board\_id: | (RO) Manufacturing board ID |

|  |
| --- |
| **/sys/class/infiniband/mlx4\_X/iov/<pci-slot-num>/ports/<m>/smi\_enabled** |
| **/sys/class/infiniband/mlx4\_X/iov/<pci-slot-num>/ports/<m>/enable\_smi\_admin** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

Enabling QP0 on VFs for selected VF/port. By default, no VFs are
enabled for QP0 operation.

|  |  |  |
| --- | --- | --- |
| smi\_enabled: | (RO) | Indicates whether smi is currently enabled for the indicated VF/port |
| enable\_smi\_admin: | (RW) | Used by the admin to request that smi capability be enabled or disabled for the indicated VF/port. 0 = disable, 1 = enable. |

The requested enablement will occur at the next reset of the VF
(e.g. driver restart on the VM which owns the VF).

sysfs interface for Chelsio T4/T5 RDMA driver (cxgb4)

|  |
| --- |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/gids/<n>** |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/admin\_guids/<n>** |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/pkeys/<n>** |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/mcgs/** |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<pci-slot-num>/ports/<m>/gid\_idx/0** |
| **/sys/class/infiniband/mlx4\_X/iov/ports/<pci-slot-num>/ports/<m>/pkey\_idx/<n>** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

The sysfs iov directory is used to manage and examine the port
P\_Key and guid paravirtualization. This directory is added only
for the master -- slaves do not have it.

Under iov/ports, the administrator may examine the gid and P\_Key
tables as they are present in the device (and as are seen in the
“network view” presented to the SM).

The “pkeys” and “gids” subdirectories contain one file for each
entry in the port’s P\_Key or GID table respectively. For
example, ports/1/pkeys/10 contains the value at index 10 in port
1’s P\_Key table.

|  |  |
| --- | --- |
| gids/<n>: | (RO) The physical port gids n = 0..127 |
| admin\_guids/<n>: | (RW) Allows examining or changing the administrative state of a given GUID n = 0..127 |
| pkeys/<n>: | (RO) Displays the contents of the physical key table n = 0..126 |
| mcgs/: | (RO) Multicast group table |
| <m>/gid\_idx/0: | (RO) Display the GID mapping m = 1..2 |
| <m>/pkey\_idx/<n>: | (RW) Writable except for RoCE pkeys. m = 1..2, n = 0..126  Under the iov/<pci slot number> directories, the admin may map the index numbers in the physical tables (as under iov/ports) to the paravirtualized index numbers that guests see.  For example, if the administrator, for port 1 on guest 2 maps physical pkey index 10 to virtual index 1, then that guest, whenever it uses its pkey index 1, will actually be using the real pkey index 10. |

|  |
| --- |
| **/sys/class/infiniband/mlx5\_X/hw\_rev** |
| **/sys/class/infiniband/mlx5\_X/hca\_type** |
| **/sys/class/infiniband/mlx5\_X/reg\_pages** |
| **/sys/class/infiniband/mlx5\_X/fw\_pages** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

[to be documented]

sysfs interface for Cisco VIC (usNIC) Verbs Driver

|  |
| --- |
| **/sys/class/infiniband/mthcaX/hw\_rev** |
| **/sys/class/infiniband/mthcaX/hca\_type** |
| **/sys/class/infiniband/mthcaX/board\_id** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| hw\_rev: | (RO) Hardware revision number |
| hca\_type: | (RO) Host Channel Adapter type: MT23108, MT25208 (MT23108 compat mode), MT25208 or MT25204 |
| board\_id: | (RO) Manufacturing board ID |

sysfs interface for Mellanox ConnectX HCA IB driver (mlx4)

|  |
| --- |
| **/sys/class/infiniband/ocrdmaX/hca\_type** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

hca\_type: (RO) Display FW version

sysfs interface for Intel Omni-Path driver (HFI1)

|  |
| --- |
| **/sys/class/infiniband/ocrdmaX/hw\_rev** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

hw\_rev: (RO) Hardware revision number

|  |
| --- |
| **/sys/class/infiniband/qedrX/hw\_rev** |
| **/sys/class/infiniband/qedrX/hca\_type** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |  |
| --- | --- | --- |
| hw\_rev: | (RO) | Hardware revision number |
| hca\_type: | (RO) | Display HCA type |

sysfs interface for VMware Paravirtual RDMA driver

|  |
| --- |
| **/sys/class/infiniband/qibX/ports/<N>/CCMgtA/cc\_settings\_bin** |
| **/sys/class/infiniband/qibX/ports/<N>/CCMgtA/cc\_table\_bin** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

Per-port congestion control. Both are binary attributes.

|  |  |
| --- | --- |
| cc\_table\_bin | (RO) Congestion control table size followed by table entries. |
| cc\_settings\_bin | (RO) Congestion settings: port control, control map and an array of 16 entries for the congestion entries - increase, timer, event log trigger threshold and the minimum injection rate delay. |

|  |
| --- |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rc\_resends** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/seq\_naks** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rdma\_seq** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rnr\_naks** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/other\_naks** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rc\_timeouts** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/look\_pkts** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/pkt\_drops** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/dma\_wait** |
| **/sys/class/infiniband/qibX/ports/<N>/diag\_counters/unaligned** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

[to be documented]

sysfs interface for Mellanox Connect-IB HCA driver mlx5

|  |
| --- |
| **/sys/class/infiniband/qibX/ports/<N>/linkstate/loopback** |
| **/sys/class/infiniband/qibX/ports/<N>/linkstate/led\_override** |
| **/sys/class/infiniband/qibX/ports/<N>/linkstate/hrtbt\_enable** |
| **/sys/class/infiniband/qibX/ports/<N>/linkstate/status** |
| **/sys/class/infiniband/qibX/ports/<N>/linkstate/status\_str** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

[to be documented]

|  |  |
| --- | --- |
| loopback: | (WO) |
| led\_override: | (WO) |
| hrtbt\_enable: | (RW) |
| status: | (RO) |
| status\_str: | (RO) Displays information about the link state, possible cable/switch problems, and hardware errors. Possible states are- “Initted”, “Present”, “IB\_link\_up”, “IB\_configured” or “Fatal\_Hardware\_Error”. |

|  |
| --- |
| **/sys/class/infiniband/qibX/ports/<N>/sl2vl/[0-15]** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RO) The directory contains 16 files numbered 0-15 that specify
the Service Level (SL). Listing the SL files returns the Virtual
Lane (VL) as programmed by the SL.

|  |
| --- |
| **/sys/class/infiniband/qibX/version** |
| **/sys/class/infiniband/qibX/hw\_rev** |
| **/sys/class/infiniband/qibX/hca\_type** |
| **/sys/class/infiniband/qibX/board\_id** |
| **/sys/class/infiniband/qibX/boardversion** |
| **/sys/class/infiniband/qibX/nctxts** |
| **/sys/class/infiniband/qibX/localbus\_info** |
| **/sys/class/infiniband/qibX/tempsense** |
| **/sys/class/infiniband/qibX/serial** |
| **/sys/class/infiniband/qibX/nfreectxts** |
| **/sys/class/infiniband/qibX/chip\_reset** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| version: | (RO) Display version information of installed software and drivers. |
| hw\_rev: | (RO) Hardware revision number |
| hca\_type: | (RO) Host channel adapter type |
| board\_id: | (RO) Manufacturing board id |
| boardversion: | (RO) Current version of the chip architecture |
| nctxts: | (RO) Return the number of user ports (contexts) available |
| localbus\_info: | (RO) Human readable localbus info |
| tempsense: | (RO) Display temp sense registers in decimal |
| serial: | (RO) Serial number of the HCA |
| nfreectxts: | (RO) The number of free user ports (contexts) available. |
| chip\_reset: | (WO) Reset the chip if possible by writing “reset” to this file. Only allowed if no user contexts are open that use chip resources. |

|  |
| --- |
| **/sys/class/infiniband/usnic\_X/board\_id** |
| **/sys/class/infiniband/usnic\_X/config** |
| **/sys/class/infiniband/usnic\_X/qp\_per\_vf** |
| **/sys/class/infiniband/usnic\_X/max\_vf** |
| **/sys/class/infiniband/usnic\_X/cq\_per\_vf** |
| **/sys/class/infiniband/usnic\_X/iface** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| board\_id: | (RO) Manufacturing board id |
| config: | (RO) Report the configuration for this PF |
| qp\_per\_vf: | (RO) Queue pairs per virtual function. |
| max\_vf: | (RO) Max virtual functions |
| cq\_per\_vf: | (RO) Completion queue per virtual function |
| iface: | (RO) Shows which network interface this usNIC entry is associated to (visible with ifconfig). |

|  |
| --- |
| **/sys/class/infiniband/usnic\_X/qpn/summary** |
| **/sys/class/infiniband/usnic\_X/qpn/context** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

[to be documented]

sysfs interface for Emulex RoCE HCA Driver

|  |
| --- |
| **/sys/class/infiniband/vmw\_pvrdmaX/hw\_rev** |
| **/sys/class/infiniband/vmw\_pvrdmaX/hca\_type** |
| **/sys/class/infiniband/vmw\_pvrdmaX/board\_id** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |  |
| --- | --- | --- |
| hw\_rev: | (RO) | Hardware revision number |
| hca\_type: | (RO) | Host channel adapter type |
| board\_id: | (RO) | Display PVRDMA manufacturing board ID |

sysfs interface for Broadcom NetXtreme-E RoCE driver

|  |
| --- |
| **/sys/class/infiniband\_mad/abi\_version** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RO) Value is incremented if any changes are made that break
userspace ABI compatibility of umad & issm devices.

|  |
| --- |
| **/sys/class/infiniband\_mad/umad<N>/ibdev** |
| **/sys/class/infiniband\_mad/umad<N>/port** |
| **/sys/class/infiniband\_mad/issm<N>/ibdev** |
| **/sys/class/infiniband\_mad/issm<N>/port** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

Each port of each InfiniBand device has a “umad” device and an
“issm” device attached. For example, a two-port HCA will have
two umad devices and two issm devices, while a switch will have
one device of each type (for switch port 0).

|  |  |
| --- | --- |
| ibdev: | (RO) Show Infiniband (IB) device name |
| port: | (RO) Display port number |

|  |
| --- |
| **/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/add\_target** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Interface for making ib\_srp connect to a new target.
One can request ib\_srp to connect to a new target by writing
a comma-separated list of login parameters to this sysfs
attribute. The supported parameters are:

* id\_ext, a 16-digit hexadecimal number specifying the eight
  byte identifier extension in the 16-byte SRP target port
  identifier. The target port identifier is sent by ib\_srp
  to the target in the SRP\_LOGIN\_REQ request.
* ioc\_guid, a 16-digit hexadecimal number specifying the eight
  byte I/O controller GUID portion of the 16-byte target port
  identifier.
* dgid, a 32-digit hexadecimal number specifying the
  destination GID.
* pkey, a four-digit hexadecimal number specifying the
  InfiniBand partition key.
* service\_id, a 16-digit hexadecimal number specifying the
  InfiniBand service ID used to establish communication with
  the SRP target. How to find out the value of the service ID
  is specified in the documentation of the SRP target.
* max\_sect, a decimal number specifying the maximum number of
  512-byte sectors to be transferred via a single SCSI command.
* max\_cmd\_per\_lun, a decimal number specifying the maximum
  number of outstanding commands for a single LUN.
* io\_class, a hexadecimal number specifying the SRP I/O class.
  Must be either 0xff00 (rev 10) or 0x0100 (rev 16a). The I/O
  class defines the format of the SRP initiator and target
  port identifiers.
* initiator\_ext, a 16-digit hexadecimal number specifying the
  identifier extension portion of the SRP initiator port
  identifier. This data is sent by the initiator to the target
  in the SRP\_LOGIN\_REQ request.
* cmd\_sg\_entries, a number in the range 1..255 that specifies
  the maximum number of data buffer descriptors stored in the
  SRP\_CMD information unit itself. With allow\_ext\_sg=0 the
  parameter cmd\_sg\_entries defines the maximum S/G list length
  for a single SRP\_CMD, and commands whose S/G list length
  exceeds this limit after S/G list collapsing will fail.
* allow\_ext\_sg, whether ib\_srp is allowed to include a partial
  memory descriptor list in an SRP\_CMD instead of the entire
  list. If a partial memory descriptor list has been included
  in an SRP\_CMD the remaining memory descriptors are
  communicated from initiator to target via an additional RDMA
  transfer. Setting allow\_ext\_sg to 1 increases the maximum
  amount of data that can be transferred between initiator and
  target via a single SCSI command. Since not all SRP target
  implementations support partial memory descriptor lists the
  default value for this option is 0.
* sg\_tablesize, a number in the range 1..2048 specifying the
  maximum S/G list length the SCSI layer is allowed to pass to
  ib\_srp. Specifying a value that exceeds cmd\_sg\_entries is
  only safe with partial memory descriptor list support enabled
  (allow\_ext\_sg=1).
* comp\_vector, a number in the range 0..n-1 specifying the
  MSI-X completion vector of the first RDMA channel. Some
  HCA’s allocate multiple (n) MSI-X vectors per HCA port. If
  the IRQ affinity masks of these interrupts have been
  configured such that each MSI-X interrupt is handled by a
  different CPU then the comp\_vector parameter can be used to
  spread the SRP completion workload over multiple CPU’s.
* tl\_retry\_count, a number in the range 2..7 specifying the
  IB RC retry count.
* queue\_size, the maximum number of commands that the
  initiator is allowed to queue per SCSI host. The default
  value for this parameter is 62. The lowest supported value
  is 2.
* max\_it\_iu\_size, a decimal number specifying the maximum
  initiator to target information unit length.

|  |
| --- |
| **/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/ibdev** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

HCA name (<hca>).

|  |
| --- |
| **/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/port** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

HCA port number (<port\_number>).

|  |
| --- |
| **/sys/class/infiniband\_verbs/abi\_version** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

(RO) Value is incremented if any changes are made that break
userspace ABI compatibility of uverbs devices.

sysfs interface for Mellanox IB HCA low-level driver (mthca)

|  |
| --- |
| **/sys/class/infiniband\_verbs/uverbs<N>/ibdev** |
| **/sys/class/infiniband\_verbs/uverbs<N>/abi\_version** |

Defined on file [sysfs-class-infiniband](abi-stable-files.html#abi-file-stable-sysfs-class-infiniband)

|  |  |
| --- | --- |
| ibdev: | (RO) Display Infiniband (IB) device name |
| abi\_version: | (RO) Show ABI version of IB device specific interfaces. |

|  |
| --- |
| **/sys/class/rfkill** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

The rfkill class subsystem folder.
Each registered rfkill driver is represented by an rfkillX
subfolder (X being an integer >= 0).

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/hard** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Current hardblock state. This file is read only.
Values: A numeric value.

> 0: inactive
> :   The transmitter is (potentially) active.
>
> 1: active
> :   The transmitter is forced off by something outside of
>     the driver’s control.

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/name** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Name assigned by driver to this key (interface or driver name).
Values: arbitrary string.

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/persistent** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Whether the soft blocked state is initialised from non-volatile
storage at startup.
Values: A numeric value:

> * 0: false
> * 1: true

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/soft** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Current softblock state. This file is read and write.
Values: A numeric value.

> 0: inactive
> :   The transmitter is (potentially) active.
>
> 1: active
> :   The transmitter is turned off by software.

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/state** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Current state of the transmitter.
This file was scheduled to be removed in 2014, but due to its
large number of users it will be sticking around for a bit
longer. Despite it being marked as stable, the newer “hard” and
“soft” interfaces should be preferred, since it is not possible
to express the ‘soft and hard block’ state of the rfkill driver
through this interface. There will likely be another attempt to
remove it in the future.
Values: A numeric value.

> 0: RFKILL\_STATE\_SOFT\_BLOCKED
> :   transmitter is turned off by software
>
> 1: RFKILL\_STATE\_UNBLOCKED
> :   transmitter is (potentially) active
>
> 2: RFKILL\_STATE\_HARD\_BLOCKED
> :   transmitter is forced off by something outside of
>     the driver’s control.

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/type** |

Defined on file [sysfs-class-rfkill](abi-stable-files.html#abi-file-stable-sysfs-class-rfkill)

Driver type string (“wlan”, “bluetooth”, etc).
Values: See include/linux/rfkill.h.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/allow\_ext\_sg** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Whether ib\_srp is allowed to include a partial memory
descriptor list in an SRP\_CMD when communicating with an SRP
target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/ch\_count** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Number of RDMA channels used for communication with the SRP
target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/cmd\_sg\_entries** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Maximum number of data buffer descriptors that may be sent to
the target in a single SRP\_CMD request.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/comp\_vector** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Completion vector used for the first RDMA channel.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/dgid** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

InfiniBand destination GID used for communication with the SRP
target. Differs from orig\_dgid if port redirection has happened.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/id\_ext** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Eight-byte identifier extension portion of the 16-byte target
port identifier.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/ioc\_guid** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Eight-byte I/O controller GUID portion of the 16-byte target
port identifier.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/local\_ib\_device** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Name of the InfiniBand HCA used for communicating with the
SRP target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/local\_ib\_port** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Number of the HCA port used for communicating with the
SRP target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/orig\_dgid** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

InfiniBand destination GID specified in the parameters
written to the add\_target sysfs attribute.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/pkey** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

A 16-bit number representing the InfiniBand partition key used
for communication with the SRP target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/req\_lim** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Number of requests ib\_srp can send to the target before it has
to wait for more credits. For more information see also the
SRP credit algorithm in the SRP specification.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/service\_id** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

InfiniBand service ID used for establishing communication with
the SRP target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/sgid** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

InfiniBand GID of the source port used for communication with
the SRP target.

|  |
| --- |
| **/sys/class/scsi\_host/host<n>/zero\_req\_lim** |

Defined on file [sysfs-driver-ib\_srp](abi-stable-files.html#abi-file-stable-sysfs-driver-ib-srp)

Number of times the initiator had to wait before sending a
request to the target because it ran out of credits. For more
information see also the SRP credit algorithm in the SRP
specification.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/delete** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

Instructs an SRP initiator to disconnect from a target and to
remove all LUNs imported from that target.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/dev\_loss\_tmo** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

Number of seconds the SCSI layer will wait after a transport
layer error has been observed before removing a target port.
Zero means immediate removal. Setting this attribute to “off”
will disable the dev\_loss timer.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/fast\_io\_fail\_tmo** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

Number of seconds the SCSI layer will wait after a transport
layer error has been observed before failing I/O. Zero means
failing I/O immediately. Setting this attribute to “off” will
disable the fast\_io\_fail timer.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/port\_id** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

16-byte local SRP port identifier in hexadecimal format. An
example: 4c:49:4e:55:58:20:56:49:4f:00:00:00:00:00:00:00.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/reconnect\_delay** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

Number of seconds the SCSI layer will wait after a reconnect
attempt failed before retrying. Setting this attribute to
“off” will disable time-based reconnecting.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/roles** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

Role of the remote port. Either “SRP Initiator” or “SRP Target”.

|  |
| --- |
| **/sys/class/srp\_remote\_ports/port-<h>:<n>/state** |

Defined on file [sysfs-transport-srp](abi-stable-files.html#abi-file-stable-sysfs-transport-srp)

State of the transport layer used for communication with the
remote port. “running” if the transport layer is operational;
“blocked” if a transport layer error has been encountered but
the fast\_io\_fail\_tmo timer has not yet fired; “fail-fast”
after the fast\_io\_fail\_tmo timer has fired and before the
“dev\_loss\_tmo” timer has fired; “lost” after the
“dev\_loss\_tmo” timer has fired and before the port is finally
removed.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The device/ directory under a specific TPM instance exposes
the properties of that TPM chip

|  |
| --- |
| **/sys/class/tpm/tpmX/device/active** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “active” property prints a ‘1’ if the TPM chip is accepting
commands. An inactive TPM chip still contains all the state of
an active chip (Storage Root Key, NVRAM, etc), and can be
visible to the OS, but will only accept a restricted set of
commands. See the TPM Main Specification part 2, Structures,
section 17 for more information on which commands are
available.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/cancel** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “cancel” property allows you to cancel the currently
pending TPM command. Writing any value to cancel will call the
TPM vendor specific cancel operation.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/caps** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “caps” property contains TPM manufacturer and version info.

Example output:

```
Manufacturer: 0x53544d20
TCG version: 1.2
Firmware version: 8.16
```

Manufacturer is a hex dump of the 4 byte manufacturer info
space in a TPM. TCG version shows the TCG TPM spec level that
the chip supports. Firmware version is that of the chip and
is manufacturer specific.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/durations** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “durations” property shows the 3 vendor-specific values
used to wait for a short, medium and long TPM command. All
TPM commands are categorized as short, medium or long in
execution time, so that the driver doesn’t have to wait
any longer than necessary before starting to poll for a
result.

Example output:

```
3015000 4508000 180995000 [original]
```

Here the short, medium and long durations are displayed in
usecs. “[original]” indicates that the values are displayed
unmodified from when they were queried from the chip.
Durations can be modified in the case where a buggy chip
reports them in msec instead of usec and they need to be
scaled to be displayed in usecs. In this case “[adjusted]”
will be displayed in place of “[original]”.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/enabled** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “enabled” property prints a ‘1’ if the TPM chip is enabled,
meaning that it should be visible to the OS. This property
may be visible but produce a ‘0’ after some operation that
disables the TPM.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/owned** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “owned” property produces a ‘1’ if the TPM\_TakeOwnership
ordinal has been executed successfully in the chip. A ‘0’
indicates that ownership hasn’t been taken.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/pcrs** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “pcrs” property will dump the current value of all Platform
Configuration Registers in the TPM. Note that since these
values may be constantly changing, the output is only valid
for a snapshot in time.

Example output:

```
PCR-00: 3A 3F 78 0F 11 A4 B4 99 69 FC AA 80 CD 6E 39 57 C3 3B 22 75
PCR-01: 3A 3F 78 0F 11 A4 B4 99 69 FC AA 80 CD 6E 39 57 C3 3B 22 75
PCR-02: 3A 3F 78 0F 11 A4 B4 99 69 FC AA 80 CD 6E 39 57 C3 3B 22 75
PCR-03: 3A 3F 78 0F 11 A4 B4 99 69 FC AA 80 CD 6E 39 57 C3 3B 22 75
PCR-04: 3A 3F 78 0F 11 A4 B4 99 69 FC AA 80 CD 6E 39 57 C3 3B 22 75
...
```

The number of PCRs and hex bytes needed to represent a PCR
value will vary depending on TPM chip version. For TPM 1.1 and
1.2 chips, PCRs represent SHA-1 hashes, which are 20 bytes
long. Use the “caps” property to determine TPM version.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/pubek** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “pubek” property will return the TPM’s public endorsement
key if possible. If the TPM has had ownership established and
is version 1.2, the pubek will not be available without the
owner’s authorization. Since the TPM driver doesn’t store any
secrets, it can’t authorize its own request for the pubek,
making it unaccessible. The public endorsement key is gener-
ated at TPM manufacture time and exists for the life of the
chip.

Example output:

```
Algorithm: 00 00 00 01
Encscheme: 00 03
Sigscheme: 00 01
Parameters: 00 00 08 00 00 00 00 02 00 00 00 00
Modulus length: 256
Modulus:
B4 76 41 82 C9 20 2C 10 18 40 BC 8B E5 44 4C 6C
3A B2 92 0C A4 9B 2A 83 EB 5C 12 85 04 48 A0 B6
1E E4 81 84 CE B2 F2 45 1C F0 85 99 61 02 4D EB
86 C4 F7 F3 29 60 52 93 6B B2 E5 AB 8B A9 09 E3
D7 0E 7D CA 41 BF 43 07 65 86 3C 8C 13 7A D0 8B
82 5E 96 0B F8 1F 5F 34 06 DA A2 52 C1 A9 D5 26
0F F4 04 4B D9 3F 2D F2 AC 2F 74 64 1F 8B CD 3E
1E 30 38 6C 70 63 69 AB E2 50 DF 49 05 2E E1 8D
6F 78 44 DA 57 43 69 EE 76 6C 38 8A E9 8E A3 F0
A7 1F 3C A8 D0 12 15 3E CA 0E BD FA 24 CD 33 C6
47 AE A4 18 83 8E 22 39 75 93 86 E6 FD 66 48 B6
10 AD 94 14 65 F9 6A 17 78 BD 16 53 84 30 BF 70
E0 DC 65 FD 3C C6 B0 1E BF B9 C1 B5 6C EF B1 3A
F8 28 05 83 62 26 11 DC B4 6B 5A 97 FF 32 26 B6
F7 02 71 CF 15 AE 16 DD D1 C1 8E A8 CF 9B 50 7B
C3 91 FF 44 1E CF 7C 39 FE 17 77 21 20 BD CE 9B
```

Possible values:

```
Algorithm:    TPM_ALG_RSA                     (1)
Encscheme:    TPM_ES_RSAESPKCSv15             (2)
              TPM_ES_RSAESOAEP_SHA1_MGF1      (3)
Sigscheme:    TPM_SS_NONE                     (1)
Parameters, a byte string of 3 u32 values:
      Key Length (bits):      00 00 08 00     (2048)
      Num primes:             00 00 00 02     (2)
      Exponent Size:          00 00 00 00     (0 means the
                                               default exp)
Modulus Length: 256 (bytes)
Modulus:      The 256 byte Endorsement Key modulus
```

|  |
| --- |
| **/sys/class/tpm/tpmX/device/temp\_deactivated** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “temp\_deactivated” property returns a ‘1’ if the chip has
been temporarily deactivated, usually until the next power
cycle. Whether a warm boot (reboot) will clear a TPM chip
from a temp\_deactivated state is platform specific.

|  |
| --- |
| **/sys/class/tpm/tpmX/device/timeouts** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “timeouts” property shows the 4 vendor-specific values
for the TPM’s interface spec timeouts. The use of these
timeouts is defined by the TPM interface spec that the chip
conforms to.

Example output:

```
750000 750000 750000 750000 [original]
```

The four timeout values are shown in usecs, with a trailing
“[original]” or “[adjusted]” depending on whether the values
were scaled by the driver to be reported in usec from msecs.

|  |
| --- |
| **/sys/class/tpm/tpmX/pcr-<H>/<N>** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

produces output in compact hex representation for PCR
number N from hash bank H. N is the numeric value of
the PCR number and H is the crypto string
representation of the hash

Example output:

```
cat /sys/class/tpm/tpm0/pcr-sha256/7
2ED93F199692DC6788EFA6A1FE74514AB9760B2A6CEEAEF6C808C13E4ABB0D42
```

|  |
| --- |
| **/sys/class/tpm/tpmX/tpm\_version\_major** |

Defined on file [sysfs-class-tpm](abi-stable-files.html#abi-file-stable-sysfs-class-tpm)

The “tpm\_version\_major” property shows the TCG spec major version
implemented by the TPM device.

Example output:

```
2
```

|  |
| --- |
| **/sys/class/ubi/** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

The ubi/ class sub-directory belongs to the UBI subsystem and
provides general UBI information, per-UBI device information
and per-UBI volume information.

|  |
| --- |
| **/sys/class/ubi/ubiX/avail\_eraseblocks** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Amount of available logical eraseblock. For example, one may
create a new UBI volume which has this amount of logical
eraseblocks.

|  |
| --- |
| **/sys/class/ubi/ubiX/bad\_peb\_count** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Count of bad physical eraseblocks on the underlying MTD device.

|  |
| --- |
| **/sys/class/ubi/ubiX/bgt\_enabled** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Contains ASCII “0n” if the UBI background thread is disabled,
and ASCII “1n” if it is enabled.

|  |
| --- |
| **/sys/class/ubi/ubiX/dev** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Major and minor numbers of the character device corresponding
to this UBI device (in <major>:<minor> format).

|  |
| --- |
| **/sys/class/ubi/ubiX/eraseblock\_size** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Maximum logical eraseblock size this UBI device may provide. UBI
volumes may have smaller logical eraseblock size because of their
alignment.

|  |
| --- |
| **/sys/class/ubi/ubiX/max\_ec** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Maximum physical eraseblock erase counter value.

|  |
| --- |
| **/sys/class/ubi/ubiX/max\_vol\_count** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Maximum number of volumes which this UBI device may have.

|  |
| --- |
| **/sys/class/ubi/ubiX/min\_io\_size** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Minimum input/output unit size. All the I/O may only be done
in fractions of the contained number.

|  |
| --- |
| **/sys/class/ubi/ubiX/mtd\_num** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Number of the underlying MTD device.

|  |
| --- |
| **/sys/class/ubi/ubiX/reserved\_for\_bad** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Number of physical eraseblocks reserved for bad block handling.

|  |
| --- |
| **/sys/class/ubi/ubiX/ro\_mode** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Contains ASCII “1n” if the read-only flag is set on this
device, and “0n” if it is cleared. UBI devices mark themselves
as read-only when they detect an unrecoverable error.

|  |
| --- |
| **/sys/class/ubi/ubiX/total\_eraseblocks** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Total number of good (not marked as bad) physical eraseblocks on
the underlying MTD device.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

The /sys/class/ubi/ubiX/ubiX\_0/, /sys/class/ubi/ubiX/ubiX\_1/,
etc directories describe UBI volumes on UBI device X (volumes
0, 1, etc).

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/alignment** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Volume alignment - the value the logical eraseblock size of
this volume has to be aligned on. For example, 2048 means that
logical eraseblock size is multiple of 2048. In other words,
volume logical eraseblock size is UBI device logical eraseblock
size aligned to the alignment value.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/corrupted** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Contains ASCII “0n” if the UBI volume is OK, and ASCII “1n”
if it is corrupted (e.g., due to an interrupted volume update).

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/data\_bytes** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

The amount of data this volume contains. This value makes sense
only for static volumes, and for dynamic volume it equivalent
to the total volume size in bytes.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/dev** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Major and minor numbers of the character device corresponding
to this UBI volume (in <major>:<minor> format).

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/name** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Volume name.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/reserved\_ebs** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Count of physical eraseblock reserved for this volume.
Equivalent to the volume size in logical eraseblocks.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/type** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Volume type. Contains ASCII “dynamicn” for dynamic volumes and
“staticn” for static volumes.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/upd\_marker** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Contains ASCII “0n” if the update marker is not set for this
volume, and “1n” if it is set. The update marker is set when
volume update starts, and cleaned when it ends. So the presence
of the update marker indicates that the volume is being updated
at the moment of the update was interrupted. The later may be
checked using the “corrupted” sysfs file.

|  |
| --- |
| **/sys/class/ubi/ubiX/ubiX\_Y/usable\_eb\_size** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Logical eraseblock size of this volume. Equivalent to logical
eraseblock size of the device aligned on the volume alignment
value.

|  |
| --- |
| **/sys/class/ubi/ubiX/volumes\_count** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

Count of volumes on this UBI device.

|  |
| --- |
| **/sys/class/ubi/version** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

This file contains version of the latest supported UBI on-media
format. Currently it is 1, and there is no plan to change this.
However, if in the future UBI needs on-flash format changes
which cannot be done in a compatible manner, a new format
version will be added. So this is a mechanism for possible
future backward-compatible (but forward-incompatible)
improvements.

|  |
| --- |
| **/sys/class/ubiX/** |

Defined on file [sysfs-class-ubi](abi-stable-files.html#abi-file-stable-sysfs-class-ubi)

The /sys/class/ubi0, /sys/class/ubi1, etc directories describe
UBI devices (UBI device 0, 1, etc). They contain general UBI
device information and per UBI volume information (each UBI
device may have many UBI volumes)

|  |
| --- |
| **/sys/class/udc/<udc>/a\_alt\_hnp\_support** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates if an OTG A-Host supports HNP at an alternate port.

|  |
| --- |
| **/sys/class/udc/<udc>/a\_hnp\_support** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates if an OTG A-Host supports HNP at this port.

|  |
| --- |
| **/sys/class/udc/<udc>/b\_hnp\_enable** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates if an OTG A-Host enabled HNP support.

|  |
| --- |
| **/sys/class/udc/<udc>/current\_speed** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates the current negotiated speed at this port.

|  |
| --- |
| **/sys/class/udc/<udc>/function** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Prints out name of currently running USB Gadget Driver.

|  |
| --- |
| **/sys/class/udc/<udc>/is\_a\_peripheral** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates that this port is the default Host on an OTG session
but HNP was used to switch roles.

|  |
| --- |
| **/sys/class/udc/<udc>/is\_otg** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates that this port support OTG.

|  |
| --- |
| **/sys/class/udc/<udc>/maximum\_speed** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates the maximum USB speed supported by this port.

|  |
| --- |
| **/sys/class/udc/<udc>/soft\_connect** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Allows users to disconnect data pullup resistors thus causing a
logical disconnection from the USB Host.

|  |
| --- |
| **/sys/class/udc/<udc>/srp** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Allows users to manually start Session Request Protocol.

|  |
| --- |
| **/sys/class/udc/<udc>/state** |

Defined on file [sysfs-class-udc](abi-stable-files.html#abi-file-stable-sysfs-class-udc)

Indicates current state of the USB Device Controller. Valid
states are: ‘not-attached’, ‘attached’, ‘powered’,
‘reconnecting’, ‘unauthenticated’, ‘default’, ‘addressed’,
‘configured’, and ‘suspended’; however not all USB Device
Controllers support reporting all states.

## Symbols under /sys/devices

|  |
| --- |
| **/sys/devices/\*/dev** |

Defined on file [sysfs-devices](abi-stable-files.html#abi-file-stable-sysfs-devices)

Major and minor numbers of the character device corresponding
to the device (in <major>:<minor> format).

|  |
| --- |
| **/sys/devices/\*/devspec** |

Defined on file [sysfs-devices](abi-stable-files.html#abi-file-stable-sysfs-devices)

If CONFIG\_OF is enabled, then this file is present. When
read, it returns full name of the device node.

|  |
| --- |
| **/sys/devices/\*/obppath** |

Defined on file [sysfs-devices](abi-stable-files.html#abi-file-stable-sysfs-devices)

If CONFIG\_OF is enabled, then this file is present. When
read, it returns full name of the device node.

|  |
| --- |
| **/sys/devices/\*/of\_node** |

Defined on file [sysfs-devices](abi-stable-files.html#abi-file-stable-sysfs-devices)

Any device associated with a device-tree node will have
an of\_path symlink pointing to the corresponding device
node in /sys/firmware/devicetree/

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/0000:XX:XX.X/keep\_cfg** |

Defined on file [sysfs-driver-misc-cp500](abi-stable-files.html#abi-file-stable-sysfs-driver-misc-cp500)

Flag which signals if FPGA shall keep or reload configuration
bitstream on reset. Normal FPGA behavior and default is to keep
configuration bitstream and to only reset the configured logic.

Reloading configuration on reset enables an update of the
configuration bitstream with a simple reboot. Otherwise it is
necessary to power cycle the device to reload the new
configuration bitstream.

This file is read/write. The values are as follows:
1 = keep configuration bitstream on reset, default
0 = reload configuration bitstream on reset

Users:
:   KEBA

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/0000:XX:XX.X/version** |

Defined on file [sysfs-driver-misc-cp500](abi-stable-files.html#abi-file-stable-sysfs-driver-misc-cp500)

Version of the FPGA configuration bitstream as printable string.
This file is read only.

Users:
:   KEBA

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/cap** |

Defined on file [sysfs-driver-dma-ioatdma](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-ioatdma)

Capabilities the DMA supports.Currently there are DMA\_PQ, DMA\_PQ\_VAL,
DMA\_XOR,DMA\_XOR\_VAL,DMA\_INTERRUPT.

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/intr\_coalesce** |

Defined on file [sysfs-driver-dma-ioatdma](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-ioatdma)

Tune-able interrupt delay value per channel basis.

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/ring\_active** |

Defined on file [sysfs-driver-dma-ioatdma](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-ioatdma)

The number of descriptors active in the ring.

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/ring\_size** |

Defined on file [sysfs-driver-dma-ioatdma](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-ioatdma)

Descriptor ring size, total number of descriptors available.

|  |
| --- |
| **/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/version** |

Defined on file [sysfs-driver-dma-ioatdma](abi-stable-files.html#abi-file-stable-sysfs-driver-dma-ioatdma)

Version of ioatdma device.

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/feature\_config\_id** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

This sysfs interface allows user to configure features at
runtime. The user can enable or disable features running at
firmware as well as the user can configure the parameters of
the features at runtime. The supported features are over
temperature and external watchdog. Here, the external watchdog
is completely different than the /dev/watchdog as the external
watchdog is running on the firmware and it is used to monitor
the health of firmware not APU(Linux). Also, the external
watchdog is interfaced outside of the zynqmp soc.

The supported config ids are for the feature configuration is,
1. PM\_FEATURE\_OVERTEMP\_STATUS = 1, the user can enable or
disable the over temperature feature.
2. PM\_FEATURE\_OVERTEMP\_VALUE = 2, the user can configure the
over temperature limit in Degree Celsius.
3. PM\_FEATURE\_EXTWDT\_STATUS = 3, the user can enable or disable
the external watchdog feature.
4. PM\_FEATURE\_EXTWDT\_VALUE = 4, the user can configure the
external watchdog feature.

Usage:

Select over temperature config ID to enable/disable feature
# echo 1 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id

Check over temperature config ID is selected or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
The expected result is 1.

Select over temperature config ID to configure OT limit
# echo 2 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id

Check over temperature config ID is selected or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
The expected result is 2.

Select external watchdog config ID to enable/disable feature
# echo 3 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id

Check external watchdog config ID is selected or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
The expected result is 3.

Select external watchdog config ID to configure time interval
# echo 4 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id

Check external watchdog config ID is selected or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
The expected result is 4.

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/feature\_config\_value** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

This sysfs interface allows to configure features at runtime.
The user can enable or disable features running at firmware.
Also, the user can configure the parameters of the features
at runtime. The supported features are over temperature and
external watchdog. Here, the external watchdog is completely
different than the /dev/watchdog as the external watchdog is
running on the firmware and it is used to monitor the health
of firmware not APU(Linux). Also, the external watchdog is
interfaced outside of the zynqmp soc.

By default the features are disabled in the firmware. The user
can enable features by querying appropriate config id of the
features.

The default limit for the over temperature is 90 Degree Celsius.
The default timer interval for the external watchdog is 570ms.

The supported config ids are for the feature configuration is,
1. PM\_FEATURE\_OVERTEMP\_STATUS = 1, the user can enable or
disable the over temperature feature.
2. PM\_FEATURE\_OVERTEMP\_VALUE = 2, the user can configure the
over temperature limit in Degree Celsius.
3. PM\_FEATURE\_EXTWDT\_STATUS = 3, the user can enable or disable
the external watchdog feature.
4. PM\_FEATURE\_EXTWDT\_VALUE = 4, the user can configure the
external watchdog feature.

Usage:

Enable over temperature feature
# echo 1 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 1 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the over temperature feature is enabled or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 1.

Disable over temperature feature
# echo 1 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 0 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the over temperature feature is disabled or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 0.

Configure over temperature limit to 50 Degree Celsius
# echo 2 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 50 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the over temperature limit is configured or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 50.

Enable external watchdog feature
# echo 3 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 1 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the external watchdog feature is enabled or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 1.

Disable external watchdog feature
# echo 3 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 0 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the external watchdog feature is disabled or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 0.

Configure external watchdog timer interval to 500ms
# echo 4 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_id
# echo 500 > /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value

Check whether the external watchdog timer interval is configured or not
# cat /sys/devices/platform/firmware:zynqmp-firmware/feature\_config\_value
The expected result is 500.

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/ggs\*** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

Read/Write PMU global general storage register value,
GLOBAL\_GEN\_STORAGE{0:3}.
Global general storage register that can be used
by system to pass information between masters.

The register is reset during system or power-on
resets. Three registers are used by the FSBL and
other Xilinx software products: GLOBAL\_GEN\_STORAGE{4:6}.

Usage:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/ggs0
# echo <value> > /sys/devices/platform/firmware\:zynqmp-firmware/ggs0
```

Example:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/ggs0
# echo 0x1234ABCD > /sys/devices/platform/firmware\:zynqmp-firmware/ggs0
```

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/health\_status** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

This sysfs interface allows to set the health status. If PMUFW
is compiled with CHECK\_HEALTHY\_BOOT, it will check the healthy
bit on FPD WDT expiration. If healthy bit is set by a user
application running in Linux, PMUFW will do APU only restart. If
healthy bit is not set during FPD WDT expiration, PMUFW will do
system restart.

Usage:

Set healthy bit:

```
# echo 1 > /sys/devices/platform/firmware\:zynqmp-firmware/health_status
```

Unset healthy bit:

```
# echo 0 > /sys/devices/platform/firmware\:zynqmp-firmware/health_status
```

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/pggs\*** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

Read/Write PMU persistent global general storage register
value, PERS\_GLOB\_GEN\_STORAGE{0:3}.
Persistent global general storage register that
can be used by system to pass information between
masters.

This register is only reset by the power-on reset
and maintains its value through a system reset.
Four registers are used by the FSBL and other Xilinx
software products: PERS\_GLOB\_GEN\_STORAGE{4:7}.
Register is reset only by a POR reset.

Usage:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/pggs0
# echo <value> > /sys/devices/platform/firmware\:zynqmp-firmware/pggs0
```

Example:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/pggs0
# echo 0x1234ABCD > /sys/devices/platform/firmware\:zynqmp-firmware/pggs0
```

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/firmware\:zynqmp-firmware/shutdown\_scope** |

Defined on file [sysfs-driver-firmware-zynqmp](abi-stable-files.html#abi-file-stable-sysfs-driver-firmware-zynqmp)

This sysfs interface allows to set the shutdown scope for the
next shutdown request. When the next shutdown is performed, the
platform specific portion of PSCI-system\_off can use the chosen
shutdown scope.

Following are available shutdown scopes(subtypes):

subsystem:
:   Only the APU along with all of its peripherals
    not used by other processing units will be
    shut down. This may result in the FPD power
    domain being shut down provided that no other
    processing unit uses FPD peripherals or DRAM.

ps\_only:
:   The complete PS will be shut down, including the
    RPU, PMU, etc. Only the PL domain (FPGA)
    remains untouched.

system:
:   The complete system/device is shut down.

Usage:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/shutdown_scope
# echo <scope> > /sys/devices/platform/firmware\:zynqmp-firmware/shutdown_scope
```

Example:

```
# cat /sys/devices/platform/firmware\:zynqmp-firmware/shutdown_scope
# echo "subsystem" > /sys/devices/platform/firmware\:zynqmp-firmware/shutdown_scope
```

Users:
:   Xilinx

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/boot\_progress** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the Data Process Unit board boot progress
state. Valid states are:
- 4 : OS starting.
- 5 : OS running.
- 6 : Low-Power Standby.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/dpu\_id** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows hardware Id of Data Process Unit board.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/perst\_rst** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/phy\_rst** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/tpm\_rst** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/usbphy\_rst** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow to reset hardware components of Data Process
Unit board. Respectively PCI, Ethernet PHY, TPM and USB PHY
resets.
Default values for all the attributes is 1. Writing 0 will
cause reset of the related component.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_aux\_pwr\_or\_reload** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_dpu\_thermal** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_from\_main\_board** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files expose the cause of the most recent reset of the Data
Processing Unit (DPU) board. The possible causes are:
- Power auxiliary outage or power reload.
- Thermal shutdown.
- Reset request from the main board.
Value 1 in file means this is reset cause, 0 - otherwise. Only one of
the above causes could be 1 at the same time, representing only last
reset cause.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/ufm\_upgrade** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show status of Unified Fabric Manager upgrade.
state. 0 - means upgrade is done, 1 - otherwise.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/agb\_spi\_burn\_en** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga\_spi\_burn\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow gearboxes and FPGA SPI flash burning.
The attributes are set 1 to enable burning, 0 - to disable.
If the system is in locked-down mode writing these files will
not be allowed.
The purpose of these files to allow line card Gearboxes and FPGA
burning during production flow.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_pn** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_version** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_version\_min** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD major and minor versions
and part number has been burned CPLD device on line card.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld\_upgrade\_en** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga\_upgrade\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow CPLD and FPGA burning. Value 1 in file means burning
is enabled, 0 - otherwise.
If the system is in locked-down mode writing these files will
not be allowed.
The purpose of these files to allow line card CPLD and FPGA
upgrade through the JTAG daisy-chain.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_pn** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_version** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_version\_min** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which FPGA major and minor versions
and part number has been burned FPGA device on line card.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/max\_power** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/config** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files provide the maximum powered required for line card
feeding and line card configuration Id.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/qsfp\_pwr\_en** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/pwr\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow to power on/off all QSFP ports and whole line card.
The attributes are set 1 for power on, 0 - for power off.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_aux\_pwr\_or\_ref** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_dc\_dc\_pwr\_fail** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_fpga\_not\_done** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_from\_chassis** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_line\_card** |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_pwr\_off\_from\_chassis** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the line reset cause, as following: power
auxiliary outage or power refresh, DC-to-DC power failure, FPGA reset
failed, line card reset failed, power off from chassis.
Value 1 in file means this is reset cause, 0 - otherwise. Only one of
the above causes could be 1 at the same time, representing only last
reset cause.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/vpd\_wp** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allow to overwrite line card VPD hardware write
protection mode. When attribute is set 1 - write protection is
disabled, when 0 - enabled.
Default is 0.
If the system is in locked-down mode writing this file will not
be allowed.
The purpose if this file is to allow line card VPD burning
during production flow.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic2\_health** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows 2-nd ASIC health status. The possible values are:
0 - health failed, 2 - health OK, 3 - ASIC in booting state.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_health** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows ASIC health status. The possible values are:
0 - health failed, 2 - health OK, 3 - ASIC in booting state.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_pg\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows ASIC Power Good status.
Value 1 in file means ASIC Power Good failed, 0 - otherwise.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_reset** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic2\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow to each of ASICs by writing 1.

The files are write only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_active\_image** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_auth\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_upgrade\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

The files represent BIOS statuses:

bios\_active\_image: location of current active BIOS image:
0: Top, 1: Bottom.
The reported value should correspond to value expected by OS
in case of BIOS safe mode is 0. This bit is related to Intel
top-swap feature of DualBios on the same flash.

bios\_auth\_fail: BIOS upgrade is failed because provided BIOS
image is not signed correctly.

bios\_upgrade\_fail: BIOS upgrade is failed by some other
reason not because authentication. For example due to
physical SPI flash problem.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd1\_boot\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd2\_boot\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files are related to clock boards status in system.
- clk\_brd1\_boot\_fail: warning about 1-st clock board failed to boot from CI.
- clk\_brd2\_boot\_fail: warning about 2-nd clock board failed to boot from CI.
- clk\_brd\_fail: error about common clock board boot failure.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd\_prog\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file enables programming of clock boards.
Default is 0 (programming disabled).
If the system is in locked-down mode writing this file will not be allowed.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/comm\_chnl\_ready** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file is used to indicate remote end (for example BMC) that system
host CPU is ready for sending telemetry data to remote end.
For indication the file should be written 1.

The file is write only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config1** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config2** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show system static topology identification
like system’s static I2C topology, number and type of FPGA
devices within the system and so on.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config3** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

The file indicates COME module hardware configuration.
The value is pushed by hardware through GPIO pins.
The purpose is to expose some minor BOM changes for the same system SKU.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_pn** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_pn** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_pn** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_pn** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_version\_min** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_version\_min** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_version\_min** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_version\_min** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD part numbers and minor
versions have been burned CPLD devices equipped on a
system.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_version** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_version** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD versions have been burned
on carrier and switch boards.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_version** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD versions have been burned
on LED or Gearbox board.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_version** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD versions have been burned
on LED board.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_pn** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_version** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_version\_min** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show with which CPLD part numbers, version and minor
versions have been burned the 5-th CPLD device equipped on a
system.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_ap\_reset** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_ap\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files aim to monitor the status of the External Root of Trust (EROT)
processor’s RESET output to the Application Processor (AP).
By reading this file, could be determined if the EROT has invalidated or
revoked AP Firmware, at which point it will hold the AP in RESET until a
valid firmware is loaded. This protects the AP from running an
unauthorized firmware. In the normal flow, the AP reset should be released
after the EROT validates the integrity of the FW, and it should be done so
as quickly as possible so that the AP boots before the CPU starts to
communicate to each ASIC.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_recovery** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_recovery** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_reset** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files aim to perform External Root of Trust (EROT) recovery
sequence after EROT device failure.
These EROT devices protect ASICs from unauthorized access and in normal
flow their reset should be released with system power – earliest power
up stage, so that EROTs can begin boot and authentication process before
CPU starts to communicate to ASICs.
Issuing a reset to the EROT while asserting the recovery signal will cause
the EROT Application Processor to enter recovery mode so that the EROT FW
can be updated/recovered.
For reset/recovery the related file should be toggled by 1/0.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_wp** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_wp** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow access to External Root of Trust (EROT) for reset
and recovery sequence after EROT device failure.
Default is 0 (programming disabled).
If the system is in locked-down mode writing this file will not be allowed.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/fan\_dir** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows the system fans direction:
forward direction - relevant bit is set 0;
reversed direction - relevant bit is set 1.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/global\_wp\_request** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file when written 1 activates request to allow access to
the write protected flashes. Such request can be performed only
for system equipped with BMC (Board Management Controller),
which can grant access to protected flashes. In case BMC allows
access - it will respond with “global\_wp\_response”. BMC decides
regarding time window of granted access. After granted window is
expired, BMC will change value back to 0.
Default value is 0.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/global\_wp\_response** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file, when set 1, indicates that access to protected
flashes have been granted to host CPU by BMC.
Default value is 0.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/jtag\_cap** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file indicates the available method of CPLD/FPGA devices
field update through the JTAG chain:

b00 - field update through LPC bus register memory space.
b01 - Reserved.
b10 - Reserved.
b11 - field update through CPU GPIOs bit-banging.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/jtag\_enable** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files enable and disable the access to the JTAG domain.
By default access to the JTAG domain is disabled.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_enable** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_enable** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow line cards enable state control.
Expected behavior:
When lc{n}\_enable is written 1, related line card is released
from the reset state, when 0 - is hold in reset state.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_pwr** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_pwr** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files switching line cards power on and off.
Expected behavior:
When lc{n}\_pwr is written 1, related line card is powered
on, when written 0 - powered off.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_rst\_mask** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_rst\_mask** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files clear line card reset bit enforced by ASIC, when it
sets it due to some abnormal ASIC behavior.
Expected behavior:
When lc{n}\_rst\_mask is written 1, related line card reset bit
is cleared, when written 0 - no effect.

The files are write only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lid\_open** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

1 - indicates that system lid is opened, otherwise 0.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/mac\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows to reset ASIC MT52132 when attribute is set 0
due to some abnormal ASIC behavior.
Expected behavior:
When mac\_reset is written 1, the ASIC MT52132 is released
from the reset state, when 0 - is hold in reset state.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/os\_started** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file, when written 1, indicates to programmable devices
that OS is taking control over it.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pcie\_asic\_reset\_dis** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows to retain ASIC up during PCIe root complex
reset, when attribute is set 1.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/phy\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows to reset PHY 88E1548 when attribute is set 0
due to some abnormal PHY behavior.
Expected behavior:
When phy\_reset is written 1, all PHY 88E1548 are released
from the reset state, when 0 - are hold in reset state.

The files are read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pm\_mgmt\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file assigns power management control ownership.
When power management control is provided by hardware, hardware
will automatically power off one or more line previously
powered line cards in case system power budget is getting
insufficient. It could be in case when some of power units lost
power good state.
When pm\_mgmt\_en is written 1, power management control by
software is enabled, 0 - power management control by hardware.
Note that for any setting of pm\_mgmt\_en attribute hardware will
not allow to power on any new line card in case system power
budget is insufficient.
Same in case software will try to power on several line cards
at once - hardware will power line cards while system has
enough power budget.
Default is 0.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu1\_on** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu2\_on** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pwr\_cycle** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pwr\_down** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files allow asserting system power cycling, switching
power supply units on and off and system’s main power domain
shutdown.
Expected behavior:
When pwr\_cycle is written 1: auxiliary power domain will go
down and after short period (about 1 second) up.
When psu1\_on or psu2\_on is written 1, related unit will be
disconnected from the power source, when written 0 - connected.
If both are written 1 - power supplies main power domain will
go down.
When pwr\_down is written 1, system’s main power domain will go
down.

The files are write only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu3\_on** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu4\_on** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files switching power supply units on and off.
Expected behavior:
When psu3\_on or psu4\_on is written 1, related unit will be
disconnected from the power source, when written 0 - connected.

The files are write only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pwr\_converter\_prog\_en** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file enables programming of power converters.
Default is 0 (programming disabled).
If the system is in locked-down mode writing this file will not be allowed.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/qsfp\_pwr\_good** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows QSFP ports power status. The value is set to 0
when one of any QSFP ports is plugged. The value is set to 1 when
there are no any QSFP ports are plugged.
The possible values are:
0 - Power good, 1 - Not power good.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_ac\_ok\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows the system reset cause due to AC power failure.
Value 1 in file means this is reset cause, 0 - otherwise.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_ac\_pwr\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_platform** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_soc** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sw\_pwr\_off** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the system reset causes, as following: reset
due to AC power failure, reset invoked from software by
assertion reset signal through CPLD. reset caused by signal
asserted by SOC through ACPI register, reset invoked from
software by assertion power off signal through CPLD.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_aux\_pwr\_or\_ref** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_asic\_thermal** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_hotswap\_or\_halt** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_hotswap\_or\_wd** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_fw\_reset** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_long\_pb** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_main\_pwr\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_short\_pb** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sw\_reset** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the system reset cause, as following: power
auxiliary outage or power refresh, ASIC thermal shutdown, halt,
hotswap, watchdog, firmware reset, long press power button,
short press power button, software reset. Value 1 in file means
this is reset cause, 0 - otherwise. Only one of the above
causes could be 1 at the same time, representing only last
reset cause.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_pwr\_fail** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_from\_comex** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_system** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_voltmon\_upgrade\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the system reset cause, as following: ComEx
power fail, reset from ComEx, system platform reset, reset
due to voltage monitor devices upgrade failure,
Value 1 in file means this is reset cause, 0 - otherwise.
Only one bit could be 1 at the same time, representing only
the last reset cause.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_thermal** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_wd** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_from\_asic** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_reload\_bios** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sff\_wd** |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_swb\_wd** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

These files show the system reset cause, as following:
COMEX thermal shutdown; wathchdog power off or reset was derived
by one of the next components: COMEX, switch board or by Small Form
Factor mezzanine, reset requested from ASIC, reset caused by BIOS
reload. Value 1 in file means this is reset cause, 0 - otherwise.
Only one of the above causes could be 1 at the same time, representing
only last reset cause.

The files are read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_long\_pwr\_pb** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file if set 1 indicates that system has been reset by
long press of power button.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_pwr\_converter\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows the system reset cause due to power converter
devices failure.
Value 1 in file means this is reset cause, 0 - otherwise.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_swb\_dc\_dc\_pwr\_fail** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file shows 1 in case the system reset happened due to the
failure of any DC-DC power converter devices equipped on the
switch board.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/select\_iio** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows iio devices selection.

Attribute select\_iio can be written with 0 or with 1. It
selects which one of iio devices can be accessed.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/shutdown\_unlock** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows to unlock ASIC after thermal shutdown event.
When system thermal shutdown is enforced by ASIC, ASIC is
getting locked and after system boot it will not be available.
Software can decide to unlock it by setting this attribute to
1 and then perform system power cycle by setting pwr\_cycle
attribute to 1 (power cycle of main power domain).
Before setting shutdown\_unlock to 1 it is recommended to
validate that system reboot cause is reset\_asic\_thermal or
reset\_thermal\_spc\_or\_pciesw.
In case shutdown\_unlock is not set 1, the only way to release
ASIC from locking - is full system power cycle through the
external power distribution unit.
Default is 1.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/shutdown\_unlock** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

When ASICs are getting overheated, system protection
hardware mechanism enforces system reboot. After system
reboot ASICs come up in locked state. To unlock ASICs,
this file should be written 1
Default value is 0.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/spi\_chnl\_select** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows SPI chip selection for External Root of Trust (EROT)
device Out-of-Band recovery.
File can be written with 0 or with 1. It selects which EROT can be accessed
through SPI device.

The file is read/write.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/ufm\_version** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file exposes the firmware version of burnable voltage
regulator devices.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/voltreg\_update\_status** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file exposes the configuration update status of burnable
voltage regulator devices. The status values are as following:
0 - OK; 1 - CRC failure; 2 = I2C failure; 3 - in progress.

The file is read only.

|  |
| --- |
| **/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/vpd\_wp** |

Defined on file [sysfs-driver-mlxreg-io](abi-stable-files.html#abi-file-stable-sysfs-driver-mlxreg-io)

This file allows to overwrite system VPD hardware write
protection when attribute is set 1.

The file is read/write.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/book\_id** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

the book ID of cpuX. Typically it is the hardware platform’s
identifier (rather than the kernel’s). The actual value is
architecture and platform dependent. it’s only used on s390.
Values: integer

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/book\_siblings** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of cpuX’s hardware threads within the same
book\_id. it’s only used on s390.
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/book\_siblings\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of cpuX’s hardware threads within the same
book\_id.
The format is like 0-3, 8-11, 14,17. it’s only used on s390.
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/cluster\_cpus** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of CPUs within the same cluster.
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/cluster\_cpus\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of CPUs within the same cluster.
The format is like 0-3, 8-11, 14,17.
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/cluster\_id** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

the cluster ID of cpuX. Typically it is the hardware platform’s
identifier (rather than the kernel’s). The actual value is
architecture and platform dependent.
Values: integer

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/core\_cpus** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of CPUs within the same core.
(deprecated name: “thread\_siblings”)
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/core\_cpus\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of CPUs within the same core.
The format is like 0-3, 8-11, 14,17.
(deprecated name: “thread\_siblings\_list”).
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/core\_id** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

the CPU core ID of cpuX. Typically it is the hardware platform’s
identifier (rather than the kernel’s). The actual value is
architecture and platform dependent.
Values: integer

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/die\_cpus** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of CPUs within the same die.
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/die\_cpus\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of CPUs within the same die.
The format is like 0-3, 8-11, 14,17.
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/die\_id** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

the CPU die ID of cpuX. Typically it is the hardware platform’s
identifier (rather than the kernel’s). The actual value is
architecture and platform dependent.
Values: integer

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/drawer\_id** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

the drawer ID of cpuX. Typically it is the hardware platform’s
identifier (rather than the kernel’s). The actual value is
architecture and platform dependent. it’s only used on s390.
Values: integer

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/drawer\_siblings** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of cpuX’s hardware threads within the same
drawer\_id. it’s only used on s390.
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/drawer\_siblings\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of cpuX’s hardware threads within the same
drawer\_id.
The format is like 0-3, 8-11, 14,17. it’s only used on s390.
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/package\_cpus** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

internal kernel map of the CPUs sharing the same physical\_package\_id.
(deprecated name: “core\_siblings”).
Values: hexadecimal bitmask.

|  |
| --- |
| **/sys/devices/system/cpu/cpuX/topology/package\_cpus\_list** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

human-readable list of CPUs sharing the same physical\_package\_id.
The format is like 0-3, 8-11, 14,17.
(deprecated name: “core\_siblings\_list”)
Values: decimal list.

|  |
| --- |
| **/sys/devices/system/cpu/cpu[0-9]+/dscr** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

Default value for the Data Stream Control Register (DSCR) on
a CPU.
This default value is used when the kernel is executing and
for any process that has not set the DSCR itself.
If a process ever sets the DSCR (via direct access to the
SPR) that value will be persisted for that process and used
on any CPU where it executes (overriding the value described
here).
If set by a process it will be inherited by child processes.
Values: 64 bit unsigned integer (bit field)

|  |
| --- |
| **/sys/devices/system/cpu/dscr\_default** |

Defined on file [sysfs-devices-system-cpu](abi-stable-files.html#abi-file-stable-sysfs-devices-system-cpu)

Writes are equivalent to writing to
/sys/devices/system/cpu/cpuN/dscr on all CPUs.
Reads return the last written value or 0.
This value is not a global default: it is a way to set
all per-CPU defaults at the same time.
Values: 64 bit unsigned integer (bit field)

|  |
| --- |
| **/sys/devices/system/node/has\_cpu** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Nodes that have one or more CPUs.

|  |
| --- |
| **/sys/devices/system/node/has\_high\_memory** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Nodes that have regular or high memory.
Depends on CONFIG\_HIGHMEM.

|  |
| --- |
| **/sys/devices/system/node/has\_normal\_memory** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Nodes that have regular memory.

|  |
| --- |
| **/sys/devices/system/node/nodeX** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

When CONFIG\_NUMA is enabled, this is a directory containing
information on node X such as what CPUs are local to the
node. Each file is detailed next.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The node’s relationship to other nodes for access class “Y”.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/initiators/** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The directory containing symlinks to memory initiator
nodes that have class “Y” access to this target node’s
memory. CPUs and other memory initiators in nodes not in
the list accessing this node’s memory may have different
performance.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/initiators/read\_bandwidth** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

This node’s read bandwidth in MB/s when accessed from
nodes found in this access class’s linked initiators.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/initiators/read\_latency** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

This node’s read latency in nanoseconds when accessed
from nodes found in this access class’s linked initiators.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/initiators/write\_bandwidth** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

This node’s write bandwidth in MB/s when accessed from
found in this access class’s linked initiators.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/initiators/write\_latency** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

This node’s write latency in nanoseconds when access
from nodes found in this class’s linked initiators.

|  |
| --- |
| **/sys/devices/system/node/nodeX/accessY/targets/** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The directory containing symlinks to memory targets that
this initiator node has class “Y” access.

|  |
| --- |
| **/sys/devices/system/node/nodeX/compact** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

When this file is written to, all memory within that node
will be compacted. When it completes, memory will be freed
into blocks which have as many contiguous pages as possible

|  |
| --- |
| **/sys/devices/system/node/nodeX/cpulist** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The CPUs associated to the node.

|  |
| --- |
| **/sys/devices/system/node/nodeX/cpumap** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The node’s cpumap.

|  |
| --- |
| **/sys/devices/system/node/nodeX/distance** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Distance between the node and all the other nodes
in the system.

|  |
| --- |
| **/sys/devices/system/node/nodeX/hugepages/hugepages-<size>/** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The node’s huge page size control/query attributes.
See [HugeTLB Pages](mm/hugetlbpage.html)

|  |
| --- |
| **/sys/devices/system/node/nodeX/meminfo** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Provides information about the node’s distribution and memory
utilization. Similar to /proc/meminfo, see [The /proc Filesystem](../filesystems/proc.html)

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_failure/delayed** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Of the raw poisoned pages on a NUMA node, how many pages are
delayed by memory error recovery attempt. Delayed poisoned
pages usually will be retried by kernel.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_failure/failed** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Of the raw poisoned pages on a NUMA node, how many pages are
failed by memory error recovery attempt. This usually means
a key recovery operation failed.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_failure/ignored** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Of the raw poisoned pages on a NUMA node, how many pages are
ignored by memory error recovery attempt, usually because
support for this type of pages is unavailable, and kernel
gives up the recovery.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_failure/recovered** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Of the raw poisoned pages on a NUMA node, how many pages are
recovered by memory error recovery attempt.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_failure/total** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The total number of raw poisoned pages (pages containing
corrupted data due to memory errors) on a NUMA node.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The directory containing attributes for the memory-side cache
level ‘Y’.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/address\_mode** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The address mode: 0 for reserved, 1 for extended-linear.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/indexing** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The caches associativity indexing: 0 for direct mapped,
non-zero if indexed.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/line\_size** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The number of bytes accessed from the next cache level on a
cache miss.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/size** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The size of this memory side cache in bytes.

|  |
| --- |
| **/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/write\_policy** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The cache write policy: 0 for write-back, 1 for write-through,
other or unknown.

|  |
| --- |
| **/sys/devices/system/node/nodeX/numastat** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The node’s hit/miss statistics, in units of pages.
See [Numa policy hit/miss statistics](numastat.html)

|  |
| --- |
| **/sys/devices/system/node/nodeX/reclaim** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Perform user-triggered proactive reclaim on a NUMA node.
This interface is equivalent to the memcg variant.

See [Control Group v2](cgroup-v2.html)

|  |
| --- |
| **/sys/devices/system/node/nodeX/vmstat** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The node’s zoned virtual memory statistics.
This is a superset of numastat.

|  |
| --- |
| **/sys/devices/system/node/nodeX/x86/sgx\_total\_bytes** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

The total amount of SGX physical memory in bytes.

|  |
| --- |
| **/sys/devices/system/node/online** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Nodes that are online.

|  |
| --- |
| **/sys/devices/system/node/possible** |

Defined on file [sysfs-devices-node](abi-stable-files.html#abi-file-stable-sysfs-devices-node)

Nodes that could be possibly become online at some point.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/info/current\_kb** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

Current size (in KiB) of this domain’s memory
reservation.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/info/high\_kb** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

Amount (in KiB) of high memory in the balloon.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/info/low\_kb** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

Amount (in KiB) of low (or normal) memory in the
balloon.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/max\_retry\_count** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

The maximum number of times the balloon driver will
attempt to increase the balloon before giving up. See
also ‘retry\_count’ below.
A value of zero means retry forever and is the default one.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/max\_schedule\_delay** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

The limit that ‘schedule\_delay’ (see below) will be
increased to. The default value is 32 seconds.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/retry\_count** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

The current number of times that the balloon driver
has attempted to increase the size of the balloon.
The default value is one. With max\_retry\_count being
zero (unlimited), this means that the driver will attempt
to retry with a ‘schedule\_delay’ delay.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/schedule\_delay** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

The time (in seconds) to wait between attempts to
increase the balloon. Each time the balloon cannot be
increased, ‘schedule\_delay’ is increased (until
‘max\_schedule\_delay’ is reached at which point it
will use the max value).

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/scrub\_pages** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

Control scrubbing pages before returning them to Xen for others domains
use. Can be set with xen\_scrub\_pages cmdline
parameter. Default value controlled with CONFIG\_XEN\_SCRUB\_PAGES\_DEFAULT.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/target** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

The target number of pages to adjust this domain’s
memory reservation to.

|  |
| --- |
| **/sys/devices/system/xen\_memory/xen\_memory0/target\_kb** |

Defined on file [sysfs-devices-system-xen\_memory](abi-stable-files.html#abi-file-stable-sysfs-devices-system-xen-memory)

As target above, except the value is in KiB.

## Symbols under /sys/firmware

|  |
| --- |
| **/sys/firmware/acpi/pm\_profile** |

Defined on file [sysfs-acpi-pmprofile](abi-stable-files.html#abi-file-stable-sysfs-acpi-pmprofile)

The ACPI pm\_profile sysfs interface exposes the preferred
power management (and performance) profile of the platform
as provided in the ACPI FADT Preferred\_PM\_Profile field.

The integer value is directly passed as retrieved from the FADT.

Values: For the possible values refer to the Preferred\_PM\_Profile field
:   definition in Table 5.9 “FADT Format”, Section 5.2.9 “Fixed ACPI
    Description Table (FADT)” of the ACPI specification.

    As of ACPI 6.5, the following values are defined:

    |  |  |
    | --- | --- |
    | 0 | Unspecified |
    | 1 | Desktop |
    | 2 | Mobile |
    | 3 | Workstation |
    | 4 | Enterprise Server |
    | 5 | SOHO Server |
    | 6 | Appliance PC |
    | 7 | Performance Server |
    | 8 | Tablet |
    | >8 | Reserved |

|  |
| --- |
| **/sys/firmware/opal/dump** |

Defined on file [sysfs-firmware-opal-dump](abi-stable-files.html#abi-file-stable-sysfs-firmware-opal-dump)

This directory exposes interfaces for interacting with
the FSP and platform dumps through OPAL firmware interface.

This is only for the powerpc/powernv platform.

|  |  |
| --- | --- |
| initiate\_dump: | When ‘1’ is written to it, we will initiate a dump. Read this file for supported commands. |
| 0xXX-0xYYYY: | A directory for dump of type 0xXX and id 0xYYYY (in hex). The name of this directory should not be relied upon to be in this format, only that it’s unique among all dumps. For determining the type and ID of the dump, use the id and type files. Do not rely on any particular size of dump type or dump id. |

Each dump has the following files:

|  |  |
| --- | --- |
| id: | An ASCII representation of the dump ID in hex (e.g. ‘0x01’) |
| type: | An ASCII representation of the type of dump in the format “0x%x %s” with the ID in hex and a description of the dump type (or ‘unknown’). Type ‘0xffffffff unknown’ is used when we could not get the type from firmware. e.g. ‘0x02 System/Platform Dump’ |
| dump: | A binary file containing the dump. The size of the dump is the size of this file. |
| acknowledge: | When ‘ack’ is written to this, we will acknowledge that we’ve retrieved the dump to the service processor. It will then remove it, making the dump inaccessible. Reading this file will get a list of supported actions. |

|  |
| --- |
| **/sys/firmware/opal/elog** |

Defined on file [sysfs-firmware-opal-elog](abi-stable-files.html#abi-file-stable-sysfs-firmware-opal-elog)

This directory exposes error log entries retrieved
through the OPAL firmware interface.

Each error log is identified by a unique ID and will
exist until explicitly acknowledged to firmware.

Each log entry has a directory in /sys/firmware/opal/elog.

Log entries may be purged by the service processor
before retrieved by firmware or retrieved/acknowledged by
Linux if there is no room for more log entries.

In the event that Linux has retrieved the log entries
but not explicitly acknowledged them to firmware and
the service processor needs more room for log entries,
the only remaining copy of a log message may be in
Linux.

Typically, a user space daemon will monitor for new
entries, read them out and acknowledge them.

The service processor may be able to store more log
entries than firmware can, so after you acknowledge
an event from Linux you may instantly get another one
from the queue that was generated some time in the past.

The raw log format is a binary format. We currently
do not parse this at all in kernel, leaving it up to
user space to solve the problem. In future, we may
do more parsing in kernel and add more files to make
it easier for simple user space processes to extract
more information.

For each log entry (directory), there are the following
files:

|  |  |
| --- | --- |
| id: | An ASCII representation of the ID of the error log, in hex - e.g. “0x01”. |
| type: | An ASCII representation of the type id and description of the type of error log. Currently just “0x00 PEL” - platform error log. In the future there may be additional types. |
| raw: | A read-only binary file that can be read to get the raw log entry. These are <16kb, often just hundreds of bytes and “average” 2kb. |
| acknowledge: | Writing ‘ack’ to this file will acknowledge the error log to firmware (and in turn the service processor, if applicable). Shortly after acknowledging it, the log entry will be removed from sysfs. Reading this file will list the supported operations (currently just acknowledge). |

## Symbols under /sys/fs

|  |
| --- |
| **/sys/fs/o2cb/** |

Defined on file [o2cb](abi-stable-files.html#abi-file-stable-o2cb)

Ocfs2-tools looks at ‘interface-revision’ for versioning
information. Each logmask/ file controls a set of debug prints
and can be written into with the strings “allow”, “deny”, or
“off”. Reading the file returns the current state.

Users:
:   ocfs2-tools. It’s sufficient to mail proposed changes to
    [ocfs2-devel@lists.linux.dev](mailto:ocfs2-devel%40lists.linux.dev).

|  |
| --- |
| **/sys/fs/orangefs/acache/\*** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Attribute cache configurable settings.

|  |
| --- |
| **/sys/fs/orangefs/capcache/\*** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Capability cache configurable settings.

|  |
| --- |
| **/sys/fs/orangefs/ccache/\*** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Credential cache configurable settings.

|  |
| --- |
| **/sys/fs/orangefs/ncache/\*** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Name cache configurable settings.

|  |
| --- |
| **/sys/fs/orangefs/op\_timeout\_secs** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Service operation timeout in seconds.

|  |
| --- |
| **/sys/fs/orangefs/perf\_counter\_reset** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

echo a 0 or a 1 into perf\_counter\_reset to
reset all the counters in
/sys/fs/orangefs/perf\_counters
except ones with PINT\_PERF\_PRESERVE set.

|  |
| --- |
| **/sys/fs/orangefs/perf\_counters/\*** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Counters and settings for various caches.
Read only.

|  |
| --- |
| **/sys/fs/orangefs/perf\_history\_size** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

The perf\_counters cache statistics have N, or
perf\_history\_size, samples. The default is
one.

Every perf\_time\_interval\_secs the (first)
samples are reset.

If N is greater than one, the “current” set
of samples is reset, and the samples from the
other N-1 intervals remain available.

|  |
| --- |
| **/sys/fs/orangefs/perf\_time\_interval\_secs** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

Length of perf counter intervals in
seconds.

|  |
| --- |
| **/sys/fs/orangefs/slot\_timeout\_secs** |

Defined on file [sysfs-fs-orangefs](abi-stable-files.html#abi-file-stable-sysfs-fs-orangefs)

“Slot” timeout in seconds. A “slot”
is an indexed buffer in the shared
memory segment used for communication
between the kernel module and userspace.
Slots are requested and waited for,
the wait times out after slot\_timeout\_secs.

## Symbols under /sys/hypervisor

|  |
| --- |
| **/sys/hypervisor/compilation/compile\_date** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Contains the build time stamp of the Xen hypervisor
Might return “<denied>” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/compilation/compiled\_by** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Contains information who built the Xen hypervisor
Might return “<denied>” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/compilation/compiler** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Compiler which was used to build the Xen hypervisor
Might return “<denied>” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/properties/capabilities** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Space separated list of supported guest system types. Each type
is in the format: <class>-<major>.<minor>-<arch>
With:

> |  |  |
> | --- | --- |
> | <class>: | “xen” -- x86: paravirtualized, arm: standard “hvm” -- x86 only: fully virtualized |
> | <major>: | major guest interface version |
> | <minor>: | minor guest interface version |
> | <arch>: | architecture, e.g.: “x86\_32”: 32 bit x86 guest without PAE “x86\_32p”: 32 bit x86 guest with PAE “x86\_64”: 64 bit x86 guest “armv7l”: 32 bit arm guest “aarch64”: 64 bit arm guest |

|  |
| --- |
| **/sys/hypervisor/properties/changeset** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Changeset of the hypervisor (git commit)
Might return “<denied>” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/properties/features** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Features the Xen hypervisor supports for the guest as defined
in include/xen/interface/features.h printed as a hex value.

|  |
| --- |
| **/sys/hypervisor/properties/pagesize** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Default page size of the hypervisor printed as a hex value.
Might return “0” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/properties/virtual\_start** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Virtual address of the hypervisor as a hex value.

|  |
| --- |
| **/sys/hypervisor/start\_flags/\*** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
All bits in Xen’s start-flags are represented as
boolean files, returning ‘1’ if set, ‘0’ otherwise.
This takes the place of the defunct /proc/xen/capabilities,
which would contain “control\_d” on dom0, and be empty
otherwise. This flag is now exposed as “initdomain” in
addition to the “privileged” flag; all other possible flags
are accessible as “unknownXX”.

|  |
| --- |
| **/sys/hypervisor/type** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
Type of hypervisor:
“xen”: Xen hypervisor

|  |
| --- |
| **/sys/hypervisor/uuid** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
UUID of the guest as known to the Xen hypervisor.

|  |
| --- |
| **/sys/hypervisor/version/extra** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
The Xen version is in the format <major>.<minor><extra>
This is the <extra> part of it.
Might return “<denied>” in case of special security settings
in the hypervisor.

|  |
| --- |
| **/sys/hypervisor/version/major** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
The Xen version is in the format <major>.<minor><extra>
This is the <major> part of it.

|  |
| --- |
| **/sys/hypervisor/version/minor** |

Defined on file [sysfs-hypervisor-xen](abi-stable-files.html#abi-file-stable-sysfs-hypervisor-xen)

If running under Xen:
The Xen version is in the format <major>.<minor><extra>
This is the <minor> part of it.

## Symbols under /sys/kernel

|  |
| --- |
| **/sys/kernel/notes** |

Defined on file [sysfs-kernel-notes](abi-stable-files.html#abi-file-stable-sysfs-kernel-notes)

The /sys/kernel/notes file contains the binary representation
of the running vmlinux’s .notes section.

|  |
| --- |
| **/sys/kernel/time/aux\_clocks/<ID>/enable** |

Defined on file [sysfs-kernel-time-aux-clocks](abi-stable-files.html#abi-file-stable-sysfs-kernel-time-aux-clocks)

Controls the enablement of auxiliary clock timekeepers.

## Symbols under /sys/module

|  |
| --- |
| **/sys/module/<MODULENAME>** |

Defined on file [sysfs-module](abi-stable-files.html#abi-file-stable-sysfs-module)

The name of the module that is in the kernel. This
module name will always show up if the module is loaded as a
dynamic module. If it is built directly into the kernel, it
will only show up if it has a version or at least one
parameter.

Note: The conditions of creation in the built-in case are not
by design and may be removed in the future.

|  |
| --- |
| **/sys/module/<MODULENAME>/parameters** |

Defined on file [sysfs-module](abi-stable-files.html#abi-file-stable-sysfs-module)

This directory contains individual files that are each
individual parameters of the module that are able to be
changed at runtime. See the individual module
documentation as to the contents of these parameters and
what they accomplish.

Note: The individual parameter names and values are not
considered stable, only the fact that they will be
placed in this location within sysfs. See the
individual driver documentation for details as to the
stability of the different parameters.

|  |
| --- |
| **/sys/module/<MODULENAME>/refcnt** |

Defined on file [sysfs-module](abi-stable-files.html#abi-file-stable-sysfs-module)

If the module is able to be unloaded from the kernel, this file
will contain the current reference count of the module.

Note: If the module is built into the kernel, or if the
CONFIG\_MODULE\_UNLOAD kernel configuration value is not enabled,
this file will not be present.

|  |
| --- |
| **/sys/module/<MODULENAME>/srcversion** |

Defined on file [sysfs-module](abi-stable-files.html#abi-file-stable-sysfs-module)

If the module source has MODULE\_VERSION, this file will contain
the checksum of the source code.

|  |
| --- |
| **/sys/module/<MODULENAME>/version** |

Defined on file [sysfs-module](abi-stable-files.html#abi-file-stable-sysfs-module)

If the module source has MODULE\_VERSION, this file will contain
the version of the source code.

## A notification mechanism for thermal related events

|  |
| --- |
| **A notification mechanism for thermal related events** |

Defined on file [thermal-notification](abi-stable-files.html#abi-file-stable-thermal-notification)

This interface enables notification for thermal related events.
The notification is in the form of a netlink event.

## Audit Login Session ID

|  |
| --- |
| **Audit Login Session ID** |

Defined on file [procfs-audit\_loginuid](abi-stable-files.html#abi-file-stable-procfs-audit-loginuid)

The /proc/$pid/sessionid pseudofile is read to get the
audit login session ID of process $pid as a decimal
unsigned int (%u, u32). It is set automatically,
serially assigned with each new login.

Users:
:   audit and login applications

## Audit Login UID

|  |
| --- |
| **Audit Login UID** |

Defined on file [procfs-audit\_loginuid](abi-stable-files.html#abi-file-stable-procfs-audit-loginuid)

The /proc/$pid/loginuid pseudofile is written to set and
read to get the audit login UID of process $pid as a
decimal unsigned int (%u, u32). If it is unset,
permissions are not needed to set it. The accessor must
have CAP\_AUDIT\_CONTROL in the initial user namespace to
write it if it has been set. It cannot be written again
if AUDIT\_FEATURE\_LOGINUID\_IMMUTABLE is enabled. It
cannot be unset if AUDIT\_FEATURE\_ONLY\_UNSET\_LOGINUID is
enabled.

Users:
:   audit and login applications

## The kernel syscall interface

|  |
| --- |
| **The kernel syscall interface** |

Defined on file [syscalls](abi-stable-files.html#abi-file-stable-syscalls)

This interface matches much of the POSIX interface and is based
on it and other Unix based interfaces. It will only be added to
over time, and not have things removed from it.

Note that this interface is different for every architecture
that Linux supports. Please see the architecture-specific
documentation for details on the syscall numbers that are to be
mapped to each syscall.

## vDSO

|  |
| --- |
| **vDSO** |

Defined on file [vdso](abi-stable-files.html#abi-file-stable-vdso)

On some architectures, when the kernel loads any userspace program it
maps an ELF DSO into that program’s address space. This DSO is called
the vDSO and it often contains useful and highly-optimized alternatives
to real syscalls.

These functions are called according to your platform’s ABI. On many
platforms they are called just like ordinary C function. On other platforms
(ex: powerpc) they are called with the same convention as system calls which
is different from ordinary C functions. Call them from a sensible context.
(For example, if you set CS on x86 to something strange, the vDSO functions are
within their rights to crash.) In addition, if you pass a bad
pointer to a vDSO function, you might get SIGSEGV instead of -EFAULT.

To find the DSO, parse the auxiliary vector passed to the program’s
entry point. The AT\_SYSINFO\_EHDR entry will point to the vDSO.

The vDSO uses symbol versioning; whenever you request a symbol from the
vDSO, specify the version you are expecting.

Programs that dynamically link to glibc will use the vDSO automatically.
Otherwise, you can use the reference parser in
tools/testing/selftests/vDSO/parse\_vdso.c.

Unless otherwise noted, the set of symbols with any given version and the
ABI of those symbols is considered stable. It may vary across architectures,
though.

Note:
:   As of this writing, this ABI documentation as been confirmed for x86\_64.
    The maintainers of the other vDSO-using architectures should confirm
    that it is correct for their architecture.
