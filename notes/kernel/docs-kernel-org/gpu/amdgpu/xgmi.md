# AMDGPU XGMI Support

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/xgmi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# AMDGPU XGMI Support

**AMDGPU XGMI Support**

XGMI is a high speed interconnect that joins multiple GPU cards
into a homogeneous memory space that is organized by a collective
hive ID and individual node IDs, both of which are 64-bit numbers.

The file xgmi\_device\_id contains the unique per GPU device ID and
is stored in the /sys/class/drm/card${cardno}/device/ directory.

Inside the device directory a sub-directory ‘xgmi\_hive\_info’ is
created which contains the hive ID and the list of nodes.

The hive ID is stored in:
:   /sys/class/drm/card${cardno}/device/xgmi\_hive\_info/xgmi\_hive\_id

The node information is stored in numbered directories:
:   /sys/class/drm/card${cardno}/device/xgmi\_hive\_info/node${nodeno}/xgmi\_device\_id

Each device has their own xgmi\_hive\_info direction with a mirror
set of node sub-directories.

The XGMI memory space is built by contiguously adding the power of
two padded VRAM space from each node to each other.
