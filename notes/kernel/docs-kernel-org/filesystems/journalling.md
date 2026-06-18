# The Linux Journalling API

> 출처(원문): https://docs.kernel.org/filesystems/journalling.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux Journalling API

## Overview

### Details

The journalling layer is easy to use. You need to first of all create a
journal\_t data structure. There are two calls to do this dependent on
how you decide to allocate the physical media on which the journal
resides. The [`jbd2_journal_init_inode()`](#c.jbd2_journal_init_inode "jbd2_journal_init_inode") call is for journals stored in
filesystem inodes, or the [`jbd2_journal_init_dev()`](#c.jbd2_journal_init_dev "jbd2_journal_init_dev") call can be used
for journal stored on a raw device (in a continuous range of blocks). A
journal\_t is a typedef for a `struct pointer`, so when you are finally
finished make sure you call [`jbd2_journal_destroy()`](#c.jbd2_journal_destroy "jbd2_journal_destroy") on it to free up
any used kernel memory.

Once you have got your journal\_t object you need to ‘mount’ or load the
journal file. The journalling layer expects the space for the journal
was already allocated and initialized properly by the userspace tools.
When loading the journal you must call [`jbd2_journal_load()`](#c.jbd2_journal_load "jbd2_journal_load") to process
journal contents. If the client file system detects the journal contents
does not need to be processed (or even need not have valid contents), it
may call [`jbd2_journal_wipe()`](#c.jbd2_journal_wipe "jbd2_journal_wipe") to clear the journal contents before
calling [`jbd2_journal_load()`](#c.jbd2_journal_load "jbd2_journal_load").

Note that jbd2\_journal\_wipe(..,0) calls
[`jbd2_journal_skip_recovery()`](#c.jbd2_journal_skip_recovery "jbd2_journal_skip_recovery") for you if it detects any outstanding
transactions in the journal and similarly [`jbd2_journal_load()`](#c.jbd2_journal_load "jbd2_journal_load") will
call [`jbd2_journal_recover()`](#c.jbd2_journal_recover "jbd2_journal_recover") if necessary. I would advise reading
`ext4_load_journal()` in fs/ext4/super.c for examples on this stage.

Now you can go ahead and start modifying the underlying filesystem.
Almost.

You still need to actually journal your filesystem changes, this is done
by wrapping them into transactions. Additionally you also need to wrap
the modification of each of the buffers with calls to the journal layer,
so it knows what the modifications you are actually making are. To do
this use [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") which returns a transaction handle.

[`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") and its counterpart [`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop"),
which indicates the end of a transaction are nestable calls, so you can
reenter a transaction if necessary, but remember you must call
[`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop") the same number of times as
[`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") before the transaction is completed (or more
accurately leaves the update phase). Ext4/VFS makes use of this feature to
simplify handling of inode dirtying, quota support, etc.

Inside each transaction you need to wrap the modifications to the
individual buffers (blocks). Before you start to modify a buffer you
need to call [`jbd2_journal_get_create_access()`](#c.jbd2_journal_get_create_access "jbd2_journal_get_create_access") /
[`jbd2_journal_get_write_access()`](#c.jbd2_journal_get_write_access "jbd2_journal_get_write_access") /
[`jbd2_journal_get_undo_access()`](#c.jbd2_journal_get_undo_access "jbd2_journal_get_undo_access") as appropriate, this allows the
journalling layer to copy the unmodified
data if it needs to. After all the buffer may be part of a previously
uncommitted transaction. At this point you are at last ready to modify a
buffer, and once you are have done so you need to call
[`jbd2_journal_dirty_metadata()`](#c.jbd2_journal_dirty_metadata "jbd2_journal_dirty_metadata"). Or if you’ve asked for access to a
buffer you now know is now longer required to be pushed back on the
device you can call [`jbd2_journal_forget()`](#c.jbd2_journal_forget "jbd2_journal_forget") in much the same way as you
might have used [`bforget()`](buffer.html#c.bforget "bforget") in the past.

A [`jbd2_journal_flush()`](#c.jbd2_journal_flush "jbd2_journal_flush") may be called at any time to commit and
checkpoint all your transactions.

Then at umount time , in your `put_super()` you can then call
[`jbd2_journal_destroy()`](#c.jbd2_journal_destroy "jbd2_journal_destroy") to clean up your in-core journal object.

Unfortunately there a couple of ways the journal layer can cause a
deadlock. The first thing to note is that each task can only have a
single outstanding transaction at any one time, remember nothing commits
until the outermost [`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop"). This means you must complete
the transaction at the end of each file/inode/address etc. operation you
perform, so that the journalling system isn’t re-entered on another
journal. Since transactions can’t be nested/batched across differing
journals, and another filesystem other than yours (say ext4) may be
modified in a later syscall.

The second case to bear in mind is that [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") can block
if there isn’t enough space in the journal for your transaction (based
on the passed nblocks param) - when it blocks it merely(!) needs to wait
for transactions to complete and be committed from other tasks, so
essentially we are waiting for [`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop"). So to avoid
deadlocks you must treat [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") /
[`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop") as if they were semaphores and include them in
your semaphore ordering rules to prevent
deadlocks. Note that [`jbd2_journal_extend()`](#c.jbd2_journal_extend "jbd2_journal_extend") has similar blocking
behaviour to [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") so you can deadlock here just as
easily as on [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start").

Try to reserve the right number of blocks the first time. ;-). This will
be the maximum number of blocks you are going to touch in this
transaction. I advise having a look at at least ext4\_jbd.h to see the
basis on which ext4 uses to make these decisions.

Another wriggle to watch out for is your on-disk block allocation
strategy. Why? Because, if you do a delete, you need to ensure you
haven’t reused any of the freed blocks until the transaction freeing
these blocks commits. If you reused these blocks and crash happens,
there is no way to restore the contents of the reallocated blocks at the
end of the last fully committed transaction. One simple way of doing
this is to mark blocks as free in internal in-memory block allocation
structures only after the transaction freeing them commits. Ext4 uses
journal commit callback for this purpose.

With journal commit callbacks you can ask the journalling layer to call
a callback function when the transaction is finally committed to disk,
so that you can do some of your own management. You ask the journalling
layer for calling the callback by simply setting
`journal->j_commit_callback` function pointer and that function is
called after each transaction commit.

JBD2 also provides a way to block all transaction updates via
[`jbd2_journal_lock_updates()`](#c.jbd2_journal_lock_updates "jbd2_journal_lock_updates") /
[`jbd2_journal_unlock_updates()`](#c.jbd2_journal_unlock_updates "jbd2_journal_unlock_updates"). Ext4 uses this when it wants a
window with a clean and stable fs for a moment. E.g.

```
jbd2_journal_lock_updates() //stop new stuff happening..
jbd2_journal_flush()        // checkpoint everything.
..do stuff on stable fs
jbd2_journal_unlock_updates() // carry on with filesystem use.
```

The opportunities for abuse and DOS attacks with this should be obvious,
if you allow unprivileged userspace to trigger codepaths containing
these calls.

### Fast commits

JBD2 to also allows you to perform file-system specific delta commits known as
fast commits. In order to use fast commits, you will need to set following
callbacks that perform corresponding work:

journal->j\_fc\_cleanup\_cb: Cleanup function called after every full commit and
fast commit.

journal->j\_fc\_replay\_cb: Replay function called for replay of fast commit
blocks.

File system is free to perform fast commits as and when it wants as long as it
gets permission from JBD2 to do so by calling the function
`jbd2_fc_begin_commit()`. Once a fast commit is done, the client
file system should tell JBD2 about it by calling
`jbd2_fc_end_commit()`. If the file system wants JBD2 to perform a full
commit immediately after stopping the fast commit it can do so by calling
`jbd2_fc_end_commit_fallback()`. This is useful if fast commit operation
fails for some reason and the only way to guarantee consistency is for JBD2 to
perform the full traditional commit.

JBD2 helper functions to manage fast commit buffers. File system can use
`jbd2_fc_get_buf()` and `jbd2_fc_wait_bufs()` to allocate
and wait on IO completion of fast commit buffers.

Currently, only Ext4 implements fast commits. For details of its implementation
of fast commits, please refer to the top level comments in
fs/ext4/fast\_commit.c.

### Summary

Using the journal is a matter of wrapping the different context changes,
being each mount, each modification (transaction) and each changed
buffer to tell the journalling layer about them.

## Data Types

The journalling layer uses typedefs to ‘hide’ the concrete definitions
of the structures used. As a client of the JBD2 layer you can just rely
on the using the pointer as a magic cookie of some sort. Obviously the
hiding is not enforced as this is ‘C’.

### Structures

type handle\_t
:   The handle\_t type represents a single atomic update being performed by some process.

**Description**

All filesystem modifications made by the process go
through this handle. Recursive operations (such as quota operations)
are gathered into a single update.

The buffer credits field is used to account for journaled buffers
being modified by the running process. To ensure that there is
enough log space for all outstanding operations, we need to limit the
number of outstanding buffers possible at any time. When the
operation completes, any buffer credits not used are credited back to
the transaction, so that at all times we know how many buffers the
outstanding updates on a transaction might possibly touch.

This is an opaque datatype.

type journal\_t
:   The journal\_t maintains all of the journaling state information for a single filesystem.

**Description**

journal\_t is linked to from the fs superblock structure.

We use the journal\_t to keep track of all outstanding transaction
activity on the filesystem, and to manage the state of the log
writing process.

This is an opaque datatype.

struct jbd2\_inode
:   The jbd\_inode type is the structure linking inodes in ordered mode present in a transaction so that we can sync them during commit.

**Definition**:

```
struct jbd2_inode {
    transaction_t *i_transaction;
    transaction_t *i_next_transaction;
    struct list_head i_list;
    struct inode *i_vfs_inode;
    unsigned long i_flags;
    pgoff_t i_dirty_start_page;
    pgoff_t i_dirty_end_page;
};
```

**Members**

`i_transaction`
:   Which transaction does this inode belong to? Either the running
    transaction or the committing one. [j\_list\_lock]

`i_next_transaction`
:   Pointer to the running transaction modifying inode’s data in case
    there is already a committing transaction touching it. [j\_list\_lock]

`i_list`
:   List of inodes in the i\_transaction [j\_list\_lock]

`i_vfs_inode`
:   VFS inode this inode belongs to [constant for lifetime of structure]

`i_flags`
:   Flags of inode [j\_list\_lock]

`i_dirty_start_page`
:   Dirty range start in PAGE\_SIZE units.

    The dirty range is empty if **i\_dirty\_start\_page** is greater than or
    equal to **i\_dirty\_end\_page**.

    [j\_list\_lock]

`i_dirty_end_page`
:   Dirty range end in PAGE\_SIZE units (exclusive).

    [j\_list\_lock]

struct jbd2\_journal\_handle
:   The jbd2\_journal\_handle type is the concrete type associated with handle\_t.

**Definition**:

```
struct jbd2_journal_handle {
    union {
        transaction_t *h_transaction;
        journal_t *h_journal;
    };
    handle_t *h_rsv_handle;
    int h_total_credits;
    int h_revoke_credits;
    int h_revoke_credits_requested;
    int h_ref;
    int h_err;
    unsigned int    h_sync:         1;
    unsigned int    h_reserved:     1;
    unsigned int    h_aborted:      1;
    unsigned int    h_type:         8;
    unsigned int    h_line_no:      16;
    unsigned long           h_start_jiffies;
    unsigned int            h_requested_credits;
    unsigned int            saved_alloc_context;
};
```

**Members**

`{unnamed_union}`
:   anonymous

`h_transaction`
:   Which compound transaction is this update a part of?

`h_journal`
:   Which journal handle belongs to - used iff h\_reserved set.

`h_rsv_handle`
:   Handle reserved for finishing the logical operation.

`h_total_credits`
:   Number of remaining buffers we are allowed to add to
    journal. These are dirty buffers and revoke descriptor blocks.

`h_revoke_credits`
:   Number of remaining revoke records available for handle

`h_revoke_credits_requested`
:   Holds **h\_revoke\_credits** after handle is started.

`h_ref`
:   Reference count on this handle.

`h_err`
:   Field for caller’s use to track errors through large fs operations.

`h_sync`
:   Flag for sync-on-close.

`h_reserved`
:   Flag for handle for reserved credits.

`h_aborted`
:   Flag indicating fatal error on handle.

`h_type`
:   For handle statistics.

`h_line_no`
:   For handle statistics.

`h_start_jiffies`
:   Handle Start time.

`h_requested_credits`
:   Holds **h\_total\_credits** after handle is started.

`saved_alloc_context`
:   Saved context while transaction is open.

struct journal\_s
:   The journal\_s type is the concrete type associated with journal\_t.

**Definition**:

```
struct journal_s {
    unsigned long           j_flags;
    int j_errno;
    struct mutex            j_abort_mutex;
    struct buffer_head      *j_sb_buffer;
    journal_superblock_t *j_superblock;
    rwlock_t j_state_lock;
    int j_barrier_count;
    struct mutex            j_barrier;
    transaction_t *j_running_transaction;
    transaction_t *j_committing_transaction;
    transaction_t *j_checkpoint_transactions;
    wait_queue_head_t j_wait_transaction_locked;
    wait_queue_head_t j_wait_done_commit;
    wait_queue_head_t j_wait_commit;
    wait_queue_head_t j_wait_updates;
    wait_queue_head_t j_wait_reserved;
    wait_queue_head_t j_fc_wait;
    struct mutex            j_checkpoint_mutex;
    struct buffer_head      *j_chkpt_bhs[JBD2_NR_BATCH];
    struct shrinker         *j_shrinker;
    struct percpu_counter   j_checkpoint_jh_count;
    transaction_t *j_shrink_transaction;
    unsigned long           j_head;
    unsigned long           j_tail;
    unsigned long           j_free;
    unsigned long           j_first;
    unsigned long           j_last;
    unsigned long           j_fc_first;
    unsigned long           j_fc_off;
    unsigned long           j_fc_last;
    struct block_device     *j_dev;
    int j_blocksize;
    unsigned long long      j_blk_offset;
    char j_devname[BDEVNAME_SIZE+24];
    struct block_device     *j_fs_dev;
    errseq_t j_fs_dev_wb_err;
    unsigned int            j_total_len;
    atomic_t j_reserved_credits;
    spinlock_t j_list_lock;
    struct inode            *j_inode;
    tid_t j_tail_sequence;
    tid_t j_transaction_sequence;
    tid_t j_commit_sequence;
    tid_t j_commit_request;
    __u8 j_uuid[16];
    struct task_struct      *j_task;
    int j_max_transaction_buffers;
    int j_revoke_records_per_block;
    int j_transaction_overhead_buffers;
    unsigned long           j_commit_interval;
    struct timer_list       j_commit_timer;
    spinlock_t j_revoke_lock;
    struct jbd2_revoke_table_s *j_revoke;
    struct jbd2_revoke_table_s *j_revoke_table[2];
    struct buffer_head      **j_wbuf;
    struct buffer_head      **j_fc_wbuf;
    int j_wbufsize;
    int j_fc_wbufsize;
    pid_t j_last_sync_writer;
    u64 j_average_commit_time;
    u32 j_min_batch_time;
    u32 j_max_batch_time;
    void (*j_commit_callback)(journal_t *, transaction_t *);
    int (*j_submit_inode_data_buffers) (struct jbd2_inode *);
    int (*j_finish_inode_data_buffers) (struct jbd2_inode *);
    spinlock_t j_history_lock;
    struct proc_dir_entry   *j_proc_entry;
    struct transaction_stats_s j_stats;
    unsigned int            j_failed_commit;
    void *j_private;
    __u32 j_csum_seed;
#ifdef CONFIG_DEBUG_LOCK_ALLOC;
    struct lockdep_map      j_trans_commit_map;
#endif;
    struct lock_class_key   jbd2_trans_commit_key;
    void (*j_fc_cleanup_callback)(struct journal_s *journal, int full, tid_t tid);
    int (*j_fc_replay_callback)(struct journal_s *journal, struct buffer_head *bh, enum passtype pass, int off, tid_t expected_commit_id);
    int (*j_bmap)(struct journal_s *journal, sector_t *block);
};
```

**Members**

`j_flags`
:   General journaling state flags [j\_state\_lock,
    no lock for quick racy checks]

`j_errno`
:   Is there an outstanding uncleared error on the journal (from a prior
    abort)? [j\_state\_lock]

`j_abort_mutex`
:   Lock the whole aborting procedure.

`j_sb_buffer`
:   The first part of the superblock buffer.

`j_superblock`
:   The second part of the superblock buffer.

`j_state_lock`
:   Protect the various scalars in the journal.

`j_barrier_count`
:   Number of processes waiting to create a barrier lock [j\_state\_lock,
    no lock for quick racy checks]

`j_barrier`
:   The barrier lock itself.

`j_running_transaction`
:   Transactions: The current running transaction...
    [j\_state\_lock, no lock for quick racy checks] [caller holding
    open handle]

`j_committing_transaction`
:   the transaction we are pushing to disk
    [j\_state\_lock] [caller holding open handle]

`j_checkpoint_transactions`
:   ... and a linked circular list of all transactions waiting for
    checkpointing. [j\_list\_lock]

`j_wait_transaction_locked`
:   Wait queue for waiting for a locked transaction to start committing,
    or for a barrier lock to be released.

`j_wait_done_commit`
:   Wait queue for waiting for commit to complete.

`j_wait_commit`
:   Wait queue to trigger commit.

`j_wait_updates`
:   Wait queue to wait for updates to complete.

`j_wait_reserved`
:   Wait queue to wait for reserved buffer credits to drop.

`j_fc_wait`
:   Wait queue to wait for completion of async fast commits.

`j_checkpoint_mutex`
:   Semaphore for locking against concurrent checkpoints.

`j_chkpt_bhs`
:   List of buffer heads used by the checkpoint routine. This
    was moved from `jbd2_log_do_checkpoint()` to reduce stack
    usage. Access to this array is controlled by the
    **j\_checkpoint\_mutex**. [j\_checkpoint\_mutex]

`j_shrinker`
:   Journal head shrinker, reclaim buffer’s journal head which
    has been written back.

`j_checkpoint_jh_count`
:   Number of journal buffers on the checkpoint list. [j\_list\_lock]

`j_shrink_transaction`
:   Record next transaction will shrink on the checkpoint list.
    [j\_list\_lock]

`j_head`
:   Journal head: identifies the first unused block in the journal.
    [j\_state\_lock]

`j_tail`
:   Journal tail: identifies the oldest still-used block in the journal.
    [j\_state\_lock]

`j_free`
:   Journal free: how many free blocks are there in the journal?
    [j\_state\_lock]

`j_first`
:   The block number of the first usable block in the journal
    [j\_state\_lock].

`j_last`
:   The block number one beyond the last usable block in the journal
    [j\_state\_lock].

`j_fc_first`
:   The block number of the first fast commit block in the journal
    [j\_state\_lock].

`j_fc_off`
:   Number of fast commit blocks currently allocated. Accessed only
    during fast commit. Currently only process can do fast commit, so
    this field is not protected by any lock.

`j_fc_last`
:   The block number one beyond the last fast commit block in the journal
    [j\_state\_lock].

`j_dev`
:   Device where we store the journal.

`j_blocksize`
:   Block size for the location where we store the journal.

`j_blk_offset`
:   Starting block offset into the device where we store the journal.

`j_devname`
:   Journal device name.

`j_fs_dev`
:   Device which holds the client fs. For internal journal this will be
    equal to j\_dev.

`j_fs_dev_wb_err`
:   Records the errseq of the client fs’s backing block device.

`j_total_len`
:   Total maximum capacity of the journal region on disk.

`j_reserved_credits`
:   Number of buffers reserved from the running transaction.

`j_list_lock`
:   Protects the buffer lists and internal buffer state.

`j_inode`
:   Optional inode where we store the journal. If present, all
    journal block numbers are mapped into this inode via [`bmap()`](api-summary.html#c.bmap "bmap").

`j_tail_sequence`
:   Sequence number of the oldest transaction in the log [j\_state\_lock]

`j_transaction_sequence`
:   Sequence number of the next transaction to grant [j\_state\_lock]

`j_commit_sequence`
:   Sequence number of the most recently committed transaction
    [j\_state\_lock, no lock for quick racy checks]

`j_commit_request`
:   Sequence number of the most recent transaction wanting commit
    [j\_state\_lock, no lock for quick racy checks]

`j_uuid`
:   Journal uuid: identifies the object (filesystem, LVM volume etc)
    backed by this journal. This will eventually be replaced by an array
    of uuids, allowing us to index multiple devices within a single
    journal and to perform atomic updates across them.

`j_task`
:   Pointer to the current commit thread for this journal.

`j_max_transaction_buffers`
:   Maximum number of metadata buffers to allow in a single compound
    commit transaction.

`j_revoke_records_per_block`
:   Number of revoke records that fit in one descriptor block.

`j_transaction_overhead_buffers`
:   Number of blocks each transaction needs for its own bookkeeping

`j_commit_interval`
:   What is the maximum transaction lifetime before we begin a commit?

`j_commit_timer`
:   The timer used to wakeup the commit thread.

`j_revoke_lock`
:   Protect the revoke table.

`j_revoke`
:   The revoke table - maintains the list of revoked blocks in the
    current transaction.

`j_revoke_table`
:   Alternate revoke tables for j\_revoke.

`j_wbuf`
:   Array of bhs for jbd2\_journal\_commit\_transaction.

`j_fc_wbuf`
:   Array of fast commit bhs for fast commit. Accessed only
    during a fast commit. Currently only process can do fast commit, so
    this field is not protected by any lock.

`j_wbufsize`
:   Size of **j\_wbuf** array.

`j_fc_wbufsize`
:   Size of **j\_fc\_wbuf** array.

`j_last_sync_writer`
:   The pid of the last person to run a synchronous operation
    through the journal.

`j_average_commit_time`
:   The average amount of time in nanoseconds it takes to commit a
    transaction to disk. [j\_state\_lock]

`j_min_batch_time`
:   Minimum time that we should wait for additional filesystem operations
    to get batched into a synchronous handle in microseconds.

`j_max_batch_time`
:   Maximum time that we should wait for additional filesystem operations
    to get batched into a synchronous handle in microseconds.

`j_commit_callback`
:   This function is called when a transaction is closed.

`j_submit_inode_data_buffers`
:   This function is called for all inodes associated with the
    committing transaction marked with JI\_WRITE\_DATA flag
    before we start to write out the transaction to the journal.

`j_finish_inode_data_buffers`
:   This function is called for all inodes associated with the
    committing transaction marked with JI\_WAIT\_DATA flag
    after we have written the transaction to the journal
    but before we write out the commit block.

`j_history_lock`
:   Protect the transactions statistics history.

`j_proc_entry`
:   procfs entry for the jbd statistics directory.

`j_stats`
:   Overall statistics.

`j_failed_commit`
:   Failed journal commit ID.

`j_private`
:   An opaque pointer to fs-private information. ext3 puts its
    superblock pointer here.

`j_csum_seed`
:   Precomputed journal UUID checksum for seeding other checksums.

`j_trans_commit_map`
:   Lockdep entity to track transaction commit dependencies. Handles
    hold this “lock” for read, when we wait for commit, we acquire the
    “lock” for writing. This matches the properties of jbd2 journalling
    where the running transaction has to wait for all handles to be
    dropped to commit that transaction and also acquiring a handle may
    require transaction commit to finish.

`jbd2_trans_commit_key`
:   “`struct lock_class_key`” for **j\_trans\_commit\_map**

`j_fc_cleanup_callback`
:   Clean-up after fast commit or full commit. JBD2 calls this function
    after every commit operation.

`j_fc_replay_callback`
:   File-system specific function that performs replay of a fast
    commit. JBD2 calls this function for each fast commit block found in
    the journal. This function should return JBD2\_FC\_REPLAY\_CONTINUE
    to indicate that the block was processed correctly and more fast
    commit replay should continue. Return value of JBD2\_FC\_REPLAY\_STOP
    indicates the end of replay (no more blocks remaining). A negative
    return value indicates error.

`j_bmap`
:   Bmap function that should be used instead of the generic
    VFS bmap function.

## Functions

The functions here are split into two groups those that affect a journal
as a whole, and those which are used to manage transactions

### Journal Level

int jbd2\_journal\_force\_commit\_nested([journal\_t](#c.journal_t "journal_t") \*journal)
:   Force and wait upon a commit if the calling process is not within transaction.

**Parameters**

`journal_t *journal`
:   journal to force
    Returns true if progress was made.

**Description**

This is used for forcing out undo-protected data which contains
bitmaps, when the fs is running out of space.

int jbd2\_journal\_force\_commit([journal\_t](#c.journal_t "journal_t") \*journal)
:   force any uncommitted transactions

**Parameters**

`journal_t *journal`
:   journal to force

**Description**

Caller want unconditional commit. We can only force the running transaction
if we don’t have an active handle, otherwise, we will deadlock.

[journal\_t](#c.journal_t "journal_t") \*jbd2\_journal\_init\_dev(struct block\_device \*bdev, struct block\_device \*fs\_dev, unsigned long long start, int len, int blocksize)
:   creates and initialises a journal structure

**Parameters**

`struct block_device *bdev`
:   Block device on which to create the journal

`struct block_device *fs_dev`
:   Device which hold journalled filesystem for this journal.

`unsigned long long start`
:   Block nr Start of journal.

`int len`
:   Length of the journal in blocks.

`int blocksize`
:   blocksize of journalling device

**Return**

a newly created journal\_t \*

**Description**

> jbd2\_journal\_init\_dev creates a journal which maps a fixed contiguous
> range of blocks on an arbitrary block device.

[journal\_t](#c.journal_t "journal_t") \*jbd2\_journal\_init\_inode(struct [inode](#c.jbd2_journal_init_inode "inode") \*inode)
:   creates a journal which maps to a inode.

**Parameters**

`struct inode *inode`
:   An inode to create the journal in

**Description**

jbd2\_journal\_init\_inode creates a journal which maps an on-disk inode as
the journal. The inode must exist already, must support [`bmap()`](api-summary.html#c.bmap "bmap") and
must have all data blocks preallocated.

void jbd2\_journal\_update\_sb\_errno([journal\_t](#c.journal_t "journal_t") \*journal)
:   Update error in the journal.

**Parameters**

`journal_t *journal`
:   The journal to update.

**Description**

Update a journal’s errno. Write updated superblock to disk waiting for IO
to complete.

int jbd2\_journal\_load([journal\_t](#c.journal_t "journal_t") \*journal)
:   Read journal from disk.

**Parameters**

`journal_t *journal`
:   Journal to act on.

**Description**

Given a journal\_t structure which tells us which disk blocks contain
a journal, read the journal from disk to initialise the in-memory
structures.

int jbd2\_journal\_destroy([journal\_t](#c.journal_t "journal_t") \*journal)
:   Release a journal\_t structure.

**Parameters**

`journal_t *journal`
:   Journal to act on.

**Description**

Release a journal\_t structure once it is no longer in use by the
journaled object.
Return <0 if we couldn’t clean up the journal.

int jbd2\_journal\_check\_used\_features([journal\_t](#c.journal_t "journal_t") \*journal, unsigned long compat, unsigned long ro, unsigned long incompat)
:   Check if features specified are used.

**Parameters**

`journal_t *journal`
:   Journal to check.

`unsigned long compat`
:   bitmask of compatible features

`unsigned long ro`
:   bitmask of features that force read-only mount

`unsigned long incompat`
:   bitmask of incompatible features

**Description**

Check whether the journal uses all of a given set of
features. Return true (non-zero) if it does.

int jbd2\_journal\_check\_available\_features([journal\_t](#c.journal_t "journal_t") \*journal, unsigned long compat, unsigned long ro, unsigned long incompat)
:   Check feature set in journalling layer

**Parameters**

`journal_t *journal`
:   Journal to check.

`unsigned long compat`
:   bitmask of compatible features

`unsigned long ro`
:   bitmask of features that force read-only mount

`unsigned long incompat`
:   bitmask of incompatible features

**Description**

Check whether the journaling code supports the use of
all of a given set of features on this journal. Return true

int jbd2\_journal\_set\_features([journal\_t](#c.journal_t "journal_t") \*journal, unsigned long compat, unsigned long ro, unsigned long incompat)
:   Mark a given journal feature in the superblock

**Parameters**

`journal_t *journal`
:   Journal to act on.

`unsigned long compat`
:   bitmask of compatible features

`unsigned long ro`
:   bitmask of features that force read-only mount

`unsigned long incompat`
:   bitmask of incompatible features

**Description**

Mark a given journal feature as present on the
superblock. Returns true if the requested features could be set.

int jbd2\_journal\_flush([journal\_t](#c.journal_t "journal_t") \*journal, unsigned int flags)
:   Flush journal

**Parameters**

`journal_t *journal`
:   Journal to act on.

`unsigned int flags`
:   optional operation on the journal blocks after the flush (see below)

**Description**

Flush all data for a given journal to disk and empty the journal.
Filesystems can use this when remounting readonly to ensure that
recovery does not need to happen on remount. Optionally, a discard or zeroout
can be issued on the journal blocks after flushing.

flags:
:   JBD2\_JOURNAL\_FLUSH\_DISCARD: issues discards for the journal blocks
    JBD2\_JOURNAL\_FLUSH\_ZEROOUT: issues zeroouts for the journal blocks

int jbd2\_journal\_wipe([journal\_t](#c.journal_t "journal_t") \*journal, int write)
:   Wipe journal contents

**Parameters**

`journal_t *journal`
:   Journal to act on.

`int write`
:   flag (see below)

**Description**

Wipe out all of the contents of a journal, safely. This will produce
a warning if the journal contains any valid recovery information.
Must be called between journal\_init\_\*() and [`jbd2_journal_load()`](#c.jbd2_journal_load "jbd2_journal_load").

If ‘write’ is non-zero, then we wipe out the journal on disk; otherwise
we merely suppress recovery.

void jbd2\_journal\_abort([journal\_t](#c.journal_t "journal_t") \*journal, int errno)
:   Shutdown the journal immediately.

**Parameters**

`journal_t *journal`
:   the journal to shutdown.

`int errno`
:   an error number to record in the journal indicating
    the reason for the shutdown.

**Description**

Perform a complete, immediate shutdown of the ENTIRE
journal (not of a single transaction). This operation cannot be
undone without closing and reopening the journal.

The jbd2\_journal\_abort function is intended to support higher level error
recovery mechanisms such as the ext2/ext3 remount-readonly error
mode.

Journal abort has very specific semantics. Any existing dirty,
unjournaled buffers in the main filesystem will still be written to
disk by bdflush, but the journaling mechanism will be suspended
immediately and no further transaction commits will be honoured.

Any dirty, journaled buffers will be written back to disk without
hitting the journal. Atomicity cannot be guaranteed on an aborted
filesystem, but we \_do\_ attempt to leave as much data as possible
behind for fsck to use for cleanup.

Any attempt to get a new transaction handle on a journal which is in
ABORT state will just result in an -EROFS error return. A
jbd2\_journal\_stop on an existing handle will return -EIO if we have
entered abort state during the update.

Recursive transactions are not disturbed by journal abort until the
final jbd2\_journal\_stop, which will receive the -EIO error.

Finally, the jbd2\_journal\_abort call allows the caller to supply an errno
which will be recorded (if possible) in the journal superblock. This
allows a client to record failure conditions in the middle of a
transaction without having to complete the transaction to record the
failure to disk. ext3\_error, for example, now uses this
functionality.

int jbd2\_journal\_errno([journal\_t](#c.journal_t "journal_t") \*journal)
:   returns the journal’s error state.

**Parameters**

`journal_t *journal`
:   journal to examine.

**Description**

This is the errno number set with [`jbd2_journal_abort()`](#c.jbd2_journal_abort "jbd2_journal_abort"), the last
time the journal was mounted - if the journal was stopped
without calling abort this will be 0.

If the journal has been aborted on this mount time -EROFS will
be returned.

int jbd2\_journal\_clear\_err([journal\_t](#c.journal_t "journal_t") \*journal)
:   clears the journal’s error state

**Parameters**

`journal_t *journal`
:   journal to act on.

**Description**

An error must be cleared or acked to take a FS out of readonly
mode.

void jbd2\_journal\_ack\_err([journal\_t](#c.journal_t "journal_t") \*journal)
:   Ack journal err.

**Parameters**

`journal_t *journal`
:   journal to act on.

**Description**

An error must be cleared or acked to take a FS out of readonly
mode.

int jbd2\_journal\_recover([journal\_t](#c.journal_t "journal_t") \*journal)
:   recovers a on-disk journal

**Parameters**

`journal_t *journal`
:   the journal to recover

**Description**

The primary function for recovering the log contents when mounting a
journaled device.

Recovery is done in three passes. In the first pass, we look for the
end of the log. In the second, we assemble the list of revoke
blocks. In the third and final pass, we replay any un-revoked blocks
in the log.

int jbd2\_journal\_skip\_recovery([journal\_t](#c.journal_t "journal_t") \*journal)
:   Start journal and wipe exiting records

**Parameters**

`journal_t *journal`
:   journal to startup

**Description**

Locate any valid recovery information from the journal and set up the
journal structures in memory to ignore it (presumably because the
caller has evidence that it is out of date).
This function doesn’t appear to be exported..

We perform one pass over the journal to allow us to tell the user how
much recovery information is being erased, and to let us initialise
the journal transaction sequence numbers to the next unused ID.

### Transaction Level

[handle\_t](#c.handle_t "handle_t") \*jbd2\_journal\_start([journal\_t](#c.journal_t "journal_t") \*journal, int nblocks)
:   Obtain a new handle.

**Parameters**

`journal_t *journal`
:   Journal to start transaction on.

`int nblocks`
:   number of block buffer we might modify

**Description**

We make sure that the transaction can guarantee at least nblocks of
modified buffers in the log. We block until the log can guarantee
that much space. Additionally, if rsv\_blocks > 0, we also create another
handle with rsv\_blocks reserved blocks in the journal. This handle is
stored in h\_rsv\_handle. It is not attached to any particular transaction
and thus doesn’t block transaction commit. If the caller uses this reserved
handle, it has to set h\_rsv\_handle to NULL as otherwise [`jbd2_journal_stop()`](#c.jbd2_journal_stop "jbd2_journal_stop")
on the parent handle will dispose the reserved one. Reserved handle has to
be converted to a normal handle using [`jbd2_journal_start_reserved()`](#c.jbd2_journal_start_reserved "jbd2_journal_start_reserved") before
it can be used.

Return a pointer to a newly allocated handle, or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") value
on failure.

int jbd2\_journal\_start\_reserved([handle\_t](#c.handle_t "handle_t") \*handle, unsigned int type, unsigned int line\_no)
:   start reserved handle

**Parameters**

`handle_t *handle`
:   handle to start

`unsigned int type`
:   for handle statistics

`unsigned int line_no`
:   for handle statistics

**Description**

Start handle that has been previously reserved with `jbd2_journal_reserve()`.
This attaches **handle** to the running transaction (or creates one if there’s
not transaction running). Unlike [`jbd2_journal_start()`](#c.jbd2_journal_start "jbd2_journal_start") this function cannot
block on journal commit, checkpointing, or similar stuff. It can block on
memory allocation or frozen journal though.

Return 0 on success, non-zero on error - handle is freed in that case.

int jbd2\_journal\_extend([handle\_t](#c.handle_t "handle_t") \*handle, int nblocks, int revoke\_records)
:   extend buffer credits.

**Parameters**

`handle_t *handle`
:   handle to ‘extend’

`int nblocks`
:   nr blocks to try to extend by.

`int revoke_records`
:   number of revoke records to try to extend by.

**Description**

Some transactions, such as large extends and truncates, can be done
atomically all at once or in several stages. The operation requests
a credit for a number of buffer modifications in advance, but can
extend its credit if it needs more.

jbd2\_journal\_extend tries to give the running handle more buffer credits.
It does not guarantee that allocation - this is a best-effort only.
The calling process MUST be able to deal cleanly with a failure to
extend here.

Return 0 on success, non-zero on failure.

return code < 0 implies an error
return code > 0 implies normal transaction-full status.

int jbd2\_\_journal\_restart([handle\_t](#c.handle_t "handle_t") \*handle, int nblocks, int revoke\_records, gfp\_t gfp\_mask)
:   restart a handle .

**Parameters**

`handle_t *handle`
:   handle to restart

`int nblocks`
:   nr credits requested

`int revoke_records`
:   number of revoke record credits requested

`gfp_t gfp_mask`
:   memory allocation flags (for start\_this\_handle)

**Description**

Restart a handle for a multi-transaction filesystem
operation.

If the [`jbd2_journal_extend()`](#c.jbd2_journal_extend "jbd2_journal_extend") call above fails to grant new buffer credits
to a running handle, a call to jbd2\_journal\_restart will commit the
handle’s transaction so far and reattach the handle to a new
transaction capable of guaranteeing the requested number of
credits. We preserve reserved handle if there’s any attached to the
passed in handle.

void jbd2\_journal\_lock\_updates([journal\_t](#c.journal_t "journal_t") \*journal)
:   establish a transaction barrier.

**Parameters**

`journal_t *journal`
:   Journal to establish a barrier on.

**Description**

This locks out any further updates from being started, and blocks
until all existing updates have completed, returning only once the
journal is in a quiescent state with no updates running.

The journal lock should not be held on entry.

void jbd2\_journal\_unlock\_updates([journal\_t](#c.journal_t "journal_t") \*journal)
:   release barrier

**Parameters**

`journal_t *journal`
:   Journal to release the barrier on.

**Description**

Release a transaction barrier obtained with [`jbd2_journal_lock_updates()`](#c.jbd2_journal_lock_updates "jbd2_journal_lock_updates").

Should be called without the journal lock held.

int jbd2\_journal\_get\_write\_access([handle\_t](#c.handle_t "handle_t") \*handle, struct buffer\_head \*bh)
:   notify intent to modify a buffer for metadata (not data) update.

**Parameters**

`handle_t *handle`
:   transaction to add buffer modifications to

`struct buffer_head *bh`
:   bh to be used for metadata writes

**Return**

error code or 0 on success.

**Description**

In full data journalling mode the buffer may be of type BJ\_AsyncData,
because we’re `write()ing` a buffer which is also part of a shared mapping.

int jbd2\_journal\_get\_create\_access([handle\_t](#c.handle_t "handle_t") \*handle, struct buffer\_head \*bh)
:   notify intent to use newly created bh

**Parameters**

`handle_t *handle`
:   transaction to new buffer to

`struct buffer_head *bh`
:   new buffer.

**Description**

Call this if you create a new bh.

int jbd2\_journal\_get\_undo\_access([handle\_t](#c.handle_t "handle_t") \*handle, struct buffer\_head \*bh)
:   Notify intent to modify metadata with non-rewindable consequences

**Parameters**

`handle_t *handle`
:   transaction

`struct buffer_head *bh`
:   buffer to undo

**Description**

Sometimes there is a need to distinguish between metadata which has
been committed to disk and that which has not. The ext3fs code uses
this for freeing and allocating space, we have to make sure that we
do not reuse freed space until the deallocation has been committed,
since if we overwrote that space we would make the delete
un-rewindable in case of a crash.

To deal with that, jbd2\_journal\_get\_undo\_access requests write access to a
buffer for parts of non-rewindable operations such as delete
operations on the bitmaps. The journaling code must keep a copy of
the buffer’s contents prior to the undo\_access call until such time
as we know that the buffer has definitely been committed to disk.

We never need to know which transaction the committed data is part
of, buffers touched here are guaranteed to be dirtied later and so
will be committed to a new transaction in due course, at which point
we can discard the old committed data pointer.

Returns error number or 0 on success.

void jbd2\_journal\_set\_triggers(struct buffer\_head \*bh, struct jbd2\_buffer\_trigger\_type \*type)
:   Add triggers for commit writeout

**Parameters**

`struct buffer_head *bh`
:   buffer to trigger on

`struct jbd2_buffer_trigger_type *type`
:   `struct jbd2_buffer_trigger_type` containing the trigger(s).

**Description**

Set any triggers on this journal\_head. This is always safe, because
triggers for a committing buffer will be saved off, and triggers for
a running transaction will match the buffer in that transaction.

Call with NULL to clear the triggers.

int jbd2\_journal\_dirty\_metadata([handle\_t](#c.handle_t "handle_t") \*handle, struct buffer\_head \*bh)
:   mark a buffer as containing dirty metadata

**Parameters**

`handle_t *handle`
:   transaction to add buffer to.

`struct buffer_head *bh`
:   buffer to mark

**Description**

mark dirty metadata which needs to be journaled as part of the current
transaction.

The buffer must have previously had [`jbd2_journal_get_write_access()`](#c.jbd2_journal_get_write_access "jbd2_journal_get_write_access")
called so that it has a valid journal\_head attached to the buffer
head.

The buffer is placed on the transaction’s metadata list and is marked
as belonging to the transaction.

Returns error number or 0 on success.

Special care needs to be taken if the buffer already belongs to the
current committing transaction (in which case we should have frozen
data present for that commit). In that case, we don’t relink the
buffer: that only gets done when the old transaction finally
completes its commit.

int jbd2\_journal\_forget([handle\_t](#c.handle_t "handle_t") \*handle, struct buffer\_head \*bh)
:   [`bforget()`](buffer.html#c.bforget "bforget") for potentially-journaled buffers.

**Parameters**

`handle_t *handle`
:   transaction handle

`struct buffer_head *bh`
:   bh to ‘forget’

**Description**

We can only do the bforget if there are no commits pending against the
buffer. If the buffer is dirty in the current running transaction we
can safely unlink it.

bh may not be a journalled buffer at all - it may be a non-JBD
buffer which came off the hashtable. Check for this.

Decrements bh->b\_count by one.

Allow this call even if the handle has aborted --- it may be part of
the caller’s cleanup after an abort.

int jbd2\_journal\_stop([handle\_t](#c.handle_t "handle_t") \*handle)
:   complete a transaction

**Parameters**

`handle_t *handle`
:   transaction to complete.

**Description**

All done for a particular handle.

There is not much action needed here. We just return any remaining
buffer credits to the transaction and remove the handle. The only
complication is that we need to start a commit operation if the
filesystem is marked for synchronous update.

jbd2\_journal\_stop itself will not usually return an error, but it may
do so in unusual circumstances. In particular, expect it to
return -EIO if a jbd2\_journal\_abort has been executed since the
transaction began.

bool jbd2\_journal\_try\_to\_free\_buffers([journal\_t](#c.journal_t "journal_t") \*journal, struct [folio](#c.jbd2_journal_try_to_free_buffers "folio") \*folio)
:   try to free page buffers.

**Parameters**

`journal_t *journal`
:   journal for operation

`struct folio *folio`
:   Folio to detach data from.

**Description**

For all the buffers on this page,
if they are fully written out ordered data, move them onto BUF\_CLEAN
so [`try_to_free_buffers()`](buffer.html#c.try_to_free_buffers "try_to_free_buffers") can reap them.

This function returns non-zero if we wish [`try_to_free_buffers()`](buffer.html#c.try_to_free_buffers "try_to_free_buffers")
to be called. We do this if the page is releasable by [`try_to_free_buffers()`](buffer.html#c.try_to_free_buffers "try_to_free_buffers").
We also do it if the page has locked or dirty buffers and the caller wants
us to perform sync or async writeout.

This complicates JBD locking somewhat. We aren’t protected by the
BKL here. We wish to remove the buffer from its committing or
running transaction’s ->t\_datalist via \_\_jbd2\_journal\_unfile\_buffer.

This may *change* the value of transaction\_t->t\_datalist, so anyone
who looks at t\_datalist needs to lock against this function.

Even worse, someone may be doing a jbd2\_journal\_dirty\_data on this
buffer. So we need to lock against that. `jbd2_journal_dirty_data()`
will come out of the lock with the buffer dirty, which makes it
ineligible for release here.

Who else is affected by this? hmm... Really the only contender
is `do_get_write_access()` - it could be looking at the buffer while
`journal_try_to_free_buffer()` is changing its state. But that
cannot happen because we never reallocate freed data as metadata
while the data is part of a transaction. Yes?

Return false on failure, true on success

int jbd2\_journal\_invalidate\_folio([journal\_t](#c.journal_t "journal_t") \*journal, struct [folio](#c.jbd2_journal_invalidate_folio "folio") \*folio, size\_t offset, size\_t length)

**Parameters**

`journal_t *journal`
:   journal to use for flush...

`struct folio *folio`
:   folio to flush

`size_t offset`
:   start of the range to invalidate

`size_t length`
:   length of the range to invalidate

**Description**

Reap page buffers containing data after in the specified range in page.
Can return -EBUSY if buffers are part of the committing transaction and
the page is straddling i\_size. Caller then has to wait for current commit
and try again.

## See also

[Journaling the Linux ext2fs Filesystem, LinuxExpo 98, Stephen
Tweedie](http://kernel.org/pub/linux/kernel/people/sct/ext3/journal-design.ps.gz)

[Ext3 Journalling FileSystem, OLS 2000, Dr. Stephen
Tweedie](http://olstrans.sourceforge.net/release/OLS2000-ext3/OLS2000-ext3.html)
