# The ALSA Driver API

> 출처(원문): https://docs.kernel.org/sound/kernel-api/alsa-driver-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The ALSA Driver API

## Management of Cards and Devices

### Card Management

int snd\_device\_alloc(struct [device](../../driver-api/infrastructure.html#c.device "device") \*\*dev\_p, struct snd\_card \*card)
:   Allocate and initialize [`struct device`](../../driver-api/infrastructure.html#c.device "device") for sound devices

**Parameters**

`struct device **dev_p`
:   pointer to store the allocated device

`struct snd_card *card`
:   card to assign, optional

**Description**

For releasing the allocated device, call [`put_device()`](../../driver-api/infrastructure.html#c.put_device "put_device").

int snd\_card\_new(struct [device](../../driver-api/infrastructure.html#c.device "device") \*parent, int idx, const char \*xid, struct [module](#c.snd_card_new "module") \*module, int extra\_size, struct snd\_card \*\*card\_ret)
:   create and initialize a soundcard structure

**Parameters**

`struct device *parent`
:   the parent device object

`int idx`
:   card index (address) [0 ... (SNDRV\_CARDS-1)]

`const char *xid`
:   card identification (ASCII string)

`struct module *module`
:   top level module for locking

`int extra_size`
:   allocate this extra size after the main soundcard structure

`struct snd_card **card_ret`
:   the pointer to store the created card instance

**Description**

> The function allocates snd\_card instance via kzalloc with the given
> space for the driver to use freely. The allocated `struct is` stored
> in the given card\_ret pointer.

**Return**

Zero if successful or a negative error code.

int snd\_devm\_card\_new(struct [device](../../driver-api/infrastructure.html#c.device "device") \*parent, int idx, const char \*xid, struct [module](#c.snd_devm_card_new "module") \*module, size\_t extra\_size, struct snd\_card \*\*card\_ret)
:   managed snd\_card object creation

**Parameters**

`struct device *parent`
:   the parent device object

`int idx`
:   card index (address) [0 ... (SNDRV\_CARDS-1)]

`const char *xid`
:   card identification (ASCII string)

`struct module *module`
:   top level module for locking

`size_t extra_size`
:   allocate this extra size after the main soundcard structure

`struct snd_card **card_ret`
:   the pointer to store the created card instance

**Description**

This function works like [`snd_card_new()`](#c.snd_card_new "snd_card_new") but manages the allocated resource
via devres, i.e. you don’t need to free explicitly.

When a snd\_card object is created with this function and registered via
[`snd_card_register()`](#c.snd_card_register "snd_card_register"), the very first devres action to call [`snd_card_free()`](#c.snd_card_free "snd_card_free")
is added automatically. In that way, the resource disconnection is assured
at first, then released in the expected order.

If an error happens at the probe before [`snd_card_register()`](#c.snd_card_register "snd_card_register") is called and
there have been other devres resources, you’d need to free the card manually
via [`snd_card_free()`](#c.snd_card_free "snd_card_free") call in the error; otherwise it may lead to UAF due to
devres call orders. You can use [`snd_card_free_on_error()`](#c.snd_card_free_on_error "snd_card_free_on_error") helper for
handling it more easily.

**Return**

zero if successful, or a negative error code

int snd\_card\_free\_on\_error(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, int ret)
:   a small helper for handling devm probe errors

**Parameters**

`struct device *dev`
:   the managed device object

`int ret`
:   the return code from the probe callback

**Description**

This function handles the explicit [`snd_card_free()`](#c.snd_card_free "snd_card_free") call at the error from
the probe callback. It’s just a small helper for simplifying the error
handling with the managed devices.

**Return**

zero if successful, or a negative error code

struct snd\_card \*snd\_card\_ref(int idx)
:   Get the card object from the index

**Parameters**

`int idx`
:   the card index

**Description**

Returns a card object corresponding to the given index or NULL if not found.
Release the object via [`snd_card_unref()`](#c.snd_card_unref "snd_card_unref").

**Return**

a card object or NULL

void snd\_card\_disconnect(struct snd\_card \*card)
:   disconnect all APIs from the file-operations (user space)

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

> Disconnects all APIs from the file-operations (user space).

**Return**

Zero, otherwise a negative error code.

**Note**

The current implementation replaces all active file->f\_op with special
:   dummy file operations (they do nothing except release).

void snd\_card\_disconnect\_sync(struct snd\_card \*card)
:   disconnect card and wait until files get closed

**Parameters**

`struct snd_card *card`
:   card object to disconnect

**Description**

This calls [`snd_card_disconnect()`](#c.snd_card_disconnect "snd_card_disconnect") for disconnecting all belonging components
and waits until all pending files get closed.
It assures that all accesses from user-space finished so that the driver
can release its resources gracefully.

void snd\_card\_free\_when\_closed(struct snd\_card \*card)
:   Disconnect the card, free it later eventually

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

Unlike [`snd_card_free()`](#c.snd_card_free "snd_card_free"), this function doesn’t try to release the card
resource immediately, but tries to disconnect at first. When the card
is still in use, the function returns before freeing the resources.
The card resources will be freed when the refcount gets to zero.

**Return**

zero if successful, or a negative error code

void snd\_card\_free(struct snd\_card \*card)
:   frees given soundcard structure

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

This function releases the soundcard structure and the all assigned
devices automatically. That is, you don’t have to release the devices
by yourself.

This function waits until the all resources are properly released.

**Return**

Zero. Frees all associated devices and frees the control
interface associated to given soundcard.

void snd\_card\_set\_id(struct snd\_card \*card, const char \*nid)
:   set card identification name

**Parameters**

`struct snd_card *card`
:   soundcard structure

`const char *nid`
:   new identification string

**Description**

> This function sets the card identification and checks for name
> collisions.

int snd\_card\_add\_dev\_attr(struct snd\_card \*card, const struct attribute\_group \*group)
:   Append a new sysfs attribute group to card

**Parameters**

`struct snd_card *card`
:   card instance

`const struct attribute_group *group`
:   attribute group to append

**Return**

zero if successful, or a negative error code

int snd\_card\_register(struct snd\_card \*card)
:   register the soundcard

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

> This function registers all the devices assigned to the soundcard.
> Until calling this, the ALSA control interface is blocked from the
> external accesses. Thus, you should call this function at the end
> of the initialization of the card.

**Return**

Zero otherwise a negative error code if the registration failed.

int snd\_component\_add(struct snd\_card \*card, const char \*component)
:   add a component string

**Parameters**

`struct snd_card *card`
:   soundcard structure

`const char *component`
:   the component id string

**Description**

> This function adds the component id string to the supported list.
> The component can be referred from the alsa-lib.

**Return**

Zero otherwise a negative error code.

int snd\_card\_file\_add(struct snd\_card \*card, struct [file](#c.snd_card_file_add "file") \*file)
:   add the file to the file list of the card

**Parameters**

`struct snd_card *card`
:   soundcard structure

`struct file *file`
:   file pointer

**Description**

> This function adds the file to the file linked-list of the card.
> This linked-list is used to keep tracking the connection state,
> and to avoid the release of busy resources by hotplug.

**Return**

zero or a negative error code.

int snd\_card\_file\_remove(struct snd\_card \*card, struct [file](#c.snd_card_file_remove "file") \*file)
:   remove the file from the file list

**Parameters**

`struct snd_card *card`
:   soundcard structure

`struct file *file`
:   file pointer

**Description**

> This function removes the file formerly added to the card via
> [`snd_card_file_add()`](#c.snd_card_file_add "snd_card_file_add") function.
> If all files are removed and [`snd_card_free_when_closed()`](#c.snd_card_free_when_closed "snd_card_free_when_closed") was
> called beforehand, it processes the pending release of
> resources.

**Return**

Zero or a negative error code.

int snd\_power\_ref\_and\_wait(struct snd\_card \*card)
:   wait until the card gets powered up

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

Take the power\_ref reference count of the given card, and
wait until the card gets powered up to SNDRV\_CTL\_POWER\_D0 state.
The refcount is down again while sleeping until power-up, hence this
function can be used for syncing the floating control ops accesses,
typically around calling control ops.

The caller needs to pull down the refcount via [`snd_power_unref()`](#c.snd_power_unref "snd_power_unref") later
no matter whether the error is returned from this function or not.

**Return**

Zero if successful, or a negative error code.

int snd\_power\_wait(struct snd\_card \*card)
:   wait until the card gets powered up (old form)

**Parameters**

`struct snd_card *card`
:   soundcard structure

**Description**

Wait until the card gets powered up to SNDRV\_CTL\_POWER\_D0 state.

**Return**

Zero if successful, or a negative error code.

### Device Components

int snd\_device\_new(struct snd\_card \*card, enum snd\_device\_type type, void \*device\_data, const struct snd\_device\_ops \*ops)
:   create an ALSA device component

**Parameters**

`struct snd_card *card`
:   the card instance

`enum snd_device_type type`
:   the device type, SNDRV\_DEV\_XXX

`void *device_data`
:   the data pointer of this device

`const struct snd_device_ops *ops`
:   the operator table

**Description**

Creates a new device component for the given data pointer.
The device will be assigned to the card and managed together
by the card.

The data pointer plays a role as the identifier, too, so the
pointer address must be unique and unchanged.

**Return**

Zero if successful, or a negative error code on failure.

void snd\_device\_disconnect(struct snd\_card \*card, void \*device\_data)
:   disconnect the device

**Parameters**

`struct snd_card *card`
:   the card instance

`void *device_data`
:   the data pointer to disconnect

**Description**

Turns the device into the disconnection state, invoking
dev\_disconnect callback, if the device was already registered.

Usually called from [`snd_card_disconnect()`](#c.snd_card_disconnect "snd_card_disconnect").

**Return**

Zero if successful, or a negative error code on failure or if the
device not found.

void snd\_device\_free(struct snd\_card \*card, void \*device\_data)
:   release the device from the card

**Parameters**

`struct snd_card *card`
:   the card instance

`void *device_data`
:   the data pointer to release

**Description**

Removes the device from the list on the card and invokes the
callbacks, dev\_disconnect and dev\_free, corresponding to the state.
Then release the device.

int snd\_device\_register(struct snd\_card \*card, void \*device\_data)
:   register the device

**Parameters**

`struct snd_card *card`
:   the card instance

`void *device_data`
:   the data pointer to register

**Description**

Registers the device which was already created via
[`snd_device_new()`](#c.snd_device_new "snd_device_new"). Usually this is called from [`snd_card_register()`](#c.snd_card_register "snd_card_register"),
but it can be called later if any new devices are created after
invocation of [`snd_card_register()`](#c.snd_card_register "snd_card_register").

**Return**

Zero if successful, or a negative error code on failure or if the
device not found.

### Module requests and Device File Entries

void snd\_request\_card(int card)
:   try to load the card module

**Parameters**

`int card`
:   the card number

**Description**

Tries to load the module “snd-card-X” for the given card number
via request\_module. Returns immediately if already loaded.

void \*snd\_lookup\_minor\_data(unsigned int minor, int type)
:   get user data of a registered device

**Parameters**

`unsigned int minor`
:   the minor number

`int type`
:   device type (SNDRV\_DEVICE\_TYPE\_XXX)

**Description**

Checks that a minor device with the specified type is registered, and returns
its user data pointer.

This function increments the reference counter of the card instance
if an associated instance with the given minor number and type is found.
The caller must call [`snd_card_unref()`](#c.snd_card_unref "snd_card_unref") appropriately later.

**Return**

The user data pointer if the specified device is found. `NULL`
otherwise.

int snd\_register\_device(int type, struct snd\_card \*card, int dev, const struct file\_operations \*f\_ops, void \*private\_data, struct [device](#c.snd_register_device "device") \*device)
:   Register the ALSA device file for the card

**Parameters**

`int type`
:   the device type, SNDRV\_DEVICE\_TYPE\_XXX

`struct snd_card *card`
:   the card instance

`int dev`
:   the device index

`const struct file_operations *f_ops`
:   the file operations

`void *private_data`
:   user pointer for f\_ops->open()

`struct device *device`
:   the device to register

**Description**

Registers an ALSA device file for the given card.
The operators have to be set in reg parameter.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_unregister\_device(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev)
:   unregister the device on the given card

**Parameters**

`struct device *dev`
:   the device instance

**Description**

Unregisters the device file already registered via
[`snd_register_device()`](#c.snd_register_device "snd_register_device").

**Return**

Zero if successful, or a negative error code on failure.

### Memory Management Helpers

int copy\_to\_user\_fromio(void \_\_user \*dst, volatile const void \_\_iomem \*src, size\_t count)
:   copy data from mmio-space to user-space

**Parameters**

`void __user *dst`
:   the destination pointer on user-space

`const volatile void __iomem *src`
:   the source pointer on mmio

`size_t count`
:   the data size to copy in bytes

**Description**

Copies the data from mmio-space to user-space.

**Return**

Zero if successful, or non-zero on failure.

size\_t copy\_to\_iter\_fromio(const void \_\_iomem \*src, size\_t count, struct iov\_iter \*dst)
:   copy data from mmio-space to iov\_iter

**Parameters**

`const void __iomem *src`
:   the source pointer on mmio

`size_t count`
:   the data size to copy in bytes

`struct iov_iter *dst`
:   the destination iov\_iter

**Description**

Copies the data from mmio-space to iov\_iter.

**Return**

number of bytes to be copied

int copy\_from\_user\_toio(volatile void \_\_iomem \*dst, const void \_\_user \*src, size\_t count)
:   copy data from user-space to mmio-space

**Parameters**

`volatile void __iomem *dst`
:   the destination pointer on mmio-space

`const void __user *src`
:   the source pointer on user-space

`size_t count`
:   the data size to copy in bytes

**Description**

Copies the data from user-space to mmio-space.

**Return**

Zero if successful, or non-zero on failure.

size\_t copy\_from\_iter\_toio(void \_\_iomem \*dst, size\_t count, struct iov\_iter \*src)
:   copy data from iov\_iter to mmio-space

**Parameters**

`void __iomem *dst`
:   the destination pointer on mmio-space

`size_t count`
:   the data size to copy in bytes

`struct iov_iter *src`
:   the source iov\_iter

**Description**

Copies the data from iov\_iter to mmio-space.

**Return**

number of bytes to be copied

int snd\_dma\_alloc\_dir\_pages(int type, struct [device](#c.snd_dma_alloc_dir_pages "device") \*device, enum dma\_data\_direction dir, size\_t size, struct snd\_dma\_buffer \*dmab)
:   allocate the buffer area according to the given type and direction

**Parameters**

`int type`
:   the DMA buffer type

`struct device *device`
:   the device pointer

`enum dma_data_direction dir`
:   DMA direction

`size_t size`
:   the buffer size to allocate

`struct snd_dma_buffer *dmab`
:   buffer allocation record to store the allocated data

**Description**

Calls the memory-allocator function for the corresponding
buffer type.

**Return**

Zero if the buffer with the given size is allocated successfully,
otherwise a negative value on error.

int snd\_dma\_alloc\_pages\_fallback(int type, struct [device](#c.snd_dma_alloc_pages_fallback "device") \*device, size\_t size, struct snd\_dma\_buffer \*dmab)
:   allocate the buffer area according to the given type with fallback

**Parameters**

`int type`
:   the DMA buffer type

`struct device *device`
:   the device pointer

`size_t size`
:   the buffer size to allocate

`struct snd_dma_buffer *dmab`
:   buffer allocation record to store the allocated data

**Description**

Calls the memory-allocator function for the corresponding
buffer type. When no space is left, this function reduces the size and
tries to allocate again. The size actually allocated is stored in
res\_size argument.

**Return**

Zero if the buffer with the given size is allocated successfully,
otherwise a negative value on error.

void snd\_dma\_free\_pages(struct snd\_dma\_buffer \*dmab)
:   release the allocated buffer

**Parameters**

`struct snd_dma_buffer *dmab`
:   the buffer allocation record to release

**Description**

Releases the allocated buffer via `snd_dma_alloc_pages()`.

struct snd\_dma\_buffer \*snd\_devm\_alloc\_dir\_pages(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, int type, enum dma\_data\_direction dir, size\_t size)
:   allocate the buffer and manage with devres

**Parameters**

`struct device *dev`
:   the device pointer

`int type`
:   the DMA buffer type

`enum dma_data_direction dir`
:   DMA direction

`size_t size`
:   the buffer size to allocate

**Description**

Allocate buffer pages depending on the given type and manage using devres.
The pages will be released automatically at the device removal.

Unlike `snd_dma_alloc_pages()`, this function requires the real device pointer,
hence it can’t work with SNDRV\_DMA\_TYPE\_CONTINUOUS or
SNDRV\_DMA\_TYPE\_VMALLOC type.

**Return**

the snd\_dma\_buffer object at success, or NULL if failed

int snd\_dma\_buffer\_mmap(struct snd\_dma\_buffer \*dmab, struct vm\_area\_struct \*area)
:   perform mmap of the given DMA buffer

**Parameters**

`struct snd_dma_buffer *dmab`
:   buffer allocation information

`struct vm_area_struct *area`
:   VM area information

**Return**

zero if successful, or a negative error code

void snd\_dma\_buffer\_sync(struct snd\_dma\_buffer \*dmab, enum snd\_dma\_sync\_mode mode)
:   sync DMA buffer between CPU and device

**Parameters**

`struct snd_dma_buffer *dmab`
:   buffer allocation information

`enum snd_dma_sync_mode mode`
:   sync mode

dma\_addr\_t snd\_sgbuf\_get\_addr(struct snd\_dma\_buffer \*dmab, size\_t offset)
:   return the physical address at the corresponding offset

**Parameters**

`struct snd_dma_buffer *dmab`
:   buffer allocation information

`size_t offset`
:   offset in the ring buffer

**Return**

the physical address

struct page \*snd\_sgbuf\_get\_page(struct snd\_dma\_buffer \*dmab, size\_t offset)
:   return the physical page at the corresponding offset

**Parameters**

`struct snd_dma_buffer *dmab`
:   buffer allocation information

`size_t offset`
:   offset in the ring buffer

**Return**

the page pointer

unsigned int snd\_sgbuf\_get\_chunk\_size(struct snd\_dma\_buffer \*dmab, unsigned int ofs, unsigned int size)
:   compute the max chunk size with continuous pages on sg-buffer

**Parameters**

`struct snd_dma_buffer *dmab`
:   buffer allocation information

`unsigned int ofs`
:   offset in the ring buffer

`unsigned int size`
:   the requested size

**Return**

the chunk size

## PCM API

### PCM Core

const char \*snd\_pcm\_format\_name(snd\_pcm\_format\_t format)
:   Return a name string for the given PCM format

**Parameters**

`snd_pcm_format_t format`
:   PCM format

**Return**

the format name string

int snd\_pcm\_new\_stream(struct snd\_pcm \*pcm, int stream, int substream\_count)
:   create a new PCM stream

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

`int stream`
:   the stream direction, SNDRV\_PCM\_STREAM\_XXX

`int substream_count`
:   the number of substreams

**Description**

Creates a new stream for the pcm.
The corresponding stream on the pcm must have been empty before
calling this, i.e. zero must be given to the argument of
[`snd_pcm_new()`](#c.snd_pcm_new "snd_pcm_new").

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_new(struct snd\_card \*card, const char \*id, int device, int playback\_count, int capture\_count, struct snd\_pcm \*\*rpcm)
:   create a new PCM instance

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *id`
:   the id string

`int device`
:   the device index (zero based)

`int playback_count`
:   the number of substreams for playback

`int capture_count`
:   the number of substreams for capture

`struct snd_pcm **rpcm`
:   the pointer to store the new pcm instance

**Description**

Creates a new PCM instance.

The pcm operators have to be set afterwards to the new instance
via [`snd_pcm_set_ops()`](#c.snd_pcm_set_ops "snd_pcm_set_ops").

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_new\_internal(struct snd\_card \*card, const char \*id, int device, int playback\_count, int capture\_count, struct snd\_pcm \*\*rpcm)
:   create a new internal PCM instance

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *id`
:   the id string

`int device`
:   the device index (zero based - shared with normal PCMs)

`int playback_count`
:   the number of substreams for playback

`int capture_count`
:   the number of substreams for capture

`struct snd_pcm **rpcm`
:   the pointer to store the new pcm instance

**Description**

Creates a new internal PCM instance with no userspace device or procfs
entries. This is used by ASoC Back End PCMs in order to create a PCM that
will only be used internally by kernel drivers. i.e. it cannot be opened
by userspace. It provides existing ASoC components drivers with a substream
and access to any private data.

The pcm operators have to be set afterwards to the new instance
via [`snd_pcm_set_ops()`](#c.snd_pcm_set_ops "snd_pcm_set_ops").

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_notify(struct [snd\_pcm\_notify](#c.snd_pcm_notify "snd_pcm_notify") \*notify, int nfree)
:   Add/remove the notify list

**Parameters**

`struct snd_pcm_notify *notify`
:   PCM notify list

`int nfree`
:   0 = register, 1 = unregister

**Description**

This adds the given notifier to the global list so that the callback is
called for each registered PCM devices. This exists only for PCM OSS
emulation, so far.

**Return**

zero if successful, or a negative error code

void snd\_pcm\_set\_ops(struct snd\_pcm \*pcm, int direction, const struct snd\_pcm\_ops \*ops)
:   set the PCM operators

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

`int direction`
:   stream direction, SNDRV\_PCM\_STREAM\_XXX

`const struct snd_pcm_ops *ops`
:   the operator table

**Description**

Sets the given PCM operators to the pcm instance.

void snd\_pcm\_set\_sync\_per\_card(struct snd\_pcm\_substream \*substream, struct snd\_pcm\_hw\_params \*params, const unsigned char \*id, unsigned int len)
:   set the PCM sync id with card number

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream

`struct snd_pcm_hw_params *params`
:   modified hardware parameters

`const unsigned char *id`
:   identifier (max 12 bytes)

`unsigned int len`
:   identifier length (max 12 bytes)

**Description**

Sets the PCM sync identifier for the card with zero padding.

User space or any user should use this 16-byte identifier for a comparison only
to check if two IDs are similar or different. Special case is the identifier
containing only zeros. Interpretation for this combination is - empty (not set).
The contents of the identifier should not be interpreted in any other way.

The synchronization ID must be unique per clock source (usually one sound card,
but multiple soundcard may use one PCM word clock source which means that they
are fully synchronized).

This routine composes this ID using card number in first four bytes and
12-byte additional ID. When other ID composition is used (e.g. for multiple
sound cards), make sure that the composition does not clash with this
composition scheme.

int snd\_interval\_refine(struct snd\_interval \*i, const struct snd\_interval \*v)
:   refine the interval value of configurator

**Parameters**

`struct snd_interval *i`
:   the interval value to refine

`const struct snd_interval *v`
:   the interval value to refer to

**Description**

Refines the interval value with the reference value.
The interval is changed to the range satisfying both intervals.
The interval status (min, max, integer, etc.) are evaluated.

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

void snd\_interval\_div(const struct snd\_interval \*a, const struct snd\_interval \*b, struct snd\_interval \*c)
:   refine the interval value with division

**Parameters**

`const struct snd_interval *a`
:   dividend

`const struct snd_interval *b`
:   divisor

`struct snd_interval *c`
:   quotient

**Description**

c = a / b

Returns non-zero if the value is changed, zero if not changed.

void snd\_interval\_muldivk(const struct snd\_interval \*a, const struct snd\_interval \*b, unsigned int k, struct snd\_interval \*c)
:   refine the interval value

**Parameters**

`const struct snd_interval *a`
:   dividend 1

`const struct snd_interval *b`
:   dividend 2

`unsigned int k`
:   divisor (as integer)

`struct snd_interval *c`
:   result

**Description**

c = a \* b / k

Returns non-zero if the value is changed, zero if not changed.

void snd\_interval\_mulkdiv(const struct snd\_interval \*a, unsigned int k, const struct snd\_interval \*b, struct snd\_interval \*c)
:   refine the interval value

**Parameters**

`const struct snd_interval *a`
:   dividend 1

`unsigned int k`
:   dividend 2 (as integer)

`const struct snd_interval *b`
:   divisor

`struct snd_interval *c`
:   result

**Description**

c = a \* k / b

Returns non-zero if the value is changed, zero if not changed.

int snd\_interval\_ratnum(struct snd\_interval \*i, unsigned int rats\_count, const struct snd\_ratnum \*rats, unsigned int \*nump, unsigned int \*denp)
:   refine the interval value

**Parameters**

`struct snd_interval *i`
:   interval to refine

`unsigned int rats_count`
:   number of ratnum\_t

`const struct snd_ratnum *rats`
:   ratnum\_t array

`unsigned int *nump`
:   pointer to store the resultant numerator

`unsigned int *denp`
:   pointer to store the resultant denominator

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_interval\_ratden(struct snd\_interval \*i, unsigned int rats\_count, const struct snd\_ratden \*rats, unsigned int \*nump, unsigned int \*denp)
:   refine the interval value

**Parameters**

`struct snd_interval *i`
:   interval to refine

`unsigned int rats_count`
:   number of `struct ratden`

`const struct snd_ratden *rats`
:   `struct ratden` array

`unsigned int *nump`
:   pointer to store the resultant numerator

`unsigned int *denp`
:   pointer to store the resultant denominator

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_interval\_list(struct snd\_interval \*i, unsigned int count, const unsigned int \*list, unsigned int mask)
:   refine the interval value from the list

**Parameters**

`struct snd_interval *i`
:   the interval value to refine

`unsigned int count`
:   the number of elements in the list

`const unsigned int *list`
:   the value list

`unsigned int mask`
:   the bit-mask to evaluate

**Description**

Refines the interval value from the list.
When mask is non-zero, only the elements corresponding to bit 1 are
evaluated.

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_interval\_ranges(struct snd\_interval \*i, unsigned int count, const struct snd\_interval \*ranges, unsigned int mask)
:   refine the interval value from the list of ranges

**Parameters**

`struct snd_interval *i`
:   the interval value to refine

`unsigned int count`
:   the number of elements in the list of ranges

`const struct snd_interval *ranges`
:   the ranges list

`unsigned int mask`
:   the bit-mask to evaluate

**Description**

Refines the interval value from the list of ranges.
When mask is non-zero, only the elements corresponding to bit 1 are
evaluated.

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_pcm\_hw\_rule\_add(struct snd\_pcm\_runtime \*runtime, unsigned int cond, int var, snd\_pcm\_hw\_rule\_func\_t func, void \*private, int dep, ...)
:   add the hw-constraint rule

**Parameters**

`struct snd_pcm_runtime *runtime`
:   the pcm runtime instance

`unsigned int cond`
:   condition bits

`int var`
:   the variable to evaluate

`snd_pcm_hw_rule_func_t func`
:   the evaluation function

`void *private`
:   the private data pointer passed to function

`int dep`
:   the dependent variables

`...`
:   variable arguments

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_mask(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_hw\_param\_t var, u\_int32\_t mask)
:   apply the given bitmap mask constraint

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the mask

`u_int32_t mask`
:   the bitmap mask

**Description**

Apply the constraint of the given bitmap mask to a 32-bit mask parameter.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_mask64(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_hw\_param\_t var, u\_int64\_t mask)
:   apply the given bitmap mask constraint

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the mask

`u_int64_t mask`
:   the 64bit bitmap mask

**Description**

Apply the constraint of the given bitmap mask to a 64-bit mask parameter.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_integer(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_hw\_param\_t var)
:   apply an integer constraint to an interval

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the integer constraint

**Description**

Apply the constraint of integer to an interval parameter.

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_pcm\_hw\_constraint\_minmax(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_hw\_param\_t var, unsigned int min, unsigned int max)
:   apply a min/max range constraint to an interval

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the range

`unsigned int min`
:   the minimal value

`unsigned int max`
:   the maximal value

**Description**

Apply the min/max range constraint to an interval parameter.

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_pcm\_hw\_constraint\_list(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var, const struct [snd\_pcm\_hw\_constraint\_list](#c.snd_pcm_hw_constraint_list "snd_pcm_hw_constraint_list") \*l)
:   apply a list of constraints to a parameter

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the list constraint

`const struct snd_pcm_hw_constraint_list *l`
:   list

**Description**

Apply the list of constraints to an interval parameter.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_ranges(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var, const struct [snd\_pcm\_hw\_constraint\_ranges](#c.snd_pcm_hw_constraint_ranges "snd_pcm_hw_constraint_ranges") \*r)
:   apply list of range constraints to a parameter

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the list of range constraints

`const struct snd_pcm_hw_constraint_ranges *r`
:   ranges

**Description**

Apply the list of range constraints to an interval parameter.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_ratnums(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var, const struct [snd\_pcm\_hw\_constraint\_ratnums](#c.snd_pcm_hw_constraint_ratnums "snd_pcm_hw_constraint_ratnums") \*r)
:   apply ratnums constraint to a parameter

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the ratnums constraint

`const struct snd_pcm_hw_constraint_ratnums *r`
:   `struct snd_ratnums` constriants

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_ratdens(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var, const struct [snd\_pcm\_hw\_constraint\_ratdens](#c.snd_pcm_hw_constraint_ratdens "snd_pcm_hw_constraint_ratdens") \*r)
:   apply ratdens constraint to a parameter

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the ratdens constraint

`const struct snd_pcm_hw_constraint_ratdens *r`
:   `struct snd_ratdens` constriants

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_msbits(struct snd\_pcm\_runtime \*runtime, unsigned int cond, unsigned int width, unsigned int msbits)
:   add a hw constraint msbits rule

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`unsigned int width`
:   sample bits width

`unsigned int msbits`
:   msbits width

**Description**

This constraint will set the number of most significant bits (msbits) if a
sample format with the specified width has been select. If width is set to 0
the msbits will be set for any sample format with a width larger than the
specified msbits.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_step(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var, unsigned long step)
:   add a hw constraint step rule

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the step constraint

`unsigned long step`
:   step size

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_constraint\_pow2(struct snd\_pcm\_runtime \*runtime, unsigned int cond, snd\_pcm\_hw\_param\_t var)
:   add a hw constraint power-of-2 rule

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int cond`
:   condition bits

`snd_pcm_hw_param_t var`
:   hw\_params variable to apply the power-of-2 constraint

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_rule\_noresample(struct snd\_pcm\_runtime \*runtime, unsigned int base\_rate)
:   add a rule to allow disabling hw resampling

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`unsigned int base_rate`
:   the rate at which the hardware does not resample

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_param\_value(const struct snd\_pcm\_hw\_params \*params, snd\_pcm\_hw\_param\_t var, int \*dir)
:   return **params** field **var** value

**Parameters**

`const struct snd_pcm_hw_params *params`
:   the hw\_params instance

`snd_pcm_hw_param_t var`
:   parameter to retrieve

`int *dir`
:   pointer to the direction (-1,0,1) or `NULL`

**Return**

The value for field **var** if it’s fixed in configuration space
defined by **params**. -`EINVAL` otherwise.

int snd\_pcm\_hw\_param\_first(struct snd\_pcm\_substream \*pcm, struct snd\_pcm\_hw\_params \*params, snd\_pcm\_hw\_param\_t var, int \*dir)
:   refine config space and return minimum value

**Parameters**

`struct snd_pcm_substream *pcm`
:   PCM instance

`struct snd_pcm_hw_params *params`
:   the hw\_params instance

`snd_pcm_hw_param_t var`
:   parameter to retrieve

`int *dir`
:   pointer to the direction (-1,0,1) or `NULL`

**Description**

Inside configuration space defined by **params** remove from **var** all
values > minimum. Reduce configuration space accordingly.

**Return**

The minimum, or a negative error code on failure.

int snd\_pcm\_hw\_param\_last(struct snd\_pcm\_substream \*pcm, struct snd\_pcm\_hw\_params \*params, snd\_pcm\_hw\_param\_t var, int \*dir)
:   refine config space and return maximum value

**Parameters**

`struct snd_pcm_substream *pcm`
:   PCM instance

`struct snd_pcm_hw_params *params`
:   the hw\_params instance

`snd_pcm_hw_param_t var`
:   parameter to retrieve

`int *dir`
:   pointer to the direction (-1,0,1) or `NULL`

**Description**

Inside configuration space defined by **params** remove from **var** all
values < maximum. Reduce configuration space accordingly.

**Return**

The maximum, or a negative error code on failure.

int snd\_pcm\_hw\_params\_bits(const struct snd\_pcm\_hw\_params \*p)
:   Get the number of bits per the sample.

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hardware parameters

**Return**

The number of bits per sample based on the format,
subformat and msbits the specified hw params has.

int snd\_pcm\_lib\_ioctl(struct snd\_pcm\_substream \*substream, unsigned int cmd, void \*arg)
:   a generic PCM ioctl callback

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

`unsigned int cmd`
:   ioctl command

`void *arg`
:   ioctl argument

**Description**

Processes the generic ioctl commands for PCM.
Can be passed as the ioctl callback for PCM ops.

**Return**

Zero if successful, or a negative error code on failure.

void snd\_pcm\_period\_elapsed\_under\_stream\_lock(struct snd\_pcm\_substream \*substream)
:   update the status of runtime for the next period under acquired lock of PCM substream.

**Parameters**

`struct snd_pcm_substream *substream`
:   the instance of pcm substream.

**Description**

This function is called when the batch of audio data frames as the same size as the period of
buffer is already processed in audio data transmission.

The call of function updates the status of runtime with the latest position of audio data
transmission, checks overrun and underrun over buffer, awaken user processes from waiting for
available audio data frames, sampling audio timestamp, and performs stop or drain the PCM
substream according to configured threshold.

The function is intended to use for the case that PCM driver operates audio data frames under
acquired lock of PCM substream; e.g. in callback of any operation of `snd_pcm_ops` in process
context. In any interrupt context, it’s preferrable to use `snd_pcm_period_elapsed()` instead
since lock of PCM substream should be acquired in advance.

Developer should pay enough attention that some callbacks in `snd_pcm_ops` are done by the call of
function:

* .pointer - to retrieve current position of audio data transmission by frame count or XRUN state.
* .trigger - with SNDRV\_PCM\_TRIGGER\_STOP at XRUN or DRAINING state.
* .get\_time\_info - to retrieve audio time stamp if needed.

Even if more than one periods have elapsed since the last call, you have to call this only once.

void snd\_pcm\_period\_elapsed(struct snd\_pcm\_substream \*substream)
:   update the status of runtime for the next period by acquiring lock of PCM substream.

**Parameters**

`struct snd_pcm_substream *substream`
:   the instance of PCM substream.

**Description**

This function is mostly similar to `snd_pcm_period_elapsed_under_stream_lock()` except for
acquiring lock of PCM substream voluntarily.

It’s typically called by any type of IRQ handler when hardware IRQ occurs to notify event that
the batch of audio data frames as the same size as the period of buffer is already processed in
audio data transmission.

int snd\_pcm\_add\_chmap\_ctls(struct snd\_pcm \*pcm, int stream, const struct snd\_pcm\_chmap\_elem \*chmap, int max\_channels, unsigned long private\_value, struct snd\_pcm\_chmap \*\*info\_ret)
:   create channel-mapping control elements

**Parameters**

`struct snd_pcm *pcm`
:   the assigned PCM instance

`int stream`
:   stream direction

`const struct snd_pcm_chmap_elem *chmap`
:   channel map elements (for query)

`int max_channels`
:   the max number of channels for the stream

`unsigned long private_value`
:   the value passed to each kcontrol’s private\_value field

`struct snd_pcm_chmap **info_ret`
:   store `struct snd_pcm_chmap` instance if non-NULL

**Description**

Create channel-mapping control elements assigned to the given PCM stream(s).

**Return**

Zero if successful, or a negative error value.

void snd\_pcm\_stream\_lock(struct snd\_pcm\_substream \*substream)
:   Lock the PCM stream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This locks the PCM stream’s spinlock or mutex depending on the nonatomic
flag of the given substream. This also takes the global link rw lock
(or rw sem), too, for avoiding the race with linked streams.

void snd\_pcm\_stream\_unlock(struct snd\_pcm\_substream \*substream)
:   Unlock the PCM stream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This unlocks the PCM stream that has been locked via [`snd_pcm_stream_lock()`](#c.snd_pcm_stream_lock "snd_pcm_stream_lock").

void snd\_pcm\_stream\_lock\_irq(struct snd\_pcm\_substream \*substream)
:   Lock the PCM stream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This locks the PCM stream like [`snd_pcm_stream_lock()`](#c.snd_pcm_stream_lock "snd_pcm_stream_lock") and disables the local
IRQ (only when nonatomic is false). In nonatomic case, this is identical
as [`snd_pcm_stream_lock()`](#c.snd_pcm_stream_lock "snd_pcm_stream_lock").

void snd\_pcm\_stream\_unlock\_irq(struct snd\_pcm\_substream \*substream)
:   Unlock the PCM stream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This is a counter-part of [`snd_pcm_stream_lock_irq()`](#c.snd_pcm_stream_lock_irq "snd_pcm_stream_lock_irq").

void snd\_pcm\_stream\_unlock\_irqrestore(struct snd\_pcm\_substream \*substream, unsigned long flags)
:   Unlock the PCM stream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`unsigned long flags`
:   irq flags

**Description**

This is a counter-part of [`snd_pcm_stream_lock_irqsave()`](#c.snd_pcm_stream_lock_irqsave "snd_pcm_stream_lock_irqsave").

void snd\_pcm\_set\_state(struct snd\_pcm\_substream \*substream, snd\_pcm\_state\_t state)
:   Set the PCM runtime state with stream lock

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`snd_pcm_state_t state`
:   state to set

snd\_pcm\_state\_t snd\_pcm\_get\_state(struct snd\_pcm\_substream \*substream)
:   Read the PCM runtime state with stream lock

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Return**

the current PCM state

int snd\_pcm\_hw\_params\_choose(struct snd\_pcm\_substream \*pcm, struct snd\_pcm\_hw\_params \*params)
:   choose a configuration defined by **params**

**Parameters**

`struct snd_pcm_substream *pcm`
:   PCM instance

`struct snd_pcm_hw_params *params`
:   the hw\_params instance

**Description**

Choose one configuration from configuration space defined by **params**.
The configuration chosen is that obtained fixing in this order:
first access, first format, first subformat, min channels,
min rate, min period time, max buffer size, min tick time

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_start(struct snd\_pcm\_substream \*substream)
:   start all linked streams

**Parameters**

`struct snd_pcm_substream *substream`
:   the PCM substream instance

**Return**

Zero if successful, or a negative error code.
The stream lock must be acquired before calling this function.

int snd\_pcm\_stop(struct snd\_pcm\_substream \*substream, snd\_pcm\_state\_t state)
:   try to stop all running streams in the substream group

**Parameters**

`struct snd_pcm_substream *substream`
:   the PCM substream instance

`snd_pcm_state_t state`
:   PCM state after stopping the stream

**Description**

The state of each stream is then changed to the given state unconditionally.

**Return**

Zero if successful, or a negative error code.

int snd\_pcm\_drain\_done(struct snd\_pcm\_substream \*substream)
:   stop the DMA only when the given stream is playback

**Parameters**

`struct snd_pcm_substream *substream`
:   the PCM substream

**Description**

After stopping, the state is changed to SETUP.
Unlike [`snd_pcm_stop()`](#c.snd_pcm_stop "snd_pcm_stop"), this affects only the given stream.

**Return**

Zero if successful, or a negative error code.

int snd\_pcm\_stop\_xrun(struct snd\_pcm\_substream \*substream)
:   stop the running streams as XRUN

**Parameters**

`struct snd_pcm_substream *substream`
:   the PCM substream instance

**Description**

This stops the given running substream (and all linked substreams) as XRUN.
Unlike [`snd_pcm_stop()`](#c.snd_pcm_stop "snd_pcm_stop"), this function takes the substream lock by itself.

**Return**

Zero if successful, or a negative error code.

int snd\_pcm\_suspend\_all(struct snd\_pcm \*pcm)
:   trigger SUSPEND to all substreams in the given pcm

**Parameters**

`struct snd_pcm *pcm`
:   the PCM instance

**Description**

Takes and releases pcm->open\_mutex to serialize against
concurrent open/close while walking the substreams.

After this call, all streams are changed to SUSPENDED state.

**Return**

Zero if successful (or **pcm** is `NULL`), or a negative error code.

int snd\_pcm\_prepare(struct snd\_pcm\_substream \*substream, struct [file](#c.snd_pcm_prepare "file") \*file)
:   prepare the PCM substream to be triggerable

**Parameters**

`struct snd_pcm_substream *substream`
:   the PCM substream instance

`struct file *file`
:   file to refer f\_flags

**Return**

Zero if successful, or a negative error code.

int snd\_pcm\_kernel\_ioctl(struct snd\_pcm\_substream \*substream, unsigned int cmd, void \*arg)
:   Execute PCM ioctl in the kernel-space

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`unsigned int cmd`
:   IOCTL cmd

`void *arg`
:   IOCTL argument

**Description**

The function is provided primarily for OSS layer and USB gadget drivers,
and it allows only the limited set of ioctls (hw\_params, sw\_params,
prepare, start, drain, drop, forward).

**Return**

zero if successful, or a negative error code

int snd\_pcm\_lib\_default\_mmap(struct snd\_pcm\_substream \*substream, struct vm\_area\_struct \*area)
:   Default PCM data mmap function

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`struct vm_area_struct *area`
:   VMA

**Description**

This is the default mmap handler for PCM data. When mmap pcm\_ops is NULL,
this function is invoked implicitly.

**Return**

zero if successful, or a negative error code

int snd\_pcm\_lib\_mmap\_iomem(struct snd\_pcm\_substream \*substream, struct vm\_area\_struct \*area)
:   Default PCM data mmap function for I/O mem

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`struct vm_area_struct *area`
:   VMA

**Description**

When your hardware uses the iomapped pages as the hardware buffer and
wants to mmap it, pass this function as mmap pcm\_ops. Note that this
is supposed to work only on limited architectures.

**Return**

zero if successful, or a negative error code

int snd\_pcm\_stream\_linked(struct snd\_pcm\_substream \*substream)
:   Check whether the substream is linked with others

**Parameters**

`struct snd_pcm_substream *substream`
:   substream to check

**Return**

true if the given substream is being linked with others

snd\_pcm\_stream\_lock\_irqsave

`snd_pcm_stream_lock_irqsave (substream, flags)`

> Lock the PCM stream

**Parameters**

`substream`
:   PCM substream

`flags`
:   irq flags

**Description**

This locks the PCM stream like [`snd_pcm_stream_lock()`](#c.snd_pcm_stream_lock "snd_pcm_stream_lock") but with the local
IRQ (only when nonatomic is false). In nonatomic case, this is identical
as [`snd_pcm_stream_lock()`](#c.snd_pcm_stream_lock "snd_pcm_stream_lock").

snd\_pcm\_stream\_lock\_irqsave\_nested

`snd_pcm_stream_lock_irqsave_nested (substream, flags)`

> Single-nested PCM stream locking

**Parameters**

`substream`
:   PCM substream

`flags`
:   irq flags

**Description**

This locks the PCM stream like [`snd_pcm_stream_lock_irqsave()`](#c.snd_pcm_stream_lock_irqsave "snd_pcm_stream_lock_irqsave") but with
the single-depth lockdep subclass.

snd\_pcm\_group\_for\_each\_entry

`snd_pcm_group_for_each_entry (s, substream)`

> iterate over the linked substreams

**Parameters**

`s`
:   the iterator

`substream`
:   the substream

**Description**

Iterate over the all linked substreams to the given **substream**.
When **substream** isn’t linked with any others, this gives returns **substream**
itself once.

int snd\_pcm\_running(struct snd\_pcm\_substream \*substream)
:   Check whether the substream is in a running state

**Parameters**

`struct snd_pcm_substream *substream`
:   substream to check

**Return**

true if the given substream is in the state RUNNING, or in the
state DRAINING for playback.

void \_\_snd\_pcm\_set\_state(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_state\_t state)
:   Change the current PCM state

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime to set

`snd_pcm_state_t state`
:   the current state to set

**Description**

Call within the stream lock

ssize\_t bytes\_to\_samples(struct snd\_pcm\_runtime \*runtime, ssize\_t size)
:   Unit conversion of the size from bytes to samples

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`ssize_t size`
:   size in bytes

**Return**

the size in samples

snd\_pcm\_sframes\_t bytes\_to\_frames(struct snd\_pcm\_runtime \*runtime, ssize\_t size)
:   Unit conversion of the size from bytes to frames

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`ssize_t size`
:   size in bytes

**Return**

the size in frames

ssize\_t samples\_to\_bytes(struct snd\_pcm\_runtime \*runtime, ssize\_t size)
:   Unit conversion of the size from samples to bytes

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`ssize_t size`
:   size in samples

**Return**

the byte size

ssize\_t frames\_to\_bytes(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_sframes\_t size)
:   Unit conversion of the size from frames to bytes

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_sframes_t size`
:   size in frames

**Return**

the byte size

int frame\_aligned(struct snd\_pcm\_runtime \*runtime, ssize\_t bytes)
:   Check whether the byte size is aligned to frames

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`ssize_t bytes`
:   size in bytes

**Return**

true if aligned, or false if not

size\_t snd\_pcm\_lib\_buffer\_bytes(struct snd\_pcm\_substream \*substream)
:   Get the buffer size of the current PCM in bytes

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Return**

buffer byte size

size\_t snd\_pcm\_lib\_period\_bytes(struct snd\_pcm\_substream \*substream)
:   Get the period size of the current PCM in bytes

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Return**

period byte size

snd\_pcm\_uframes\_t snd\_pcm\_playback\_avail(struct snd\_pcm\_runtime \*runtime)
:   Get the available (writable) space for playback

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

**Description**

Result is between 0 ... (boundary - 1)

**Return**

available frame size

snd\_pcm\_uframes\_t snd\_pcm\_capture\_avail(struct snd\_pcm\_runtime \*runtime)
:   Get the available (readable) space for capture

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

**Description**

Result is between 0 ... (boundary - 1)

**Return**

available frame size

snd\_pcm\_sframes\_t snd\_pcm\_playback\_hw\_avail(struct snd\_pcm\_runtime \*runtime)
:   Get the queued space for playback

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

**Return**

available frame size

snd\_pcm\_sframes\_t snd\_pcm\_capture\_hw\_avail(struct snd\_pcm\_runtime \*runtime)
:   Get the free space for capture

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

**Return**

available frame size

int snd\_pcm\_playback\_ready(struct snd\_pcm\_substream \*substream)
:   check whether the playback buffer is available

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Checks whether enough free space is available on the playback buffer.

**Return**

Non-zero if available, or zero if not.

int snd\_pcm\_capture\_ready(struct snd\_pcm\_substream \*substream)
:   check whether the capture buffer is available

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Checks whether enough capture data is available on the capture buffer.

**Return**

Non-zero if available, or zero if not.

int snd\_pcm\_playback\_data(struct snd\_pcm\_substream \*substream)
:   check whether any data exists on the playback buffer

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Checks whether any data exists on the playback buffer.

**Return**

Non-zero if any data exists, or zero if not. If stop\_threshold
is bigger or equal to boundary, then this function returns always non-zero.

int snd\_pcm\_playback\_empty(struct snd\_pcm\_substream \*substream)
:   check whether the playback buffer is empty

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Checks whether the playback buffer is empty.

**Return**

Non-zero if empty, or zero if not.

int snd\_pcm\_capture\_empty(struct snd\_pcm\_substream \*substream)
:   check whether the capture buffer is empty

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Checks whether the capture buffer is empty.

**Return**

Non-zero if empty, or zero if not.

void snd\_pcm\_trigger\_done(struct snd\_pcm\_substream \*substream, struct snd\_pcm\_substream \*master)
:   Mark the master substream

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

`struct snd_pcm_substream *master`
:   the linked master substream

**Description**

When multiple substreams of the same card are linked and the hardware
supports the single-shot operation, the driver calls this in the loop
in [`snd_pcm_group_for_each_entry()`](#c.snd_pcm_group_for_each_entry "snd_pcm_group_for_each_entry") for marking the substream as “done”.
Then most of trigger operations are performed only to the given master
substream.

The trigger\_master mark is cleared at timestamp updates at the end
of trigger operations.

unsigned int params\_channels(const struct snd\_pcm\_hw\_params \*p)
:   Get the number of channels from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the number of channels

unsigned int params\_rate(const struct snd\_pcm\_hw\_params \*p)
:   Get the sample rate from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the sample rate

unsigned int params\_period\_size(const struct snd\_pcm\_hw\_params \*p)
:   Get the period size (in frames) from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the period size in frames

unsigned int params\_periods(const struct snd\_pcm\_hw\_params \*p)
:   Get the number of periods from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the number of periods

unsigned int params\_buffer\_size(const struct snd\_pcm\_hw\_params \*p)
:   Get the buffer size (in frames) from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the buffer size in frames

unsigned int params\_buffer\_bytes(const struct snd\_pcm\_hw\_params \*p)
:   Get the buffer size (in bytes) from the hw params

**Parameters**

`const struct snd_pcm_hw_params *p`
:   hw params

**Return**

the buffer size in bytes

int snd\_pcm\_hw\_constraint\_single(struct snd\_pcm\_runtime \*runtime, snd\_pcm\_hw\_param\_t var, unsigned int val)
:   Constrain parameter to a single value

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`snd_pcm_hw_param_t var`
:   The hw\_params variable to constrain

`unsigned int val`
:   The value to constrain to

**Return**

Positive if the value is changed, zero if it’s not changed, or a
negative error code.

int snd\_pcm\_format\_cpu\_endian(snd\_pcm\_format\_t format)
:   Check the PCM format is CPU-endian

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is CPU-endian, 0 if
opposite, or a negative error code if endian not specified.

void snd\_pcm\_set\_sync(struct snd\_pcm\_substream \*substream)
:   set the PCM sync id

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream

**Description**

Use the default PCM sync identifier for the specific card.

void snd\_pcm\_set\_runtime\_buffer(struct snd\_pcm\_substream \*substream, struct snd\_dma\_buffer \*bufp)
:   Set the PCM runtime buffer

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream to set

`struct snd_dma_buffer *bufp`
:   the buffer information, NULL to clear

**Description**

Copy the buffer information to runtime->dma\_buffer when **bufp** is non-NULL.
Otherwise it clears the current buffer information.

void snd\_pcm\_gettime(struct snd\_pcm\_runtime \*runtime, struct timespec64 \*tv)
:   Fill the timespec64 depending on the timestamp mode

**Parameters**

`struct snd_pcm_runtime *runtime`
:   PCM runtime instance

`struct timespec64 *tv`
:   timespec64 to fill

int snd\_pcm\_set\_fixed\_buffer(struct snd\_pcm\_substream \*substream, int type, struct [device](../../driver-api/infrastructure.html#c.device "device") \*data, size\_t size)
:   Preallocate and set up the fixed size PCM buffer

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`struct device *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

**Description**

This is a variant of [`snd_pcm_set_managed_buffer()`](#c.snd_pcm_set_managed_buffer "snd_pcm_set_managed_buffer"), but this pre-allocates
only the given sized buffer and doesn’t allow re-allocation nor dynamic
allocation of a larger buffer unlike the standard one.
The function may return -ENOMEM error, hence the caller must check it.

**Return**

zero if successful, or a negative error code

int snd\_pcm\_set\_fixed\_buffer\_all(struct snd\_pcm \*pcm, int type, struct [device](../../driver-api/infrastructure.html#c.device "device") \*data, size\_t size)
:   Preallocate and set up the fixed size PCM buffer

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`struct device *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

**Description**

Apply the set up of the fixed buffer via [`snd_pcm_set_fixed_buffer()`](#c.snd_pcm_set_fixed_buffer "snd_pcm_set_fixed_buffer") for
all substream. If any of allocation fails, it returns -ENOMEM, hence the
caller must check the return value.

**Return**

zero if successful, or a negative error code

dma\_addr\_t snd\_pcm\_sgbuf\_get\_addr(struct snd\_pcm\_substream \*substream, unsigned int ofs)
:   Get the DMA address at the corresponding offset

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`unsigned int ofs`
:   byte offset

**Return**

DMA address

unsigned int snd\_pcm\_sgbuf\_get\_chunk\_size(struct snd\_pcm\_substream \*substream, unsigned int ofs, unsigned int size)
:   Compute the max size that fits within the contig. page from the given size

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`unsigned int ofs`
:   byte offset

`unsigned int size`
:   byte size to examine

**Return**

chunk size

void snd\_pcm\_limit\_isa\_dma\_size(int dma, size\_t \*max)
:   Get the max size fitting with ISA DMA transfer

**Parameters**

`int dma`
:   DMA number

`size_t *max`
:   pointer to store the max size

const char \*snd\_pcm\_direction\_name(int direction)
:   Get a string naming the direction of a stream

**Parameters**

`int direction`
:   Stream’s direction, one of SNDRV\_PCM\_STREAM\_XXX

**Description**

Returns a string naming the direction of the stream.

const char \*snd\_pcm\_stream\_str(struct snd\_pcm\_substream \*substream)
:   Get a string naming the direction of a stream

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Return**

A string naming the direction of the stream.

struct snd\_pcm\_substream \*snd\_pcm\_chmap\_substream(struct snd\_pcm\_chmap \*info, unsigned int idx)
:   get the PCM substream assigned to the given chmap info

**Parameters**

`struct snd_pcm_chmap *info`
:   chmap information

`unsigned int idx`
:   the substream number index

**Return**

the matched PCM substream, or NULL if not found

u64 pcm\_format\_to\_bits(snd\_pcm\_format\_t pcm\_format)
:   Strong-typed conversion of pcm\_format to bitwise

**Parameters**

`snd_pcm_format_t pcm_format`
:   PCM format

**Return**

64bit mask corresponding to the given PCM format

pcm\_for\_each\_format

`pcm_for_each_format (f)`

> helper to iterate for each format type

**Parameters**

`f`
:   the iterator variable in snd\_pcm\_format\_t type

### PCM Format Helpers

int snd\_pcm\_format\_signed(snd\_pcm\_format\_t format)
:   Check the PCM format is signed linear

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is signed linear, 0 if unsigned
linear, and a negative error code for non-linear formats.

int snd\_pcm\_format\_unsigned(snd\_pcm\_format\_t format)
:   Check the PCM format is unsigned linear

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is unsigned linear, 0 if signed
linear, and a negative error code for non-linear formats.

int snd\_pcm\_format\_linear(snd\_pcm\_format\_t format)
:   Check the PCM format is linear

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is linear, 0 if not.

int snd\_pcm\_format\_little\_endian(snd\_pcm\_format\_t format)
:   Check the PCM format is little-endian

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is little-endian, 0 if
big-endian, or a negative error code if endian not specified.

int snd\_pcm\_format\_big\_endian(snd\_pcm\_format\_t format)
:   Check the PCM format is big-endian

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

1 if the given PCM format is big-endian, 0 if
little-endian, or a negative error code if endian not specified.

int snd\_pcm\_format\_width(snd\_pcm\_format\_t format)
:   return the bit-width of the format

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

The bit-width of the format, or a negative error code
if unknown format.

int snd\_pcm\_format\_physical\_width(snd\_pcm\_format\_t format)
:   return the physical bit-width of the format

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

The physical bit-width of the format, or a negative error code
if unknown format.

ssize\_t snd\_pcm\_format\_size(snd\_pcm\_format\_t format, size\_t samples)
:   return the byte size of samples on the given format

**Parameters**

`snd_pcm_format_t format`
:   the format to check

`size_t samples`
:   sampling rate

**Return**

The byte size of the given samples for the format, or a
negative error code if unknown format.

const unsigned char \*snd\_pcm\_format\_silence\_64(snd\_pcm\_format\_t format)
:   return the silent data in 8 bytes array

**Parameters**

`snd_pcm_format_t format`
:   the format to check

**Return**

The format pattern to fill or `NULL` if error.

int snd\_pcm\_format\_set\_silence(snd\_pcm\_format\_t format, void \*data, unsigned int samples)
:   set the silence data on the buffer

**Parameters**

`snd_pcm_format_t format`
:   the PCM format

`void *data`
:   the buffer pointer

`unsigned int samples`
:   the number of samples to set silence

**Description**

Sets the silence data on the buffer for the given samples.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_pcm\_hw\_limit\_rates(struct snd\_pcm\_hardware \*hw)
:   determine rate\_min/rate\_max fields

**Parameters**

`struct snd_pcm_hardware *hw`
:   the pcm hw instance

**Description**

Determines the rate\_min and rate\_max fields from the rates bits of
the given hw.

**Return**

Zero if successful.

unsigned int snd\_pcm\_rate\_to\_rate\_bit(unsigned int rate)
:   converts sample rate to SNDRV\_PCM\_RATE\_xxx bit

**Parameters**

`unsigned int rate`
:   the sample rate to convert

**Return**

The SNDRV\_PCM\_RATE\_xxx flag that corresponds to the given rate, or
SNDRV\_PCM\_RATE\_KNOT for an unknown rate.

unsigned int snd\_pcm\_rate\_bit\_to\_rate(unsigned int rate\_bit)
:   converts SNDRV\_PCM\_RATE\_xxx bit to sample rate

**Parameters**

`unsigned int rate_bit`
:   the rate bit to convert

**Return**

The sample rate that corresponds to the given SNDRV\_PCM\_RATE\_xxx flag
or 0 for an unknown rate bit.

unsigned int snd\_pcm\_rate\_mask\_intersect(unsigned int rates\_a, unsigned int rates\_b)
:   computes the intersection between two rate masks

**Parameters**

`unsigned int rates_a`
:   The first rate mask

`unsigned int rates_b`
:   The second rate mask

**Description**

This function computes the rates that are supported by both rate masks passed
to the function. It will take care of the special handling of
SNDRV\_PCM\_RATE\_CONTINUOUS and SNDRV\_PCM\_RATE\_KNOT.

**Return**

A rate mask containing the rates that are supported by both rates\_a
and rates\_b.

### PCM Memory Management

void snd\_pcm\_lib\_preallocate\_free(struct snd\_pcm\_substream \*substream)
:   release the preallocated buffer of the specified substream.

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

**Description**

Releases the pre-allocated buffer of the given substream.

void snd\_pcm\_lib\_preallocate\_free\_for\_all(struct snd\_pcm \*pcm)
:   release all pre-allocated buffers on the pcm

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

**Description**

Releases all the pre-allocated buffers on the given pcm.

void snd\_pcm\_lib\_preallocate\_pages(struct snd\_pcm\_substream \*substream, int type, struct [device](../../driver-api/infrastructure.html#c.device "device") \*data, size\_t size, size\_t max)
:   pre-allocation for the given DMA type

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`struct device *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

`size_t max`
:   the max. allowed pre-allocation size

**Description**

Do pre-allocation for the given DMA buffer type.

void snd\_pcm\_lib\_preallocate\_pages\_for\_all(struct snd\_pcm \*pcm, int type, void \*data, size\_t size, size\_t max)
:   pre-allocation for continuous memory type (all substreams)

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`void *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

`size_t max`
:   the max. allowed pre-allocation size

**Description**

Do pre-allocation to all substreams of the given pcm for the
specified DMA type.

int snd\_pcm\_set\_managed\_buffer(struct snd\_pcm\_substream \*substream, int type, struct [device](../../driver-api/infrastructure.html#c.device "device") \*data, size\_t size, size\_t max)
:   set up buffer management for a substream

**Parameters**

`struct snd_pcm_substream *substream`
:   the pcm substream instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`struct device *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

`size_t max`
:   the max. allowed pre-allocation size

**Description**

Do pre-allocation for the given DMA buffer type, and set the managed
buffer allocation mode to the given substream.
In this mode, PCM core will allocate a buffer automatically before PCM
hw\_params ops call, and release the buffer after PCM hw\_free ops call
as well, so that the driver doesn’t need to invoke the allocation and
the release explicitly in its callback.
When a buffer is actually allocated before the PCM hw\_params call, it
turns on the runtime buffer\_changed flag for drivers changing their h/w
parameters accordingly.

When **size** is non-zero and **max** is zero, this tries to allocate for only
the exact buffer size without fallback, and may return -ENOMEM.
Otherwise, the function tries to allocate smaller chunks if the allocation
fails. This is the behavior of [`snd_pcm_set_fixed_buffer()`](#c.snd_pcm_set_fixed_buffer "snd_pcm_set_fixed_buffer").

When both **size** and **max** are zero, the function only sets up the buffer
for later dynamic allocations. It’s used typically for buffers with
SNDRV\_DMA\_TYPE\_VMALLOC type.

Upon successful buffer allocation and setup, the function returns 0.

**Return**

zero if successful, or a negative error code

int snd\_pcm\_set\_managed\_buffer\_all(struct snd\_pcm \*pcm, int type, struct [device](../../driver-api/infrastructure.html#c.device "device") \*data, size\_t size, size\_t max)
:   set up buffer management for all substreams for all substreams

**Parameters**

`struct snd_pcm *pcm`
:   the pcm instance

`int type`
:   DMA type (SNDRV\_DMA\_TYPE\_\*)

`struct device *data`
:   DMA type dependent data

`size_t size`
:   the requested pre-allocation size in bytes

`size_t max`
:   the max. allowed pre-allocation size

**Description**

Do pre-allocation to all substreams of the given pcm for the specified DMA
type and size, and set the managed\_buffer\_alloc flag to each substream.

**Return**

zero if successful, or a negative error code

int snd\_pcm\_lib\_malloc\_pages(struct snd\_pcm\_substream \*substream, size\_t size)
:   allocate the DMA buffer

**Parameters**

`struct snd_pcm_substream *substream`
:   the substream to allocate the DMA buffer to

`size_t size`
:   the requested buffer size in bytes

**Description**

Allocates the DMA buffer on the BUS type given earlier to
`snd_pcm_lib_preallocate_xxx_pages()`.

**Return**

1 if the buffer is changed, 0 if not changed, or a negative
code on failure.

int snd\_pcm\_lib\_free\_pages(struct snd\_pcm\_substream \*substream)
:   release the allocated DMA buffer.

**Parameters**

`struct snd_pcm_substream *substream`
:   the substream to release the DMA buffer

**Description**

Releases the DMA buffer allocated via [`snd_pcm_lib_malloc_pages()`](#c.snd_pcm_lib_malloc_pages "snd_pcm_lib_malloc_pages").

**Return**

Zero if successful, or a negative error code on failure.

### PCM DMA Engine API

int snd\_hwparams\_to\_dma\_slave\_config(const struct snd\_pcm\_substream \*substream, const struct snd\_pcm\_hw\_params \*params, struct dma\_slave\_config \*slave\_config)
:   Convert hw\_params to dma\_slave\_config

**Parameters**

`const struct snd_pcm_substream *substream`
:   PCM substream

`const struct snd_pcm_hw_params *params`
:   hw\_params

`struct dma_slave_config *slave_config`
:   DMA slave config

**Description**

This function can be used to initialize a dma\_slave\_config from a substream
and hw\_params in a dmaengine based PCM driver implementation.

**Return**

zero if successful, or a negative error code

void snd\_dmaengine\_pcm\_set\_config\_from\_dai\_data(const struct snd\_pcm\_substream \*substream, const struct [snd\_dmaengine\_dai\_dma\_data](#c.snd_dmaengine_dai_dma_data "snd_dmaengine_dai_dma_data") \*dma\_data, struct dma\_slave\_config \*slave\_config)
:   Initializes a dma slave config using DAI DMA data.

**Parameters**

`const struct snd_pcm_substream *substream`
:   PCM substream

`const struct snd_dmaengine_dai_dma_data *dma_data`
:   DAI DMA data

`struct dma_slave_config *slave_config`
:   DMA slave configuration

**Description**

Initializes the {dst,src}\_addr, {dst,src}\_maxburst, {dst,src}\_addr\_width
fields of the DMA slave config from the same fields of the DAI DMA
data struct. The src and dst fields will be initialized depending on the
direction of the substream. If the substream is a playback stream the dst
fields will be initialized, if it is a capture stream the src fields will be
initialized. The {dst,src}\_addr\_width field will only be initialized if the
SND\_DMAENGINE\_PCM\_DAI\_FLAG\_PACK flag is set or if the addr\_width field of
the DAI DMA data `struct is` not equal to DMA\_SLAVE\_BUSWIDTH\_UNDEFINED. If
both conditions are met the latter takes priority.

int snd\_dmaengine\_pcm\_trigger(struct snd\_pcm\_substream \*substream, int cmd)
:   dmaengine based PCM trigger implementation

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`int cmd`
:   Trigger command

**Description**

This function can be used as the PCM trigger callback for dmaengine based PCM
driver implementations.

**Return**

0 on success, a negative error code otherwise

snd\_pcm\_uframes\_t snd\_dmaengine\_pcm\_pointer\_no\_residue(struct snd\_pcm\_substream \*substream)
:   dmaengine based PCM pointer implementation

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This function is deprecated and should not be used by new drivers, as its
results may be unreliable.

**Return**

PCM position in frames

snd\_pcm\_uframes\_t snd\_dmaengine\_pcm\_pointer(struct snd\_pcm\_substream \*substream)
:   dmaengine based PCM pointer implementation

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

This function can be used as the PCM pointer callback for dmaengine based PCM
driver implementations.

**Return**

PCM position in frames

struct dma\_chan \*snd\_dmaengine\_pcm\_request\_channel(dma\_filter\_fn filter\_fn, void \*filter\_data)
:   Request channel for the dmaengine PCM

**Parameters**

`dma_filter_fn filter_fn`
:   Filter function used to request the DMA channel

`void *filter_data`
:   Data passed to the DMA filter function

**Description**

This function request a DMA channel for usage with dmaengine PCM.

**Return**

NULL or the requested DMA channel

int snd\_dmaengine\_pcm\_open(struct snd\_pcm\_substream \*substream, struct dma\_chan \*chan)
:   Open a dmaengine based PCM substream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`struct dma_chan *chan`
:   DMA channel to use for data transfers

**Description**

The function should usually be called from the pcm open callback. Note that
this function will use private\_data field of the substream’s runtime. So it
is not available to your pcm driver implementation.

**Return**

0 on success, a negative error code otherwise

int snd\_dmaengine\_pcm\_close(struct snd\_pcm\_substream \*substream)
:   Close a dmaengine based PCM substream

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Return**

0 on success, a negative error code otherwise

int snd\_dmaengine\_pcm\_close\_release\_chan(struct snd\_pcm\_substream \*substream)
:   Close a dmaengine based PCM substream and release channel

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

**Description**

Releases the DMA channel associated with the PCM substream.

**Return**

zero if successful, or a negative error code

int snd\_dmaengine\_pcm\_refine\_runtime\_hwparams(struct snd\_pcm\_substream \*substream, struct [snd\_dmaengine\_dai\_dma\_data](#c.snd_dmaengine_dai_dma_data "snd_dmaengine_dai_dma_data") \*dma\_data, struct snd\_pcm\_hardware \*hw, struct dma\_chan \*chan)
:   Refine runtime hw params

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`struct snd_dmaengine_dai_dma_data *dma_data`
:   DAI DMA data

`struct snd_pcm_hardware *hw`
:   PCM hw params

`struct dma_chan *chan`
:   DMA channel to use for data transfers

**Description**

This function will query DMA capability, then refine the pcm hardware
parameters.

**Return**

0 on success, a negative error code otherwise

enum dma\_transfer\_direction snd\_pcm\_substream\_to\_dma\_direction(const struct snd\_pcm\_substream \*substream)
:   Get dma\_transfer\_direction for a PCM substream

**Parameters**

`const struct snd_pcm_substream *substream`
:   PCM substream

**Return**

DMA transfer direction

struct snd\_dmaengine\_dai\_dma\_data
:   DAI DMA configuration data

**Definition**:

```
struct snd_dmaengine_dai_dma_data {
    dma_addr_t addr;
    enum dma_slave_buswidth addr_width;
    u32 maxburst;
    void *filter_data;
    const char *chan_name;
    unsigned int fifo_size;
    unsigned int flags;
    void *peripheral_config;
    size_t peripheral_size;
    u32 port_window_size;
};
```

**Members**

`addr`
:   Address of the DAI data source or destination register.

`addr_width`
:   Width of the DAI data source or destination register.

`maxburst`
:   Maximum number of words(note: words, as in units of the
    src\_addr\_width member, not bytes) that can be send to or received from the
    DAI in one burst.

`filter_data`
:   Custom DMA channel filter data, this will usually be used when
    requesting the DMA channel.

`chan_name`
:   Custom channel name to use when requesting DMA channel.

`fifo_size`
:   FIFO size of the DAI controller in bytes

`flags`
:   PCM\_DAI flags, only SND\_DMAENGINE\_PCM\_DAI\_FLAG\_PACK for now

`peripheral_config`
:   peripheral configuration for programming peripheral
    for dmaengine transfer

`peripheral_size`
:   peripheral configuration buffer size

`port_window_size`
:   The length of the register area in words the data need
    to be accessed on the device side. It is only used for devices which is using
    an area instead of a single register to send/receive the data. Typically the
    DMA loops in this area in order to transfer the data.

struct snd\_dmaengine\_pcm\_config
:   Configuration data for dmaengine based PCM

**Definition**:

```
struct snd_dmaengine_pcm_config {
    int (*prepare_slave_config)(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct dma_slave_config *slave_config);
    struct dma_chan *(*compat_request_channel)( struct snd_soc_pcm_runtime *rtd, struct snd_pcm_substream *substream);
    int (*process)(struct snd_pcm_substream *substream, int channel, unsigned long hwoff, unsigned long bytes);
    const char *name;
    dma_filter_fn compat_filter_fn;
    struct device *dma_dev;
    const char *chan_names[SNDRV_PCM_STREAM_LAST + 1];
    const struct snd_pcm_hardware *pcm_hardware;
    unsigned int prealloc_buffer_size;
};
```

**Members**

`prepare_slave_config`
:   Callback used to fill in the DMA slave\_config for a
    PCM substream. Will be called from the PCM drivers hwparams callback.

`compat_request_channel`
:   Callback to request a DMA channel for platforms
    which do not use devicetree.

`process`
:   Callback used to apply processing on samples transferred from/to
    user space.

`name`
:   Component name. If null, dev\_name will be used.

`compat_filter_fn`
:   Will be used as the filter function when requesting a
    channel for platforms which do not use devicetree. The filter parameter
    will be the DAI’s DMA data.

`dma_dev`
:   If set, request DMA channel on this device rather than the DAI
    device.

`chan_names`
:   If set, these custom DMA channel names will be requested at
    registration time.

`pcm_hardware`
:   snd\_pcm\_hardware `struct to` be used for the PCM.

`prealloc_buffer_size`
:   Size of the preallocated audio buffer.

**Note**

If both compat\_request\_channel and compat\_filter\_fn are set
compat\_request\_channel will be used to request the channel and
compat\_filter\_fn will be ignored. Otherwise the channel will be requested
using dma\_request\_channel with compat\_filter\_fn as the filter function.

## Control/Mixer API

### General Control Interface

void snd\_ctl\_notify(struct snd\_card \*card, unsigned int mask, struct snd\_ctl\_elem\_id \*id)
:   Send notification to user-space for a control change

**Parameters**

`struct snd_card *card`
:   the card to send notification

`unsigned int mask`
:   the event mask, SNDRV\_CTL\_EVENT\_\*

`struct snd_ctl_elem_id *id`
:   the ctl element id to send notification

**Description**

This function adds an event record with the given id and mask, appends
to the list and wakes up the user-space for notification. This can be
called in the atomic context.

void snd\_ctl\_notify\_one(struct snd\_card \*card, unsigned int mask, struct snd\_kcontrol \*kctl, unsigned int ioff)
:   Send notification to user-space for a control change

**Parameters**

`struct snd_card *card`
:   the card to send notification

`unsigned int mask`
:   the event mask, SNDRV\_CTL\_EVENT\_\*

`struct snd_kcontrol *kctl`
:   the pointer with the control instance

`unsigned int ioff`
:   the additional offset to the control index

**Description**

This function calls [`snd_ctl_notify()`](#c.snd_ctl_notify "snd_ctl_notify") and does additional jobs
like LED state changes.

int snd\_ctl\_new(struct snd\_kcontrol \*\*kctl, unsigned int count, unsigned int access, struct snd\_ctl\_file \*file)
:   create a new control instance with some elements

**Parameters**

`struct snd_kcontrol **kctl`
:   the pointer to store new control instance

`unsigned int count`
:   the number of elements in this control

`unsigned int access`
:   the default access flags for elements in this control

`struct snd_ctl_file *file`
:   given when locking these elements

**Description**

Allocates a memory object for a new control instance. The instance has
elements as many as the given number (**count**). Each element has given
access permissions (**access**). Each element is locked when **file** is given.

**Return**

0 on success, error code on failure

struct snd\_kcontrol \*snd\_ctl\_new1(const struct snd\_kcontrol\_new \*ncontrol, void \*private\_data)
:   create a control instance from the template

**Parameters**

`const struct snd_kcontrol_new *ncontrol`
:   the initialization record

`void *private_data`
:   the private data to set

**Description**

Allocates a new `struct snd_kcontrol` instance and initialize from the given
template. When the access field of ncontrol is 0, it’s assumed as
READWRITE access. When the count field is 0, it’s assumes as one.

**Return**

The pointer of the newly generated instance, or `NULL` on failure.

void snd\_ctl\_free\_one(struct snd\_kcontrol \*kcontrol)
:   release the control instance

**Parameters**

`struct snd_kcontrol *kcontrol`
:   the control instance

**Description**

Releases the control instance created via [`snd_ctl_new()`](#c.snd_ctl_new "snd_ctl_new")
or [`snd_ctl_new1()`](#c.snd_ctl_new1 "snd_ctl_new1").
Don’t call this after the control was added to the card.

int snd\_ctl\_add(struct snd\_card \*card, struct snd\_kcontrol \*kcontrol)
:   add the control instance to the card

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_kcontrol *kcontrol`
:   the control instance to add

**Description**

Adds the control instance created via [`snd_ctl_new()`](#c.snd_ctl_new "snd_ctl_new") or
[`snd_ctl_new1()`](#c.snd_ctl_new1 "snd_ctl_new1") to the given card. Assigns also an unique
numid used for fast search.

It frees automatically the control which cannot be added.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ctl\_replace(struct snd\_card \*card, struct snd\_kcontrol \*kcontrol, bool add\_on\_replace)
:   replace the control instance of the card

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_kcontrol *kcontrol`
:   the control instance to replace

`bool add_on_replace`
:   add the control if not already added

**Description**

Replaces the given control. If the given control does not exist
and the add\_on\_replace flag is set, the control is added. If the
control exists, it is destroyed first.

It frees automatically the control which cannot be added or replaced.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ctl\_remove(struct snd\_card \*card, struct snd\_kcontrol \*kcontrol)
:   remove the control from the card and release it

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_kcontrol *kcontrol`
:   the control instance to remove

**Description**

Removes the control from the card and then releases the instance.
You don’t need to call [`snd_ctl_free_one()`](#c.snd_ctl_free_one "snd_ctl_free_one").
Passing NULL to **kcontrol** argument is allowed as noop.

Note that this function takes card->controls\_rwsem lock internally.

**Return**

0 if successful, or a negative error code on failure.

int snd\_ctl\_remove\_id(struct snd\_card \*card, struct snd\_ctl\_elem\_id \*id)
:   remove the control of the given id and release it

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_ctl_elem_id *id`
:   the control id to remove

**Description**

Finds the control instance with the given id, removes it from the
card list and releases it.

**Return**

0 if successful, or a negative error code on failure.

int snd\_ctl\_remove\_user\_ctl(struct snd\_ctl\_file \*file, struct snd\_ctl\_elem\_id \*id)
:   remove and release the unlocked user control

**Parameters**

`struct snd_ctl_file * file`
:   active control handle

`struct snd_ctl_elem_id *id`
:   the control id to remove

**Description**

Finds the control instance with the given id, removes it from the
card list and releases it.

**Return**

0 if successful, or a negative error code on failure.

int snd\_ctl\_activate\_id(struct snd\_card \*card, struct snd\_ctl\_elem\_id \*id, int active)
:   activate/inactivate the control of the given id

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_ctl_elem_id *id`
:   the control id to activate/inactivate

`int active`
:   non-zero to activate

**Description**

Finds the control instance with the given id, and activate or
inactivate the control together with notification, if changed.
The given ID data is filled with full information.

**Return**

0 if unchanged, 1 if changed, or a negative error code on failure.

int snd\_ctl\_rename\_id(struct snd\_card \*card, struct snd\_ctl\_elem\_id \*src\_id, struct snd\_ctl\_elem\_id \*dst\_id)
:   replace the id of a control on the card

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_ctl_elem_id *src_id`
:   the old id

`struct snd_ctl_elem_id *dst_id`
:   the new id

**Description**

Finds the control with the old id from the card, and replaces the
id with the new one.

The function tries to keep the already assigned numid while replacing
the rest.

Note that this function should be used only in the card initialization
phase. Calling after the card instantiation may cause issues with
user-space expecting persistent numids.

**Return**

Zero if successful, or a negative error code on failure.

void snd\_ctl\_rename(struct snd\_card \*card, struct snd\_kcontrol \*kctl, const char \*name)
:   rename the control on the card

**Parameters**

`struct snd_card *card`
:   the card instance

`struct snd_kcontrol *kctl`
:   the control to rename

`const char *name`
:   the new name

**Description**

Renames the specified control on the card to the new name.

Note that this function takes card->controls\_rwsem lock internally.

struct snd\_kcontrol \*snd\_ctl\_find\_numid(struct snd\_card \*card, unsigned int numid)
:   find the control instance with the given number-id

**Parameters**

`struct snd_card *card`
:   the card instance

`unsigned int numid`
:   the number-id to search

**Description**

Finds the control instance with the given number-id from the card.

Note that this function takes card->controls\_rwlock lock internally.

**Return**

The pointer of the instance if found, or `NULL` if not.

struct snd\_kcontrol \*snd\_ctl\_find\_id(struct snd\_card \*card, const struct snd\_ctl\_elem\_id \*id)
:   find the control instance with the given id

**Parameters**

`struct snd_card *card`
:   the card instance

`const struct snd_ctl_elem_id *id`
:   the id to search

**Description**

Finds the control instance with the given id from the card.

Note that this function takes card->controls\_rwlock lock internally.

**Return**

The pointer of the instance if found, or `NULL` if not.

int snd\_ctl\_register\_ioctl(snd\_kctl\_ioctl\_func\_t fcn)
:   register the device-specific control-ioctls

**Parameters**

`snd_kctl_ioctl_func_t fcn`
:   ioctl callback function

**Description**

called from each device manager like pcm.c, hwdep.c, etc.

**Return**

zero if successful, or a negative error code

int snd\_ctl\_register\_ioctl\_compat(snd\_kctl\_ioctl\_func\_t fcn)
:   register the device-specific 32bit compat control-ioctls

**Parameters**

`snd_kctl_ioctl_func_t fcn`
:   ioctl callback function

**Return**

zero if successful, or a negative error code

int snd\_ctl\_unregister\_ioctl(snd\_kctl\_ioctl\_func\_t fcn)
:   de-register the device-specific control-ioctls

**Parameters**

`snd_kctl_ioctl_func_t fcn`
:   ioctl callback function to unregister

**Return**

zero if successful, or a negative error code

int snd\_ctl\_unregister\_ioctl\_compat(snd\_kctl\_ioctl\_func\_t fcn)
:   de-register the device-specific compat 32bit control-ioctls

**Parameters**

`snd_kctl_ioctl_func_t fcn`
:   ioctl callback function to unregister

**Return**

zero if successful, or a negative error code

int snd\_ctl\_request\_layer(const char \*module\_name)
:   request to use the layer

**Parameters**

`const char *module_name`
:   Name of the kernel module (NULL == build-in)

**Return**

zero if successful, or an error code when the module cannot be loaded

void snd\_ctl\_register\_layer(struct snd\_ctl\_layer\_ops \*lops)
:   register new control layer

**Parameters**

`struct snd_ctl_layer_ops *lops`
:   operation structure

**Description**

The new layer can track all control elements and do additional
operations on top (like audio LED handling).

void snd\_ctl\_disconnect\_layer(struct snd\_ctl\_layer\_ops \*lops)
:   disconnect control layer

**Parameters**

`struct snd_ctl_layer_ops *lops`
:   operation structure

**Description**

It is expected that the information about tracked cards
is freed before this call (the disconnect callback is
not called here).

int snd\_ctl\_boolean\_mono\_info(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   Helper function for a standard boolean info callback with a mono channel

**Parameters**

`struct snd_kcontrol *kcontrol`
:   the kcontrol instance

`struct snd_ctl_elem_info *uinfo`
:   info to store

**Description**

This is a function that can be used as info callback for a standard
boolean control with a single mono channel.

**Return**

Zero (always successful)

int snd\_ctl\_boolean\_stereo\_info(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   Helper function for a standard boolean info callback with stereo two channels

**Parameters**

`struct snd_kcontrol *kcontrol`
:   the kcontrol instance

`struct snd_ctl_elem_info *uinfo`
:   info to store

**Description**

This is a function that can be used as info callback for a standard
boolean control with stereo two channels.

**Return**

Zero (always successful)

int snd\_ctl\_enum\_info(struct snd\_ctl\_elem\_info \*info, unsigned int channels, unsigned int items, const char \*const names[])
:   fills the info structure for an enumerated control

**Parameters**

`struct snd_ctl_elem_info *info`
:   the structure to be filled

`unsigned int channels`
:   the number of the control’s channels; often one

`unsigned int items`
:   the number of control values; also the size of **names**

`const char *const names[]`
:   an array containing the names of all control values

**Description**

Sets all required fields in **info** to their appropriate values.
If the control’s accessibility is not the default (readable and writable),
the caller has to fill **info->access**.

**Return**

Zero (always successful)

### AC97 Codec API

void snd\_ac97\_write(struct snd\_ac97 \*ac97, unsigned short reg, unsigned short value)
:   write a value on the given register

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`unsigned short reg`
:   the register to change

`unsigned short value`
:   the value to set

**Description**

Writes a value on the given register. This will invoke the write
callback directly after the register check.
This function doesn’t change the register cache unlike
#`snd_ca97_write_cache()`, so use this only when you don’t want to
reflect the change to the suspend/resume state.

unsigned short snd\_ac97\_read(struct snd\_ac97 \*ac97, unsigned short reg)
:   read a value from the given register

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`unsigned short reg`
:   the register to read

**Description**

Reads a value from the given register. This will invoke the read
callback directly after the register check.

**Return**

The read value.

void snd\_ac97\_write\_cache(struct snd\_ac97 \*ac97, unsigned short reg, unsigned short value)
:   write a value on the given register and update the cache

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`unsigned short reg`
:   the register to change

`unsigned short value`
:   the value to set

**Description**

Writes a value on the given register and updates the register
cache. The cached values are used for the cached-read and the
suspend/resume.

int snd\_ac97\_update(struct snd\_ac97 \*ac97, unsigned short reg, unsigned short value)
:   update the value on the given register

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`unsigned short reg`
:   the register to change

`unsigned short value`
:   the value to set

**Description**

Compares the value with the register cache and updates the value
only when the value is changed.

**Return**

1 if the value is changed, 0 if no change, or a negative
code on failure.

int snd\_ac97\_update\_bits(struct snd\_ac97 \*ac97, unsigned short reg, unsigned short mask, unsigned short value)
:   update the bits on the given register

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`unsigned short reg`
:   the register to change

`unsigned short mask`
:   the bit-mask to change

`unsigned short value`
:   the value to set

**Description**

Updates the masked-bits on the given register only when the value
is changed.

**Return**

1 if the bits are changed, 0 if no change, or a negative
code on failure.

const char \*snd\_ac97\_get\_short\_name(struct snd\_ac97 \*ac97)
:   retrieve codec name

**Parameters**

`struct snd_ac97 *ac97`
:   the codec instance

**Return**

The short identifying name of the codec.

int snd\_ac97\_bus(struct snd\_card \*card, int num, const struct snd\_ac97\_bus\_ops \*ops, void \*private\_data, struct [snd\_ac97\_bus](#c.snd_ac97_bus "snd_ac97_bus") \*\*rbus)
:   create an AC97 bus component

**Parameters**

`struct snd_card *card`
:   the card instance

`int num`
:   the bus number

`const struct snd_ac97_bus_ops *ops`
:   the bus callbacks table

`void *private_data`
:   private data pointer for the new instance

`struct snd_ac97_bus **rbus`
:   the pointer to store the new AC97 bus instance.

**Description**

Creates an AC97 bus component. An [`struct snd_ac97_bus`](#c.snd_ac97_bus "snd_ac97_bus") instance is newly
allocated and initialized.

The ops table must include valid callbacks (at least read and
write). The other callbacks, wait and reset, are not mandatory.

The clock is set to 48000. If another clock is needed, set
`(*rbus)->clock` manually.

The AC97 bus instance is registered as a low-level device, so you don’t
have to release it manually.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_mixer(struct [snd\_ac97\_bus](#c.snd_ac97_bus "snd_ac97_bus") \*bus, struct snd\_ac97\_template \*template, struct snd\_ac97 \*\*rac97)
:   create an Codec97 component

**Parameters**

`struct snd_ac97_bus *bus`
:   the AC97 bus which codec is attached to

`struct snd_ac97_template *template`
:   the template of ac97, including index, callbacks and
    the private data.

`struct snd_ac97 **rac97`
:   the pointer to store the new ac97 instance.

**Description**

Creates an Codec97 component. An `struct snd_ac97` instance is newly
allocated and initialized from the template. The codec
is then initialized by the standard procedure.

The template must include the codec number (num) and address (addr),
and the private data (private\_data).

The ac97 instance is registered as a low-level device, so you don’t
have to release it manually.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_update\_power(struct snd\_ac97 \*ac97, int reg, int powerup)
:   update the powerdown register

**Parameters**

`struct snd_ac97 *ac97`
:   the codec instance

`int reg`
:   the rate register, e.g. AC97\_PCM\_FRONT\_DAC\_RATE

`int powerup`
:   non-zero when power up the part

**Description**

Update the AC97 powerdown register bits of the given part.

**Return**

Zero.

void snd\_ac97\_suspend(struct snd\_ac97 \*ac97)
:   General suspend function for AC97 codec

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

**Description**

Suspends the codec, power down the chip.

void snd\_ac97\_resume(struct snd\_ac97 \*ac97)
:   General resume function for AC97 codec

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

**Description**

Do the standard resume procedure, power up and restoring the
old register values.

int snd\_ac97\_tune\_hardware(struct snd\_ac97 \*ac97, const struct ac97\_quirk \*quirk, const char \*override)
:   tune up the hardware

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`const struct ac97_quirk *quirk`
:   quirk list

`const char *override`
:   explicit quirk value (overrides the list if non-NULL)

**Description**

Do some workaround for each pci device, such as renaming of the
headphone (true line-out) control as “Master”.
The quirk-list must be terminated with a zero-filled entry.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_set\_rate(struct snd\_ac97 \*ac97, int reg, unsigned int rate)
:   change the rate of the given input/output.

**Parameters**

`struct snd_ac97 *ac97`
:   the ac97 instance

`int reg`
:   the register to change

`unsigned int rate`
:   the sample rate to set

**Description**

Changes the rate of the given input/output on the codec.
If the codec doesn’t support VAR, the rate must be 48000 (except
for SPDIF).

The valid registers are AC97\_PCM\_MIC\_ADC\_RATE,
AC97\_PCM\_FRONT\_DAC\_RATE, AC97\_PCM\_LR\_ADC\_RATE.
AC97\_PCM\_SURR\_DAC\_RATE and AC97\_PCM\_LFE\_DAC\_RATE are accepted
if the codec supports them.
AC97\_SPDIF is accepted as a pseudo register to modify the SPDIF
status bits.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_pcm\_assign(struct [snd\_ac97\_bus](#c.snd_ac97_bus "snd_ac97_bus") \*bus, unsigned short pcms\_count, const struct ac97\_pcm \*pcms)
:   assign AC97 slots to given PCM streams

**Parameters**

`struct snd_ac97_bus *bus`
:   the ac97 bus instance

`unsigned short pcms_count`
:   count of PCMs to be assigned

`const struct ac97_pcm *pcms`
:   PCMs to be assigned

**Description**

It assigns available AC97 slots for given PCMs. If none or only
some slots are available, pcm->xxx.slots and pcm->xxx.rslots[] members
are reduced and might be zero.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_pcm\_open(struct ac97\_pcm \*pcm, unsigned int rate, enum ac97\_pcm\_cfg cfg, unsigned short slots)
:   opens the given AC97 pcm

**Parameters**

`struct ac97_pcm *pcm`
:   the ac97 pcm instance

`unsigned int rate`
:   rate in Hz, if codec does not support VRA, this value must be 48000Hz

`enum ac97_pcm_cfg cfg`
:   output stream characteristics

`unsigned short slots`
:   a subset of allocated slots (snd\_ac97\_pcm\_assign) for this pcm

**Description**

It locks the specified slots and sets the given rate to AC97 registers.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_ac97\_pcm\_close(struct ac97\_pcm \*pcm)
:   closes the given AC97 pcm

**Parameters**

`struct ac97_pcm *pcm`
:   the ac97 pcm instance

**Description**

It frees the locked AC97 slots.

**Return**

Zero.

int snd\_ac97\_pcm\_double\_rate\_rules(struct snd\_pcm\_runtime \*runtime)
:   set double rate constraints

**Parameters**

`struct snd_pcm_runtime *runtime`
:   the runtime of the ac97 front playback pcm

**Description**

Installs the hardware constraint rules to prevent using double rates and
more than two channels at the same time.

**Return**

Zero if successful, or a negative error code on failure.

### Virtual Master Control API

int snd\_ctl\_add\_followers(struct snd\_card \*card, struct snd\_kcontrol \*master, const char \*const \*list)
:   add multiple followers to vmaster

**Parameters**

`struct snd_card *card`
:   card instance

`struct snd_kcontrol *master`
:   the target vmaster kcontrol object

`const char * const *list`
:   NULL-terminated list of name strings of followers to be added

**Description**

Adds the multiple follower kcontrols with the given names.
Returns 0 for success or a negative error code.

struct snd\_kcontrol \*snd\_ctl\_make\_virtual\_master(char \*name, const unsigned int \*tlv)
:   Create a virtual master control

**Parameters**

`char *name`
:   name string of the control element to create

`const unsigned int *tlv`
:   optional TLV int array for dB information

**Description**

Creates a virtual master control with the given name string.

After creating a vmaster element, you can add the follower controls
via [`snd_ctl_add_follower()`](#c.snd_ctl_add_follower "snd_ctl_add_follower") or [`snd_ctl_add_follower_uncached()`](#c.snd_ctl_add_follower_uncached "snd_ctl_add_follower_uncached").

The optional argument **tlv** can be used to specify the TLV information
for dB scale of the master control. It should be a single element
with #SNDRV\_CTL\_TLVT\_DB\_SCALE, #SNDRV\_CTL\_TLV\_DB\_MINMAX or
#SNDRV\_CTL\_TLVT\_DB\_MINMAX\_MUTE type, and should be the max 0dB.

**Return**

The created control element, or `NULL` for errors (ENOMEM).

int snd\_ctl\_add\_vmaster\_hook(struct snd\_kcontrol \*kcontrol, void (\*hook)(void \*private\_data, int), void \*private\_data)
:   Add a hook to a vmaster control

**Parameters**

`struct snd_kcontrol *kcontrol`
:   vmaster kctl element

`void (*hook)(void *private_data, int)`
:   the hook function

`void *private_data`
:   the private\_data pointer to be saved

**Description**

Adds the given hook to the vmaster control element so that it’s called
at each time when the value is changed.

**Return**

Zero.

void snd\_ctl\_sync\_vmaster(struct snd\_kcontrol \*kcontrol, bool hook\_only)
:   Sync the vmaster followers and hook

**Parameters**

`struct snd_kcontrol *kcontrol`
:   vmaster kctl element

`bool hook_only`
:   sync only the hook

**Description**

Forcibly call the put callback of each follower and call the hook function
to synchronize with the current value of the given vmaster element.
NOP when NULL is passed to **kcontrol**.

int snd\_ctl\_apply\_vmaster\_followers(struct snd\_kcontrol \*kctl, int (\*func)(struct snd\_kcontrol \*vfollower, struct snd\_kcontrol \*follower, void \*arg), void \*arg)
:   Apply function to each vmaster follower

**Parameters**

`struct snd_kcontrol *kctl`
:   vmaster kctl element

`int (*func)(struct snd_kcontrol *vfollower, struct snd_kcontrol *follower, void *arg)`
:   function to apply

`void *arg`
:   optional function argument

**Description**

Apply the function **func** to each follower kctl of the given vmaster kctl.

**Return**

0 if successful, or a negative error code

struct snd\_kcontrol \*snd\_ctl\_find\_id\_mixer(struct snd\_card \*card, const char \*name)
:   find the control instance with the given name string

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *name`
:   the name string

**Description**

Finds the control instance with the given name and
**SNDRV\_CTL\_ELEM\_IFACE\_MIXER**. Other fields are set to zero.

This is merely a wrapper to [`snd_ctl_find_id()`](#c.snd_ctl_find_id "snd_ctl_find_id").

**Return**

The pointer of the instance if found, or `NULL` if not.

int snd\_ctl\_add\_follower(struct snd\_kcontrol \*master, struct snd\_kcontrol \*follower)
:   Add a virtual follower control

**Parameters**

`struct snd_kcontrol *master`
:   vmaster element

`struct snd_kcontrol *follower`
:   follower element to add

**Description**

Add a virtual follower control to the given master element created via
`snd_ctl_create_virtual_master()` beforehand.

All followers must be the same type (returning the same information
via info callback). The function doesn’t check it, so it’s your
responsibility.

Also, some additional limitations:
at most two channels,
logarithmic volume control (dB level) thus no linear volume,
master can only attenuate the volume without gain

**Return**

Zero if successful or a negative error code.

int snd\_ctl\_add\_follower\_uncached(struct snd\_kcontrol \*master, struct snd\_kcontrol \*follower)
:   Add a virtual follower control

**Parameters**

`struct snd_kcontrol *master`
:   vmaster element

`struct snd_kcontrol *follower`
:   follower element to add

**Description**

Add a virtual follower control to the given master.
Unlike [`snd_ctl_add_follower()`](#c.snd_ctl_add_follower "snd_ctl_add_follower"), the element added via this function
is supposed to have volatile values, and get callback is called
at each time queried from the master.

When the control peeks the hardware values directly and the value
can be changed by other means than the put callback of the element,
this function should be used to keep the value always up-to-date.

**Return**

Zero if successful or a negative error code.

## MIDI API

### Raw MIDI API

int snd\_rawmidi\_receive(struct snd\_rawmidi\_substream \*substream, const unsigned char \*buffer, int count)
:   receive the input data from the device

**Parameters**

`struct snd_rawmidi_substream *substream`
:   the rawmidi substream

`const unsigned char *buffer`
:   the buffer pointer

`int count`
:   the data size to read

**Description**

Reads the data from the internal buffer.

**Return**

The size of read data, or a negative error code on failure.

int snd\_rawmidi\_transmit\_empty(struct snd\_rawmidi\_substream \*substream)
:   check whether the output buffer is empty

**Parameters**

`struct snd_rawmidi_substream *substream`
:   the rawmidi substream

**Return**

1 if the internal output buffer is empty, 0 if not.

int snd\_rawmidi\_transmit\_peek(struct snd\_rawmidi\_substream \*substream, unsigned char \*buffer, int count)
:   copy data from the internal buffer

**Parameters**

`struct snd_rawmidi_substream *substream`
:   the rawmidi substream

`unsigned char *buffer`
:   the buffer pointer

`int count`
:   data size to transfer

**Description**

Copies data from the internal output buffer to the given buffer.

Call this in the interrupt handler when the midi output is ready,
and call [`snd_rawmidi_transmit_ack()`](#c.snd_rawmidi_transmit_ack "snd_rawmidi_transmit_ack") after the transmission is
finished.

**Return**

The size of copied data, or a negative error code on failure.

int snd\_rawmidi\_transmit\_ack(struct snd\_rawmidi\_substream \*substream, int count)
:   acknowledge the transmission

**Parameters**

`struct snd_rawmidi_substream *substream`
:   the rawmidi substream

`int count`
:   the transferred count

**Description**

Advances the hardware pointer for the internal output buffer with
the given size and updates the condition.
Call after the transmission is finished.

**Return**

The advanced size if successful, or a negative error code on failure.

int snd\_rawmidi\_transmit(struct snd\_rawmidi\_substream \*substream, unsigned char \*buffer, int count)
:   copy from the buffer to the device

**Parameters**

`struct snd_rawmidi_substream *substream`
:   the rawmidi substream

`unsigned char *buffer`
:   the buffer pointer

`int count`
:   the data size to transfer

**Description**

Copies data from the buffer to the device and advances the pointer.

**Return**

The copied size if successful, or a negative error code on failure.

int snd\_rawmidi\_proceed(struct snd\_rawmidi\_substream \*substream)
:   Discard the all pending bytes and proceed

**Parameters**

`struct snd_rawmidi_substream *substream`
:   rawmidi substream

**Return**

the number of discarded bytes

int snd\_rawmidi\_new(struct snd\_card \*card, char \*id, int device, int output\_count, int input\_count, struct snd\_rawmidi \*\*rrawmidi)
:   create a rawmidi instance

**Parameters**

`struct snd_card *card`
:   the card instance

`char *id`
:   the id string

`int device`
:   the device index

`int output_count`
:   the number of output streams

`int input_count`
:   the number of input streams

`struct snd_rawmidi **rrawmidi`
:   the pointer to store the new rawmidi instance

**Description**

Creates a new rawmidi instance.
Use [`snd_rawmidi_set_ops()`](#c.snd_rawmidi_set_ops "snd_rawmidi_set_ops") to set the operators to the new instance.

**Return**

Zero if successful, or a negative error code on failure.

void snd\_rawmidi\_set\_ops(struct snd\_rawmidi \*rmidi, int stream, const struct snd\_rawmidi\_ops \*ops)
:   set the rawmidi operators

**Parameters**

`struct snd_rawmidi *rmidi`
:   the rawmidi instance

`int stream`
:   the stream direction, SNDRV\_RAWMIDI\_STREAM\_XXX

`const struct snd_rawmidi_ops *ops`
:   the operator table

**Description**

Sets the rawmidi operators for the given stream direction.

### MPU401-UART API

irqreturn\_t snd\_mpu401\_uart\_interrupt(int irq, void \*dev\_id)
:   generic MPU401-UART interrupt handler

**Parameters**

`int irq`
:   the irq number

`void *dev_id`
:   mpu401 instance

**Description**

Processes the interrupt for MPU401-UART i/o.

**Return**

`IRQ_HANDLED` if the interrupt was handled. `IRQ_NONE` otherwise.

irqreturn\_t snd\_mpu401\_uart\_interrupt\_tx(int irq, void \*dev\_id)
:   generic MPU401-UART transmit irq handler

**Parameters**

`int irq`
:   the irq number

`void *dev_id`
:   mpu401 instance

**Description**

Processes the interrupt for MPU401-UART output.

**Return**

`IRQ_HANDLED` if the interrupt was handled. `IRQ_NONE` otherwise.

int snd\_mpu401\_uart\_new(struct snd\_card \*card, int device, unsigned short hardware, unsigned long port, unsigned int info\_flags, int irq, struct snd\_rawmidi \*\*rrawmidi)
:   create an MPU401-UART instance

**Parameters**

`struct snd_card *card`
:   the card instance

`int device`
:   the device index, zero-based

`unsigned short hardware`
:   the hardware type, MPU401\_HW\_XXXX

`unsigned long port`
:   the base address of MPU401 port

`unsigned int info_flags`
:   bitflags MPU401\_INFO\_XXX

`int irq`
:   the ISA irq number, -1 if not to be allocated

`struct snd_rawmidi ** rrawmidi`
:   the pointer to store the new rawmidi instance

**Description**

Creates a new MPU-401 instance.

Note that the rawmidi instance is returned on the rrawmidi argument,
not the mpu401 instance itself. To access to the mpu401 instance,
cast from rawmidi->private\_data (with `struct snd_mpu401` magic-cast).

**Return**

Zero if successful, or a negative error code.

## Proc Info API

### Proc Info Interface

int snd\_info\_get\_line(struct snd\_info\_buffer \*buffer, char \*line, int len)
:   read one line from the procfs buffer

**Parameters**

`struct snd_info_buffer *buffer`
:   the procfs buffer

`char *line`
:   the buffer to store

`int len`
:   the max. buffer size

**Description**

Reads one line from the buffer and stores the string.

**Return**

Zero if successful, or 1 if error or EOF.

const char \*snd\_info\_get\_str(char \*dest, const char \*src, int len)
:   parse a string token

**Parameters**

`char *dest`
:   the buffer to store the string token

`const char *src`
:   the original string

`int len`
:   the max. length of token - 1

**Description**

Parses the original string and copy a token to the given
string buffer.

**Return**

The updated pointer of the original string so that
it can be used for the next call.

struct snd\_info\_entry \*snd\_info\_create\_module\_entry(struct [module](#c.snd_info_create_module_entry "module") \*module, const char \*name, struct snd\_info\_entry \*parent)
:   create an info entry for the given module

**Parameters**

`struct module * module`
:   the module pointer

`const char *name`
:   the file name

`struct snd_info_entry *parent`
:   the parent directory

**Description**

Creates a new info entry and assigns it to the given module.

**Return**

The pointer of the new instance, or `NULL` on failure.

struct snd\_info\_entry \*snd\_info\_create\_card\_entry(struct snd\_card \*card, const char \*name, struct snd\_info\_entry \*parent)
:   create an info entry for the given card

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *name`
:   the file name

`struct snd_info_entry * parent`
:   the parent directory

**Description**

Creates a new info entry and assigns it to the given card.

**Return**

The pointer of the new instance, or `NULL` on failure.

void snd\_info\_free\_entry(struct snd\_info\_entry \*entry)
:   release the info entry

**Parameters**

`struct snd_info_entry * entry`
:   the info entry

**Description**

Releases the info entry.

int snd\_info\_register(struct snd\_info\_entry \*entry)
:   register the info entry

**Parameters**

`struct snd_info_entry *entry`
:   the info entry

**Description**

Registers the proc info entry.
The all children entries are registered recursively.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_card\_rw\_proc\_new(struct snd\_card \*card, const char \*name, void \*private\_data, void (\*read)(struct snd\_info\_entry\*, struct snd\_info\_buffer\*), void (\*write)(struct snd\_info\_entry \*entry, struct snd\_info\_buffer \*buffer))
:   Create a read/write text proc file entry for the card

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *name`
:   the file name

`void *private_data`
:   the arbitrary private data

`void (*read)(struct snd_info_entry *, struct snd_info_buffer *)`
:   the read callback

`void (*write)(struct snd_info_entry *entry, struct snd_info_buffer *buffer)`
:   the write callback, NULL for read-only

**Description**

This proc file entry will be registered via [`snd_card_register()`](#c.snd_card_register "snd_card_register") call, and
it will be removed automatically at the card removal, too.

**Return**

zero if successful, or a negative error code

## Compress Offload

### Compress Offload API

int snd\_compr\_stop\_error(struct [snd\_compr\_stream](#c.snd_compr_stream "snd_compr_stream") \*stream, snd\_pcm\_state\_t state)
:   Report a fatal error on a stream

**Parameters**

`struct snd_compr_stream *stream`
:   pointer to stream

`snd_pcm_state_t state`
:   state to transition the stream to

**Description**

Stop the stream and set its state.

Should be called with compressed device lock held.

**Return**

zero if successful, or a negative error code

void snd\_compr\_task\_finished(struct [snd\_compr\_stream](#c.snd_compr_stream "snd_compr_stream") \*stream, struct [snd\_compr\_task\_runtime](#c.snd_compr_task_runtime "snd_compr_task_runtime") \*task)
:   Notify that the task was finished

**Parameters**

`struct snd_compr_stream *stream`
:   pointer to stream

`struct snd_compr_task_runtime *task`
:   runtime task structure

**Description**

Set the finished task state and notify waiters.

int snd\_compress\_new(struct snd\_card \*card, int device, int dirn, const char \*id, struct [snd\_compr](#c.snd_compr "snd_compr") \*compr)
:   create new compress device

**Parameters**

`struct snd_card *card`
:   sound card pointer

`int device`
:   device number

`int dirn`
:   device direction, should be of type `enum snd_compr_direction`

`const char *id`
:   ID string

`struct snd_compr *compr`
:   compress device pointer

**Return**

zero if successful, or a negative error code

struct snd\_compressed\_buffer
:   compressed buffer

**Definition**:

```
struct snd_compressed_buffer {
    __u32 fragment_size;
    __u32 fragments;
};
```

**Members**

`fragment_size`
:   size of buffer fragment in bytes

`fragments`
:   number of such fragments

struct snd\_compr\_params
:   compressed stream params

**Definition**:

```
struct snd_compr_params {
    struct snd_compressed_buffer buffer;
    struct snd_codec codec;
    __u8 no_wake_mode;
};
```

**Members**

`buffer`
:   buffer description

`codec`
:   codec parameters

`no_wake_mode`
:   dont wake on fragment elapsed

struct snd\_compr\_tstamp
:   timestamp descriptor

**Definition**:

```
struct snd_compr_tstamp {
    __u32 byte_offset;
    __u32 copied_total;
    __u32 pcm_frames;
    __u32 pcm_io_frames;
    __u32 sampling_rate;
};
```

**Members**

`byte_offset`
:   Byte offset in ring buffer to DSP

`copied_total`
:   Total number of bytes copied from/to ring buffer to/by DSP

`pcm_frames`
:   Frames decoded or encoded by DSP. This field will evolve by
    large steps and should only be used to monitor encoding/decoding
    progress. It shall not be used for timing estimates.

`pcm_io_frames`
:   Frames rendered or received by DSP into a mixer or an audio
    output/input. This field should be used for A/V sync or time estimates.

`sampling_rate`
:   sampling rate of audio

struct snd\_compr\_tstamp64
:   timestamp descriptor with fields in 64 bit

**Definition**:

```
struct snd_compr_tstamp64 {
    __u32 byte_offset;
    __u64 copied_total;
    __u64 pcm_frames;
    __u64 pcm_io_frames;
    __u32 sampling_rate;
};
```

**Members**

`byte_offset`
:   Byte offset in ring buffer to DSP

`copied_total`
:   Total number of bytes copied from/to ring buffer to/by DSP

`pcm_frames`
:   Frames decoded or encoded by DSP. This field will evolve by
    large steps and should only be used to monitor encoding/decoding
    progress. It shall not be used for timing estimates.

`pcm_io_frames`
:   Frames rendered or received by DSP into a mixer or an audio
    output/input. This field should be used for A/V sync or time estimates.

`sampling_rate`
:   sampling rate of audio

struct snd\_compr\_avail
:   avail descriptor

**Definition**:

```
struct snd_compr_avail {
    __u64 avail;
    struct snd_compr_tstamp tstamp;
};
```

**Members**

`avail`
:   Number of bytes available in ring buffer for writing/reading

`tstamp`
:   timestamp information

struct snd\_compr\_avail64
:   avail descriptor with tstamp in 64 bit format

**Definition**:

```
struct snd_compr_avail64 {
    __u64 avail;
    struct snd_compr_tstamp64 tstamp;
};
```

**Members**

`avail`
:   Number of bytes available in ring buffer for writing/reading

`tstamp`
:   timestamp information

struct snd\_compr\_caps
:   caps descriptor

**Definition**:

```
struct snd_compr_caps {
    __u32 num_codecs;
    __u32 direction;
    __u32 min_fragment_size;
    __u32 max_fragment_size;
    __u32 min_fragments;
    __u32 max_fragments;
    __u32 codecs[MAX_NUM_CODECS];
    __u32 reserved[11];
};
```

**Members**

`num_codecs`
:   number of codecs supported

`direction`
:   direction supported. Of type snd\_compr\_direction

`min_fragment_size`
:   minimum fragment supported by DSP

`max_fragment_size`
:   maximum fragment supported by DSP

`min_fragments`
:   min fragments supported by DSP

`max_fragments`
:   max fragments supported by DSP

`codecs`
:   pointer to array of codecs

`reserved`
:   reserved field

struct snd\_compr\_codec\_caps
:   query capability of codec

**Definition**:

```
struct snd_compr_codec_caps {
    __u32 codec;
    __u32 num_descriptors;
    struct snd_codec_desc descriptor[MAX_NUM_CODEC_DESCRIPTORS];
};
```

**Members**

`codec`
:   codec for which capability is queried

`num_descriptors`
:   number of codec descriptors

`descriptor`
:   array of codec capability descriptor

enum sndrv\_compress\_encoder
:   encoder metadata key

**Constants**

`SNDRV_COMPRESS_ENCODER_PADDING`
:   no of samples appended by the encoder at the
    end of the track

`SNDRV_COMPRESS_ENCODER_DELAY`
:   no of samples inserted by the encoder at the
    beginning of the track

struct snd\_compr\_metadata
:   compressed stream metadata

**Definition**:

```
struct snd_compr_metadata {
    __u32 key;
    __u32 value[8];
};
```

**Members**

`key`
:   key id

`value`
:   key value

struct snd\_compr\_task
:   task primitive for non-realtime operation

**Definition**:

```
struct snd_compr_task {
    __u64 seqno;
    __u64 origin_seqno;
    int input_fd;
    int output_fd;
    __u64 input_size;
    __u32 flags;
    __u8 reserved[16];
};
```

**Members**

`seqno`
:   sequence number (task identifier)

`origin_seqno`
:   previous sequence number (task identifier) - for reuse

`input_fd`
:   data input file descriptor (dma-buf)

`output_fd`
:   data output file descriptor (dma-buf)

`input_size`
:   filled data in bytes (from caller, must not exceed fragment size)

`flags`
:   see SND\_COMPRESS\_TFLG\_\* defines

`reserved`
:   reserved for future extension

enum snd\_compr\_state
:   task state

**Constants**

`SND_COMPRESS_TASK_STATE_IDLE`
:   task is not queued

`SND_COMPRESS_TASK_STATE_ACTIVE`
:   task is in the queue

`SND_COMPRESS_TASK_STATE_FINISHED`
:   task was processed, output is available

struct snd\_compr\_task\_status
:   task status

**Definition**:

```
struct snd_compr_task_status {
    __u64 seqno;
    __u64 input_size;
    __u64 output_size;
    __u32 output_flags;
    __u8 state;
    __u8 reserved[15];
};
```

**Members**

`seqno`
:   sequence number (task identifier)

`input_size`
:   filled data in bytes (from user space)

`output_size`
:   filled data in bytes (from driver)

`output_flags`
:   reserved for future (all zeros - from driver)

`state`
:   actual task state (SND\_COMPRESS\_TASK\_STATE\_\*)

`reserved`
:   reserved for future extension

struct snd\_enc\_vorbis
:   Vorbis encoder parameters

**Definition**:

```
struct snd_enc_vorbis {
    __s32 quality;
    __u32 managed;
    __u32 max_bit_rate;
    __u32 min_bit_rate;
    __u32 downmix;
};
```

**Members**

`quality`
:   Sets encoding quality to n, between -1 (low) and 10 (high).
    In the default mode of operation, the quality level is 3.
    Normal quality range is 0 - 10.

`managed`
:   Boolean. Set bitrate management mode. This turns off the
    normal VBR encoding, but allows hard or soft bitrate constraints to be
    enforced by the encoder. This mode can be slower, and may also be
    lower quality. It is primarily useful for streaming.

`max_bit_rate`
:   Enabled only if managed is TRUE

`min_bit_rate`
:   Enabled only if managed is TRUE

`downmix`
:   Boolean. Downmix input from stereo to mono (has no effect on
    non-stereo streams). Useful for lower-bitrate encoding.

**Description**

These options were extracted from the OpenMAX IL spec and Gstreamer vorbisenc
properties

For best quality users should specify VBR mode and set quality levels.

struct snd\_enc\_real
:   RealAudio encoder parameters

**Definition**:

```
struct snd_enc_real {
    __u32 quant_bits;
    __u32 start_region;
    __u32 num_regions;
};
```

**Members**

`quant_bits`
:   number of coupling quantization bits in the stream

`start_region`
:   coupling start region in the stream

`num_regions`
:   number of regions value

**Description**

These options were extracted from the OpenMAX IL spec

struct snd\_enc\_flac
:   FLAC encoder parameters

**Definition**:

```
struct snd_enc_flac {
    __u32 num;
    __u32 gain;
};
```

**Members**

`num`
:   serial number, valid only for OGG formats
    needs to be set by application

`gain`
:   Add replay gain tags

**Description**

These options were extracted from the FLAC online documentation
at <http://flac.sourceforge.net/documentation_tools_flac.html>

To make the API simpler, it is assumed that the user will select quality
profiles. Additional options that affect encoding quality and speed can
be added at a later stage if needed.

By default the Subset format is used by encoders.

TAGS such as pictures, etc, cannot be handled by an offloaded encoder and are
not supported in this API.

struct snd\_dec\_opus
:   Opus decoder parameters (raw opus packets)

**Definition**:

```
struct snd_dec_opus {
    __u8 version;
    __u8 num_channels;
    __u16 pre_skip;
    __u32 sample_rate;
    __u16 output_gain;
    __u8 mapping_family;
    struct snd_dec_opus_ch_map {
        __u8 stream_count;
        __u8 coupled_count;
        __u8 channel_map[8];
    } chan_map;
};
```

**Members**

`version`
:   Usually should be ‘1’ but can be split into major (4 upper bits)
    and minor (4 lower bits) sub-fields.

`num_channels`
:   Number of output channels.

`pre_skip`
:   Number of samples to discard at 48 kHz.

`sample_rate`
:   Sample rate of original input.

`output_gain`
:   Gain to apply when decoding (in Q7.8 format).

`mapping_family`
:   Order and meaning of output channels. Only values 0 and 1
    are expected; values 2..255 are not recommended for playback.

`chan_map`
:   Optional channel mapping table. Describes mapping of opus streams
    to decoded channels. Fields:

`chan_map.stream_count`
:   Number of streams encoded in each Ogg packet.

`chan_map.coupled_count`
:   Number of streams whose decoders are used
    for two channels.

`chan_map.channel_map`
:   Which decoded channel to be used for each one.
    Supports only mapping families 0 and 1,
    max number of channels is 8.

**Description**

These options were extracted from RFC7845 Section 5.

struct snd\_compr\_task\_runtime
:   task runtime description

**Definition**:

```
struct snd_compr_task_runtime {
    struct list_head list;
    struct dma_buf *input;
    struct dma_buf *output;
    u64 seqno;
    u64 input_size;
    u64 output_size;
    u32 flags;
    u8 state;
    void *private_value;
};
```

**Members**

`list`
:   list of all managed tasks

`input`
:   input DMA buffer

`output`
:   output DMA buffer

`seqno`
:   sequence number

`input_size`
:   really used data in the input buffer

`output_size`
:   really used data in the output buffer

`flags`
:   see SND\_COMPRESS\_TFLG\_\*

`state`
:   actual task state

`private_value`
:   used by the lowlevel driver (opaque)

struct snd\_compr\_runtime
:   runtime stream description

**Definition**:

```
struct snd_compr_runtime {
    snd_pcm_state_t state;
    struct snd_compr_ops *ops;
    void *buffer;
    u64 buffer_size;
    u32 fragment_size;
    u32 fragments;
    u64 total_bytes_available;
    u64 total_bytes_transferred;
    wait_queue_head_t sleep;
    void *private_data;
    unsigned char *dma_area;
    dma_addr_t dma_addr;
    size_t dma_bytes;
    struct snd_dma_buffer *dma_buffer_p;
#if IS_ENABLED(CONFIG_SND_COMPRESS_ACCEL);
    u32 active_tasks;
    u32 total_tasks;
    u64 task_seqno;
    struct list_head tasks;
#endif;
};
```

**Members**

`state`
:   stream state

`ops`
:   pointer to DSP callbacks

`buffer`
:   pointer to kernel buffer, valid only when not in mmap mode or
    DSP doesn’t implement copy

`buffer_size`
:   size of the above buffer

`fragment_size`
:   size of buffer fragment in bytes

`fragments`
:   number of such fragments

`total_bytes_available`
:   cumulative number of bytes made available in
    the ring buffer

`total_bytes_transferred`
:   cumulative bytes transferred by offload DSP

`sleep`
:   poll sleep

`private_data`
:   driver private data pointer

`dma_area`
:   virtual buffer address

`dma_addr`
:   physical buffer address (not accessible from main CPU)

`dma_bytes`
:   size of DMA area

`dma_buffer_p`
:   runtime dma buffer pointer

`active_tasks`
:   count of active tasks

`total_tasks`
:   count of all tasks

`task_seqno`
:   last task sequence number (!= 0)

`tasks`
:   list of all tasks

struct snd\_compr\_stream
:   compressed stream

**Definition**:

```
struct snd_compr_stream {
    const char *name;
    struct snd_compr_ops *ops;
    struct snd_compr_runtime *runtime;
    struct snd_compr *device;
    struct delayed_work error_work;
    enum snd_compr_direction direction;
    bool metadata_set;
    bool next_track;
    bool partial_drain;
    bool pause_in_draining;
    void *private_data;
    struct snd_dma_buffer dma_buffer;
};
```

**Members**

`name`
:   device name

`ops`
:   pointer to DSP callbacks

`runtime`
:   pointer to runtime structure

`device`
:   device pointer

`error_work`
:   delayed work used when closing the stream due to an error

`direction`
:   stream direction, playback/recording

`metadata_set`
:   metadata set flag, true when set

`next_track`
:   has userspace signal next track transition, true when set

`partial_drain`
:   undergoing partial\_drain for stream, true when set

`pause_in_draining`
:   paused during draining state, true when set

`private_data`
:   pointer to DSP private data

`dma_buffer`
:   allocated buffer if any

struct snd\_compr\_ops
:   compressed path DSP operations

**Definition**:

```
struct snd_compr_ops {
    int (*open)(struct snd_compr_stream *stream);
    int (*free)(struct snd_compr_stream *stream);
    int (*set_params)(struct snd_compr_stream *stream, struct snd_compr_params *params);
    int (*get_params)(struct snd_compr_stream *stream, struct snd_codec *params);
    int (*set_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata);
    int (*get_metadata)(struct snd_compr_stream *stream, struct snd_compr_metadata *metadata);
    int (*trigger)(struct snd_compr_stream *stream, int cmd);
    int (*pointer)(struct snd_compr_stream *stream, struct snd_compr_tstamp64 *tstamp);
    int (*copy)(struct snd_compr_stream *stream, char __user *buf, size_t count);
    int (*mmap)(struct snd_compr_stream *stream, struct vm_area_struct *vma);
    int (*ack)(struct snd_compr_stream *stream, size_t bytes);
    int (*get_caps) (struct snd_compr_stream *stream, struct snd_compr_caps *caps);
    int (*get_codec_caps) (struct snd_compr_stream *stream, struct snd_compr_codec_caps *codec);
#if IS_ENABLED(CONFIG_SND_COMPRESS_ACCEL);
    int (*task_create) (struct snd_compr_stream *stream, struct snd_compr_task_runtime *task);
    int (*task_start) (struct snd_compr_stream *stream, struct snd_compr_task_runtime *task);
    int (*task_stop) (struct snd_compr_stream *stream, struct snd_compr_task_runtime *task);
    int (*task_free) (struct snd_compr_stream *stream, struct snd_compr_task_runtime *task);
#endif;
};
```

**Members**

`open`
:   Open the compressed stream
    This callback is mandatory and shall keep dsp ready to receive the stream
    parameter

`free`
:   Close the compressed stream, mandatory

`set_params`
:   Sets the compressed stream parameters, mandatory
    This can be called in during stream creation only to set codec params
    and the stream properties

`get_params`
:   retrieve the codec parameters, mandatory

`set_metadata`
:   Set the metadata values for a stream

`get_metadata`
:   retrieves the requested metadata values from stream

`trigger`
:   Trigger operations like start, pause, resume, drain, stop.
    This callback is mandatory

`pointer`
:   Retrieve current h/w pointer information. Mandatory

`copy`
:   Copy the compressed data to/from userspace, Optional
    Can’t be implemented if DSP supports mmap

`mmap`
:   DSP mmap method to mmap DSP memory

`ack`
:   Ack for DSP when data is written to audio buffer, Optional
    Not valid if copy is implemented

`get_caps`
:   Retrieve DSP capabilities, mandatory

`get_codec_caps`
:   Retrieve capabilities for a specific codec, mandatory

`task_create`
:   Create a set of input/output buffers for accel operations

`task_start`
:   Start (queue) a task for accel operations

`task_stop`
:   Stop (dequeue) a task for accel operations

`task_free`
:   Free a set of input/output buffers for accel operations

struct snd\_compr
:   Compressed device

**Definition**:

```
struct snd_compr {
    const char *name;
    struct device *dev;
    struct snd_compr_ops *ops;
    void *private_data;
    struct snd_card *card;
    unsigned int direction;
    struct mutex lock;
    int device;
    bool use_pause_in_draining;
#ifdef CONFIG_SND_VERBOSE_PROCFS;
};
```

**Members**

`name`
:   DSP device name

`dev`
:   associated device instance

`ops`
:   pointer to DSP callbacks

`private_data`
:   pointer to DSP pvt data

`card`
:   sound card pointer

`direction`
:   Playback or capture direction

`lock`
:   device lock

`device`
:   device id

`use_pause_in_draining`
:   allow pause in draining, true when set

void snd\_compr\_use\_pause\_in\_draining(struct [snd\_compr\_stream](#c.snd_compr_stream "snd_compr_stream") \*substream)
:   Allow pause and resume in draining state

**Parameters**

`struct snd_compr_stream *substream`
:   compress substream to set

**Description**

Allow pause and resume in draining state.
Only HW driver supports this transition can call this API.

void snd\_compr\_set\_runtime\_buffer(struct [snd\_compr\_stream](#c.snd_compr_stream "snd_compr_stream") \*stream, struct snd\_dma\_buffer \*bufp)
:   Set the Compress runtime buffer

**Parameters**

`struct snd_compr_stream *stream`
:   compress stream to set

`struct snd_dma_buffer *bufp`
:   the buffer information, NULL to clear

**Description**

Copy the buffer information to runtime buffer when **bufp** is non-NULL.
Otherwise it clears the current buffer information.

## ASoC

### ASoC Core API

struct snd\_soc\_dai \*snd\_soc\_find\_dai(const struct snd\_soc\_dai\_link\_component \*dlc)
:   Find a registered DAI

**Parameters**

`const struct snd_soc_dai_link_component *dlc`
:   name of the DAI or the DAI driver and optional component info to match

**Description**

This function will search all registered components and their DAIs to
find the DAI of the same name. The component’s of\_node and name
should also match if being specified.

**Return**

pointer of DAI, or NULL if not found.

void snd\_soc\_remove\_pcm\_runtime(struct snd\_soc\_card \*card, struct snd\_soc\_pcm\_runtime \*rtd)
:   Remove a pcm\_runtime from card

**Parameters**

`struct snd_soc_card *card`
:   The ASoC card to which the pcm\_runtime has

`struct snd_soc_pcm_runtime *rtd`
:   The pcm\_runtime to remove

**Description**

This function removes a pcm\_runtime from the ASoC card.

int snd\_soc\_add\_pcm\_runtime(struct snd\_soc\_card \*card, struct snd\_soc\_dai\_link \*dai\_link)
:   Add a pcm\_runtime dynamically via dai\_link

**Parameters**

`struct snd_soc_card *card`
:   The ASoC card to which the pcm\_runtime is added

`struct snd_soc_dai_link *dai_link`
:   The DAI link to find pcm\_runtime

**Description**

This function adds a pcm\_runtime ASoC card by using dai\_link.

**Note**

Topology can use this API to add pcm\_runtime when probing the
topology component. And machine drivers can still define static
DAI links in dai\_link array.

int snd\_soc\_runtime\_set\_dai\_fmt(struct snd\_soc\_pcm\_runtime \*rtd, unsigned int dai\_fmt)
:   Change DAI link format for a ASoC runtime

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   The runtime for which the DAI link format should be changed

`unsigned int dai_fmt`
:   The new DAI link format

**Description**

This function updates the DAI link format for all DAIs connected to the DAI
link for the specified runtime.

**Note**

For setups with a static format set the dai\_fmt field in the
corresponding snd\_dai\_link `struct instead` of using this function.

Returns 0 on success, otherwise a negative error code.

int snd\_soc\_set\_dmi\_name(struct snd\_soc\_card \*card)
:   Register DMI names to card

**Parameters**

`struct snd_soc_card *card`
:   The card to register DMI names

**Description**

An Intel machine driver may be used by many different devices but are
difficult for userspace to differentiate, since machine drivers usually
use their own name as the card short name and leave the card long name
blank. To differentiate such devices and fix bugs due to lack of
device-specific configurations, this function allows DMI info to be used
as the sound card long name, in the format of
“vendor-product-version-board”
(Character ‘-’ is used to separate different DMI fields here).
This will help the user space to load the device-specific Use Case Manager
(UCM) configurations for the card.

Possible card long names may be:
DellInc.-XPS139343-01-0310JH
ASUSTeKCOMPUTERINC.-T100TA-1.0-T100TA
Circuitco-MinnowboardMaxD0PLATFORM-D0-MinnowBoardMAX

This function also supports flavoring the card longname to provide
the extra differentiation, like “vendor-product-version-board-flavor”.

We only keep number and alphabet characters and a few separator characters
in the card long name since UCM in the user space uses the card long names
as card configuration directory names and AudoConf cannot support special
characters like SPACE.

Returns 0 on success, otherwise a negative error code.

struct snd\_kcontrol \*snd\_soc\_cnew(const struct snd\_kcontrol\_new \*\_template, void \*data, const char \*long\_name, const char \*prefix)
:   create new control

**Parameters**

`const struct snd_kcontrol_new *_template`
:   control template

`void *data`
:   control private data

`const char *long_name`
:   control long name

`const char *prefix`
:   control name prefix

**Description**

Create a new mixer control from a template control.

Returns 0 for success, else error.

int snd\_soc\_add\_component\_controls(struct snd\_soc\_component \*component, const struct snd\_kcontrol\_new \*controls, unsigned int num\_controls)
:   Add an array of controls to a component.

**Parameters**

`struct snd_soc_component *component`
:   Component to add controls to

`const struct snd_kcontrol_new *controls`
:   Array of controls to add

`unsigned int num_controls`
:   Number of elements in the array

**Return**

0 for success, else error.

int snd\_soc\_add\_card\_controls(struct snd\_soc\_card \*soc\_card, const struct snd\_kcontrol\_new \*controls, int num\_controls)
:   add an array of controls to a SoC card. Convenience function to add a list of controls.

**Parameters**

`struct snd_soc_card *soc_card`
:   SoC card to add controls to

`const struct snd_kcontrol_new *controls`
:   array of controls to add

`int num_controls`
:   number of elements in the array

**Description**

Return 0 for success, else error.

int snd\_soc\_add\_dai\_controls(struct snd\_soc\_dai \*dai, const struct snd\_kcontrol\_new \*controls, int num\_controls)
:   add an array of controls to a DAI. Convenience function to add a list of controls.

**Parameters**

`struct snd_soc_dai *dai`
:   DAI to add controls to

`const struct snd_kcontrol_new *controls`
:   array of controls to add

`int num_controls`
:   number of elements in the array

**Description**

Return 0 for success, else error.

int snd\_soc\_register\_card(struct snd\_soc\_card \*card)
:   Register a card with the ASoC core

**Parameters**

`struct snd_soc_card *card`
:   Card to register

void snd\_soc\_unregister\_card(struct snd\_soc\_card \*card)
:   Unregister a card with the ASoC core

**Parameters**

`struct snd_soc_card *card`
:   Card to unregister

struct snd\_soc\_dai \*snd\_soc\_register\_dai(struct snd\_soc\_component \*component, struct snd\_soc\_dai\_driver \*dai\_drv, bool legacy\_dai\_naming)
:   Register a DAI dynamically & create its widgets

**Parameters**

`struct snd_soc_component *component`
:   The component the DAIs are registered for

`struct snd_soc_dai_driver *dai_drv`
:   DAI driver to use for the DAI

`bool legacy_dai_naming`
:   if `true`, use legacy single-name format;
    if `false`, use multiple-name format;

**Description**

Topology can use this API to register DAIs when probing a component.
These DAIs’s widgets will be freed in the card cleanup and the DAIs
will be freed in the component cleanup.

void snd\_soc\_unregister\_dais(struct snd\_soc\_component \*component)
:   Unregister DAIs from the ASoC core

**Parameters**

`struct snd_soc_component *component`
:   The component for which the DAIs should be unregistered

int snd\_soc\_register\_dais(struct snd\_soc\_component \*component, struct snd\_soc\_dai\_driver \*dai\_drv, size\_t count)
:   Register a DAI with the ASoC core

**Parameters**

`struct snd_soc_component *component`
:   The component the DAIs are registered for

`struct snd_soc_dai_driver *dai_drv`
:   DAI driver to use for the DAIs

`size_t count`
:   Number of DAIs

void snd\_soc\_unregister\_component\_by\_driver(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, const struct snd\_soc\_component\_driver \*component\_driver)
:   Unregister component using a given driver from the ASoC core

**Parameters**

`struct device *dev`
:   The device to unregister

`const struct snd_soc_component_driver *component_driver`
:   The component driver to unregister

int devm\_snd\_soc\_register\_component(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, const struct snd\_soc\_component\_driver \*cmpnt\_drv, struct snd\_soc\_dai\_driver \*dai\_drv, int num\_dai)
:   resource managed component registration

**Parameters**

`struct device *dev`
:   Device used to manage component

`const struct snd_soc_component_driver *cmpnt_drv`
:   Component driver

`struct snd_soc_dai_driver *dai_drv`
:   DAI driver

`int num_dai`
:   Number of DAIs to register

**Description**

Register a component with automatic unregistration when the device is
unregistered.

int devm\_snd\_soc\_register\_card(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, struct snd\_soc\_card \*card)
:   resource managed card registration

**Parameters**

`struct device *dev`
:   Device used to manage card

`struct snd_soc_card *card`
:   Card to register

**Description**

Register a card with automatic unregistration when the device is
unregistered.

int devm\_snd\_dmaengine\_pcm\_register(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, const struct [snd\_dmaengine\_pcm\_config](#c.snd_dmaengine_pcm_config "snd_dmaengine_pcm_config") \*config, unsigned int flags)
:   resource managed dmaengine PCM registration

**Parameters**

`struct device *dev`
:   The parent device for the PCM device

`const struct snd_dmaengine_pcm_config *config`
:   Platform specific PCM configuration

`unsigned int flags`
:   Platform specific quirks

**Description**

Register a dmaengine based PCM device with automatic unregistration when the
device is unregistered.

int snd\_soc\_component\_set\_sysclk(struct snd\_soc\_component \*component, int clk\_id, int source, unsigned int freq, int dir)
:   configure COMPONENT system or master clock.

**Parameters**

`struct snd_soc_component *component`
:   COMPONENT

`int clk_id`
:   DAI specific clock ID

`int source`
:   Source for the clock

`unsigned int freq`
:   new clock frequency in Hz

`int dir`
:   new clock direction - input/output.

**Description**

Configures the CODEC master (MCLK) or system (SYSCLK) clocking.

int snd\_soc\_component\_set\_jack(struct snd\_soc\_component \*component, struct snd\_soc\_jack \*jack, void \*data)
:   configure component jack.

**Parameters**

`struct snd_soc_component *component`
:   COMPONENTs

`struct snd_soc_jack *jack`
:   structure to use for the jack

`void *data`
:   can be used if codec driver need extra data for configuring jack

**Description**

Configures and enables jack detection function.

int snd\_soc\_component\_get\_jack\_type(struct snd\_soc\_component \*component)

**Parameters**

`struct snd_soc_component *component`
:   COMPONENTs

**Description**

Returns the jack type of the component
This can either be the supported type or one read from
devicetree with the property: jack-type.

void snd\_soc\_component\_init\_regmap(struct snd\_soc\_component \*component, struct [regmap](#c.snd_soc_component_init_regmap "regmap") \*regmap)
:   Initialize regmap instance for the component

**Parameters**

`struct snd_soc_component *component`
:   The component for which to initialize the regmap instance

`struct regmap *regmap`
:   The regmap instance that should be used by the component

**Description**

This function allows deferred assignment of the regmap instance that is
associated with the component. Only use this if the regmap instance is not
yet ready when the component is registered. The function must also be called
before the first IO attempt of the component.

void snd\_soc\_component\_exit\_regmap(struct snd\_soc\_component \*component)
:   De-initialize regmap instance for the component

**Parameters**

`struct snd_soc_component *component`
:   The component for which to de-initialize the regmap instance

**Description**

Calls `regmap_exit()` on the regmap instance associated to the component and
removes the regmap instance from the component.

This function should only be used if [`snd_soc_component_init_regmap()`](#c.snd_soc_component_init_regmap "snd_soc_component_init_regmap") was used
to initialize the regmap instance.

unsigned int snd\_soc\_component\_read(struct snd\_soc\_component \*component, unsigned int reg)
:   Read register value

**Parameters**

`struct snd_soc_component *component`
:   Component to read from

`unsigned int reg`
:   Register to read

**Return**

read value

int snd\_soc\_component\_write(struct snd\_soc\_component \*component, unsigned int reg, unsigned int val)
:   Write register value

**Parameters**

`struct snd_soc_component *component`
:   Component to write to

`unsigned int reg`
:   Register to write

`unsigned int val`
:   Value to write to the register

**Return**

0 on success, a negative error code otherwise.

int snd\_soc\_component\_update\_bits(struct snd\_soc\_component \*component, unsigned int reg, unsigned int mask, unsigned int val)
:   Perform read/modify/write cycle

**Parameters**

`struct snd_soc_component *component`
:   Component to update

`unsigned int reg`
:   Register to update

`unsigned int mask`
:   Mask that specifies which bits to update

`unsigned int val`
:   New value for the bits specified by mask

**Return**

1 if the operation was successful and the value of the register
changed, 0 if the operation was successful, but the value did not change.
Returns a negative error code otherwise.

int snd\_soc\_component\_update\_bits\_async(struct snd\_soc\_component \*component, unsigned int reg, unsigned int mask, unsigned int val)
:   Perform asynchronous read/modify/write cycle

**Parameters**

`struct snd_soc_component *component`
:   Component to update

`unsigned int reg`
:   Register to update

`unsigned int mask`
:   Mask that specifies which bits to update

`unsigned int val`
:   New value for the bits specified by mask

**Description**

This function is similar to [`snd_soc_component_update_bits()`](#c.snd_soc_component_update_bits "snd_soc_component_update_bits"), but the update
operation is scheduled asynchronously. This means it may not be completed
when the function returns. To make sure that all scheduled updates have been
completed [`snd_soc_component_async_complete()`](#c.snd_soc_component_async_complete "snd_soc_component_async_complete") must be called.

**Return**

1 if the operation was successful and the value of the register
changed, 0 if the operation was successful, but the value did not change.
Returns a negative error code otherwise.

unsigned int snd\_soc\_component\_read\_field(struct snd\_soc\_component \*component, unsigned int reg, unsigned int mask)
:   Read register field value

**Parameters**

`struct snd_soc_component *component`
:   Component to read from

`unsigned int reg`
:   Register to read

`unsigned int mask`
:   mask of the register field

**Return**

read value of register field.

int snd\_soc\_component\_write\_field(struct snd\_soc\_component \*component, unsigned int reg, unsigned int mask, unsigned int val)
:   write to register field

**Parameters**

`struct snd_soc_component *component`
:   Component to write to

`unsigned int reg`
:   Register to write

`unsigned int mask`
:   mask of the register field to update

`unsigned int val`
:   value of the field to write

**Return**

1 for change, otherwise 0.

void snd\_soc\_component\_async\_complete(struct snd\_soc\_component \*component)
:   Ensure asynchronous I/O has completed

**Parameters**

`struct snd_soc_component *component`
:   Component for which to wait

**Description**

This function blocks until all asynchronous I/O which has previously been
scheduled using [`snd_soc_component_update_bits_async()`](#c.snd_soc_component_update_bits_async "snd_soc_component_update_bits_async") has completed.

int snd\_soc\_component\_test\_bits(struct snd\_soc\_component \*component, unsigned int reg, unsigned int mask, unsigned int value)
:   Test register for change

**Parameters**

`struct snd_soc_component *component`
:   component

`unsigned int reg`
:   Register to test

`unsigned int mask`
:   Mask that specifies which bits to test

`unsigned int value`
:   Value to test against

**Description**

Tests a register with a new value and checks if the new value is
different from the old value.

**Return**

1 for change, otherwise 0.

void snd\_soc\_runtime\_action(struct snd\_soc\_pcm\_runtime \*rtd, int stream, int action)
:   Increment/Decrement active count for PCM runtime components

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   ASoC PCM runtime that is activated

`int stream`
:   Direction of the PCM stream

`int action`
:   Activate stream if 1. Deactivate if -1.

**Description**

Increments/Decrements the active count for all the DAIs and components
attached to a PCM runtime.
Should typically be called when a stream is opened.

Must be called with the rtd->card->pcm\_mutex being held

bool snd\_soc\_runtime\_ignore\_pmdown\_time(struct snd\_soc\_pcm\_runtime \*rtd)
:   Check whether to ignore the power down delay

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   The ASoC PCM runtime that should be checked.

**Description**

This function checks whether the power down delay should be ignored for a
specific PCM runtime. Returns true if the delay is 0, if the DAI link has
been configured to ignore the delay, or if none of the components benefits
from having the delay.

int snd\_soc\_runtime\_calc\_hw(struct snd\_soc\_pcm\_runtime \*rtd, struct snd\_pcm\_hardware \*hw, int stream)
:   Calculate hw limits for a PCM stream

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   ASoC PCM runtime

`struct snd_pcm_hardware *hw`
:   PCM hardware parameters (output)

`int stream`
:   Direction of the PCM stream

**Description**

Calculates the subset of stream parameters supported by all DAIs
associated with the PCM stream.

int snd\_soc\_info\_enum\_double(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   enumerated double mixer info callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_info *uinfo`
:   control element information

**Description**

Callback to provide information about a double enumerated
mixer control.

Returns 0 for success.

int snd\_soc\_get\_enum\_double(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   enumerated double mixer get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value of a double enumerated mixer.

Returns 0 for success.

int snd\_soc\_put\_enum\_double(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   enumerated double mixer put callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value of a double enumerated mixer.

Returns 0 for success.

int snd\_soc\_info\_volsw(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   single mixer info callback with range.

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_info *uinfo`
:   control element information

**Description**

Callback to provide information, with a range, about a single mixer control,
or a double mixer control that spans 2 registers.

Returns 0 for success.

int snd\_soc\_info\_volsw\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   Mixer info callback for SX TLV controls

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_info *uinfo`
:   control element information

**Description**

Callback to provide information about a single mixer control, or a double
mixer control that spans 2 registers of the SX TLV type. SX TLV controls
have a range that represents both positive and negative values either side
of zero but without a sign bit. min is the minimum register value, max is
the number of steps.

Returns 0 for success.

int snd\_soc\_get\_volsw(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   single mixer get callback with range

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value, within a range, of a single mixer control, or a
double mixer control that spans 2 registers.

Returns 0 for success.

int snd\_soc\_put\_volsw(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   single mixer put callback with range

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value , within a range, of a single mixer control, or
a double mixer control that spans 2 registers.

Returns 0 for success.

int snd\_soc\_get\_volsw\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   single mixer get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value of a single mixer control, or a double mixer
control that spans 2 registers.

Returns 0 for success.

int snd\_soc\_put\_volsw\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   double mixer set callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value of a double mixer control that spans 2 registers.

Returns 0 for success.

int snd\_soc\_limit\_volume(struct snd\_soc\_card \*card, const char \*name, int max)
:   Set new limit to an existing volume control.

**Parameters**

`struct snd_soc_card *card`
:   where to look for the control

`const char *name`
:   Name of the control

`int max`
:   new maximum limit

**Description**

Return 0 for success, else error.

int snd\_soc\_info\_xr\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   signed multi register info callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mreg control

`struct snd_ctl_elem_info *uinfo`
:   control element information

**Description**

Callback to provide information of a control that can span multiple
codec registers which together forms a single signed value. Note
that unlike the non-xr variant of sx controls these may or may not
include the sign bit, depending on nbits, and there is no shift.

Returns 0 for success.

int snd\_soc\_get\_xr\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   signed multi register get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mreg control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value of a control that can span multiple codec
registers which together forms a single signed value. The control
supports specifying total no of bits used to allow for bitfields
across the multiple codec registers. Note that unlike the non-xr
variant of sx controls these may or may not include the sign bit,
depending on nbits, and there is no shift.

Returns 0 for success.

int snd\_soc\_put\_xr\_sx(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   signed multi register get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mreg control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value of a control that can span multiple codec
registers which together forms a single signed value. The control
supports specifying total no of bits used to allow for bitfields
across the multiple codec registers. Note that unlike the non-xr
variant of sx controls these may or may not include the sign bit,
depending on nbits, and there is no shift.

Returns 0 for success.

int snd\_soc\_get\_strobe(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   strobe get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback get the value of a strobe mixer control.

Returns 0 for success.

int snd\_soc\_put\_strobe(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   strobe put callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback strobe a register bit to high then low (or the inverse)
in one pass of a single mixer `enum control`.

Returns 1 for success.

int snd\_soc\_new\_compress(struct snd\_soc\_pcm\_runtime \*rtd)
:   create a new compress.

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   The runtime for which we will create compress

**Return**

0 for success, else error.

### ASoC DAPM API

struct snd\_soc\_dapm\_widget \*snd\_soc\_dapm\_kcontrol\_to\_widget(struct snd\_kcontrol \*kcontrol)
:   Returns the widget associated to a kcontrol

**Parameters**

`struct snd_kcontrol *kcontrol`
:   The kcontrol

struct snd\_soc\_dapm\_context \*snd\_soc\_dapm\_kcontrol\_to\_dapm(struct snd\_kcontrol \*kcontrol)
:   Returns the dapm context associated to a kcontrol

**Parameters**

`struct snd_kcontrol *kcontrol`
:   The kcontrol

**Note**

This function must only be used on kcontrols that are known to have
been registered for a CODEC. Otherwise the behaviour is undefined.

struct snd\_soc\_component \*snd\_soc\_dapm\_kcontrol\_to\_component(struct snd\_kcontrol \*kcontrol)
:   Returns the component associated to a kcontrol

**Parameters**

`struct snd_kcontrol *kcontrol`
:   The kcontrol

**Description**

This function must only be used on DAPM contexts that are known to be part of
a COMPONENT (e.g. in a COMPONENT driver). Otherwise the behavior is undefined

int snd\_soc\_dapm\_force\_bias\_level(struct snd\_soc\_dapm\_context \*dapm, enum snd\_soc\_bias\_level level)
:   Sets the DAPM bias level

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   The DAPM context for which to set the level

`enum snd_soc_bias_level level`
:   The level to set

**Description**

Forces the DAPM bias level to a specific state. It will call the bias level
callback of DAPM context with the specified level. This will even happen if
the context is already at the same level. Furthermore it will not go through
the normal bias level sequencing, meaning any intermediate states between the
current and the target state will not be entered.

Note that the change in bias level is only temporary and the next time
[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") is called the state will be set to the level as
determined by the DAPM core. The function is mainly intended to be used to
used during probe or resume from suspend to power up the device so
initialization can be done, before the DAPM core takes over.

void snd\_soc\_dapm\_init\_bias\_level(struct snd\_soc\_dapm\_context \*dapm, enum snd\_soc\_bias\_level level)
:   Initialize DAPM bias level

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   The DAPM context to initialize

`enum snd_soc_bias_level level`
:   The DAPM level to initialize to

**Description**

This function only sets the driver internal state of the DAPM level and will
not modify the state of the device. Hence it should not be used during normal
operation, but only to synchronize the internal state to the device state.
E.g. during driver probe to set the DAPM level to the one corresponding with
the power-on reset state of the device.

To change the DAPM state of the device use [`snd_soc_dapm_set_bias_level()`](#c.snd_soc_dapm_set_bias_level "snd_soc_dapm_set_bias_level").

int snd\_soc\_dapm\_set\_bias\_level(struct snd\_soc\_dapm\_context \*dapm, enum snd\_soc\_bias\_level level)
:   set the bias level for the system

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`enum snd_soc_bias_level level`
:   level to configure

**Description**

Configure the bias (power) levels for the SoC audio device.

Returns 0 for success else error.

enum snd\_soc\_bias\_level snd\_soc\_dapm\_get\_bias\_level(struct snd\_soc\_dapm\_context \*dapm)
:   Get current DAPM bias level

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   The context for which to get the bias level

**Return**

The current bias level of the passed DAPM context.

int snd\_soc\_dapm\_dai\_get\_connected\_widgets(struct snd\_soc\_dai \*dai, int stream, struct snd\_soc\_dapm\_widget\_list \*\*list, bool (\*custom\_stop\_condition)(struct snd\_soc\_dapm\_widget\*, enum snd\_soc\_dapm\_direction))
:   query audio path and it’s widgets.

**Parameters**

`struct snd_soc_dai *dai`
:   the soc DAI.

`int stream`
:   stream direction.

`struct snd_soc_dapm_widget_list **list`
:   list of active widgets for this stream.

`bool (*custom_stop_condition)(struct snd_soc_dapm_widget *, enum snd_soc_dapm_direction)`
:   (optional) a function meant to stop the widget graph
    walk based on custom logic.

**Description**

Queries DAPM graph as to whether a valid audio stream path exists for
the initial stream specified by name. This takes into account
current mixer and mux kcontrol settings. Creates list of valid widgets.

Optionally, can be supplied with a function acting as a stopping condition.
This function takes the dapm widget currently being examined and the walk
direction as an arguments, it should return true if the walk should be
stopped and false otherwise.

Returns the number of valid paths or negative error.

void snd\_soc\_dapm\_free\_widget(struct snd\_soc\_dapm\_widget \*w)
:   Free specified widget

**Parameters**

`struct snd_soc_dapm_widget *w`
:   widget to free

**Description**

Removes widget from all paths and frees memory occupied by it.

int snd\_soc\_dapm\_sync\_unlocked(struct snd\_soc\_dapm\_context \*dapm)
:   scan and power dapm paths

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

**Description**

Walks all dapm audio paths and powers widgets according to their
stream or path usage.

Requires external locking.

Returns 0 for success.

int snd\_soc\_dapm\_sync(struct snd\_soc\_dapm\_context \*dapm)
:   scan and power dapm paths

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

**Description**

Walks all dapm audio paths and powers widgets according to their
stream or path usage.

Returns 0 for success.

int snd\_soc\_dapm\_add\_routes(struct snd\_soc\_dapm\_context \*dapm, const struct snd\_soc\_dapm\_route \*route, int num)
:   Add routes between DAPM widgets

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const struct snd_soc_dapm_route *route`
:   audio routes

`int num`
:   number of routes

**Description**

Connects 2 dapm widgets together via a named audio path. The sink is
the widget receiving the audio signal, whilst the source is the sender
of the audio signal.

Returns 0 for success else error. On error all resources can be freed
with a call to `snd_soc_card_free()`.

int snd\_soc\_dapm\_del\_routes(struct snd\_soc\_dapm\_context \*dapm, const struct snd\_soc\_dapm\_route \*route, int num)
:   Remove routes between DAPM widgets

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const struct snd_soc_dapm_route *route`
:   audio routes

`int num`
:   number of routes

**Description**

Removes routes from the DAPM context.

int snd\_soc\_dapm\_new\_widgets(struct snd\_soc\_card \*card)
:   add new dapm widgets

**Parameters**

`struct snd_soc_card *card`
:   card to be checked for new dapm widgets

**Description**

Checks the codec for any new dapm widgets and creates them if found.

Returns 0 for success.

int snd\_soc\_dapm\_get\_volsw(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   dapm mixer get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value of a dapm mixer control.

Returns 0 for success.

int snd\_soc\_dapm\_put\_volsw(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   dapm mixer set callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value of a dapm mixer control.

Returns 0 for success.

int snd\_soc\_dapm\_get\_enum\_double(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   dapm enumerated double mixer get callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to get the value of a dapm enumerated double mixer control.

Returns 0 for success.

int snd\_soc\_dapm\_put\_enum\_double(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   dapm enumerated double mixer set callback

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   control element information

**Description**

Callback to set the value of a dapm enumerated double mixer control.

Returns 0 for success.

int snd\_soc\_dapm\_info\_pin\_switch(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_info \*uinfo)
:   Info for a pin switch

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_info *uinfo`
:   control element information

**Description**

Callback to provide information about a pin switch control.

int snd\_soc\_dapm\_get\_pin\_switch(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   Get information for a pin switch

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   Value

**Description**

Callback to provide information for a pin switch added at the card
level.

int snd\_soc\_dapm\_get\_component\_pin\_switch(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   Get information for a pin switch

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   Value

**Description**

Callback to provide information for a pin switch added at the component
level.

int snd\_soc\_dapm\_put\_pin\_switch(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   Set information for a pin switch

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   Value

**Description**

Callback to provide information for a pin switch added at the card
level.

int snd\_soc\_dapm\_put\_component\_pin\_switch(struct snd\_kcontrol \*kcontrol, struct snd\_ctl\_elem\_value \*ucontrol)
:   Set information for a pin switch

**Parameters**

`struct snd_kcontrol *kcontrol`
:   mixer control

`struct snd_ctl_elem_value *ucontrol`
:   Value

**Description**

Callback to provide information for a pin switch added at the component
level.

struct snd\_soc\_dapm\_widget \*snd\_soc\_dapm\_new\_control(struct snd\_soc\_dapm\_context \*dapm, const struct snd\_soc\_dapm\_widget \*widget)
:   create new dapm control

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const struct snd_soc_dapm_widget *widget`
:   widget template

**Description**

Creates new DAPM control based upon a template.

Returns a widget pointer on success or an error pointer on failure

int snd\_soc\_dapm\_new\_controls(struct snd\_soc\_dapm\_context \*dapm, const struct snd\_soc\_dapm\_widget \*widget, unsigned int num)
:   create new dapm controls

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const struct snd_soc_dapm_widget *widget`
:   widget array

`unsigned int num`
:   number of widgets

**Description**

Creates new DAPM controls based upon the templates.

Returns 0 for success else error.

int snd\_soc\_dapm\_new\_dai\_widgets(struct snd\_soc\_dapm\_context \*dapm, struct snd\_soc\_dai \*dai)
:   Create new DAPM widgets

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`struct snd_soc_dai *dai`
:   parent DAI

**Description**

Returns 0 on success, error code otherwise.

void snd\_soc\_dapm\_stream\_event(struct snd\_soc\_pcm\_runtime \*rtd, int stream, int event)
:   send a stream event to the dapm core

**Parameters**

`struct snd_soc_pcm_runtime *rtd`
:   PCM runtime data

`int stream`
:   stream name

`int event`
:   stream event

**Description**

Sends a stream event to the dapm core. The core then makes any
necessary widget power changes.

Returns 0 for success else error.

int snd\_soc\_dapm\_enable\_pin\_unlocked(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   enable pin.

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Enables input/output pin and its parents or children widgets iff there is
a valid audio route and active audio stream.

Requires external locking.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_enable\_pin(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   enable pin.

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Enables input/output pin and its parents or children widgets iff there is
a valid audio route and active audio stream.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_force\_enable\_pin\_unlocked(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   force a pin to be enabled

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Enables input/output pin regardless of any other state. This is
intended for use with microphone bias supplies used in microphone
jack detection.

Requires external locking.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_force\_enable\_pin(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   force a pin to be enabled

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Enables input/output pin regardless of any other state. This is
intended for use with microphone bias supplies used in microphone
jack detection.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_disable\_pin\_unlocked(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   disable pin.

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Disables input/output pin and its parents or children widgets.

Requires external locking.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_disable\_pin(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   disable pin.

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   pin name

**Description**

Disables input/output pin and its parents or children widgets.

**NOTE**

[`snd_soc_dapm_sync()`](#c.snd_soc_dapm_sync "snd_soc_dapm_sync") needs to be called after this for DAPM to
do any widget power switching.

int snd\_soc\_dapm\_get\_pin\_status(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   get audio pin status

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   audio signal pin endpoint (or start point)

**Description**

Get audio pin status - connected or disconnected.

Returns 1 for connected otherwise 0.

int snd\_soc\_dapm\_ignore\_suspend(struct snd\_soc\_dapm\_context \*dapm, const char \*pin)
:   ignore suspend status for DAPM endpoint

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

`const char *pin`
:   audio signal pin endpoint (or start point)

**Description**

Mark the given endpoint or pin as ignoring suspend. When the
system is disabled a path between two endpoints flagged as ignoring
suspend will not be disabled. The path must already be enabled via
normal means at suspend time, it will not be turned on if it was not
already enabled.

void snd\_soc\_dapm\_free(struct snd\_soc\_dapm\_context \*dapm)
:   free dapm resources

**Parameters**

`struct snd_soc_dapm_context *dapm`
:   DAPM context

**Description**

Free all dapm widgets and resources.

### ASoC DMA Engine API

int snd\_dmaengine\_pcm\_prepare\_slave\_config(struct snd\_pcm\_substream \*substream, struct snd\_pcm\_hw\_params \*params, struct dma\_slave\_config \*slave\_config)
:   Generic prepare\_slave\_config callback

**Parameters**

`struct snd_pcm_substream *substream`
:   PCM substream

`struct snd_pcm_hw_params *params`
:   hw\_params

`struct dma_slave_config *slave_config`
:   DMA slave config to prepare

**Description**

This function can be used as a generic prepare\_slave\_config callback for
platforms which make use of the snd\_dmaengine\_dai\_dma\_data struct for their
DAI DMA data. Internally the function will first call
snd\_hwparams\_to\_dma\_slave\_config to fill in the slave config based on the
hw\_params, followed by snd\_dmaengine\_pcm\_set\_config\_from\_dai\_data to fill in
the remaining fields based on the DAI DMA data.

int snd\_dmaengine\_pcm\_register(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, const struct [snd\_dmaengine\_pcm\_config](#c.snd_dmaengine_pcm_config "snd_dmaengine_pcm_config") \*config, unsigned int flags)
:   Register a dmaengine based PCM device

**Parameters**

`struct device *dev`
:   The parent device for the PCM device

`const struct snd_dmaengine_pcm_config *config`
:   Platform specific PCM configuration

`unsigned int flags`
:   Platform specific quirks

void snd\_dmaengine\_pcm\_unregister(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev)
:   Removes a dmaengine based PCM device

**Parameters**

`struct device *dev`
:   Parent device the PCM was register with

**Description**

Removes a dmaengine based PCM device previously registered with
snd\_dmaengine\_pcm\_register.

## Miscellaneous Functions

### Hardware-Dependent Devices API

int snd\_hwdep\_new(struct snd\_card \*card, char \*id, int device, struct snd\_hwdep \*\*rhwdep)
:   create a new hwdep instance

**Parameters**

`struct snd_card *card`
:   the card instance

`char *id`
:   the id string

`int device`
:   the device index (zero-based)

`struct snd_hwdep **rhwdep`
:   the pointer to store the new hwdep instance

**Description**

Creates a new hwdep instance with the given index on the card.
The callbacks (hwdep->ops) must be set on the returned instance
after this call manually by the caller.

**Return**

Zero if successful, or a negative error code on failure.

### Jack Abstraction Layer API

enum snd\_jack\_types
:   Jack types which can be reported

**Constants**

`SND_JACK_HEADPHONE`
:   Headphone

`SND_JACK_MICROPHONE`
:   Microphone

`SND_JACK_HEADSET`
:   Headset

`SND_JACK_LINEOUT`
:   Line out

`SND_JACK_MECHANICAL`
:   Mechanical switch

`SND_JACK_VIDEOOUT`
:   Video out

`SND_JACK_AVOUT`
:   AV (Audio Video) out

`SND_JACK_LINEIN`
:   Line in

`SND_JACK_USB`
:   USB audio device

`SND_JACK_BTN_0`
:   Button 0

`SND_JACK_BTN_1`
:   Button 1

`SND_JACK_BTN_2`
:   Button 2

`SND_JACK_BTN_3`
:   Button 3

`SND_JACK_BTN_4`
:   Button 4

`SND_JACK_BTN_5`
:   Button 5

**Description**

These values are used as a bitmask.

Note that this must be kept in sync with the lookup table in
sound/core/jack.c.

int snd\_jack\_add\_new\_kctl(struct snd\_jack \*jack, const char \*name, int mask)
:   Create a new snd\_jack\_kctl and add it to jack

**Parameters**

`struct snd_jack *jack`
:   the jack instance which the kctl will attaching to

`const char * name`
:   the name for the snd\_kcontrol object

`int mask`
:   a bitmask of `enum snd_jack_type` values that can be detected
    by this snd\_jack\_kctl object.

**Description**

Creates a new snd\_kcontrol object and adds it to the jack kctl\_list.

**Return**

Zero if successful, or a negative error code on failure.

int snd\_jack\_new(struct snd\_card \*card, const char \*id, int type, struct snd\_jack \*\*jjack, bool initial\_kctl, bool phantom\_jack)
:   Create a new jack

**Parameters**

`struct snd_card *card`
:   the card instance

`const char *id`
:   an identifying string for this jack

`int type`
:   a bitmask of `enum snd_jack_type` values that can be detected by
    this jack

`struct snd_jack **jjack`
:   Used to provide the allocated jack object to the caller.

`bool initial_kctl`
:   if true, create a kcontrol and add it to the jack list.

`bool phantom_jack`
:   Don’t create a input device for phantom jacks.

**Description**

Creates a new jack object.

**Return**

Zero if successful, or a negative error code on failure.
On success **jjack** will be initialised.

int snd\_jack\_set\_key(struct snd\_jack \*jack, enum [snd\_jack\_types](#c.snd_jack_types "snd_jack_types") type, int keytype)
:   Set a key mapping on a jack

**Parameters**

`struct snd_jack *jack`
:   The jack to configure

`enum snd_jack_types type`
:   Jack report type for this key

`int keytype`
:   Input layer key type to be reported

**Description**

Map a SND\_JACK\_BTN\_\* button type to an input layer key, allowing
reporting of keys on accessories via the jack abstraction. If no
mapping is provided but keys are enabled in the jack type then
BTN\_n numeric buttons will be reported.

If jacks are not reporting via the input API this call will have no
effect.

Note that this is intended to be use by simple devices with small
numbers of keys that can be reported. It is also possible to
access the input device directly - devices with complex input
capabilities on accessories should consider doing this rather than
using this abstraction.

This function may only be called prior to registration of the jack.

**Return**

Zero if successful, or a negative error code on failure.

void snd\_jack\_report(struct snd\_jack \*jack, int status)
:   Report the current status of a jack

**Parameters**

`struct snd_jack *jack`
:   The jack to report status for

`int status`
:   The current status of the jack

**Note**

This function uses mutexes and should be called from a
context which can sleep (such as a workqueue).

void snd\_soc\_jack\_report(struct snd\_soc\_jack \*jack, int status, int mask)
:   Report the current status for a jack

**Parameters**

`struct snd_soc_jack *jack`
:   the jack

`int status`
:   a bitmask of `enum snd_jack_type` values that are currently detected.

`int mask`
:   a bitmask of `enum snd_jack_type` values that being reported.

**Description**

If configured using [`snd_soc_jack_add_pins()`](#c.snd_soc_jack_add_pins "snd_soc_jack_add_pins") then the associated
DAPM pins will be enabled or disabled as appropriate and DAPM
synchronised.

**Note**

This function uses mutexes and should be called from a
context which can sleep (such as a workqueue).

int snd\_soc\_jack\_add\_zones(struct snd\_soc\_jack \*jack, int count, struct snd\_soc\_jack\_zone \*zones)
:   Associate voltage zones with jack

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`int count`
:   Number of zones

`struct snd_soc_jack_zone *zones`
:   Array of zones

**Description**

After this function has been called the zones specified in the
array will be associated with the jack.

int snd\_soc\_jack\_get\_type(struct snd\_soc\_jack \*jack, int micbias\_voltage)
:   Based on the mic bias value, this function returns the type of jack from the zones declared in the jack type

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`int micbias_voltage`
:   mic bias voltage at adc channel when jack is plugged in

**Description**

Based on the mic bias value passed, this function helps identify
the type of jack from the already declared jack zones

int snd\_soc\_jack\_add\_pins(struct snd\_soc\_jack \*jack, int count, struct snd\_soc\_jack\_pin \*pins)
:   Associate DAPM pins with an ASoC jack

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack created with `snd_soc_card_jack_new_pins()`

`int count`
:   Number of pins

`struct snd_soc_jack_pin *pins`
:   Array of pins

**Description**

After this function has been called the DAPM pins specified in the
pins array will have their status updated to reflect the current
state of the jack whenever the jack status is updated.

void snd\_soc\_jack\_notifier\_register(struct snd\_soc\_jack \*jack, struct notifier\_block \*nb)
:   Register a notifier for jack status

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`struct notifier_block *nb`
:   Notifier block to register

**Description**

Register for notification of the current status of the jack. Note
that it is not possible to report additional jack events in the
callback from the notifier, this is intended to support
applications such as enabling electrical detection only when a
mechanical detection event has occurred.

void snd\_soc\_jack\_notifier\_unregister(struct snd\_soc\_jack \*jack, struct notifier\_block \*nb)
:   Unregister a notifier for jack status

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`struct notifier_block *nb`
:   Notifier block to unregister

**Description**

Stop notifying for status changes.

int snd\_soc\_jack\_add\_gpios(struct snd\_soc\_jack \*jack, int count, struct snd\_soc\_jack\_gpio \*gpios)
:   Associate GPIO pins with an ASoC jack

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`int count`
:   number of pins

`struct snd_soc_jack_gpio *gpios`
:   array of gpio pins

**Description**

This function will request gpio, set data direction and request irq
for each gpio in the array.

int snd\_soc\_jack\_add\_gpiods(struct [device](../../driver-api/infrastructure.html#c.device "device") \*gpiod\_dev, struct snd\_soc\_jack \*jack, int count, struct snd\_soc\_jack\_gpio \*gpios)
:   Associate GPIO descriptor pins with an ASoC jack

**Parameters**

`struct device *gpiod_dev`
:   GPIO consumer device

`struct snd_soc_jack *jack`
:   ASoC jack

`int count`
:   number of pins

`struct snd_soc_jack_gpio *gpios`
:   array of gpio pins

**Description**

This function will request gpio, set data direction and request irq
for each gpio in the array.

void snd\_soc\_jack\_free\_gpios(struct snd\_soc\_jack \*jack, int count, struct snd\_soc\_jack\_gpio \*gpios)
:   Release GPIO pins’ resources of an ASoC jack

**Parameters**

`struct snd_soc_jack *jack`
:   ASoC jack

`int count`
:   number of pins

`struct snd_soc_jack_gpio *gpios`
:   array of gpio pins

**Description**

Release gpio and irq resources for gpio pins associated with an ASoC jack.

### ISA DMA Helpers

void snd\_dma\_program(unsigned long dma, unsigned long addr, unsigned int size, unsigned short mode)
:   program an ISA DMA transfer

**Parameters**

`unsigned long dma`
:   the dma number

`unsigned long addr`
:   the physical address of the buffer

`unsigned int size`
:   the DMA transfer size

`unsigned short mode`
:   the DMA transfer mode, DMA\_MODE\_XXX

**Description**

Programs an ISA DMA transfer for the given buffer.

void snd\_dma\_disable(unsigned long dma)
:   stop the ISA DMA transfer

**Parameters**

`unsigned long dma`
:   the dma number

**Description**

Stops the ISA DMA transfer.

unsigned int snd\_dma\_pointer(unsigned long dma, unsigned int size)
:   return the current pointer to DMA transfer buffer in bytes

**Parameters**

`unsigned long dma`
:   the dma number

`unsigned int size`
:   the dma transfer size

**Return**

The current pointer in DMA transfer buffer in bytes.

int snd\_devm\_request\_dma(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, int dma, const char \*name)
:   the managed version of [`request_dma()`](../../core-api/kernel-api.html#c.request_dma "request_dma")

**Parameters**

`struct device *dev`
:   the device pointer

`int dma`
:   the dma number

`const char *name`
:   the name string of the requester

**Description**

The requested DMA will be automatically released at unbinding via devres.

**Return**

zero on success, or a negative error code

### Other Helper Macros

void snd\_power\_ref(struct snd\_card \*card)
:   Take the reference count for power control

**Parameters**

`struct snd_card *card`
:   sound card object

**Description**

The power\_ref reference of the card is used for managing to block
the [`snd_power_sync_ref()`](#c.snd_power_sync_ref "snd_power_sync_ref") operation. This function increments the reference.
The counterpart [`snd_power_unref()`](#c.snd_power_unref "snd_power_unref") has to be called appropriately later.

void snd\_power\_unref(struct snd\_card \*card)
:   Release the reference count for power control

**Parameters**

`struct snd_card *card`
:   sound card object

void snd\_power\_sync\_ref(struct snd\_card \*card)
:   wait until the card power\_ref is freed

**Parameters**

`struct snd_card *card`
:   sound card object

**Description**

This function is used to synchronize with the pending power\_ref being
released.

void snd\_card\_unref(struct snd\_card \*card)
:   Unreference the card object

**Parameters**

`struct snd_card *card`
:   the card object to unreference

**Description**

Call this function for the card object that was obtained via [`snd_card_ref()`](#c.snd_card_ref "snd_card_ref")
or [`snd_lookup_minor_data()`](#c.snd_lookup_minor_data "snd_lookup_minor_data").

snd\_BUG

`snd_BUG ()`

> > give a BUG warning message and stack trace
>
> **Description**
>
> Calls `WARN()` if CONFIG\_SND\_DEBUG is set.
> Ignored when CONFIG\_SND\_DEBUG is not set.

snd\_BUG\_ON

`snd_BUG_ON (cond)`

> debugging check macro

**Parameters**

`cond`
:   condition to evaluate

**Description**

Has the same behavior as WARN\_ON when CONFIG\_SND\_DEBUG is set,
otherwise just evaluates the conditional and returns the value.

int register\_sound\_special\_device(const struct file\_operations \*fops, int unit, struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev)
:   register a special sound node

**Parameters**

`const struct file_operations *fops`
:   File operations for the driver

`int unit`
:   Unit number to allocate

`struct device *dev`
:   device pointer

**Description**

> Allocate a special sound device by minor number from the sound
> subsystem.

**Return**

The allocated number is returned on success. On failure,
a negative error code is returned.

int register\_sound\_mixer(const struct file\_operations \*fops, int dev)
:   register a mixer device

**Parameters**

`const struct file_operations *fops`
:   File operations for the driver

`int dev`
:   Unit number to allocate

**Description**

> Allocate a mixer device. Unit is the number of the mixer requested.
> Pass -1 to request the next free mixer unit.

**Return**

On success, the allocated number is returned. On failure,
a negative error code is returned.

int register\_sound\_dsp(const struct file\_operations \*fops, int dev)
:   register a DSP device

**Parameters**

`const struct file_operations *fops`
:   File operations for the driver

`int dev`
:   Unit number to allocate

**Description**

> Allocate a DSP device. Unit is the number of the DSP requested.
> Pass -1 to request the next free DSP unit.
>
> This function allocates both the audio and dsp device entries together
> and will always allocate them as a matching pair - eg dsp3/audio3

**Return**

On success, the allocated number is returned. On failure,
a negative error code is returned.

void unregister\_sound\_special(int unit)
:   unregister a special sound device

**Parameters**

`int unit`
:   unit number to allocate

**Description**

> Release a sound device that was allocated with
> `register_sound_special()`. The unit passed is the return value from
> the register function.

void unregister\_sound\_mixer(int unit)
:   unregister a mixer

**Parameters**

`int unit`
:   unit number to allocate

**Description**

> Release a sound device that was allocated with [`register_sound_mixer()`](#c.register_sound_mixer "register_sound_mixer").
> The unit passed is the return value from the register function.

void unregister\_sound\_dsp(int unit)
:   unregister a DSP device

**Parameters**

`int unit`
:   unit number to allocate

**Description**

> Release a sound device that was allocated with [`register_sound_dsp()`](#c.register_sound_dsp "register_sound_dsp").
> The unit passed is the return value from the register function.
>
> Both of the allocated units are released together automatically.
