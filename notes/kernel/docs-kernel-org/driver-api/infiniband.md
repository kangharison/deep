# InfiniBand and Remote DMA (RDMA) Interfaces

> 출처(원문): https://docs.kernel.org/driver-api/infiniband.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# InfiniBand and Remote DMA (RDMA) Interfaces

## Introduction and Overview

TBD

## InfiniBand core interfaces

struct iwpm\_nlmsg\_request \*iwpm\_get\_nlmsg\_request(\_\_u32 nlmsg\_seq, u8 nl\_client, gfp\_t gfp)
:   Allocate and initialize netlink message request

**Parameters**

`__u32 nlmsg_seq`
:   Sequence number of the netlink message

`u8 nl_client`
:   The index of the netlink client

`gfp_t gfp`
:   Indicates how the memory for the request should be allocated

**Description**

Returns the newly allocated netlink request object if successful,
otherwise returns NULL

void iwpm\_free\_nlmsg\_request(struct [kref](#c.iwpm_free_nlmsg_request "kref") \*kref)
:   Deallocate netlink message request

**Parameters**

`struct kref *kref`
:   Holds reference of netlink message request

struct iwpm\_nlmsg\_request \*iwpm\_find\_nlmsg\_request(\_\_u32 echo\_seq)
:   Find netlink message request in the request list

**Parameters**

`__u32 echo_seq`
:   Sequence number of the netlink request to find

**Description**

Returns the found netlink message request,
if not found, returns NULL

int iwpm\_wait\_complete\_req(struct iwpm\_nlmsg\_request \*nlmsg\_request)
:   Block while servicing the netlink request

**Parameters**

`struct iwpm_nlmsg_request *nlmsg_request`
:   Netlink message request to service

**Description**

Wakes up, after the request is completed or expired
Returns 0 if the request is complete without error

int iwpm\_get\_nlmsg\_seq(void)
:   Get the sequence number for a netlink message to send to the port mapper

**Parameters**

`void`
:   no arguments

**Description**

Returns the sequence number for the netlink message.

void iwpm\_add\_remote\_info(struct iwpm\_remote\_info \*reminfo)
:   Add remote address info of the connecting peer to the remote info hash table

**Parameters**

`struct iwpm_remote_info *reminfo`
:   The remote info to be added

u32 iwpm\_check\_registration(u8 nl\_client, u32 reg)
:   Check if the client registration matches the given one

**Parameters**

`u8 nl_client`
:   The index of the netlink client

`u32 reg`
:   The given registration type to compare with

**Description**

Call `iwpm_register_pid()` to register a client
Returns true if the client registration matches reg,
otherwise returns false

void iwpm\_set\_registration(u8 nl\_client, u32 reg)
:   Set the client registration

**Parameters**

`u8 nl_client`
:   The index of the netlink client

`u32 reg`
:   Registration type to set

u32 iwpm\_get\_registration(u8 nl\_client)
:   Get the client registration

**Parameters**

`u8 nl_client`
:   The index of the netlink client

**Description**

Returns the client registration type

int iwpm\_send\_mapinfo(u8 nl\_client, int iwpm\_pid)
:   Send local and mapped IPv4/IPv6 address info of a client to the user space port mapper

**Parameters**

`u8 nl_client`
:   The index of the netlink client

`int iwpm_pid`
:   The pid of the user space port mapper

**Description**

If successful, returns the number of sent mapping info records

int iwpm\_mapinfo\_available(void)
:   Check if any mapping info records is available in the hash table

**Parameters**

`void`
:   no arguments

**Description**

Returns 1 if mapping information is available, otherwise returns 0

int iwpm\_compare\_sockaddr(struct sockaddr\_storage \*a\_sockaddr, struct sockaddr\_storage \*b\_sockaddr)
:   Compare two sockaddr storage structs

**Parameters**

`struct sockaddr_storage *a_sockaddr`
:   first sockaddr to compare

`struct sockaddr_storage *b_sockaddr`
:   second sockaddr to compare

**Return**

0 if they are holding the same ip/tcp address info,
otherwise returns 1

int iwpm\_validate\_nlmsg\_attr(struct nlattr \*nltb[], int nla\_count)
:   Check for NULL netlink attributes

**Parameters**

`struct nlattr *nltb[]`
:   Holds address of each netlink message attributes

`int nla_count`
:   Number of netlink message attributes

**Description**

Returns error if any of the nla\_count attributes is NULL

struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*iwpm\_create\_nlmsg(u32 nl\_op, struct [nlmsghdr](../userspace-api/netlink/intro.html#c.nlmsghdr "nlmsghdr") \*\*nlh, int nl\_client)
:   Allocate skb and form a netlink message

**Parameters**

`u32 nl_op`
:   Netlink message opcode

`struct nlmsghdr **nlh`
:   Holds address of the netlink message header in skb

`int nl_client`
:   The index of the netlink client

**Description**

Returns the newly allcated skb, or NULL if the tailroom of the skb
is insufficient to store the message header and payload

int iwpm\_parse\_nlmsg(struct netlink\_callback \*cb, int policy\_max, const struct nla\_policy \*nlmsg\_policy, struct nlattr \*nltb[], const char \*msg\_type)
:   Validate and parse the received netlink message

**Parameters**

`struct netlink_callback *cb`
:   Netlink callback structure

`int policy_max`
:   Maximum attribute type to be expected

`const struct nla_policy *nlmsg_policy`
:   Validation policy

`struct nlattr *nltb[]`
:   Array to store policy\_max parsed elements

`const char *msg_type`
:   Type of netlink message

**Description**

Returns 0 on success or a negative error code

void iwpm\_print\_sockaddr(struct sockaddr\_storage \*sockaddr, char \*msg)
:   Print IPv4/IPv6 address and TCP port

**Parameters**

`struct sockaddr_storage *sockaddr`
:   Socket address to print

`char *msg`
:   Message to print

int iwpm\_send\_hello(u8 nl\_client, int iwpm\_pid, u16 abi\_version)
:   Send hello response to iwpmd

**Parameters**

`u8 nl_client`
:   The index of the netlink client

`int iwpm_pid`
:   The pid of the user space port mapper

`u16 abi_version`
:   The kernel’s abi\_version

**Description**

Returns 0 on success or a negative error code

int ib\_process\_cq\_direct(struct ib\_cq \*cq, int budget)
:   process a CQ in caller context

**Parameters**

`struct ib_cq *cq`
:   CQ to process

`int budget`
:   number of CQEs to poll for

**Description**

This function is used to process all outstanding CQ entries.
It does not offload CQ processing to a different context and does
not ask for completion interrupts from the HCA.
Using direct processing on CQ with non IB\_POLL\_DIRECT type may trigger
concurrent processing.

**Note**

do not pass -1 as `budget` unless it is guaranteed that the number
of completions that will be processed is small.

struct ib\_cq \*\_\_ib\_alloc\_cq(struct ib\_device \*dev, void \*private, int nr\_cqe, int comp\_vector, enum ib\_poll\_context poll\_ctx, const char \*caller)
:   allocate a completion queue

**Parameters**

`struct ib_device *dev`
:   device to allocate the CQ for

`void *private`
:   driver private data, accessible from cq->cq\_context

`int nr_cqe`
:   number of CQEs to allocate

`int comp_vector`
:   HCA completion vectors for this CQ

`enum ib_poll_context poll_ctx`
:   context to poll the CQ from.

`const char *caller`
:   module owner name.

**Description**

This is the proper interface to allocate a CQ for in-kernel users. A
CQ allocated with this interface will automatically be polled from the
specified context. The ULP must use wr->wr\_cqe instead of wr->wr\_id
to use this CQ abstraction.

struct ib\_cq \*\_\_ib\_alloc\_cq\_any(struct ib\_device \*dev, void \*private, int nr\_cqe, enum ib\_poll\_context poll\_ctx, const char \*caller)
:   allocate a completion queue

**Parameters**

`struct ib_device *dev`
:   device to allocate the CQ for

`void *private`
:   driver private data, accessible from cq->cq\_context

`int nr_cqe`
:   number of CQEs to allocate

`enum ib_poll_context poll_ctx`
:   context to poll the CQ from

`const char *caller`
:   module owner name

**Description**

Attempt to spread ULP Completion Queues over each device’s interrupt
vectors. A simple best-effort mechanism is used.

void ib\_free\_cq(struct ib\_cq \*cq)
:   free a completion queue

**Parameters**

`struct ib_cq *cq`
:   completion queue to free.

struct ib\_cq \*ib\_cq\_pool\_get(struct ib\_device \*dev, unsigned int nr\_cqe, int comp\_vector\_hint, enum ib\_poll\_context poll\_ctx)
:   Find the least used completion queue that matches a given cpu hint (or least used for wild card affinity) and fits nr\_cqe.

**Parameters**

`struct ib_device *dev`
:   rdma device

`unsigned int nr_cqe`
:   number of needed cqe entries

`int comp_vector_hint`
:   completion vector hint (-1) for the driver to assign
    a comp vector based on internal counter

`enum ib_poll_context poll_ctx`
:   cq polling context

**Description**

Finds a cq that satisfies **comp\_vector\_hint** and **nr\_cqe** requirements and
claim entries in it for us. In case there is no available cq, allocate
a new cq with the requirements and add it to the device pool.
IB\_POLL\_DIRECT cannot be used for shared cqs so it is not a valid value
for **poll\_ctx**.

void ib\_cq\_pool\_put(struct ib\_cq \*cq, unsigned int nr\_cqe)
:   Return a CQ taken from a shared pool.

**Parameters**

`struct ib_cq *cq`
:   The CQ to return.

`unsigned int nr_cqe`
:   The max number of cqes that the user had requested.

int ib\_cm\_listen(struct ib\_cm\_id \*cm\_id, \_\_be64 service\_id)
:   Initiates listening on the specified service ID for connection and service ID resolution requests.

**Parameters**

`struct ib_cm_id *cm_id`
:   Connection identifier associated with the listen request.

`__be64 service_id`
:   Service identifier matched against incoming connection
    and service ID resolution requests. The service ID should be specified
    network-byte order. If set to IB\_CM\_ASSIGN\_SERVICE\_ID, the CM will
    assign a service ID to the caller.

struct ib\_cm\_id \*ib\_cm\_insert\_listen(struct ib\_device \*device, ib\_cm\_handler cm\_handler, \_\_be64 service\_id)
:   Create a new listening ib\_cm\_id and listen on the given service ID.

**Parameters**

`struct ib_device *device`
:   Device associated with the cm\_id. All related communication will
    be associated with the specified device.

`ib_cm_handler cm_handler`
:   Callback invoked to notify the user of CM events.

`__be64 service_id`
:   Service identifier matched against incoming connection
    and service ID resolution requests. The service ID should be specified
    network-byte order. If set to IB\_CM\_ASSIGN\_SERVICE\_ID, the CM will
    assign a service ID to the caller.

**Description**

If there’s an existing ID listening on that same device and service ID,
return it.

Callers should call ib\_destroy\_cm\_id when done with the listener ID.

int rdma\_rw\_ctx\_init(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct scatterlist \*sg, u32 sg\_cnt, u32 sg\_offset, u64 remote\_addr, u32 rkey, enum dma\_data\_direction dir)
:   initialize a RDMA READ/WRITE context

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to initialize

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct scatterlist *sg`
:   scatterlist to READ/WRITE from/to

`u32 sg_cnt`
:   number of entries in **sg**

`u32 sg_offset`
:   current byte offset into **sg**

`u64 remote_addr`
:   remote address to read/write (relative to **rkey**)

`u32 rkey`
:   remote key to operate on

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

**Description**

Returns the number of WQEs that will be needed on the workqueue if
successful, or a negative error code.

int rdma\_rw\_ctx\_init\_bvec(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, const struct bio\_vec \*bvecs, u32 nr\_bvec, struct bvec\_iter iter, u64 remote\_addr, u32 rkey, enum dma\_data\_direction dir)
:   initialize a RDMA READ/WRITE context from bio\_vec

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to initialize

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`const struct bio_vec *bvecs`
:   bio\_vec array to READ/WRITE from/to

`u32 nr_bvec`
:   number of entries in **bvecs**

`struct bvec_iter iter`
:   bvec iterator describing offset and length

`u64 remote_addr`
:   remote address to read/write (relative to **rkey**)

`u32 rkey`
:   remote key to operate on

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

**Description**

Maps the bio\_vec array directly, avoiding intermediate scatterlist
conversion. Supports MR registration for iWARP devices and force\_mr mode.

Returns the number of WQEs that will be needed on the workqueue if
successful, or a negative error code:

> * `-EINVAL`
>   :   + **nr\_bvec** is zero or **iter.bi\_size** is zero
> * -ENOMEM - DMA mapping or memory allocation failed

int rdma\_rw\_ctx\_signature\_init(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct scatterlist \*sg, u32 sg\_cnt, struct scatterlist \*prot\_sg, u32 prot\_sg\_cnt, struct ib\_sig\_attrs \*sig\_attrs, u64 remote\_addr, u32 rkey, enum dma\_data\_direction dir)
:   initialize a RW context with signature offload

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to initialize

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct scatterlist *sg`
:   scatterlist to READ/WRITE from/to

`u32 sg_cnt`
:   number of entries in **sg**

`struct scatterlist *prot_sg`
:   scatterlist to READ/WRITE protection information from/to

`u32 prot_sg_cnt`
:   number of entries in **prot\_sg**

`struct ib_sig_attrs *sig_attrs`
:   signature offloading algorithms

`u64 remote_addr`
:   remote address to read/write (relative to **rkey**)

`u32 rkey`
:   remote key to operate on

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

**Description**

Returns the number of WQEs that will be needed on the workqueue if
successful, or a negative error code.

struct ib\_send\_wr \*rdma\_rw\_ctx\_wrs(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct ib\_cqe \*cqe, struct ib\_send\_wr \*chain\_wr)
:   return chain of WRs for a RDMA READ or WRITE operation

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to operate on

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct ib_cqe *cqe`
:   completion queue entry for the last WR

`struct ib_send_wr *chain_wr`
:   WR to append to the posted chain

**Description**

Return the WR chain for the set of RDMA READ/WRITE operations described by
**ctx**, as well as any memory registration operations needed. If **chain\_wr**
is non-NULL the WR it points to will be appended to the chain of WRs posted.
If **chain\_wr** is not set **cqe** must be set so that the caller gets a
completion notification.

int rdma\_rw\_ctx\_post(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct ib\_cqe \*cqe, struct ib\_send\_wr \*chain\_wr)
:   post a RDMA READ or RDMA WRITE operation

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to operate on

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct ib_cqe *cqe`
:   completion queue entry for the last WR

`struct ib_send_wr *chain_wr`
:   WR to append to the posted chain

**Description**

Post the set of RDMA READ/WRITE operations described by **ctx**, as well as
any memory registration operations needed. If **chain\_wr** is non-NULL the
WR it points to will be appended to the chain of WRs posted. If **chain\_wr**
is not set **cqe** must be set so that the caller gets a completion
notification.

void rdma\_rw\_ctx\_destroy(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct scatterlist \*sg, u32 sg\_cnt, enum dma\_data\_direction dir)
:   release all resources allocated by rdma\_rw\_ctx\_init

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to release

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct scatterlist *sg`
:   scatterlist that was used for the READ/WRITE

`u32 sg_cnt`
:   number of entries in **sg**

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

void rdma\_rw\_ctx\_destroy\_bvec(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 \_\_maybe\_unused port\_num, const struct bio\_vec \_\_maybe\_unused \*bvecs, u32 nr\_bvec, enum dma\_data\_direction dir)
:   release resources from rdma\_rw\_ctx\_init\_bvec

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to release

`struct ib_qp *qp`
:   queue pair to operate on

`u32 __maybe_unused port_num`
:   port num to which the connection is bound (unused)

`const struct bio_vec __maybe_unused *bvecs`
:   bio\_vec array that was used for the READ/WRITE (unused)

`u32 nr_bvec`
:   number of entries in **bvecs**

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

**Description**

Releases all resources allocated by a successful [`rdma_rw_ctx_init_bvec()`](#c.rdma_rw_ctx_init_bvec "rdma_rw_ctx_init_bvec")
call. Must not be called if [`rdma_rw_ctx_init_bvec()`](#c.rdma_rw_ctx_init_bvec "rdma_rw_ctx_init_bvec") returned an error.

The **port\_num** and **bvecs** parameters are unused but present for API
symmetry with [`rdma_rw_ctx_destroy()`](#c.rdma_rw_ctx_destroy "rdma_rw_ctx_destroy").

void rdma\_rw\_ctx\_destroy\_signature(struct rdma\_rw\_ctx \*ctx, struct ib\_qp \*qp, u32 port\_num, struct scatterlist \*sg, u32 sg\_cnt, struct scatterlist \*prot\_sg, u32 prot\_sg\_cnt, enum dma\_data\_direction dir)
:   release all resources allocated by rdma\_rw\_ctx\_signature\_init

**Parameters**

`struct rdma_rw_ctx *ctx`
:   context to release

`struct ib_qp *qp`
:   queue pair to operate on

`u32 port_num`
:   port num to which the connection is bound

`struct scatterlist *sg`
:   scatterlist that was used for the READ/WRITE

`u32 sg_cnt`
:   number of entries in **sg**

`struct scatterlist *prot_sg`
:   scatterlist that was used for the READ/WRITE of the PI

`u32 prot_sg_cnt`
:   number of entries in **prot\_sg**

`enum dma_data_direction dir`
:   `DMA_TO_DEVICE` for RDMA WRITE, `DMA_FROM_DEVICE` for RDMA READ

unsigned int rdma\_rw\_mr\_factor(struct ib\_device \*device, u32 port\_num, unsigned int maxpages)
:   return number of MRs required for a payload

**Parameters**

`struct ib_device *device`
:   device handling the connection

`u32 port_num`
:   port num to which the connection is bound

`unsigned int maxpages`
:   maximum payload pages per rdma\_rw\_ctx

**Description**

Returns the number of MRs the device requires to move **maxpayload**
bytes. The returned value is used during transport creation to
compute max\_rdma\_ctxts and the size of the transport’s Send and
Send Completion Queues.

unsigned int rdma\_rw\_max\_send\_wr(struct ib\_device \*dev, u32 port\_num, unsigned int max\_rdma\_ctxs, u32 create\_flags)
:   compute max Send WRs needed for RDMA R/W contexts

**Parameters**

`struct ib_device *dev`
:   RDMA device

`u32 port_num`
:   port number

`unsigned int max_rdma_ctxs`
:   number of rdma\_rw\_ctx structures

`u32 create_flags`
:   QP create flags (pass IB\_QP\_CREATE\_INTEGRITY\_EN if
    data integrity will be enabled on the QP)

**Description**

Returns the total number of Send Queue entries needed for
**max\_rdma\_ctxs**. The result accounts for memory registration and
invalidation work requests when the device requires them.

ULPs use this to size Send Queues and Send CQs before creating a
Queue Pair.

bool rdma\_dev\_access\_netns(const struct ib\_device \*dev, const struct [net](#c.rdma_dev_access_netns "net") \*net)
:   Return whether an rdma device can be accessed from a specified net namespace or not.

**Parameters**

`const struct ib_device *dev`
:   Pointer to rdma device which needs to be checked

`const struct net *net`
:   Pointer to net namesapce for which access to be checked

**Description**

When the rdma device is in shared mode, it ignores the net namespace.
When the rdma device is exclusive to a net namespace, rdma device net
namespace is checked against the specified one.

bool rdma\_dev\_has\_raw\_cap(const struct ib\_device \*dev)
:   Returns whether a specified rdma device has CAP\_NET\_RAW capability or not.

**Parameters**

`const struct ib_device *dev`
:   Pointer to rdma device whose capability to be checked

**Description**

Returns true if a rdma device’s owning user namespace has CAP\_NET\_RAW
capability, otherwise false. When rdma subsystem is in legacy shared network,
namespace mode, the default net namespace is considered.

void ib\_device\_put(struct ib\_device \*device)
:   Release IB device reference

**Parameters**

`struct ib_device *device`
:   device whose reference to be released

**Description**

[`ib_device_put()`](#c.ib_device_put "ib_device_put") releases reference to the IB device to allow it to be
unregistered and eventually free.

struct ib\_device \*\_ib\_alloc\_device(size\_t size, struct [net](#c._ib_alloc_device "net") \*net)
:   allocate an IB device struct

**Parameters**

`size_t size`
:   size of structure to allocate

`struct net *net`
:   network namespace device should be located in, namespace
    must stay valid until [`ib_register_device()`](#c.ib_register_device "ib_register_device") is completed.

**Description**

Low-level drivers should use `ib_alloc_device()` to allocate `struct
ib_device`. **size** is the size of the structure to be allocated,
including any private data used by the low-level driver.
[`ib_dealloc_device()`](#c.ib_dealloc_device "ib_dealloc_device") must be used to free structures allocated with
`ib_alloc_device()`.

void ib\_dealloc\_device(struct ib\_device \*device)
:   free an IB device struct

**Parameters**

`struct ib_device *device`
:   structure to free

**Description**

Free a structure allocated with `ib_alloc_device()`.

const struct ib\_port\_immutable \*ib\_port\_immutable\_read(struct ib\_device \*dev, unsigned int port)
:   Read rdma port’s immutable data

**Parameters**

`struct ib_device *dev`
:   IB device

`unsigned int port`
:   port number whose immutable data to read. It starts with index 1 and
    valid upto including `rdma_end_port()`.

int ib\_register\_device(struct ib\_device \*device, const char \*name, struct [device](#c.ib_register_device "device") \*dma\_device)
:   Register an IB device with IB core

**Parameters**

`struct ib_device *device`
:   Device to register

`const char *name`
:   unique string device name. This may include a ‘%’ which will
    cause a unique index to be added to the passed device name.

`struct device *dma_device`
:   pointer to a DMA-capable device. If `NULL`, then the IB
    device will be used. In this case the caller should fully
    setup the ibdev for DMA. This usually means using dma\_virt\_ops.

**Description**

Low-level drivers use [`ib_register_device()`](#c.ib_register_device "ib_register_device") to register their
devices with the IB core. All registered clients will receive a
callback for each device that is added. **device** must be allocated
with `ib_alloc_device()`.

If the driver uses ops.dealloc\_driver and calls any [`ib_unregister_device()`](#c.ib_unregister_device "ib_unregister_device")
asynchronously then the device pointer may become freed as soon as this
function returns.

void ib\_unregister\_device(struct ib\_device \*ib\_dev)
:   Unregister an IB device

**Parameters**

`struct ib_device *ib_dev`
:   The device to unregister

**Description**

Unregister an IB device. All clients will receive a remove callback.

Callers should call this routine only once, and protect against races with
registration. Typically it should only be called as part of a remove
callback in an implementation of driver core’s [`struct device_driver`](infrastructure.html#c.device_driver "device_driver") and
related.

If ops.dealloc\_driver is used then ib\_dev will be freed upon return from
this function.

void ib\_unregister\_device\_and\_put(struct ib\_device \*ib\_dev)
:   Unregister a device while holding a ‘get’

**Parameters**

`struct ib_device *ib_dev`
:   The device to unregister

**Description**

This is the same as [`ib_unregister_device()`](#c.ib_unregister_device "ib_unregister_device"), except it includes an internal
[`ib_device_put()`](#c.ib_device_put "ib_device_put") that should match a ‘get’ obtained by the caller.

It is safe to call this routine concurrently from multiple threads while
holding the ‘get’. When the function returns the device is fully
unregistered.

Drivers using this flow MUST use the driver\_unregister callback to clean up
their resources associated with the device and dealloc it.

void ib\_unregister\_driver(enum rdma\_driver\_id driver\_id)
:   Unregister all IB devices for a driver

**Parameters**

`enum rdma_driver_id driver_id`
:   The driver to unregister

**Description**

This implements a fence for device unregistration. It only returns once all
devices associated with the driver\_id have fully completed their
unregistration and returned from ib\_unregister\_device\*().

If device’s are not yet unregistered it goes ahead and starts unregistering
them.

This does not block creation of new devices with the given driver\_id, that
is the responsibility of the caller.

void ib\_unregister\_device\_queued(struct ib\_device \*ib\_dev)
:   Unregister a device using a work queue

**Parameters**

`struct ib_device *ib_dev`
:   The device to unregister

**Description**

This schedules an asynchronous unregistration using a WQ for the device. A
driver should use this to avoid holding locks while doing unregistration,
such as holding the RTNL lock.

Drivers using this API must use ib\_unregister\_driver before module unload
to ensure that all scheduled unregistrations have completed.

int ib\_register\_client(struct ib\_client \*client)
:   Register an IB client

**Parameters**

`struct ib_client *client`
:   Client to register

**Description**

Upper level users of the IB drivers can use [`ib_register_client()`](#c.ib_register_client "ib_register_client") to
register callbacks for IB device addition and removal. When an IB
device is added, each registered client’s add method will be called
(in the order the clients were registered), and when a device is
removed, each client’s remove method will be called (in the reverse
order that clients were registered). In addition, when
[`ib_register_client()`](#c.ib_register_client "ib_register_client") is called, the client will receive an add
callback for all devices already registered.

void ib\_unregister\_client(struct ib\_client \*client)
:   Unregister an IB client

**Parameters**

`struct ib_client *client`
:   Client to unregister

**Description**

Upper level users use [`ib_unregister_client()`](#c.ib_unregister_client "ib_unregister_client") to remove their client
registration. When [`ib_unregister_client()`](#c.ib_unregister_client "ib_unregister_client") is called, the client
will receive a remove callback for each IB device still registered.

This is a full fence, once it returns no client callbacks will be called,
or are running in another thread.

void ib\_set\_client\_data(struct ib\_device \*device, struct ib\_client \*client, void \*data)
:   Set IB client context

**Parameters**

`struct ib_device *device`
:   Device to set context for

`struct ib_client *client`
:   Client to set context for

`void *data`
:   Context to set

**Description**

[`ib_set_client_data()`](#c.ib_set_client_data "ib_set_client_data") sets client context data that can be retrieved with
`ib_get_client_data()`. This can only be called while the client is
registered to the device, once the ib\_client `remove()` callback returns this
cannot be called.

void ib\_register\_event\_handler(struct ib\_event\_handler \*event\_handler)
:   Register an IB event handler

**Parameters**

`struct ib_event_handler *event_handler`
:   Handler to register

**Description**

[`ib_register_event_handler()`](#c.ib_register_event_handler "ib_register_event_handler") registers an event handler that will be
called back when asynchronous IB events occur (as defined in
chapter 11 of the InfiniBand Architecture Specification). This
callback occurs in workqueue context.

void ib\_unregister\_event\_handler(struct ib\_event\_handler \*event\_handler)
:   Unregister an event handler

**Parameters**

`struct ib_event_handler *event_handler`
:   Handler to unregister

**Description**

Unregister an event handler registered with
[`ib_register_event_handler()`](#c.ib_register_event_handler "ib_register_event_handler").

int ib\_query\_port(struct ib\_device \*device, u32 port\_num, struct ib\_port\_attr \*port\_attr)
:   Query IB port attributes

**Parameters**

`struct ib_device *device`
:   Device to query

`u32 port_num`
:   Port number to query

`struct ib_port_attr *port_attr`
:   Port attributes

**Description**

[`ib_query_port()`](#c.ib_query_port "ib_query_port") returns the attributes of a port through the
**port\_attr** pointer.

int ib\_device\_set\_netdev(struct ib\_device \*ib\_dev, struct [net\_device](../networking/kapi.html#c.net_device "net_device") \*ndev, u32 port)
:   Associate the ib\_dev with an underlying net\_device

**Parameters**

`struct ib_device *ib_dev`
:   Device to modify

`struct net_device *ndev`
:   net\_device to affiliate, may be NULL

`u32 port`
:   IB port the net\_device is connected to

**Description**

Drivers should use this to link the ib\_device to a netdev so the netdev
shows up in interfaces like ib\_enum\_roce\_netdev. Only one netdev may be
affiliated with any port.

The caller must ensure that the given ndev is not unregistered or
unregistering, and that either the ib\_device is unregistered or
[`ib_device_set_netdev()`](#c.ib_device_set_netdev "ib_device_set_netdev") is called with NULL when the ndev sends a
NETDEV\_UNREGISTER event.

int ib\_query\_netdev\_port(struct ib\_device \*ibdev, struct [net\_device](../networking/kapi.html#c.net_device "net_device") \*ndev, u32 \*port)
:   Query the port number of a net\_device associated with an ibdev

**Parameters**

`struct ib_device *ibdev`
:   IB device

`struct net_device *ndev`
:   Network device

`u32 *port`
:   IB port the net\_device is connected to

struct ib\_device \*ib\_device\_get\_by\_netdev(struct [net\_device](../networking/kapi.html#c.net_device "net_device") \*ndev, enum rdma\_driver\_id driver\_id)
:   Find an IB device associated with a netdev

**Parameters**

`struct net_device *ndev`
:   netdev to locate

`enum rdma_driver_id driver_id`
:   The driver ID that must match (RDMA\_DRIVER\_UNKNOWN matches all)

**Description**

Find and hold an ib\_device that is associated with a netdev via
[`ib_device_set_netdev()`](#c.ib_device_set_netdev "ib_device_set_netdev"). The caller must call [`ib_device_put()`](#c.ib_device_put "ib_device_put") on the
returned pointer.

int ib\_query\_pkey(struct ib\_device \*device, u32 port\_num, u16 index, u16 \*pkey)
:   Get P\_Key table entry

**Parameters**

`struct ib_device *device`
:   Device to query

`u32 port_num`
:   Port number to query

`u16 index`
:   P\_Key table index to query

`u16 *pkey`
:   Returned P\_Key

**Description**

[`ib_query_pkey()`](#c.ib_query_pkey "ib_query_pkey") fetches the specified P\_Key table entry.

int ib\_modify\_device(struct ib\_device \*device, int device\_modify\_mask, struct ib\_device\_modify \*device\_modify)
:   Change IB device attributes

**Parameters**

`struct ib_device *device`
:   Device to modify

`int device_modify_mask`
:   Mask of attributes to change

`struct ib_device_modify *device_modify`
:   New attribute values

**Description**

[`ib_modify_device()`](#c.ib_modify_device "ib_modify_device") changes a device’s attributes as specified by
the **device\_modify\_mask** and **device\_modify** structure.

int ib\_modify\_port(struct ib\_device \*device, u32 port\_num, int port\_modify\_mask, struct ib\_port\_modify \*port\_modify)
:   Modifies the attributes for the specified port.

**Parameters**

`struct ib_device *device`
:   The device to modify.

`u32 port_num`
:   The number of the port to modify.

`int port_modify_mask`
:   Mask used to specify which attributes of the port
    to change.

`struct ib_port_modify *port_modify`
:   New attribute values for the port.

**Description**

[`ib_modify_port()`](#c.ib_modify_port "ib_modify_port") changes a port’s attributes as specified by the
**port\_modify\_mask** and **port\_modify** structure.

int ib\_find\_gid(struct ib\_device \*device, union ib\_gid \*gid, u32 \*port\_num, u16 \*index)
:   Returns the port number and GID table index where a specified GID value occurs. Its searches only for IB link layer.

**Parameters**

`struct ib_device *device`
:   The device to query.

`union ib_gid *gid`
:   The GID value to search for.

`u32 *port_num`
:   The port number of the device where the GID value was found.

`u16 *index`
:   The index into the GID table where the GID was found. This
    parameter may be NULL.

int ib\_find\_pkey(struct ib\_device \*device, u32 port\_num, u16 pkey, u16 \*index)
:   Returns the PKey table index where a specified PKey value occurs.

**Parameters**

`struct ib_device *device`
:   The device to query.

`u32 port_num`
:   The port number of the device to search for the PKey.

`u16 pkey`
:   The PKey value to search for.

`u16 *index`
:   The index into the PKey table where the PKey was found.

struct [net\_device](../networking/kapi.html#c.net_device "net_device") \*ib\_get\_net\_dev\_by\_params(struct ib\_device \*dev, u32 port, u16 pkey, const union ib\_gid \*gid, const struct sockaddr \*addr)
:   Return the appropriate net\_dev for a received CM request

**Parameters**

`struct ib_device *dev`
:   An RDMA device on which the request has been received.

`u32 port`
:   Port number on the RDMA device.

`u16 pkey`
:   The Pkey the request came on.

`const union ib_gid *gid`
:   A GID that the net\_dev uses to communicate.

`const struct sockaddr *addr`
:   Contains the IP address that the request specified as its
    destination.

struct ib\_pd \*\_\_ib\_alloc\_pd(struct ib\_device \*device, unsigned int flags, const char \*caller)
:   Allocates an unused protection domain.

**Parameters**

`struct ib_device *device`
:   The device on which to allocate the protection domain.

`unsigned int flags`
:   protection domain flags

`const char *caller`
:   caller’s build-time module name

**Description**

A protection domain object provides an association between QPs, shared
receive queues, address handles, memory regions, and memory windows.

Every PD has a local\_dma\_lkey which can be used as the lkey value for local
memory operations.

int ib\_dealloc\_pd\_user(struct ib\_pd \*pd, struct ib\_udata \*udata)
:   Deallocates a protection domain.

**Parameters**

`struct ib_pd *pd`
:   The protection domain to deallocate.

`struct ib_udata *udata`
:   Valid user data or NULL for kernel object

**Description**

It is an error to call this function while any resources in the pd still
exist. The caller is responsible to synchronously destroy them and
guarantee no new allocations will happen.

void rdma\_copy\_ah\_attr(struct rdma\_ah\_attr \*dest, const struct rdma\_ah\_attr \*src)
:   Copy rdma ah attribute from source to destination.

**Parameters**

`struct rdma_ah_attr *dest`
:   Pointer to destination ah\_attr. Contents of the destination
    pointer is assumed to be invalid and attribute are overwritten.

`const struct rdma_ah_attr *src`
:   Pointer to source ah\_attr.

void rdma\_replace\_ah\_attr(struct rdma\_ah\_attr \*old, const struct rdma\_ah\_attr \*new)
:   Replace valid ah\_attr with new one.

**Parameters**

`struct rdma_ah_attr *old`
:   Pointer to existing ah\_attr which needs to be replaced.
    old is assumed to be valid or zero’d

`const struct rdma_ah_attr *new`
:   Pointer to the new ah\_attr.

**Description**

[`rdma_replace_ah_attr()`](#c.rdma_replace_ah_attr "rdma_replace_ah_attr") first releases any reference in the old ah\_attr if
old the ah\_attr is valid; after that it copies the new attribute and holds
the reference to the replaced ah\_attr.

void rdma\_move\_ah\_attr(struct rdma\_ah\_attr \*dest, struct rdma\_ah\_attr \*src)
:   Move ah\_attr pointed by source to destination.

**Parameters**

`struct rdma_ah_attr *dest`
:   Pointer to destination ah\_attr to copy to.
    dest is assumed to be valid or zero’d

`struct rdma_ah_attr *src`
:   Pointer to the new ah\_attr.

**Description**

[`rdma_move_ah_attr()`](#c.rdma_move_ah_attr "rdma_move_ah_attr") first releases any reference in the destination ah\_attr
if it is valid. This also transfers ownership of internal references from
src to dest, making src invalid in the process. No new reference of the src
ah\_attr is taken.

struct ib\_ah \*rdma\_create\_ah(struct ib\_pd \*pd, struct rdma\_ah\_attr \*ah\_attr, u32 flags)
:   Creates an address handle for the given address vector.

**Parameters**

`struct ib_pd *pd`
:   The protection domain associated with the address handle.

`struct rdma_ah_attr *ah_attr`
:   The attributes of the address vector.

`u32 flags`
:   Create address handle flags (see `enum rdma_create_ah_flags`).

**Description**

It returns 0 on success and returns appropriate error code on error.
The address handle is used to reference a local or global destination
in all UD QP post sends.

struct ib\_ah \*rdma\_create\_user\_ah(struct ib\_pd \*pd, struct rdma\_ah\_attr \*ah\_attr, struct ib\_udata \*udata)
:   Creates an address handle for the given address vector. It resolves destination mac address for ah attribute of RoCE type.

**Parameters**

`struct ib_pd *pd`
:   The protection domain associated with the address handle.

`struct rdma_ah_attr *ah_attr`
:   The attributes of the address vector.

`struct ib_udata *udata`
:   pointer to user’s input output buffer information need by
    provider driver.

**Description**

It returns 0 on success and returns appropriate error code on error.
The address handle is used to reference a local or global destination
in all UD QP post sends.

void rdma\_move\_grh\_sgid\_attr(struct rdma\_ah\_attr \*attr, union ib\_gid \*dgid, u32 flow\_label, u8 hop\_limit, u8 traffic\_class, const struct ib\_gid\_attr \*sgid\_attr)
:   Sets the sgid attribute of GRH, taking ownership of the reference

**Parameters**

`struct rdma_ah_attr *attr`
:   Pointer to AH attribute structure

`union ib_gid *dgid`
:   Destination GID

`u32 flow_label`
:   Flow label

`u8 hop_limit`
:   Hop limit

`u8 traffic_class`
:   traffic class

`const struct ib_gid_attr *sgid_attr`
:   Pointer to SGID attribute

**Description**

This takes ownership of the sgid\_attr reference. The caller must ensure
[`rdma_destroy_ah_attr()`](#c.rdma_destroy_ah_attr "rdma_destroy_ah_attr") is called before destroying the rdma\_ah\_attr after
calling this function.

void rdma\_destroy\_ah\_attr(struct rdma\_ah\_attr \*ah\_attr)
:   Release reference to SGID attribute of ah attribute.

**Parameters**

`struct rdma_ah_attr *ah_attr`
:   Pointer to ah attribute

**Description**

Release reference to the SGID attribute of the ah attribute if it is
non NULL. It is safe to call this multiple times, and safe to call it on
a zero initialized ah\_attr.

struct ib\_srq \*ib\_create\_srq\_user(struct ib\_pd \*pd, struct ib\_srq\_init\_attr \*srq\_init\_attr, struct ib\_usrq\_object \*uobject, struct ib\_udata \*udata)
:   Creates a SRQ associated with the specified protection domain.

**Parameters**

`struct ib_pd *pd`
:   The protection domain associated with the SRQ.

`struct ib_srq_init_attr *srq_init_attr`
:   A list of initial attributes required to create the
    SRQ. If SRQ creation succeeds, then the attributes are updated to
    the actual capabilities of the created SRQ.

`struct ib_usrq_object *uobject`
:   uobject pointer if this is not a kernel SRQ

`struct ib_udata *udata`
:   udata pointer if this is not a kernel SRQ

**Description**

srq\_attr->max\_wr and srq\_attr->max\_sge are read the determine the
requested size of the SRQ, and set to the actual values allocated
on return. If `ib_create_srq()` succeeds, then max\_wr and max\_sge
will always be at least as large as the requested values.

struct ib\_qp \*ib\_create\_qp\_user(struct ib\_device \*dev, struct ib\_pd \*pd, struct ib\_qp\_init\_attr \*attr, struct ib\_udata \*udata, struct ib\_uqp\_object \*uobj, const char \*caller)
:   Creates a QP associated with the specified protection domain.

**Parameters**

`struct ib_device *dev`
:   IB device

`struct ib_pd *pd`
:   The protection domain associated with the QP.

`struct ib_qp_init_attr *attr`
:   A list of initial attributes required to create the
    QP. If QP creation succeeds, then the attributes are updated to
    the actual capabilities of the created QP.

`struct ib_udata *udata`
:   User data

`struct ib_uqp_object *uobj`
:   uverbs obect

`const char *caller`
:   caller’s build-time module name

int ib\_modify\_qp\_with\_udata(struct [ib\_qp](#c.ib_modify_qp_with_udata "ib_qp") \*ib\_qp, struct ib\_qp\_attr \*attr, int attr\_mask, struct ib\_udata \*udata)
:   Modifies the attributes for the specified QP.

**Parameters**

`struct ib_qp *ib_qp`
:   The QP to modify.

`struct ib_qp_attr *attr`
:   On input, specifies the QP attributes to modify. On output,
    the current values of selected QP attributes are returned.

`int attr_mask`
:   A bit-mask used to specify which attributes of the QP
    are being modified.

`struct ib_udata *udata`
:   pointer to user’s input output buffer information
    are being modified.
    It returns 0 on success and returns appropriate error code on error.

struct ib\_mr \*ib\_alloc\_mr(struct ib\_pd \*pd, enum ib\_mr\_type mr\_type, u32 max\_num\_sg)
:   Allocates a memory region

**Parameters**

`struct ib_pd *pd`
:   protection domain associated with the region

`enum ib_mr_type mr_type`
:   memory region type

`u32 max_num_sg`
:   maximum sg entries available for registration.

**Notes**

Memory registeration page/sg lists must not exceed max\_num\_sg.
For mr\_type IB\_MR\_TYPE\_MEM\_REG, the total length cannot exceed
max\_num\_sg \* used\_page\_size.

struct ib\_mr \*ib\_alloc\_mr\_integrity(struct ib\_pd \*pd, u32 max\_num\_data\_sg, u32 max\_num\_meta\_sg)
:   Allocates an integrity memory region

**Parameters**

`struct ib_pd *pd`
:   protection domain associated with the region

`u32 max_num_data_sg`
:   maximum data sg entries available for registration

`u32 max_num_meta_sg`
:   maximum metadata sg entries available for
    registration

**Notes**

Memory registration page/sg lists must not exceed max\_num\_sg,
also the integrity page/sg lists must not exceed max\_num\_meta\_sg.

struct ib\_xrcd \*ib\_alloc\_xrcd\_user(struct ib\_device \*device, struct [inode](#c.ib_alloc_xrcd_user "inode") \*inode, struct ib\_udata \*udata)
:   Allocates an XRC domain.

**Parameters**

`struct ib_device *device`
:   The device on which to allocate the XRC domain.

`struct inode *inode`
:   inode to connect XRCD

`struct ib_udata *udata`
:   Valid user data or NULL for kernel object

int ib\_dealloc\_xrcd\_user(struct ib\_xrcd \*xrcd, struct ib\_udata \*udata)
:   Deallocates an XRC domain.

**Parameters**

`struct ib_xrcd *xrcd`
:   The XRC domain to deallocate.

`struct ib_udata *udata`
:   Valid user data or NULL for kernel object

struct ib\_wq \*ib\_create\_wq(struct ib\_pd \*pd, struct ib\_wq\_init\_attr \*wq\_attr)
:   Creates a WQ associated with the specified protection domain.

**Parameters**

`struct ib_pd *pd`
:   The protection domain associated with the WQ.

`struct ib_wq_init_attr *wq_attr`
:   A list of initial attributes required to create the
    WQ. If WQ creation succeeds, then the attributes are updated to
    the actual capabilities of the created WQ.

**Description**

wq\_attr->max\_wr and wq\_attr->max\_sge determine
the requested size of the WQ, and set to the actual values allocated
on return.
If [`ib_create_wq()`](#c.ib_create_wq "ib_create_wq") succeeds, then max\_wr and max\_sge will always be
at least as large as the requested values.

int ib\_destroy\_wq\_user(struct ib\_wq \*wq, struct ib\_udata \*udata)
:   Destroys the specified user WQ.

**Parameters**

`struct ib_wq *wq`
:   The WQ to destroy.

`struct ib_udata *udata`
:   Valid user data

int ib\_map\_mr\_sg\_pi(struct ib\_mr \*mr, struct scatterlist \*data\_sg, int data\_sg\_nents, unsigned int \*data\_sg\_offset, struct scatterlist \*meta\_sg, int meta\_sg\_nents, unsigned int \*meta\_sg\_offset, unsigned int page\_size)
:   Map the dma mapped SG lists for PI (protection information) and set an appropriate memory region for registration.

**Parameters**

`struct ib_mr *mr`
:   memory region

`struct scatterlist *data_sg`
:   dma mapped scatterlist for data

`int data_sg_nents`
:   number of entries in data\_sg

`unsigned int *data_sg_offset`
:   offset in bytes into data\_sg

`struct scatterlist *meta_sg`
:   dma mapped scatterlist for metadata

`int meta_sg_nents`
:   number of entries in meta\_sg

`unsigned int *meta_sg_offset`
:   offset in bytes into meta\_sg

`unsigned int page_size`
:   page vector desired page size

**Description**

Constraints:
- The MR must be allocated with type IB\_MR\_TYPE\_INTEGRITY.

After this completes successfully, the memory region
is ready for registration.

**Return**

0 on success.

int ib\_map\_mr\_sg(struct ib\_mr \*mr, struct scatterlist \*sg, int sg\_nents, unsigned int \*sg\_offset, unsigned int page\_size)
:   Map the largest prefix of a dma mapped SG list and set it the memory region.

**Parameters**

`struct ib_mr *mr`
:   memory region

`struct scatterlist *sg`
:   dma mapped scatterlist

`int sg_nents`
:   number of entries in sg

`unsigned int *sg_offset`
:   offset in bytes into sg

`unsigned int page_size`
:   page vector desired page size

**Description**

Constraints:

* The first sg element is allowed to have an offset.
* Each sg element must either be aligned to page\_size or virtually
  contiguous to the previous element. In case an sg element has a
  non-contiguous offset, the mapping prefix will not include it.
* The last sg element is allowed to have length less than page\_size.
* If sg\_nents total byte length exceeds the mr max\_num\_sge \* page\_size
  then only max\_num\_sg entries will be mapped.
* If the MR was allocated with type IB\_MR\_TYPE\_SG\_GAPS, none of these
  constraints holds and the page\_size argument is ignored.

Returns the number of sg elements that were mapped to the memory region.

After this completes successfully, the memory region
is ready for registration.

int ib\_sg\_to\_pages(struct ib\_mr \*mr, struct scatterlist \*sgl, int sg\_nents, unsigned int \*sg\_offset\_p, int (\*set\_page)(struct ib\_mr\*, u64))
:   Convert the largest prefix of a sg list to a page vector

**Parameters**

`struct ib_mr *mr`
:   memory region

`struct scatterlist *sgl`
:   dma mapped scatterlist

`int sg_nents`
:   number of entries in sg

`unsigned int *sg_offset_p`
:   |  |  |
    | --- | --- |
    | IN | start offset in bytes into sg |
    | OUT | offset in bytes for element n of the sg of the first byte that has not been processed where n is the return value of this function. |

`int (*set_page)(struct ib_mr *, u64)`
:   driver page assignment function pointer

**Description**

Core service helper for drivers to convert the largest
prefix of given sg list to a page vector. The sg list
prefix converted is the prefix that meet the requirements
of ib\_map\_mr\_sg.

Returns the number of sg elements that were assigned to
a page vector.

void ib\_drain\_sq(struct ib\_qp \*qp)
:   Block until all SQ CQEs have been consumed by the application.

**Parameters**

`struct ib_qp *qp`
:   queue pair to drain

**Description**

If the device has a provider-specific drain function, then
call that. Otherwise call the generic drain function
`__ib_drain_sq()`.

The caller must:

ensure there is room in the CQ and SQ for the drain work request and
completion.

allocate the CQ using `ib_alloc_cq()`.

ensure that there are no other contexts that are posting WRs concurrently.
Otherwise the drain is not guaranteed.

void ib\_drain\_rq(struct ib\_qp \*qp)
:   Block until all RQ CQEs have been consumed by the application.

**Parameters**

`struct ib_qp *qp`
:   queue pair to drain

**Description**

If the device has a provider-specific drain function, then
call that. Otherwise call the generic drain function
`__ib_drain_rq()`.

The caller must:

ensure there is room in the CQ and RQ for the drain work request and
completion.

allocate the CQ using `ib_alloc_cq()`.

ensure that there are no other contexts that are posting WRs concurrently.
Otherwise the drain is not guaranteed.

void ib\_drain\_qp(struct ib\_qp \*qp)
:   Block until all CQEs have been consumed by the application on both the RQ and SQ.

**Parameters**

`struct ib_qp *qp`
:   queue pair to drain

**Description**

The caller must:

ensure there is room in the CQ(s), SQ, and RQ for drain work requests
and completions.

allocate the CQs using `ib_alloc_cq()`.

ensure that there are no other contexts that are posting WRs concurrently.
Otherwise the drain is not guaranteed.

struct rdma\_hw\_stats \*rdma\_alloc\_hw\_stats\_struct(const struct rdma\_stat\_desc \*descs, int num\_counters, unsigned long lifespan)
:   Helper function to allocate dynamic struct for the drivers.

**Parameters**

`const struct rdma_stat_desc *descs`
:   array of static descriptors

`int num_counters`
:   number of elements in array

`unsigned long lifespan`
:   milliseconds between updates

void rdma\_free\_hw\_stats\_struct(struct rdma\_hw\_stats \*stats)
:   Helper function to release rdma\_hw\_stats

**Parameters**

`struct rdma_hw_stats *stats`
:   statistics to release

void ib\_pack(const struct ib\_field \*desc, int desc\_len, void \*structure, void \*buf)
:   Pack a structure into a buffer

**Parameters**

`const struct ib_field *desc`
:   Array of structure field descriptions

`int desc_len`
:   Number of entries in **desc**

`void *structure`
:   Structure to pack from

`void *buf`
:   Buffer to pack into

**Description**

[`ib_pack()`](#c.ib_pack "ib_pack") packs a list of structure fields into a buffer,
controlled by the array of fields in **desc**.

void ib\_unpack(const struct ib\_field \*desc, int desc\_len, void \*buf, void \*structure)
:   Unpack a buffer into a structure

**Parameters**

`const struct ib_field *desc`
:   Array of structure field descriptions

`int desc_len`
:   Number of entries in **desc**

`void *buf`
:   Buffer to unpack from

`void *structure`
:   Structure to unpack into

**Description**

[`ib_pack()`](#c.ib_pack "ib_pack") unpacks a list of structure fields from a buffer,
controlled by the array of fields in **desc**.

void ib\_sa\_cancel\_query(int id, struct ib\_sa\_query \*query)
:   try to cancel an SA query

**Parameters**

`int id`
:   ID of query to cancel

`struct ib_sa_query *query`
:   query pointer to cancel

**Description**

Try to cancel an SA query. If the id and query don’t match up or
the query has already completed, nothing is done. Otherwise the
query is canceled and will complete with a status of -EINTR.

int ib\_init\_ah\_attr\_from\_path(struct ib\_device \*device, u32 port\_num, struct sa\_path\_rec \*rec, struct rdma\_ah\_attr \*ah\_attr, const struct ib\_gid\_attr \*gid\_attr)
:   Initialize address handle attributes based on an SA path record.

**Parameters**

`struct ib_device *device`
:   Device associated ah attributes initialization.

`u32 port_num`
:   Port on the specified device.

`struct sa_path_rec *rec`
:   path record entry to use for ah attributes initialization.

`struct rdma_ah_attr *ah_attr`
:   address handle attributes to initialization from path record.

`const struct ib_gid_attr *gid_attr`
:   SGID attribute to consider during initialization.

**Description**

When [`ib_init_ah_attr_from_path()`](#c.ib_init_ah_attr_from_path "ib_init_ah_attr_from_path") returns success,
(a) for IB link layer it optionally contains a reference to SGID attribute
when GRH is present for IB link layer.
(b) for RoCE link layer it contains a reference to SGID attribute.
User must invoke [`rdma_destroy_ah_attr()`](#c.rdma_destroy_ah_attr "rdma_destroy_ah_attr") to release reference to SGID
attributes which are initialized using [`ib_init_ah_attr_from_path()`](#c.ib_init_ah_attr_from_path "ib_init_ah_attr_from_path").

int ib\_sa\_path\_rec\_get(struct ib\_sa\_client \*client, struct ib\_device \*device, u32 port\_num, struct sa\_path\_rec \*rec, ib\_sa\_comp\_mask comp\_mask, unsigned long timeout\_ms, gfp\_t gfp\_mask, void (\*callback)(int status, struct sa\_path\_rec \*resp, unsigned int num\_paths, void \*context), void \*context, struct ib\_sa\_query \*\*sa\_query)
:   Start a Path get query

**Parameters**

`struct ib_sa_client *client`
:   SA client

`struct ib_device *device`
:   device to send query on

`u32 port_num`
:   port number to send query on

`struct sa_path_rec *rec`
:   Path Record to send in query

`ib_sa_comp_mask comp_mask`
:   component mask to send in query

`unsigned long timeout_ms`
:   time to wait for response

`gfp_t gfp_mask`
:   GFP mask to use for internal allocations

`void (*callback)(int status, struct sa_path_rec *resp, unsigned int num_paths, void *context)`
:   function called when query completes, times out or is
    canceled

`void *context`
:   opaque user context passed to callback

`struct ib_sa_query **sa_query`
:   query context, used to cancel query

**Description**

Send a Path Record Get query to the SA to look up a path. The
callback function will be called when the query completes (or
fails); status is 0 for a successful response, -EINTR if the query
is canceled, -ETIMEDOUT is the query timed out, or -EIO if an error
occurred sending the query. The resp parameter of the callback is
only valid if status is 0.

If the return value of [`ib_sa_path_rec_get()`](#c.ib_sa_path_rec_get "ib_sa_path_rec_get") is negative, it is an
error code. Otherwise it is a query ID that can be used to cancel
the query.

int ib\_sa\_service\_rec\_get(struct ib\_sa\_client \*client, struct ib\_device \*device, u32 port\_num, struct sa\_service\_rec \*rec, ib\_sa\_comp\_mask comp\_mask, unsigned long timeout\_ms, gfp\_t gfp\_mask, void (\*callback)(int status, struct sa\_service\_rec \*resp, unsigned int num\_services, void \*context), void \*context, struct ib\_sa\_query \*\*sa\_query)
:   Start a Service get query

**Parameters**

`struct ib_sa_client *client`
:   SA client

`struct ib_device *device`
:   device to send query on

`u32 port_num`
:   port number to send query on

`struct sa_service_rec *rec`
:   Service Record to send in query

`ib_sa_comp_mask comp_mask`
:   component mask to send in query

`unsigned long timeout_ms`
:   time to wait for response

`gfp_t gfp_mask`
:   GFP mask to use for internal allocations

`void (*callback)(int status, struct sa_service_rec *resp, unsigned int num_services, void *context)`
:   function called when query completes, times out or is
    canceled

`void *context`
:   opaque user context passed to callback

`struct ib_sa_query **sa_query`
:   query context, used to cancel query

**Description**

Send a Service Record Get query to the SA to look up a path. The
callback function will be called when the query completes (or
fails); status is 0 for a successful response, -EINTR if the query
is canceled, -ETIMEDOUT is the query timed out, or -EIO if an error
occurred sending the query. The resp parameter of the callback is
only valid if status is 0.

If the return value of [`ib_sa_service_rec_get()`](#c.ib_sa_service_rec_get "ib_sa_service_rec_get") is negative, it is an
error code. Otherwise it is a query ID that can be used to cancel
the query.

int ib\_ud\_header\_init(int payload\_bytes, int lrh\_present, int eth\_present, int vlan\_present, int grh\_present, int ip\_version, int udp\_present, int immediate\_present, struct ib\_ud\_header \*header)
:   Initialize UD header structure

**Parameters**

`int payload_bytes`
:   Length of packet payload

`int lrh_present`
:   specify if LRH is present

`int eth_present`
:   specify if Eth header is present

`int vlan_present`
:   packet is tagged vlan

`int grh_present`
:   GRH flag (if non-zero, GRH will be included)

`int ip_version`
:   if non-zero, IP header, V4 or V6, will be included

`int udp_present`
:   if non-zero, UDP header will be included

`int immediate_present`
:   specify if immediate data is present

`struct ib_ud_header *header`
:   Structure to initialize

int ib\_ud\_header\_pack(struct ib\_ud\_header \*header, void \*buf)
:   Pack UD header `struct into` wire format

**Parameters**

`struct ib_ud_header *header`
:   UD header struct

`void *buf`
:   Buffer to pack into

**Description**

[`ib_ud_header_pack()`](#c.ib_ud_header_pack "ib_ud_header_pack") packs the UD header structure **header** into wire
format in the buffer **buf**.

unsigned long ib\_umem\_find\_best\_pgsz(struct ib\_umem \*umem, unsigned long pgsz\_bitmap, unsigned long virt)
:   Find best HW page size to use for this MR

**Parameters**

`struct ib_umem *umem`
:   umem struct

`unsigned long pgsz_bitmap`
:   bitmap of HW supported page sizes

`unsigned long virt`
:   IOVA

**Description**

This helper is intended for HW that support multiple page
sizes but can do only a single page size in an MR.

Returns 0 if the umem requires page sizes not supported by
the driver to be mapped. Drivers always supporting PAGE\_SIZE
or smaller will never see a 0 result.

struct ib\_umem \*ib\_umem\_get(struct ib\_device \*device, unsigned long addr, size\_t size, int access)
:   Pin and DMA map userspace memory.

**Parameters**

`struct ib_device *device`
:   IB device to connect UMEM

`unsigned long addr`
:   userspace virtual address to start at

`size_t size`
:   length of region to pin

`int access`
:   IB\_ACCESS\_xxx flags for memory being pinned

void ib\_umem\_release(struct ib\_umem \*umem)
:   release memory pinned with ib\_umem\_get

**Parameters**

`struct ib_umem *umem`
:   umem `struct to` release

struct ib\_umem\_odp \*ib\_umem\_odp\_alloc\_implicit(struct ib\_device \*device, int access)
:   Allocate a parent implicit ODP umem

**Parameters**

`struct ib_device *device`
:   IB device to create UMEM

`int access`
:   ib\_reg\_mr access flags

**Description**

Implicit ODP umems do not have a VA range and do not have any page lists.
They exist only to hold the per\_mm reference to help the driver create
children umems.

struct ib\_umem\_odp \*ib\_umem\_odp\_alloc\_child(struct ib\_umem\_odp \*root, unsigned long addr, size\_t size, const struct mmu\_interval\_notifier\_ops \*ops)
:   Allocate a child ODP umem under an implicit parent ODP umem

**Parameters**

`struct ib_umem_odp *root`
:   The parent umem enclosing the child. This must be allocated using
    `ib_alloc_implicit_odp_umem()`

`unsigned long addr`
:   The starting userspace VA

`size_t size`
:   The length of the userspace VA

`const struct mmu_interval_notifier_ops *ops`
:   MMU interval ops, currently only **invalidate**

struct ib\_umem\_odp \*ib\_umem\_odp\_get(struct ib\_device \*device, unsigned long addr, size\_t size, int access, const struct mmu\_interval\_notifier\_ops \*ops)
:   Create a umem\_odp for a userspace va

**Parameters**

`struct ib_device *device`
:   IB device `struct to` get UMEM

`unsigned long addr`
:   userspace virtual address to start at

`size_t size`
:   length of region to pin

`int access`
:   IB\_ACCESS\_xxx flags for memory being pinned

`const struct mmu_interval_notifier_ops *ops`
:   MMU interval ops, currently only **invalidate**

**Description**

The driver should use when the access flags indicate ODP memory. It avoids
pinning, instead, stores the mm for future page fault handling in
conjunction with MMU notifiers.

int ib\_umem\_odp\_map\_dma\_and\_lock(struct ib\_umem\_odp \*umem\_odp, u64 user\_virt, u64 bcnt, u64 access\_mask, bool fault)
:   DMA map userspace memory in an ODP MR and lock it.

**Parameters**

`struct ib_umem_odp *umem_odp`
:   the umem to map and pin

`u64 user_virt`
:   the address from which we need to map.

`u64 bcnt`
:   the minimal number of bytes to pin and map. The mapping might be
    bigger due to alignment, and may also be smaller in case of an error
    pinning or mapping a page. The actual pages mapped is returned in
    the return value.

`u64 access_mask`
:   bit mask of the requested access permissions for the given
    range.

`bool fault`
:   is faulting required for the given range

**Description**

Maps the range passed in the argument to DMA addresses.
Upon success the ODP MR will be locked to let caller complete its device
page table update.

Returns the number of pages mapped in success, negative error code
for failure.

## RDMA Verbs transport library

int rvt\_fast\_reg\_mr(struct rvt\_qp \*qp, struct ib\_mr \*ibmr, u32 key, int access)
:   fast register physical MR

**Parameters**

`struct rvt_qp *qp`
:   the queue pair where the work request comes from

`struct ib_mr *ibmr`
:   the memory region to be registered

`u32 key`
:   updated key for this memory region

`int access`
:   access flags for this memory region

**Description**

Returns 0 on success.

int rvt\_invalidate\_rkey(struct rvt\_qp \*qp, u32 rkey)
:   invalidate an MR rkey

**Parameters**

`struct rvt_qp *qp`
:   queue pair associated with the invalidate op

`u32 rkey`
:   rkey to invalidate

**Description**

Returns 0 on success.

int rvt\_lkey\_ok(struct rvt\_lkey\_table \*rkt, struct rvt\_pd \*pd, struct rvt\_sge \*isge, struct rvt\_sge \*last\_sge, struct ib\_sge \*sge, int acc)
:   check IB SGE for validity and initialize

**Parameters**

`struct rvt_lkey_table *rkt`
:   table containing lkey to check SGE against

`struct rvt_pd *pd`
:   protection domain

`struct rvt_sge *isge`
:   outgoing internal SGE

`struct rvt_sge *last_sge`
:   last outgoing SGE written

`struct ib_sge *sge`
:   SGE to check

`int acc`
:   access flags

**Description**

Check the IB SGE for validity and initialize our internal version
of it.

Increments the reference count when a new sge is stored.

**Return**

0 if compressed, 1 if added , otherwise returns -errno.

int rvt\_rkey\_ok(struct rvt\_qp \*qp, struct rvt\_sge \*sge, u32 len, u64 vaddr, u32 rkey, int acc)
:   check the IB virtual address, length, and RKEY

**Parameters**

`struct rvt_qp *qp`
:   qp for validation

`struct rvt_sge *sge`
:   SGE state

`u32 len`
:   length of data

`u64 vaddr`
:   virtual address to place data

`u32 rkey`
:   rkey to check

`int acc`
:   access flags

**Return**

1 if successful, otherwise 0.

**Description**

increments the reference count upon success

\_\_be32 rvt\_compute\_aeth(struct rvt\_qp \*qp)
:   compute the AETH (syndrome + MSN)

**Parameters**

`struct rvt_qp *qp`
:   the queue pair to compute the AETH for

**Description**

Returns the AETH.

void rvt\_get\_credit(struct rvt\_qp \*qp, u32 aeth)
:   flush the send work queue of a QP

**Parameters**

`struct rvt_qp *qp`
:   the qp who’s send work queue to flush

`u32 aeth`
:   the Acknowledge Extended Transport Header

**Description**

The QP s\_lock should be held.

u32 rvt\_restart\_sge(struct rvt\_sge\_state \*ss, struct rvt\_swqe \*wqe, u32 len)
:   rewind the sge state for a wqe

**Parameters**

`struct rvt_sge_state *ss`
:   the sge state pointer

`struct rvt_swqe *wqe`
:   the wqe to rewind

`u32 len`
:   the data length from the start of the wqe in bytes

**Description**

Returns the remaining data length.

int rvt\_check\_ah(struct ib\_device \*ibdev, struct rdma\_ah\_attr \*ah\_attr)
:   validate the attributes of AH

**Parameters**

`struct ib_device *ibdev`
:   the ib device

`struct rdma_ah_attr *ah_attr`
:   the attributes of the AH

**Description**

If driver supports a more detailed check\_ah function call back to it
otherwise just check the basics.

**Return**

0 on success

struct rvt\_dev\_info \*rvt\_alloc\_device(size\_t size, int nports)
:   allocate rdi

**Parameters**

`size_t size`
:   how big of a structure to allocate

`int nports`
:   number of ports to allocate array slots for

**Description**

Use IB core device alloc to allocate space for the rdi which is assumed to be
inside of the ib\_device. Any extra space that drivers require should be
included in size.

We also allocate a port array based on the number of ports.

**Return**

pointer to allocated rdi

void rvt\_dealloc\_device(struct rvt\_dev\_info \*rdi)
:   deallocate rdi

**Parameters**

`struct rvt_dev_info *rdi`
:   structure to free

**Description**

Free a structure allocated with [`rvt_alloc_device()`](#c.rvt_alloc_device "rvt_alloc_device")

int rvt\_register\_device(struct rvt\_dev\_info \*rdi)
:   register a driver

**Parameters**

`struct rvt_dev_info *rdi`
:   main dev structure for all of rdmavt operations

**Description**

It is up to drivers to allocate the rdi and fill in the appropriate
information.

**Return**

0 on success otherwise an errno.

void rvt\_unregister\_device(struct rvt\_dev\_info \*rdi)
:   remove a driver

**Parameters**

`struct rvt_dev_info *rdi`
:   rvt dev struct

int rvt\_init\_port(struct rvt\_dev\_info \*rdi, struct rvt\_ibport \*port, int port\_index, u16 \*pkey\_table)
:   init internal data for driver port

**Parameters**

`struct rvt_dev_info *rdi`
:   rvt\_dev\_info struct

`struct rvt_ibport *port`
:   rvt port

`int port_index`
:   0 based index of ports, different from IB core port num

`u16 *pkey_table`
:   pkey\_table for **port**

**Description**

Keep track of a list of ports. No need to have a detach port.
They persist until the driver goes away.

**Return**

always 0

bool rvt\_cq\_enter(struct rvt\_cq \*cq, struct ib\_wc \*entry, bool solicited)
:   add a new entry to the completion queue

**Parameters**

`struct rvt_cq *cq`
:   completion queue

`struct ib_wc *entry`
:   work completion entry to add

`bool solicited`
:   true if **entry** is solicited

**Description**

This may be called with qp->s\_lock held.

**Return**

return true on success, else return
false if cq is full.

int rvt\_error\_qp(struct rvt\_qp \*qp, enum ib\_wc\_status err)
:   put a QP into the error state

**Parameters**

`struct rvt_qp *qp`
:   the QP to put into the error state

`enum ib_wc_status err`
:   the receive completion error to signal if a RWQE is active

**Description**

Flushes both send and receive work queues.

**Return**

true if last WQE event should be generated.
The QP r\_lock and s\_lock should be held and interrupts disabled.
If we are already in error state, just return.

int rvt\_get\_rwqe(struct rvt\_qp \*qp, bool wr\_id\_only)
:   copy the next RWQE into the QP’s RWQE

**Parameters**

`struct rvt_qp *qp`
:   the QP

`bool wr_id_only`
:   update qp->r\_wr\_id only, not qp->r\_sge

**Description**

Return -1 if there is a local error, 0 if no RWQE is available,
otherwise return 1.

Can be called from interrupt level.

void rvt\_comm\_est(struct rvt\_qp \*qp)
:   handle trap with QP established

**Parameters**

`struct rvt_qp *qp`
:   the QP

void rvt\_add\_rnr\_timer(struct rvt\_qp \*qp, u32 aeth)
:   add/start an rnr timer on the QP

**Parameters**

`struct rvt_qp *qp`
:   the QP

`u32 aeth`
:   aeth of RNR timeout, simulated aeth for loopback

void rvt\_stop\_rc\_timers(struct rvt\_qp \*qp)
:   stop all timers

**Parameters**

`struct rvt_qp *qp`
:   the QP
    stop any pending timers

void rvt\_del\_timers\_sync(struct rvt\_qp \*qp)
:   wait for any timeout routines to exit

**Parameters**

`struct rvt_qp *qp`
:   the QP

struct [rvt\_qp\_iter](#c.rvt_qp_iter "rvt_qp_iter") \*rvt\_qp\_iter\_init(struct rvt\_dev\_info \*rdi, u64 v, void (\*cb)(struct rvt\_qp \*qp, u64 v))
:   initial for QP iteration

**Parameters**

`struct rvt_dev_info *rdi`
:   rvt devinfo

`u64 v`
:   u64 value

`void (*cb)(struct rvt_qp *qp, u64 v)`
:   user-defined callback

**Description**

This returns an iterator suitable for iterating QPs
in the system.

The **cb** is a user-defined callback and **v** is a 64-bit
value passed to and relevant for processing in the
**cb**. An example use case would be to alter QP processing
based on criteria not part of the rvt\_qp.

Use cases that require memory allocation to succeed
must preallocate appropriately.

**Return**

a pointer to an rvt\_qp\_iter or NULL

int rvt\_qp\_iter\_next(struct [rvt\_qp\_iter](#c.rvt_qp_iter "rvt_qp_iter") \*iter)
:   return the next QP in iter

**Parameters**

`struct rvt_qp_iter *iter`
:   the iterator

**Description**

Fine grained QP iterator suitable for use
with debugfs seq\_file mechanisms.

Updates iter->qp with the current QP when the return
value is 0.

**Return**

0 - iter->qp is valid 1 - no more QPs

void rvt\_qp\_iter(struct rvt\_dev\_info \*rdi, u64 v, void (\*cb)(struct rvt\_qp \*qp, u64 v))
:   iterate all QPs

**Parameters**

`struct rvt_dev_info *rdi`
:   rvt devinfo

`u64 v`
:   a 64-bit value

`void (*cb)(struct rvt_qp *qp, u64 v)`
:   a callback

**Description**

This provides a way for iterating all QPs.

The **cb** is a user-defined callback and **v** is a 64-bit
value passed to and relevant for processing in the
cb. An example use case would be to alter QP processing
based on criteria not part of the rvt\_qp.

The code has an internal iterator to simplify
non seq\_file use cases.

void rvt\_copy\_sge(struct rvt\_qp \*qp, struct rvt\_sge\_state \*ss, void \*data, u32 length, bool release, bool copy\_last)
:   copy data to SGE memory

**Parameters**

`struct rvt_qp *qp`
:   associated QP

`struct rvt_sge_state *ss`
:   the SGE state

`void *data`
:   the data to copy

`u32 length`
:   the length of the data

`bool release`
:   boolean to release MR

`bool copy_last`
:   do a separate copy of the last 8 bytes

void rvt\_ruc\_loopback(struct rvt\_qp \*sqp)
:   handle UC and RC loopback requests

**Parameters**

`struct rvt_qp *sqp`
:   the sending QP

**Description**

This is called from `rvt_do_send()` to forward a WQE addressed to the same HFI
Note that although we are single threaded due to the send engine, we still
have to protect against `post_send()`. We don’t have to worry about
receive interrupts since this is a connected protocol and all packets
will pass through here.

struct rvt\_mcast \*rvt\_mcast\_find(struct rvt\_ibport \*ibp, union ib\_gid \*mgid, u16 lid)
:   search the global table for the given multicast GID/LID

**Parameters**

`struct rvt_ibport *ibp`
:   the IB port structure

`union ib_gid *mgid`
:   the multicast GID to search for

`u16 lid`
:   the multicast LID portion of the multicast address (host order)

**NOTE**

It is valid to have 1 MLID with multiple MGIDs. It is not valid
to have 1 MGID with multiple MLIDs.

**Description**

The caller is responsible for decrementing the reference count if found.

**Return**

NULL if not found.

## Upper Layer Protocols

### iSCSI Extensions for RDMA (iSER)

struct iser\_data\_buf
:   iSER data buffer

**Definition**:

```
struct iser_data_buf {
    struct scatterlist *sg;
    int size;
    unsigned long      data_len;
    int dma_nents;
};
```

**Members**

`sg`
:   pointer to the sg list

`size`
:   num entries of this sg

`data_len`
:   total buffer byte len

`dma_nents`
:   returned by dma\_map\_sg

struct iser\_mem\_reg
:   iSER memory registration info

**Definition**:

```
struct iser_mem_reg {
    struct ib_sge sge;
    u32 rkey;
    struct iser_fr_desc *desc;
};
```

**Members**

`sge`
:   memory region sg element

`rkey`
:   memory region remote key

`desc`
:   pointer to fast registration context

struct iser\_tx\_desc
:   iSER TX descriptor

**Definition**:

```
struct iser_tx_desc {
    struct iser_ctrl             iser_header;
    struct iscsi_hdr             iscsi_header;
    enum iser_desc_type        type;
    u64 dma_addr;
    struct ib_sge                tx_sg[2];
    int num_sge;
    struct ib_cqe                cqe;
    bool mapped;
    struct ib_reg_wr             reg_wr;
    struct ib_send_wr            send_wr;
    struct ib_send_wr            inv_wr;
};
```

**Members**

`iser_header`
:   iser header

`iscsi_header`
:   iscsi header

`type`
:   command/control/dataout

`dma_addr`
:   header buffer dma\_address

`tx_sg`
:   sg[0] points to iser/iscsi headers
    sg[1] optionally points to either of immediate data
    unsolicited data-out or control

`num_sge`
:   number sges used on this TX task

`cqe`
:   completion handler

`mapped`
:   Is the task header mapped

`reg_wr`
:   registration WR

`send_wr`
:   send WR

`inv_wr`
:   invalidate WR

struct iser\_rx\_desc
:   iSER RX descriptor

**Definition**:

```
struct iser_rx_desc {
    struct iser_ctrl             iser_header;
    struct iscsi_hdr             iscsi_header;
    char data[ISER_RECV_DATA_SEG_LEN];
    u64 dma_addr;
    struct ib_sge                rx_sg;
    struct ib_cqe                cqe;
    char pad[ISER_RX_PAD_SIZE];
};
```

**Members**

`iser_header`
:   iser header

`iscsi_header`
:   iscsi header

`data`
:   received data segment

`dma_addr`
:   receive buffer dma address

`rx_sg`
:   ib\_sge of receive buffer

`cqe`
:   completion handler

`pad`
:   for sense data TODO: Modify to maximum sense length supported

struct iser\_login\_desc
:   iSER login descriptor

**Definition**:

```
struct iser_login_desc {
    void *req;
    void *rsp;
    u64 req_dma;
    u64 rsp_dma;
    struct ib_sge                sge;
    struct ib_cqe                cqe;
};
```

**Members**

`req`
:   pointer to login request buffer

`rsp`
:   pointer to login response buffer

`req_dma`
:   DMA address of login request buffer

`rsp_dma`
:   DMA address of login response buffer

`sge`
:   IB sge for login post recv

`cqe`
:   completion handler

struct iser\_device
:   iSER device handle

**Definition**:

```
struct iser_device {
    struct ib_device             *ib_device;
    struct ib_pd                 *pd;
    struct ib_event_handler      event_handler;
    struct list_head             ig_list;
    int refcount;
};
```

**Members**

`ib_device`
:   RDMA device

`pd`
:   Protection Domain for this device

`event_handler`
:   IB events handle routine

`ig_list`
:   entry in devices list

`refcount`
:   Reference counter, dominated by open iser connections

struct iser\_reg\_resources
:   Fast registration resources

**Definition**:

```
struct iser_reg_resources {
    struct ib_mr                     *mr;
    struct ib_mr                     *sig_mr;
};
```

**Members**

`mr`
:   memory region

`sig_mr`
:   signature memory region

struct iser\_fr\_desc
:   Fast registration descriptor

**Definition**:

```
struct iser_fr_desc {
    struct list_head                  list;
    struct iser_reg_resources         rsc;
    bool sig_protected;
    struct list_head                  all_list;
};
```

**Members**

`list`
:   entry in connection fastreg pool

`rsc`
:   data buffer registration resources

`sig_protected`
:   is region protected indicator

`all_list`
:   first and last list members

struct iser\_fr\_pool
:   connection fast registration pool

**Definition**:

```
struct iser_fr_pool {
    struct list_head        list;
    spinlock_t lock;
    int size;
    struct list_head        all_list;
};
```

**Members**

`list`
:   list of fastreg descriptors

`lock`
:   protects fastreg pool

`size`
:   size of the pool

`all_list`
:   first and last list members

struct ib\_conn
:   Infiniband related objects

**Definition**:

```
struct ib_conn {
    struct rdma_cm_id           *cma_id;
    struct ib_qp                *qp;
    struct ib_cq                *cq;
    u32 cq_size;
    struct iser_device          *device;
    struct iser_fr_pool          fr_pool;
    bool pi_support;
    struct ib_cqe                reg_cqe;
};
```

**Members**

`cma_id`
:   rdma\_cm connection maneger handle

`qp`
:   Connection Queue-pair

`cq`
:   Connection completion queue

`cq_size`
:   The number of max outstanding completions

`device`
:   reference to iser device

`fr_pool`
:   connection fast registration pool

`pi_support`
:   Indicate device T10-PI support

`reg_cqe`
:   completion handler

struct iser\_conn
:   iSER connection context

**Definition**:

```
struct iser_conn {
    struct ib_conn               ib_conn;
    struct iscsi_conn            *iscsi_conn;
    struct iscsi_endpoint        *ep;
    enum iser_conn_state         state;
    unsigned qp_max_recv_dtos;
    u16 max_cmds;
    char name[ISER_OBJECT_NAME_SIZE];
    struct work_struct           release_work;
    struct mutex                 state_mutex;
    struct completion            stop_completion;
    struct completion            ib_completion;
    struct completion            up_completion;
    struct list_head             conn_list;
    struct iser_login_desc       login_desc;
    struct iser_rx_desc          *rx_descs;
    u32 num_rx_descs;
    unsigned short               scsi_sg_tablesize;
    unsigned short               pages_per_mr;
    bool snd_w_inv;
};
```

**Members**

`ib_conn`
:   connection RDMA resources

`iscsi_conn`
:   link to matching iscsi connection

`ep`
:   transport handle

`state`
:   connection logical state

`qp_max_recv_dtos`
:   maximum number of data outs, corresponds
    to max number of post recvs

`max_cmds`
:   maximum cmds allowed for this connection

`name`
:   connection peer portal

`release_work`
:   deferred work for release job

`state_mutex`
:   protects iser onnection state

`stop_completion`
:   conn\_stop completion

`ib_completion`
:   RDMA cleanup completion

`up_completion`
:   connection establishment completed
    (state is ISER\_CONN\_UP)

`conn_list`
:   entry in ig conn list

`login_desc`
:   login descriptor

`rx_descs`
:   rx buffers array (cyclic buffer)

`num_rx_descs`
:   number of rx descriptors

`scsi_sg_tablesize`
:   scsi host sg\_tablesize

`pages_per_mr`
:   maximum pages available for registration

`snd_w_inv`
:   connection uses remote invalidation

struct iscsi\_iser\_task
:   iser task context

**Definition**:

```
struct iscsi_iser_task {
    struct iser_tx_desc          desc;
    struct iser_conn             *iser_conn;
    enum iser_task_status        status;
    struct scsi_cmnd             *sc;
    int command_sent;
    int dir[ISER_DIRS_NUM];
    struct iser_mem_reg          rdma_reg[ISER_DIRS_NUM];
    struct iser_data_buf         data[ISER_DIRS_NUM];
    struct iser_data_buf         prot[ISER_DIRS_NUM];
};
```

**Members**

`desc`
:   TX descriptor

`iser_conn`
:   link to iser connection

`status`
:   current task status

`sc`
:   link to scsi command

`command_sent`
:   indicate if command was sent

`dir`
:   iser data direction

`rdma_reg`
:   task rdma registration desc

`data`
:   iser data buffer desc

`prot`
:   iser protection buffer desc

struct iser\_global
:   iSER global context

**Definition**:

```
struct iser_global {
    struct mutex      device_list_mutex;
    struct list_head  device_list;
    struct mutex      connlist_mutex;
    struct list_head  connlist;
    struct kmem_cache *desc_cache;
};
```

**Members**

`device_list_mutex`
:   protects device\_list

`device_list`
:   iser devices global list

`connlist_mutex`
:   protects connlist

`connlist`
:   iser connections global list

`desc_cache`
:   kmem cache for tx dataout

int iscsi\_iser\_pdu\_alloc(struct iscsi\_task \*task, uint8\_t opcode)
:   allocate an iscsi-iser PDU

**Parameters**

`struct iscsi_task *task`
:   iscsi task

`uint8_t opcode`
:   iscsi command opcode

**Description**

Netes: This routine can’t fail, just assign iscsi task
:   hdr and max hdr size.

int iser\_initialize\_task\_headers(struct iscsi\_task \*task, struct [iser\_tx\_desc](#c.iser_tx_desc "iser_tx_desc") \*tx\_desc)
:   Initialize task headers

**Parameters**

`struct iscsi_task *task`
:   iscsi task

`struct iser_tx_desc *tx_desc`
:   iser tx descriptor

**Notes**

This routine may race with iser teardown flow for scsi
error handling TMFs. So for TMF we should acquire the
state mutex to avoid dereferencing the IB device which
may have already been terminated.

int iscsi\_iser\_task\_init(struct iscsi\_task \*task)
:   Initialize iscsi-iser task

**Parameters**

`struct iscsi_task *task`
:   iscsi task

**Description**

Initialize the task for the scsi command or mgmt command.

**Return**

Returns zero on success or -ENOMEM when failing
to init task headers (dma mapping error).

int iscsi\_iser\_mtask\_xmit(struct iscsi\_conn \*conn, struct iscsi\_task \*task)
:   xmit management (immediate) task

**Parameters**

`struct iscsi_conn *conn`
:   iscsi connection

`struct iscsi_task *task`
:   task management task

**Notes**

> The function can return -EAGAIN in which case caller must
> call it again later, or recover. ‘0’ return code means successful
> xmit.

int iscsi\_iser\_task\_xmit(struct iscsi\_task \*task)
:   xmit iscsi-iser task

**Parameters**

`struct iscsi_task *task`
:   iscsi task

**Return**

zero on success or escalates $error on failure.

void iscsi\_iser\_cleanup\_task(struct iscsi\_task \*task)
:   cleanup an iscsi-iser task

**Parameters**

`struct iscsi_task *task`
:   iscsi task

**Notes**

In case the RDMA device is already NULL (might have
:   been removed in DEVICE\_REMOVAL CM event it will bail-out
    without doing dma unmapping.

u8 iscsi\_iser\_check\_protection(struct iscsi\_task \*task, sector\_t \*sector)
:   check protection information status of task.

**Parameters**

`struct iscsi_task *task`
:   iscsi task

`sector_t *sector`
:   error sector if exsists (output)

**Return**

zero if no data-integrity errors have occurred
0x1: data-integrity error occurred in the guard-block
0x2: data-integrity error occurred in the reference tag
0x3: data-integrity error occurred in the application tag

**Description**

> In addition the error sector is marked.

struct iscsi\_cls\_conn \*iscsi\_iser\_conn\_create(struct iscsi\_cls\_session \*cls\_session, uint32\_t conn\_idx)
:   create a new iscsi-iser connection

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi class connection

`uint32_t conn_idx`
:   connection index within the session (for MCS)

**Return**

iscsi\_cls\_conn when iscsi\_conn\_setup succeeds or NULL
otherwise.

int iscsi\_iser\_conn\_bind(struct iscsi\_cls\_session \*cls\_session, struct iscsi\_cls\_conn \*cls\_conn, uint64\_t transport\_eph, int is\_leading)
:   bind iscsi and iser connection structures

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi class session

`struct iscsi_cls_conn *cls_conn`
:   iscsi class connection

`uint64_t transport_eph`
:   transport end-point handle

`int is_leading`
:   indicate if this is the session leading connection (MCS)

**Return**

zero on success, $error if iscsi\_conn\_bind fails and
-EINVAL in case end-point doesn’t exists anymore or iser connection
state is not UP (teardown already started).

int iscsi\_iser\_conn\_start(struct iscsi\_cls\_conn \*cls\_conn)
:   start iscsi-iser connection

**Parameters**

`struct iscsi_cls_conn *cls_conn`
:   iscsi class connection

**Notes**

Here iser intialize (or re-initialize) stop\_completion as
:   from this point iscsi must call conn\_stop in session/connection
    teardown so iser transport must wait for it.

void iscsi\_iser\_conn\_stop(struct iscsi\_cls\_conn \*cls\_conn, int flag)
:   stop iscsi-iser connection

**Parameters**

`struct iscsi_cls_conn *cls_conn`
:   iscsi class connection

`int flag`
:   indicate if recover or terminate (passed as is)

**Notes**

Calling iscsi\_conn\_stop might theoretically race with
:   DEVICE\_REMOVAL event and dereference a previously freed RDMA device
    handle, so we call it under iser the state lock to protect against
    this kind of race.

void iscsi\_iser\_session\_destroy(struct iscsi\_cls\_session \*cls\_session)
:   destroy iscsi-iser session

**Parameters**

`struct iscsi_cls_session *cls_session`
:   iscsi class session

**Description**

Removes and free iscsi host.

struct iscsi\_cls\_session \*iscsi\_iser\_session\_create(struct iscsi\_endpoint \*ep, uint16\_t cmds\_max, uint16\_t qdepth, uint32\_t initial\_cmdsn)
:   create an iscsi-iser session

**Parameters**

`struct iscsi_endpoint *ep`
:   iscsi end-point handle

`uint16_t cmds_max`
:   maximum commands in this session

`uint16_t qdepth`
:   session command queue depth

`uint32_t initial_cmdsn`
:   initiator command sequnce number

**Description**

Allocates and adds a scsi host, expose DIF supprot if
exists, and sets up an iscsi session.

struct iscsi\_endpoint \*iscsi\_iser\_ep\_connect(struct Scsi\_Host \*shost, struct sockaddr \*dst\_addr, int non\_blocking)
:   Initiate iSER connection establishment

**Parameters**

`struct Scsi_Host *shost`
:   scsi\_host

`struct sockaddr *dst_addr`
:   destination address

`int non_blocking`
:   indicate if routine can block

**Description**

Allocate an iscsi endpoint, an iser\_conn structure and bind them.
After that start RDMA connection establishment via rdma\_cm. We
don’t allocate iser\_conn embedded in iscsi\_endpoint since in teardown
the endpoint will be destroyed at ep\_disconnect while iser\_conn will
cleanup its resources asynchronuously.

**Return**

iscsi\_endpoint created by iscsi layer or ERR\_PTR(error)
if fails.

int iscsi\_iser\_ep\_poll(struct iscsi\_endpoint \*ep, int timeout\_ms)
:   poll for iser connection establishment to complete

**Parameters**

`struct iscsi_endpoint *ep`
:   iscsi endpoint (created at ep\_connect)

`int timeout_ms`
:   polling timeout allowed in ms.

**Description**

This routine boils down to waiting for up\_completion signaling
that cma\_id got CONNECTED event.

**Return**

1 if succeeded in connection establishment, 0 if timeout expired
(libiscsi will retry will kick in) or -1 if interrupted by signal
or more likely iser connection state transitioned to TEMINATING or
DOWN during the wait period.

void iscsi\_iser\_ep\_disconnect(struct iscsi\_endpoint \*ep)
:   Initiate connection teardown process

**Parameters**

`struct iscsi_endpoint *ep`
:   iscsi endpoint handle

**Description**

This routine is not blocked by iser and RDMA termination process
completion as we queue a deffered work for iser/RDMA destruction
and cleanup or actually call it immediately in case we didn’t pass
iscsi conn bind/start stage, thus it is safe.

int iser\_send\_command(struct iscsi\_conn \*conn, struct iscsi\_task \*task)
:   send command PDU

**Parameters**

`struct iscsi_conn *conn`
:   link to matching iscsi connection

`struct iscsi_task *task`
:   SCSI command task

int iser\_send\_data\_out(struct iscsi\_conn \*conn, struct iscsi\_task \*task, struct iscsi\_data \*hdr)
:   send data out PDU

**Parameters**

`struct iscsi_conn *conn`
:   link to matching iscsi connection

`struct iscsi_task *task`
:   SCSI command task

`struct iscsi_data *hdr`
:   pointer to the LLD’s iSCSI message header

int iser\_alloc\_fastreg\_pool(struct [ib\_conn](#c.iser_alloc_fastreg_pool "ib_conn") \*ib\_conn, unsigned cmds\_max, unsigned int size)
:   Creates pool of fast\_reg descriptors for fast registration work requests.

**Parameters**

`struct ib_conn *ib_conn`
:   connection RDMA resources

`unsigned cmds_max`
:   max number of SCSI commands for this connection

`unsigned int size`
:   max number of pages per map request

**Return**

0 on success, or errno code on failure

void iser\_free\_fastreg\_pool(struct [ib\_conn](#c.iser_free_fastreg_pool "ib_conn") \*ib\_conn)
:   releases the pool of fast\_reg descriptors

**Parameters**

`struct ib_conn *ib_conn`
:   connection RDMA resources

void iser\_free\_ib\_conn\_res(struct [iser\_conn](#c.iser_free_ib_conn_res "iser_conn") \*iser\_conn, bool destroy)
:   release IB related resources

**Parameters**

`struct iser_conn *iser_conn`
:   iser connection struct

`bool destroy`
:   indicator if we need to try to release the
    iser device and memory regoins pool (only iscsi
    shutdown and DEVICE\_REMOVAL will use this).

**Description**

This routine is called with the iser state mutex held
so the cm\_id removal is out of here. It is Safe to
be invoked multiple times.

void iser\_conn\_release(struct [iser\_conn](#c.iser_conn_release "iser_conn") \*iser\_conn)
:   Frees all conn objects and deallocs conn descriptor

**Parameters**

`struct iser_conn *iser_conn`
:   iSER connection context

int iser\_conn\_terminate(struct [iser\_conn](#c.iser_conn_terminate "iser_conn") \*iser\_conn)
:   triggers start of the disconnect procedures and waits for them to be done

**Parameters**

`struct iser_conn *iser_conn`
:   iSER connection context

**Description**

Called with state mutex held

int iser\_post\_send(struct [ib\_conn](#c.iser_post_send "ib_conn") \*ib\_conn, struct [iser\_tx\_desc](#c.iser_tx_desc "iser_tx_desc") \*tx\_desc)
:   Initiate a Send DTO operation

**Parameters**

`struct ib_conn *ib_conn`
:   connection RDMA resources

`struct iser_tx_desc *tx_desc`
:   iSER TX descriptor

**Return**

0 on success, -1 on failure

### InfiniBand SCSI RDMA protocol target support

enum srpt\_command\_state
:   SCSI command state managed by SRPT

**Constants**

`SRPT_STATE_NEW`
:   New command arrived and is being processed.

`SRPT_STATE_NEED_DATA`
:   Processing a write or bidir command and waiting
    for data arrival.

`SRPT_STATE_DATA_IN`
:   Data for the write or bidir command arrived and is
    being processed.

`SRPT_STATE_CMD_RSP_SENT`
:   SRP\_RSP for SRP\_CMD has been sent.

`SRPT_STATE_MGMT`
:   Processing a SCSI task management command.

`SRPT_STATE_MGMT_RSP_SENT`
:   SRP\_RSP for SRP\_TSK\_MGMT has been sent.

`SRPT_STATE_DONE`
:   Command processing finished successfully, command
    processing has been aborted or command processing
    failed.

struct srpt\_ioctx
:   shared SRPT I/O context information

**Definition**:

```
struct srpt_ioctx {
    struct ib_cqe           cqe;
    void *buf;
    dma_addr_t dma;
    uint32_t offset;
    uint32_t index;
};
```

**Members**

`cqe`
:   Completion queue element.

`buf`
:   Pointer to the buffer.

`dma`
:   DMA address of the buffer.

`offset`
:   Offset of the first byte in **buf** and **dma** that is actually used.

`index`
:   Index of the I/O context in its ioctx\_ring array.

struct srpt\_recv\_ioctx
:   SRPT receive I/O context

**Definition**:

```
struct srpt_recv_ioctx {
    struct srpt_ioctx       ioctx;
    struct list_head        wait_list;
    int byte_len;
};
```

**Members**

`ioctx`
:   See above.

`wait_list`
:   Node for insertion in srpt\_rdma\_ch.cmd\_wait\_list.

`byte_len`
:   Number of bytes in **ioctx.buf**.

struct srpt\_send\_ioctx
:   SRPT send I/O context

**Definition**:

```
struct srpt_send_ioctx {
    struct srpt_ioctx       ioctx;
    struct srpt_rdma_ch     *ch;
    struct srpt_recv_ioctx  *recv_ioctx;
    struct srpt_rw_ctx      s_rw_ctx;
    struct srpt_rw_ctx      *rw_ctxs;
    struct scatterlist      imm_sg;
    struct ib_cqe           rdma_cqe;
    enum srpt_command_state state;
    struct se_cmd           cmd;
    u8 n_rdma;
    u8 n_rw_ctx;
    bool queue_status_only;
    u8 sense_data[TRANSPORT_SENSE_BUFFER];
};
```

**Members**

`ioctx`
:   See above.

`ch`
:   Channel pointer.

`recv_ioctx`
:   Receive I/O context associated with this send I/O context.
    Only used for processing immediate data.

`s_rw_ctx`
:   **rw\_ctxs** points here if only a single rw\_ctx is needed.

`rw_ctxs`
:   RDMA read/write contexts.

`imm_sg`
:   Scatterlist for immediate data.

`rdma_cqe`
:   RDMA completion queue element.

`state`
:   I/O context state.

`cmd`
:   Target core command data structure.

`n_rdma`
:   Number of work requests needed to transfer this ioctx.

`n_rw_ctx`
:   Size of rw\_ctxs array.

`queue_status_only`
:   Send a SCSI status back to the initiator but no data.

`sense_data`
:   Sense data to be sent to the initiator.

enum rdma\_ch\_state
:   SRP channel state

**Constants**

`CH_CONNECTING`
:   QP is in RTR state; waiting for RTU.

`CH_LIVE`
:   QP is in RTS state.

`CH_DISCONNECTING`
:   DREQ has been sent and waiting for DREP or DREQ has
    been received.

`CH_DRAINING`
:   DREP has been received or waiting for DREP timed out
    and last work request has been queued.

`CH_DISCONNECTED`
:   Last completion has been received.

struct srpt\_rdma\_ch
:   RDMA channel

**Definition**:

```
struct srpt_rdma_ch {
    struct srpt_nexus       *nexus;
    struct ib_qp            *qp;
    union {
        struct {
            struct ib_cm_id         *cm_id;
        } ib_cm;
        struct {
            struct rdma_cm_id       *cm_id;
        } rdma_cm;
    };
    struct ib_cq            *cq;
    u32 cq_size;
    struct ib_cqe           zw_cqe;
    struct rcu_head         rcu;
    struct kref             kref;
    struct completion       *closed;
    int rq_size;
    u32 max_rsp_size;
    atomic_t sq_wr_avail;
    struct srpt_port        *sport;
    int max_ti_iu_len;
    atomic_t req_lim;
    atomic_t req_lim_delta;
    u16 imm_data_offset;
    spinlock_t spinlock;
    enum rdma_ch_state      state;
    struct kmem_cache       *rsp_buf_cache;
    struct srpt_send_ioctx  **ioctx_ring;
    struct kmem_cache       *req_buf_cache;
    struct srpt_recv_ioctx  **ioctx_recv_ring;
    struct list_head        list;
    struct list_head        cmd_wait_list;
    uint16_t pkey;
    bool using_rdma_cm;
    bool processing_wait_list;
    struct se_session       *sess;
    u8 sess_name[40];
    struct work_struct      release_work;
};
```

**Members**

`nexus`
:   I\_T nexus this channel is associated with.

`qp`
:   IB queue pair used for communicating over this channel.

`{unnamed_union}`
:   anonymous

`ib_cm`
:   See below.

`ib_cm.cm_id`
:   IB CM ID associated with the channel.

`rdma_cm`
:   See below.

`rdma_cm.cm_id`
:   RDMA CM ID associated with the channel.

`cq`
:   IB completion queue for this channel.

`cq_size`
:   Number of CQEs in **cq**.

`zw_cqe`
:   Zero-length write CQE.

`rcu`
:   RCU head.

`kref`
:   kref for this channel.

`closed`
:   Completion object that will be signaled as soon as a new
    channel object with the same identity can be created.

`rq_size`
:   IB receive queue size.

`max_rsp_size`
:   Maximum size of an RSP response message in bytes.

`sq_wr_avail`
:   number of work requests available in the send queue.

`sport`
:   pointer to the information of the HCA port used by this
    channel.

`max_ti_iu_len`
:   maximum target-to-initiator information unit length.

`req_lim`
:   request limit: maximum number of requests that may be sent
    by the initiator without having received a response.

`req_lim_delta`
:   Number of credits not yet sent back to the initiator.

`imm_data_offset`
:   Offset from start of SRP\_CMD for immediate data.

`spinlock`
:   Protects free\_list and state.

`state`
:   channel state. See also [`enum rdma_ch_state`](#c.rdma_ch_state "rdma_ch_state").

`rsp_buf_cache`
:   kmem\_cache for **ioctx\_ring**.

`ioctx_ring`
:   Send ring.

`req_buf_cache`
:   kmem\_cache for **ioctx\_recv\_ring**.

`ioctx_recv_ring`
:   Receive I/O context ring.

`list`
:   Node in srpt\_nexus.ch\_list.

`cmd_wait_list`
:   List of SCSI commands that arrived before the RTU event. This
    list contains [`struct srpt_ioctx`](#c.srpt_ioctx "srpt_ioctx") elements and is protected
    against concurrent modification by the cm\_id spinlock.

`pkey`
:   P\_Key of the IB partition for this SRP channel.

`using_rdma_cm`
:   Whether the RDMA/CM or IB/CM is used for this channel.

`processing_wait_list`
:   Whether or not cmd\_wait\_list is being processed.

`sess`
:   Session information associated with this SRP channel.

`sess_name`
:   Session name.

`release_work`
:   Allows scheduling of `srpt_release_channel()`.

struct srpt\_nexus
:   I\_T nexus

**Definition**:

```
struct srpt_nexus {
    struct rcu_head         rcu;
    struct list_head        entry;
    struct list_head        ch_list;
    u8 i_port_id[16];
    u8 t_port_id[16];
};
```

**Members**

`rcu`
:   RCU head for this data structure.

`entry`
:   srpt\_port.nexus\_list list node.

`ch_list`
:   [`struct srpt_rdma_ch`](#c.srpt_rdma_ch "srpt_rdma_ch") list. Protected by srpt\_port.mutex.

`i_port_id`
:   128-bit initiator port identifier copied from SRP\_LOGIN\_REQ.

`t_port_id`
:   128-bit target port identifier copied from SRP\_LOGIN\_REQ.

struct srpt\_port\_attrib
:   attributes for SRPT port

**Definition**:

```
struct srpt_port_attrib {
    u32 srp_max_rdma_size;
    u32 srp_max_rsp_size;
    u32 srp_sq_size;
    bool use_srq;
};
```

**Members**

`srp_max_rdma_size`
:   Maximum size of SRP RDMA transfers for new connections.

`srp_max_rsp_size`
:   Maximum size of SRP response messages in bytes.

`srp_sq_size`
:   Shared receive queue (SRQ) size.

`use_srq`
:   Whether or not to use SRQ.

struct srpt\_tpg
:   information about a single “target portal group”

**Definition**:

```
struct srpt_tpg {
    struct list_head        entry;
    struct srpt_port_id     *sport_id;
    struct se_portal_group  tpg;
};
```

**Members**

`entry`
:   Entry in **sport\_id->tpg\_list**.

`sport_id`
:   Port name this TPG is associated with.

`tpg`
:   LIO TPG data structure.

**Description**

Zero or more target portal groups are associated with each port name
(srpt\_port\_id). With each TPG an ACL list is associated.

struct srpt\_port\_id
:   LIO RDMA port information

**Definition**:

```
struct srpt_port_id {
    struct mutex            mutex;
    struct list_head        tpg_list;
    struct se_wwn           wwn;
    char name[64];
};
```

**Members**

`mutex`
:   Protects **tpg\_list** changes.

`tpg_list`
:   TPGs associated with the RDMA port name.

`wwn`
:   WWN associated with the RDMA port name.

`name`
:   ASCII representation of the port name.

**Description**

Multiple sysfs directories can be associated with a single RDMA port. This
data structure represents a single (port, name) pair.

struct srpt\_port
:   SRPT RDMA port information

**Definition**:

```
struct srpt_port {
    struct srpt_device      *sdev;
    struct ib_mad_agent     *mad_agent;
    bool enabled;
    u8 port;
    u32 sm_lid;
    u32 lid;
    union ib_gid            gid;
    struct work_struct      work;
    char guid_name[64];
    struct srpt_port_id     *guid_id;
    char gid_name[64];
    struct srpt_port_id     *gid_id;
    struct srpt_port_attrib port_attrib;
    atomic_t refcount;
    struct completion       *freed_channels;
    struct mutex            mutex;
    struct list_head        nexus_list;
};
```

**Members**

`sdev`
:   backpointer to the HCA information.

`mad_agent`
:   per-port management datagram processing information.

`enabled`
:   Whether or not this target port is enabled.

`port`
:   one-based port number.

`sm_lid`
:   cached value of the port’s sm\_lid.

`lid`
:   cached value of the port’s lid.

`gid`
:   cached value of the port’s gid.

`work`
:   work structure for refreshing the aforementioned cached values.

`guid_name`
:   port name in GUID format.

`guid_id`
:   LIO target port information for the port name in GUID format.

`gid_name`
:   port name in GID format.

`gid_id`
:   LIO target port information for the port name in GID format.

`port_attrib`
:   Port attributes that can be accessed through configfs.

`refcount`
:   Number of objects associated with this port.

`freed_channels`
:   Completion that will be signaled once **refcount** becomes 0.

`mutex`
:   Protects nexus\_list.

`nexus_list`
:   Nexus list. See also srpt\_nexus.entry.

struct srpt\_device
:   information associated by SRPT with a single HCA

**Definition**:

```
struct srpt_device {
    struct kref             refcnt;
    struct ib_device        *device;
    struct ib_pd            *pd;
    u32 lkey;
    struct ib_srq           *srq;
    struct ib_cm_id         *cm_id;
    int srq_size;
    struct mutex            sdev_mutex;
    bool use_srq;
    struct kmem_cache       *req_buf_cache;
    struct srpt_recv_ioctx  **ioctx_ring;
    struct ib_event_handler event_handler;
    struct list_head        list;
    struct srpt_port        port[];
};
```

**Members**

`refcnt`
:   Reference count for this device.

`device`
:   Backpointer to the `struct ib_device` managed by the IB core.

`pd`
:   IB protection domain.

`lkey`
:   L\_Key (local key) with write access to all local memory.

`srq`
:   Per-HCA SRQ (shared receive queue).

`cm_id`
:   Connection identifier.

`srq_size`
:   SRQ size.

`sdev_mutex`
:   Serializes use\_srq changes.

`use_srq`
:   Whether or not to use SRQ.

`req_buf_cache`
:   kmem\_cache for **ioctx\_ring** buffers.

`ioctx_ring`
:   Per-HCA SRQ.

`event_handler`
:   Per-HCA asynchronous IB event handler.

`list`
:   Node in srpt\_dev\_list.

`port`
:   Information about the ports owned by this HCA.

void srpt\_event\_handler(struct ib\_event\_handler \*handler, struct ib\_event \*event)
:   asynchronous IB event callback function

**Parameters**

`struct ib_event_handler *handler`
:   IB event handler registered by [`ib_register_event_handler()`](#c.ib_register_event_handler "ib_register_event_handler").

`struct ib_event *event`
:   Description of the event that occurred.

**Description**

Callback function called by the InfiniBand core when an asynchronous IB
event occurs. This callback may occur in interrupt context. See also
section 11.5.2, Set Asynchronous Event Handler in the InfiniBand
Architecture Specification.

void srpt\_srq\_event(struct ib\_event \*event, void \*ctx)
:   SRQ event callback function

**Parameters**

`struct ib_event *event`
:   Description of the event that occurred.

`void *ctx`
:   Context pointer specified at SRQ creation time.

void srpt\_qp\_event(struct ib\_event \*event, void \*ptr)
:   QP event callback function

**Parameters**

`struct ib_event *event`
:   Description of the event that occurred.

`void *ptr`
:   SRPT RDMA channel.

void srpt\_set\_ioc(u8 \*c\_list, u32 slot, u8 value)
:   initialize a IOUnitInfo structure

**Parameters**

`u8 *c_list`
:   controller list.

`u32 slot`
:   one-based slot number.

`u8 value`
:   four-bit value.

**Description**

Copies the lowest four bits of value in element slot of the array of four
bit elements called c\_list (controller list). The index slot is one-based.

void srpt\_get\_class\_port\_info(struct ib\_dm\_mad \*mad)
:   copy ClassPortInfo to a management datagram

**Parameters**

`struct ib_dm_mad *mad`
:   Datagram that will be sent as response to DM\_ATTR\_CLASS\_PORT\_INFO.

**Description**

See also section 16.3.3.1 ClassPortInfo in the InfiniBand Architecture
Specification.

void srpt\_get\_iou(struct ib\_dm\_mad \*mad)
:   write IOUnitInfo to a management datagram

**Parameters**

`struct ib_dm_mad *mad`
:   Datagram that will be sent as response to DM\_ATTR\_IOU\_INFO.

**Description**

See also section 16.3.3.3 IOUnitInfo in the InfiniBand Architecture
Specification. See also section B.7, table B.6 in the SRP r16a document.

void srpt\_get\_ioc(struct [srpt\_port](#c.srpt_port "srpt_port") \*sport, u32 slot, struct ib\_dm\_mad \*mad)
:   write IOControllerprofile to a management datagram

**Parameters**

`struct srpt_port *sport`
:   HCA port through which the MAD has been received.

`u32 slot`
:   Slot number specified in DM\_ATTR\_IOC\_PROFILE query.

`struct ib_dm_mad *mad`
:   Datagram that will be sent as response to DM\_ATTR\_IOC\_PROFILE.

**Description**

See also section 16.3.3.4 IOControllerProfile in the InfiniBand
Architecture Specification. See also section B.7, table B.7 in the SRP
r16a document.

void srpt\_get\_svc\_entries(u64 ioc\_guid, u16 slot, u8 hi, u8 lo, struct ib\_dm\_mad \*mad)
:   write ServiceEntries to a management datagram

**Parameters**

`u64 ioc_guid`
:   I/O controller GUID to use in reply.

`u16 slot`
:   I/O controller number.

`u8 hi`
:   End of the range of service entries to be specified in the reply.

`u8 lo`
:   Start of the range of service entries to be specified in the reply..

`struct ib_dm_mad *mad`
:   Datagram that will be sent as response to DM\_ATTR\_SVC\_ENTRIES.

**Description**

See also section 16.3.3.5 ServiceEntries in the InfiniBand Architecture
Specification. See also section B.7, table B.8 in the SRP r16a document.

void srpt\_mgmt\_method\_get(struct [srpt\_port](#c.srpt_port "srpt_port") \*sp, struct ib\_mad \*rq\_mad, struct ib\_dm\_mad \*rsp\_mad)
:   process a received management datagram

**Parameters**

`struct srpt_port *sp`
:   HCA port through which the MAD has been received.

`struct ib_mad *rq_mad`
:   received MAD.

`struct ib_dm_mad *rsp_mad`
:   response MAD.

void srpt\_mad\_send\_handler(struct ib\_mad\_agent \*mad\_agent, struct ib\_mad\_send\_wc \*mad\_wc)
:   MAD send completion callback

**Parameters**

`struct ib_mad_agent *mad_agent`
:   Return value of `ib_register_mad_agent()`.

`struct ib_mad_send_wc *mad_wc`
:   Work completion reporting that the MAD has been sent.

void srpt\_mad\_recv\_handler(struct ib\_mad\_agent \*mad\_agent, struct ib\_mad\_send\_buf \*send\_buf, struct ib\_mad\_recv\_wc \*mad\_wc)
:   MAD reception callback function

**Parameters**

`struct ib_mad_agent *mad_agent`
:   Return value of `ib_register_mad_agent()`.

`struct ib_mad_send_buf *send_buf`
:   Not used.

`struct ib_mad_recv_wc *mad_wc`
:   Work completion reporting that a MAD has been received.

int srpt\_refresh\_port(struct [srpt\_port](#c.srpt_port "srpt_port") \*sport)
:   configure a HCA port

**Parameters**

`struct srpt_port *sport`
:   SRPT HCA port.

**Description**

Enable InfiniBand management datagram processing, update the cached sm\_lid,
lid and gid values, and register a callback function for processing MADs
on the specified port.

**Note**

It is safe to call this function more than once for the same port.

void srpt\_unregister\_mad\_agent(struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, int port\_cnt)
:   unregister MAD callback functions

**Parameters**

`struct srpt_device *sdev`
:   SRPT HCA pointer.

`int port_cnt`
:   number of ports with registered MAD

**Note**

It is safe to call this function more than once for the same device.

struct [srpt\_ioctx](#c.srpt_ioctx "srpt_ioctx") \*srpt\_alloc\_ioctx(struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, int ioctx\_size, struct kmem\_cache \*buf\_cache, enum dma\_data\_direction dir)
:   allocate a SRPT I/O context structure

**Parameters**

`struct srpt_device *sdev`
:   SRPT HCA pointer.

`int ioctx_size`
:   I/O context size.

`struct kmem_cache *buf_cache`
:   I/O buffer cache.

`enum dma_data_direction dir`
:   DMA data direction.

void srpt\_free\_ioctx(struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, struct [srpt\_ioctx](#c.srpt_ioctx "srpt_ioctx") \*ioctx, struct kmem\_cache \*buf\_cache, enum dma\_data\_direction dir)
:   free a SRPT I/O context structure

**Parameters**

`struct srpt_device *sdev`
:   SRPT HCA pointer.

`struct srpt_ioctx *ioctx`
:   I/O context pointer.

`struct kmem_cache *buf_cache`
:   I/O buffer cache.

`enum dma_data_direction dir`
:   DMA data direction.

struct [srpt\_ioctx](#c.srpt_ioctx "srpt_ioctx") \*\*srpt\_alloc\_ioctx\_ring(struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, int ring\_size, int ioctx\_size, struct kmem\_cache \*buf\_cache, int alignment\_offset, enum dma\_data\_direction dir)
:   allocate a ring of SRPT I/O context structures

**Parameters**

`struct srpt_device *sdev`
:   Device to allocate the I/O context ring for.

`int ring_size`
:   Number of elements in the I/O context ring.

`int ioctx_size`
:   I/O context size.

`struct kmem_cache *buf_cache`
:   I/O buffer cache.

`int alignment_offset`
:   Offset in each ring buffer at which the SRP information
    unit starts.

`enum dma_data_direction dir`
:   DMA data direction.

void srpt\_free\_ioctx\_ring(struct [srpt\_ioctx](#c.srpt_ioctx "srpt_ioctx") \*\*ioctx\_ring, struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, int ring\_size, struct kmem\_cache \*buf\_cache, enum dma\_data\_direction dir)
:   free the ring of SRPT I/O context structures

**Parameters**

`struct srpt_ioctx **ioctx_ring`
:   I/O context ring to be freed.

`struct srpt_device *sdev`
:   SRPT HCA pointer.

`int ring_size`
:   Number of ring elements.

`struct kmem_cache *buf_cache`
:   I/O buffer cache.

`enum dma_data_direction dir`
:   DMA data direction.

enum [srpt\_command\_state](#c.srpt_command_state "srpt_command_state") srpt\_set\_cmd\_state(struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx, enum [srpt\_command\_state](#c.srpt_command_state "srpt_command_state") new)
:   set the state of a SCSI command

**Parameters**

`struct srpt_send_ioctx *ioctx`
:   Send I/O context.

`enum srpt_command_state new`
:   New I/O context state.

**Description**

Does not modify the state of aborted commands. Returns the previous command
state.

bool srpt\_test\_and\_set\_cmd\_state(struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx, enum [srpt\_command\_state](#c.srpt_command_state "srpt_command_state") old, enum [srpt\_command\_state](#c.srpt_command_state "srpt_command_state") new)
:   test and set the state of a command

**Parameters**

`struct srpt_send_ioctx *ioctx`
:   Send I/O context.

`enum srpt_command_state old`
:   Current I/O context state.

`enum srpt_command_state new`
:   New I/O context state.

**Description**

Returns true if and only if the previous command state was equal to ‘old’.

int srpt\_post\_recv(struct [srpt\_device](#c.srpt_device "srpt_device") \*sdev, struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_recv\_ioctx](#c.srpt_recv_ioctx "srpt_recv_ioctx") \*ioctx)
:   post an IB receive request

**Parameters**

`struct srpt_device *sdev`
:   SRPT HCA pointer.

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

`struct srpt_recv_ioctx *ioctx`
:   Receive I/O context pointer.

int srpt\_zerolength\_write(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   perform a zero-length RDMA write

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

**Description**

A quote from the InfiniBand specification: C9-88: For an HCA responder
using Reliable Connection service, for each zero-length RDMA READ or WRITE
request, the R\_Key shall not be validated, even if the request includes
Immediate data.

int srpt\_get\_desc\_tbl(struct [srpt\_recv\_ioctx](#c.srpt_recv_ioctx "srpt_recv_ioctx") \*recv\_ioctx, struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx, struct [srp\_cmd](#c.srpt_get_desc_tbl "srp_cmd") \*srp\_cmd, enum dma\_data\_direction \*dir, struct scatterlist \*\*sg, unsigned int \*sg\_cnt, u64 \*data\_len, u16 imm\_data\_offset)
:   parse the data descriptors of a SRP\_CMD request

**Parameters**

`struct srpt_recv_ioctx *recv_ioctx`
:   I/O context associated with the received command **srp\_cmd**.

`struct srpt_send_ioctx *ioctx`
:   I/O context that will be used for responding to the initiator.

`struct srp_cmd *srp_cmd`
:   Pointer to the SRP\_CMD request data.

`enum dma_data_direction *dir`
:   Pointer to the variable to which the transfer direction will be
    written.

`struct scatterlist **sg`
:   [out] scatterlist for the parsed SRP\_CMD.

`unsigned int *sg_cnt`
:   [out] length of **sg**.

`u64 *data_len`
:   Pointer to the variable to which the total data length of all
    descriptors in the SRP\_CMD request will be written.

`u16 imm_data_offset`
:   [in] Offset in SRP\_CMD requests at which immediate data
    starts.

**Description**

This function initializes ioctx->nrbuf and ioctx->r\_bufs.

Returns -EINVAL when the SRP\_CMD request contains inconsistent descriptors;
-ENOMEM when memory allocation fails and zero upon success.

int srpt\_init\_ch\_qp(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct ib\_qp \*qp)
:   initialize queue pair attributes

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

`struct ib_qp *qp`
:   Queue pair pointer.

**Description**

Initialized the attributes of queue pair ‘qp’ by allowing local write,
remote read and remote write. Also transitions ‘qp’ to state IB\_QPS\_INIT.

int srpt\_ch\_qp\_rtr(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct ib\_qp \*qp)
:   change the state of a channel to ‘ready to receive’ (RTR)

**Parameters**

`struct srpt_rdma_ch *ch`
:   channel of the queue pair.

`struct ib_qp *qp`
:   queue pair to change the state of.

**Description**

Returns zero upon success and a negative value upon failure.

**Note**

currently a `struct ib_qp_attr` takes 136 bytes on a 64-bit system.
If this structure ever becomes larger, it might be necessary to allocate
it dynamically instead of on the stack.

int srpt\_ch\_qp\_rts(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct ib\_qp \*qp)
:   change the state of a channel to ‘ready to send’ (RTS)

**Parameters**

`struct srpt_rdma_ch *ch`
:   channel of the queue pair.

`struct ib_qp *qp`
:   queue pair to change the state of.

**Description**

Returns zero upon success and a negative value upon failure.

**Note**

currently a `struct ib_qp_attr` takes 136 bytes on a 64-bit system.
If this structure ever becomes larger, it might be necessary to allocate
it dynamically instead of on the stack.

int srpt\_ch\_qp\_err(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   set the channel queue pair state to ‘error’

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*srpt\_get\_send\_ioctx(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   obtain an I/O context for sending to the initiator

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

int srpt\_abort\_cmd(struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx)
:   abort a SCSI command

**Parameters**

`struct srpt_send_ioctx *ioctx`
:   I/O context associated with the SCSI command.

void srpt\_rdma\_read\_done(struct ib\_cq \*cq, struct ib\_wc \*wc)
:   RDMA read completion callback

**Parameters**

`struct ib_cq *cq`
:   Completion queue.

`struct ib_wc *wc`
:   Work completion.

**Description**

XXX: what is now target\_execute\_cmd used to be asynchronous, and unmapping
the data that has been transferred via IB RDMA had to be postponed until the
`check_stop_free()` callback. None of this is necessary anymore and needs to
be cleaned up.

int srpt\_build\_cmd\_rsp(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx, u64 tag, int status)
:   build a SRP\_RSP response

**Parameters**

`struct srpt_rdma_ch *ch`
:   RDMA channel through which the request has been received.

`struct srpt_send_ioctx *ioctx`
:   I/O context associated with the SRP\_CMD request. The response will
    be built in the buffer ioctx->buf points at and hence this function will
    overwrite the request data.

`u64 tag`
:   tag of the request for which this response is being generated.

`int status`
:   value for the STATUS field of the SRP\_RSP information unit.

**Description**

Returns the size in bytes of the SRP\_RSP response.

An SRP\_RSP response contains a SCSI status or service response. See also
section 6.9 in the SRP r16a document for the format of an SRP\_RSP
response. See also SPC-2 for more information about sense data.

int srpt\_build\_tskmgmt\_rsp(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*ioctx, u8 rsp\_code, u64 tag)
:   build a task management response

**Parameters**

`struct srpt_rdma_ch *ch`
:   RDMA channel through which the request has been received.

`struct srpt_send_ioctx *ioctx`
:   I/O context in which the SRP\_RSP response will be built.

`u8 rsp_code`
:   RSP\_CODE that will be stored in the response.

`u64 tag`
:   Tag of the request for which this response is being generated.

**Description**

Returns the size in bytes of the SRP\_RSP response.

An SRP\_RSP response contains a SCSI status or service response. See also
section 6.9 in the SRP r16a document for the format of an SRP\_RSP
response.

void srpt\_handle\_cmd(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_recv\_ioctx](#c.srpt_recv_ioctx "srpt_recv_ioctx") \*recv\_ioctx, struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*send\_ioctx)
:   process a SRP\_CMD information unit

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

`struct srpt_recv_ioctx *recv_ioctx`
:   Receive I/O context.

`struct srpt_send_ioctx *send_ioctx`
:   Send I/O context.

void srpt\_handle\_tsk\_mgmt(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_recv\_ioctx](#c.srpt_recv_ioctx "srpt_recv_ioctx") \*recv\_ioctx, struct [srpt\_send\_ioctx](#c.srpt_send_ioctx "srpt_send_ioctx") \*send\_ioctx)
:   process a SRP\_TSK\_MGMT information unit

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

`struct srpt_recv_ioctx *recv_ioctx`
:   Receive I/O context.

`struct srpt_send_ioctx *send_ioctx`
:   Send I/O context.

**Description**

Returns 0 if and only if the request will be processed by the target core.

For more information about SRP\_TSK\_MGMT information units, see also section
6.7 in the SRP r16a document.

bool srpt\_handle\_new\_iu(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch, struct [srpt\_recv\_ioctx](#c.srpt_recv_ioctx "srpt_recv_ioctx") \*recv\_ioctx)
:   process a newly received information unit

**Parameters**

`struct srpt_rdma_ch *ch`
:   RDMA channel through which the information unit has been received.

`struct srpt_recv_ioctx *recv_ioctx`
:   Receive I/O context associated with the information unit.

void srpt\_send\_done(struct ib\_cq \*cq, struct ib\_wc \*wc)
:   send completion callback

**Parameters**

`struct ib_cq *cq`
:   Completion queue.

`struct ib_wc *wc`
:   Work completion.

**Note**

Although this has not yet been observed during tests, at least in
theory it is possible that the [`srpt_get_send_ioctx()`](#c.srpt_get_send_ioctx "srpt_get_send_ioctx") call invoked by
[`srpt_handle_new_iu()`](#c.srpt_handle_new_iu "srpt_handle_new_iu") fails. This is possible because the req\_lim\_delta
value in each response is set to one, and it is possible that this response
makes the initiator send a new request before the send completion for that
response has been processed. This could e.g. happen if the call to
`srpt_put_send_iotcx()` is delayed because of a higher priority interrupt or
if IB retransmission causes generation of the send completion to be
delayed. Incoming information units for which [`srpt_get_send_ioctx()`](#c.srpt_get_send_ioctx "srpt_get_send_ioctx") fails
are queued on cmd\_wait\_list. The code below processes these delayed
requests one at a time.

int srpt\_create\_ch\_ib(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   create receive and send completion queues

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

bool srpt\_close\_ch(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   close a RDMA channel

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

**Description**

Make sure all resources associated with the channel will be deallocated at
an appropriate time.

Returns true if and only if the channel state has been modified into
CH\_DRAINING.

int srpt\_cm\_req\_recv(struct [srpt\_device](#c.srpt_device "srpt_device") \*const sdev, struct [ib\_cm\_id](#c.srpt_cm_req_recv "ib_cm_id") \*ib\_cm\_id, struct [rdma\_cm\_id](#c.srpt_cm_req_recv "rdma_cm_id") \*rdma\_cm\_id, u8 port\_num, \_\_be16 pkey, const struct srp\_login\_req \*req, const char \*src\_addr)
:   process the event IB\_CM\_REQ\_RECEIVED

**Parameters**

`struct srpt_device *const sdev`
:   HCA through which the login request was received.

`struct ib_cm_id *ib_cm_id`
:   IB/CM connection identifier in case of IB/CM.

`struct rdma_cm_id *rdma_cm_id`
:   RDMA/CM connection identifier in case of RDMA/CM.

`u8 port_num`
:   Port through which the REQ message was received.

`__be16 pkey`
:   P\_Key of the incoming connection.

`const struct srp_login_req *req`
:   SRP login request.

`const char *src_addr`
:   GID (IB/CM) or IP address (RDMA/CM) of the port that submitted
    the login request.

**Description**

Ownership of the cm\_id is transferred to the target session if this
function returns zero. Otherwise the caller remains the owner of cm\_id.

void srpt\_cm\_rtu\_recv(struct [srpt\_rdma\_ch](#c.srpt_rdma_ch "srpt_rdma_ch") \*ch)
:   process an IB\_CM\_RTU\_RECEIVED or USER\_ESTABLISHED event

**Parameters**

`struct srpt_rdma_ch *ch`
:   SRPT RDMA channel.

**Description**

An RTU (ready to use) message indicates that the connection has been
established and that the recipient may begin transmitting.

int srpt\_cm\_handler(struct ib\_cm\_id \*cm\_id, const struct ib\_cm\_event \*event)
:   IB connection manager callback function

**Parameters**

`struct ib_cm_id *cm_id`
:   IB/CM connection identifier.

`const struct ib_cm_event *event`
:   IB/CM event.

**Description**

A non-zero return value will cause the caller destroy the CM ID.

**Note**

[`srpt_cm_handler()`](#c.srpt_cm_handler "srpt_cm_handler") must only return a non-zero value when transferring
ownership of the cm\_id to a channel by [`srpt_cm_req_recv()`](#c.srpt_cm_req_recv "srpt_cm_req_recv") failed. Returning
a non-zero value in any other case will trigger a race with the
`ib_destroy_cm_id()` call in `srpt_release_channel()`.

void srpt\_queue\_response(struct se\_cmd \*cmd)
:   transmit the response to a SCSI command

**Parameters**

`struct se_cmd *cmd`
:   SCSI target command.

**Description**

Callback function called by the TCM core. Must not block since it can be
invoked on the context of the IB completion handler.

int srpt\_release\_sport(struct [srpt\_port](#c.srpt_port "srpt_port") \*sport)
:   disable login and wait for associated channels

**Parameters**

`struct srpt_port *sport`
:   SRPT HCA port.

struct port\_and\_port\_id srpt\_lookup\_port(const char \*name)
:   Look up an RDMA port by name

**Parameters**

`const char *name`
:   ASCII port name

**Description**

Increments the RDMA port reference count if an RDMA port pointer is returned.
The caller must drop that reference count by calling `srpt_port_put_ref()`.

int srpt\_add\_one(struct ib\_device \*device)
:   InfiniBand device addition callback function

**Parameters**

`struct ib_device *device`
:   Describes a HCA.

void srpt\_remove\_one(struct ib\_device \*device, void \*client\_data)
:   InfiniBand device removal callback function

**Parameters**

`struct ib_device *device`
:   Describes a HCA.

`void *client_data`
:   The value passed as the third argument to [`ib_set_client_data()`](#c.ib_set_client_data "ib_set_client_data").

void srpt\_close\_session(struct se\_session \*se\_sess)
:   forcibly close a session

**Parameters**

`struct se_session *se_sess`
:   SCSI target session.

**Description**

Callback function invoked by the TCM core to clean up sessions associated
with a node ACL when the user invokes
rmdir /sys/kernel/config/target/$driver/$port/$tpg/acls/$i\_port\_id

int srpt\_parse\_i\_port\_id(u8 i\_port\_id[16], const char \*name)
:   parse an initiator port ID

**Parameters**

`u8 i_port_id[16]`
:   Binary 128-bit port ID.

`const char *name`
:   ASCII representation of a 128-bit initiator port ID.

struct se\_portal\_group \*srpt\_make\_tpg(struct se\_wwn \*wwn, const char \*name)
:   configfs callback invoked for mkdir /sys/kernel/config/target/$driver/$port/$tpg

**Parameters**

`struct se_wwn *wwn`
:   Corresponds to $driver/$port.

`const char *name`
:   $tpg.

void srpt\_drop\_tpg(struct se\_portal\_group \*tpg)
:   configfs callback invoked for rmdir /sys/kernel/config/target/$driver/$port/$tpg

**Parameters**

`struct se_portal_group *tpg`
:   Target portal group to deregister.

struct se\_wwn \*srpt\_make\_tport(struct target\_fabric\_configfs \*tf, struct config\_group \*group, const char \*name)
:   configfs callback invoked for mkdir /sys/kernel/config/target/$driver/$port

**Parameters**

`struct target_fabric_configfs *tf`
:   Not used.

`struct config_group *group`
:   Not used.

`const char *name`
:   $port.

void srpt\_drop\_tport(struct se\_wwn \*wwn)
:   configfs callback invoked for rmdir /sys/kernel/config/target/$driver/$port

**Parameters**

`struct se_wwn *wwn`
:   $port.

int srpt\_init\_module(void)
:   kernel module initialization

**Parameters**

`void`
:   no arguments

**Note**

Since [`ib_register_client()`](#c.ib_register_client "ib_register_client") registers callback functions, and since at
least one of these callback functions ([`srpt_add_one()`](#c.srpt_add_one "srpt_add_one")) calls target core
functions, this driver must be registered with the target core before
[`ib_register_client()`](#c.ib_register_client "ib_register_client") is called.

### iSCSI Extensions for RDMA (iSER) target support

void isert\_conn\_terminate(struct [isert\_conn](#c.isert_conn_terminate "isert_conn") \*isert\_conn)
:   Initiate connection termination

**Parameters**

`struct isert_conn *isert_conn`
:   isert connection struct

**Notes**

In case the connection state is BOUND, move state
to TEMINATING and start teardown sequence (rdma\_disconnect).
In case the connection state is UP, complete flush as well.

This routine must be called with mutex held. Thus it is
safe to call multiple times.

void isert\_put\_unsol\_pending\_cmds(struct iscsit\_conn \*conn)
:   Drop commands waiting for unsolicitate dataout

**Parameters**

`struct iscsit_conn *conn`
:   iscsi connection

**Description**

We might still have commands that are waiting for unsolicited
dataouts messages. We must put the extra reference on those
before blocking on the target\_wait\_for\_session\_cmds
