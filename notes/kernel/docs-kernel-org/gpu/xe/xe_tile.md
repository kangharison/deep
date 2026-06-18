# Multi-tile Devices

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_tile.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Multi-tile Devices

Different vendors use the term “tile” a bit differently, but in the Intel
world, a ‘tile’ is pretty close to what most people would think of as being
a complete GPU. When multiple GPUs are placed behind a single PCI device,
that’s what is referred to as a “multi-tile device.” In such cases, pretty
much all hardware is replicated per-tile, although certain responsibilities
like PCI communication, reporting of interrupts to the OS, etc. are handled
solely by the “root tile.” A multi-tile platform takes care of tying the
tiles together in a way such that interrupt notifications from remote tiles
are forwarded to the root tile, the per-tile vram is combined into a single
address space, etc.

In contrast, a “GT” (which officially stands for “Graphics Technology”) is
the subset of a GPU/tile that is responsible for implementing graphics
and/or media operations. The GT is where a lot of the driver implementation
happens since it’s where the hardware engines, the execution units, and the
GuC all reside.

Historically most Intel devices were single-tile devices that contained a
single GT. PVC is an example of an Intel platform built on a multi-tile
design (i.e., multiple GPUs behind a single PCI device); each PVC tile only
has a single GT. In contrast, platforms like MTL that have separate chips
for render and media IP are still only a single logical GPU, but the
graphics and media IP blocks are each exposed as a separate GT within that
single GPU. This is important from a software perspective because multi-GT
platforms like MTL only replicate a subset of the GPU hardware and behave
differently than multi-tile platforms like PVC where nearly everything is
replicated.

Per-tile functionality (shared by all GTs within the tile):
:   * Complete 4MB MMIO space (containing SGunit/SoC registers, GT
      registers, display registers, etc.)
    * Global GTT
    * VRAM (if discrete)
    * Interrupt flows
    * Migration context
    * kernel batchbuffer pool
    * Primary GT
    * Media GT (if media version >= 13)

Per-GT functionality:
:   * GuC
    * Hardware engines
    * Programmable hardware units (subslices, EUs)
    * GSI subset of registers (multiple copies of these registers reside
      within the complete MMIO space provided by the tile, but at different
      offsets --- 0 for render, 0x380000 for media)
    * Multicast register steering
    * TLBs to cache page table translations
    * Reset capability
    * Low-level power management (e.g., C6)
    * Clock frequency
    * MOCS and PAT programming

## Internal API

int xe\_tile\_alloc(struct xe\_tile \*tile)
:   Perform per-tile memory allocation

**Parameters**

`struct xe_tile *tile`
:   Tile to perform allocations for

**Description**

Allocates various per-tile data structures using DRM-managed allocations.
Does not touch the hardware.

Returns -ENOMEM if allocations fail, otherwise 0.

int xe\_tile\_alloc\_vram(struct xe\_tile \*tile)
:   Perform per-tile VRAM structs allocation

**Parameters**

`struct xe_tile *tile`
:   Tile to perform allocations for

**Description**

Allocates VRAM per-tile data structures using DRM-managed allocations.
Does not touch the hardware.

Returns -ENOMEM if allocations fail, otherwise 0.

int xe\_tile\_init\_early(struct xe\_tile \*tile, struct xe\_device \*xe, u8 id)
:   Initialize the tile and primary GT

**Parameters**

`struct xe_tile *tile`
:   Tile to initialize

`struct xe_device *xe`
:   Parent Xe device

`u8 id`
:   Tile ID

**Description**

Initializes per-tile resources that don’t require any interactions with the
hardware or any knowledge about the Graphics/Media IP version.

**Return**

0 on success, negative error code on error.

int xe\_tile\_init\_noalloc(struct xe\_tile \*tile)
:   Init tile up to the point where allocations can happen.

**Parameters**

`struct xe_tile *tile`
:   The tile to initialize.

**Description**

This function prepares the tile to allow memory allocations to VRAM, but is
not allowed to allocate memory itself. This state is useful for display
readout, because the inherited display framebuffer will otherwise be
overwritten as it is usually put at the start of VRAM.

Note that since this is tile initialization, it should not perform any
GT-specific operations, and thus does not need to hold GT forcewake.

**Return**

0 on success, negative error code on error.

struct drm\_pagemap \*xe\_tile\_local\_pagemap(struct xe\_tile \*tile)
:   Return a pointer to the tile’s local drm\_pagemap if any

**Parameters**

`struct xe_tile *tile`
:   The tile.

**Return**

A pointer to the tile’s local drm\_pagemap, or NULL if local pagemap
support has been compiled out.
