# The padata parallel execution mechanism

> 출처(원문): https://docs.kernel.org/core-api/padata.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The padata parallel execution mechanism

Date:
:   May 2020

Padata is a mechanism by which the kernel can farm jobs out to be done in
parallel on multiple CPUs while optionally retaining their ordering.

It was originally developed for IPsec, which needs to perform encryption and
decryption on large numbers of packets without reordering those packets. This
is currently the sole consumer of padata’s serialized job support.

Padata also supports multithreaded jobs, splitting up the job evenly while load
balancing and coordinating between threads.

## Running Serialized Jobs

### Initializing

The first step in using padata to run serialized jobs is to set up a
padata\_instance structure for overall control of how jobs are to be run:

```
#include <linux/padata.h>

struct padata_instance *padata_alloc(const char *name);
```

‘name’ simply identifies the instance.

Then, complete padata initialization by allocating a padata\_shell:

```
struct padata_shell *padata_alloc_shell(struct padata_instance *pinst);
```

A padata\_shell is used to submit a job to padata and allows a series of such
jobs to be serialized independently. A padata\_instance may have one or more
padata\_shells associated with it, each allowing a separate series of jobs.

### Modifying cpumasks

The CPUs used to run jobs can be changed in two ways, programmatically with
[`padata_set_cpumask()`](#c.padata_set_cpumask "padata_set_cpumask") or via sysfs. The former is defined:

```
int padata_set_cpumask(struct padata_instance *pinst, int cpumask_type,
                       cpumask_var_t cpumask);
```

Here cpumask\_type is one of PADATA\_CPU\_PARALLEL or PADATA\_CPU\_SERIAL, where a
parallel cpumask describes which processors will be used to execute jobs
submitted to this instance in parallel and a serial cpumask defines which
processors are allowed to be used as the serialization callback processor.
cpumask specifies the new cpumask to use.

There may be sysfs files for an instance’s cpumasks. For example, pcrypt’s
live in /sys/kernel/pcrypt/<instance-name>. Within an instance’s directory
there are two files, parallel\_cpumask and serial\_cpumask, and either cpumask
may be changed by echoing a bitmask into the file, for example:

```
echo f > /sys/kernel/pcrypt/pencrypt/parallel_cpumask
```

Reading one of these files shows the user-supplied cpumask, which may be
different from the ‘usable’ cpumask.

Padata maintains two pairs of cpumasks internally, the user-supplied cpumasks
and the ‘usable’ cpumasks. (Each pair consists of a parallel and a serial
cpumask.) The user-supplied cpumasks default to all possible CPUs on instance
allocation and may be changed as above. The usable cpumasks are always a
subset of the user-supplied cpumasks and contain only the online CPUs in the
user-supplied masks; these are the cpumasks padata actually uses. So it is
legal to supply a cpumask to padata that contains offline CPUs. Once an
offline CPU in the user-supplied cpumask comes online, padata is going to use
it.

Changing the CPU masks are expensive operations, so it should not be done with
great frequency.

### Running A Job

Actually submitting work to the padata instance requires the creation of a
padata\_priv structure, which represents one job:

```
struct padata_priv {
    /* Other stuff here... */
    void                    (*parallel)(struct padata_priv *padata);
    void                    (*serial)(struct padata_priv *padata);
};
```

This structure will almost certainly be embedded within some larger
structure specific to the work to be done. Most of its fields are private to
padata, but the structure should be zeroed at initialisation time, and the
`parallel()` and `serial()` functions should be provided. Those functions will
be called in the process of getting the work done as we will see
momentarily.

The submission of the job is done with:

```
int padata_do_parallel(struct padata_shell *ps,
                       struct padata_priv *padata, int *cb_cpu);
```

The ps and padata structures must be set up as described above; cb\_cpu
points to the preferred CPU to be used for the final callback when the job is
done; it must be in the current instance’s CPU mask (if not the cb\_cpu pointer
is updated to point to the CPU actually chosen). The return value from
[`padata_do_parallel()`](#c.padata_do_parallel "padata_do_parallel") is zero on success, indicating that the job is in
progress. -EBUSY means that somebody, somewhere else is messing with the
instance’s CPU mask, while -EINVAL is a complaint about cb\_cpu not being in the
serial cpumask, no online CPUs in the parallel or serial cpumasks, or a stopped
instance.

Each job submitted to [`padata_do_parallel()`](#c.padata_do_parallel "padata_do_parallel") will, in turn, be passed to
exactly one call to the above-mentioned `parallel()` function, on one CPU, so
true parallelism is achieved by submitting multiple jobs. `parallel()` runs with
software interrupts disabled and thus cannot sleep. The `parallel()`
function gets the padata\_priv structure pointer as its lone parameter;
information about the actual work to be done is probably obtained by using
[`container_of()`](../driver-api/basics.html#c.container_of "container_of") to find the enclosing structure.

Note that `parallel()` has no return value; the padata subsystem assumes that
`parallel()` will take responsibility for the job from this point. The job
need not be completed during this call, but, if `parallel()` leaves work
outstanding, it should be prepared to be called again with a new job before
the previous one completes.

### Serializing Jobs

When a job does complete, `parallel()` (or whatever function actually finishes
the work) should inform padata of the fact with a call to:

```
void padata_do_serial(struct padata_priv *padata);
```

At some point in the future, [`padata_do_serial()`](#c.padata_do_serial "padata_do_serial") will trigger a call to the
`serial()` function in the padata\_priv structure. That call will happen on
the CPU requested in the initial call to [`padata_do_parallel()`](#c.padata_do_parallel "padata_do_parallel"); it, too, is
run with local software interrupts disabled.
Note that this call may be deferred for a while since the padata code takes
pains to ensure that jobs are completed in the order in which they were
submitted.

### Destroying

Cleaning up a padata instance predictably involves calling the two free
functions that correspond to the allocation in reverse:

```
void padata_free_shell(struct padata_shell *ps);
void padata_free(struct padata_instance *pinst);
```

It is the user’s responsibility to ensure all outstanding jobs are complete
before any of the above are called.

## Running Multithreaded Jobs

A multithreaded job has a main thread and zero or more helper threads, with the
main thread participating in the job and then waiting until all helpers have
finished. padata splits the job into units called chunks, where a chunk is a
piece of the job that one thread completes in one call to the thread function.

A user has to do three things to run a multithreaded job. First, describe the
job by defining a padata\_mt\_job structure, which is explained in the Interface
section. This includes a pointer to the thread function, which padata will
call each time it assigns a job chunk to a thread. Then, define the thread
function, which accepts three arguments, `start`, `end`, and `arg`, where
the first two delimit the range that the thread operates on and the last is a
pointer to the job’s shared state, if any. Prepare the shared state, which is
typically allocated on the main thread’s stack. Last, call
[`padata_do_multithreaded()`](#c.padata_do_multithreaded "padata_do_multithreaded"), which will return once the job is finished.

## Interface

struct padata\_priv
:   Represents one job

**Definition**:

```
struct padata_priv {
    struct list_head        list;
    struct parallel_data    *pd;
    int cb_cpu;
    unsigned int            seq_nr;
    int info;
    void (*parallel)(struct padata_priv *padata);
    void (*serial)(struct padata_priv *padata);
};
```

**Members**

`list`
:   List entry, to attach to the padata lists.

`pd`
:   Pointer to the internal control structure.

`cb_cpu`
:   Callback cpu for serializatioon.

`seq_nr`
:   Sequence number of the parallelized data object.

`info`
:   Used to pass information from the parallel to the serial function.

`parallel`
:   Parallel execution function.

`serial`
:   Serial complete function.

struct padata\_list
:   one per work type per CPU

**Definition**:

```
struct padata_list {
    struct list_head        list;
    spinlock_t lock;
};
```

**Members**

`list`
:   List head.

`lock`
:   List lock.

struct padata\_serial\_queue
:   The percpu padata serial queue

**Definition**:

```
struct padata_serial_queue {
    struct padata_list    serial;
    struct work_struct    work;
    struct parallel_data *pd;
};
```

**Members**

`serial`
:   List to wait for serialization after reordering.

`work`
:   work struct for serialization.

`pd`
:   Backpointer to the internal control structure.

struct padata\_cpumask
:   The cpumasks for the parallel/serial workers

**Definition**:

```
struct padata_cpumask {
    cpumask_var_t pcpu;
    cpumask_var_t cbcpu;
};
```

**Members**

`pcpu`
:   cpumask for the parallel workers.

`cbcpu`
:   cpumask for the serial (callback) workers.

struct parallel\_data
:   Internal control structure, covers everything that depends on the cpumask in use.

**Definition**:

```
struct parallel_data {
    struct padata_shell             *ps;
    struct padata_list              __percpu *reorder_list;
    struct padata_serial_queue      __percpu *squeue;
    refcount_t refcnt;
    unsigned int                    seq_nr;
    unsigned int                    processed;
    int cpu;
    struct padata_cpumask           cpumask;
};
```

**Members**

`ps`
:   padata\_shell object.

`reorder_list`
:   percpu reorder lists

`squeue`
:   percpu padata queues used for serialuzation.

`refcnt`
:   Number of objects holding a reference on this parallel\_data.

`seq_nr`
:   Sequence number of the parallelized data object.

`processed`
:   Number of already processed objects.

`cpu`
:   Next CPU to be processed.

`cpumask`
:   The cpumasks in use for parallel and serial workers.

struct padata\_shell
:   Wrapper around [`struct parallel_data`](#c.parallel_data "parallel_data"), its purpose is to allow the underlying control structure to be replaced on the fly using RCU.

**Definition**:

```
struct padata_shell {
    struct padata_instance          *pinst;
    struct parallel_data *pd;
    struct parallel_data            *opd;
    struct list_head                list;
};
```

**Members**

`pinst`
:   padat instance.

`pd`
:   Actual parallel\_data structure which may be substituted on the fly.

`opd`
:   Pointer to old pd to be freed by padata\_replace.

`list`
:   List entry in padata\_instance list.

struct padata\_mt\_job
:   represents one multithreaded job

**Definition**:

```
struct padata_mt_job {
    void (*thread_fn)(unsigned long start, unsigned long end, void *arg);
    void *fn_arg;
    unsigned long           start;
    unsigned long           size;
    unsigned long           align;
    unsigned long           min_chunk;
    int max_threads;
    bool numa_aware;
};
```

**Members**

`thread_fn`
:   Called for each chunk of work that a padata thread does.

`fn_arg`
:   The thread function argument.

`start`
:   The start of the job (units are job-specific).

`size`
:   size of this node’s work (units are job-specific).

`align`
:   Ranges passed to the thread function fall on this boundary, with the
    possible exceptions of the beginning and end of the job.

`min_chunk`
:   The minimum chunk size in job-specific units. This allows
    the client to communicate the minimum amount of work that’s
    appropriate for one worker thread to do at once.

`max_threads`
:   Max threads to use for the job, actual number may be less
    depending on task size and minimum chunk size.

`numa_aware`
:   Distribute jobs to different nodes with CPU in a round robin fashion.

struct padata\_instance
:   The overall control structure.

**Definition**:

```
struct padata_instance {
    struct hlist_node               cpuhp_node;
    struct workqueue_struct         *parallel_wq;
    struct workqueue_struct         *serial_wq;
    struct list_head                pslist;
    struct padata_cpumask           cpumask;
    cpumask_var_t validate_cpumask;
    struct kobject                   kobj;
    struct mutex                     lock;
    u8 flags;
#define PADATA_INIT     1;
#define PADATA_RESET    2;
#define PADATA_INVALID  4;
};
```

**Members**

`cpuhp_node`
:   Linkage for CPU hotplug callbacks.

`parallel_wq`
:   The workqueue used for parallel work.

`serial_wq`
:   The workqueue used for serial work.

`pslist`
:   List of padata\_shell objects attached to this instance.

`cpumask`
:   User supplied cpumasks for parallel and serial works.

`validate_cpumask`
:   Internal cpumask used to validate **cpumask** during hotplug.

`kobj`
:   padata instance kernel object.

`lock`
:   padata instance lock.

`flags`
:   padata flags.

int padata\_do\_parallel(struct [padata\_shell](#c.padata_shell "padata_shell") \*ps, struct [padata\_priv](#c.padata_priv "padata_priv") \*padata, int \*cb\_cpu)
:   padata parallelization function

**Parameters**

`struct padata_shell *ps`
:   padatashell

`struct padata_priv *padata`
:   object to be parallelized

`int *cb_cpu`
:   pointer to the CPU that the serialization callback function should
    run on. If it’s not in the serial cpumask of **pinst**
    (i.e. cpumask.cbcpu), this function selects a fallback CPU and if
    none found, returns -EINVAL.

**Description**

The parallelization callback function will run with BHs off.

**Note**

Every object which is parallelized by padata\_do\_parallel
must be seen by padata\_do\_serial.

**Return**

0 on success or else negative error code.

void padata\_do\_serial(struct [padata\_priv](#c.padata_priv "padata_priv") \*padata)
:   padata serialization function

**Parameters**

`struct padata_priv *padata`
:   object to be serialized.

**Description**

padata\_do\_serial must be called for every parallelized object.
The serialization callback function will run with BHs off.

void padata\_do\_multithreaded(struct [padata\_mt\_job](#c.padata_mt_job "padata_mt_job") \*job)
:   run a multithreaded job

**Parameters**

`struct padata_mt_job *job`
:   Description of the job.

**Description**

See the definition of [`struct padata_mt_job`](#c.padata_mt_job "padata_mt_job") for more details.

int padata\_set\_cpumask(struct [padata\_instance](#c.padata_instance "padata_instance") \*pinst, int cpumask\_type, cpumask\_var\_t cpumask)
:   Sets specified by **cpumask\_type** cpumask to the value equivalent to **cpumask**.

**Parameters**

`struct padata_instance *pinst`
:   padata instance

`int cpumask_type`
:   PADATA\_CPU\_SERIAL or PADATA\_CPU\_PARALLEL corresponding
    to parallel and serial cpumasks respectively.

`cpumask_var_t cpumask`
:   the cpumask to use

**Return**

0 on success or negative error code

struct [padata\_instance](#c.padata_instance "padata_instance") \*padata\_alloc(const char \*name)
:   allocate and initialize a padata instance

**Parameters**

`const char *name`
:   used to identify the instance

**Return**

new instance on success, NULL on error

void padata\_free(struct [padata\_instance](#c.padata_instance "padata_instance") \*pinst)
:   free a padata instance

**Parameters**

`struct padata_instance *pinst`
:   padata instance to free

struct [padata\_shell](#c.padata_shell "padata_shell") \*padata\_alloc\_shell(struct [padata\_instance](#c.padata_instance "padata_instance") \*pinst)
:   Allocate and initialize padata shell.

**Parameters**

`struct padata_instance *pinst`
:   Parent padata\_instance object.

**Return**

new shell on success, NULL on error

void padata\_free\_shell(struct [padata\_shell](#c.padata_shell "padata_shell") \*ps)
:   free a padata shell

**Parameters**

`struct padata_shell *ps`
:   padata shell to free
