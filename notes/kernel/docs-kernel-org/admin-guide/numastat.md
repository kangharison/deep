# Numa policy hit/miss statistics

> 출처(원문): https://docs.kernel.org/admin-guide/numastat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Numa policy hit/miss statistics

/sys/devices/system/node/node\*/numastat

All units are pages. Hugepages have separate counters.

The numa\_hit, numa\_miss and numa\_foreign counters reflect how well processes
are able to allocate memory from nodes they prefer. If they succeed, numa\_hit
is incremented on the preferred node, otherwise numa\_foreign is incremented on
the preferred node and numa\_miss on the node where allocation succeeded.

Usually preferred node is the one local to the CPU where the process executes,
but restrictions such as mempolicies can change that, so there are also two
counters based on CPU local node. local\_node is similar to numa\_hit and is
incremented on allocation from a node by CPU on the same node. other\_node is
similar to numa\_miss and is incremented on the node where allocation succeeds
from a CPU from a different node. Note there is no counter analogical to
numa\_foreign.

In more detail:

|  |  |
| --- | --- |
| numa\_hit | A process wanted to allocate memory from this node, and succeeded. |
| numa\_miss | A process wanted to allocate memory from another node, but ended up with memory from this node. |
| numa\_foreign | A process wanted to allocate on this node, but ended up with memory from another node. |
| local\_node | A process ran on this node’s CPU, and got memory from this node. |
| other\_node | A process ran on a different node’s CPU and got memory from this node. |
| interleave\_hit | Interleaving wanted to allocate from this node and succeeded. |

For easier reading you can use the numastat utility from the numactl package
(<http://oss.sgi.com/projects/libnuma/>). Note that it only works
well right now on machines with a small number of CPUs.

Note that on systems with memoryless nodes (where a node has CPUs but no
memory) the numa\_hit, numa\_miss and numa\_foreign statistics can be skewed
heavily. In the current kernel implementation, if a process prefers a
memoryless node (i.e. because it is running on one of its local CPU), the
implementation actually treats one of the nearest nodes with memory as the
preferred node. As a result, such allocation will not increase the numa\_foreign
counter on the memoryless node, and will skew the numa\_hit, numa\_miss and
numa\_foreign statistics of the nearest node.
