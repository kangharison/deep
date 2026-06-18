# Job server module

> 출처(원문): https://docs.kernel.org/tools/jobserver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Job server module

Interacts with the POSIX jobserver during the Kernel build time.

A “normal” jobserver task, like the one initiated by a make subprocess would do:

> * open read/write file descriptors to communicate with the job server;
> * ask for one slot by calling:
>
>   ```
>   claim = os.read(reader, 1)
>   ```
> * when the job finishes, call:
>
>   ```
>   os.write(writer, b"+")  # os.write(writer, claim)
>   ```

Here, the goal is different: This script aims to get the remaining number
of slots available, using all of them to run a command which handle tasks in
parallel. To to that, it has a loop that ends only after there are no
slots left. It then increments the number by one, in order to allow a
call equivalent to `make -j$((claim+1))`, e.g. having a parent make creating
$claim child to do the actual work.

The end goal here is to keep the total number of build tasks under the
limit established by the initial `make -j$n_proc` call.

See:
:   <https://www.gnu.org/software/make/manual/html_node/POSIX-Jobserver.html#POSIX-Jobserver>

*class* lib.python.jobserver.JobserverExec
:   Bases: `object`

    Claim all slots from make using POSIX Jobserver.

    The main methods here are:

    * open(): reserves all slots;
    * close(): method returns all used slots back to make;
    * `run()`: executes a command setting PARALLELISM=<available slots jobs + 1>.

    close()
    :   Return all reserved slots to Jobserver.

    open()
    :   Reserve all available slots to be claimed later on.

    run(*cmd*, *\*args*, *\*\*pwargs*)
    :   Run a command setting PARALLELISM env variable to the number of
        available job slots (claim) + 1, e.g. it will reserve claim slots
        to do the actual build work, plus one to monitor its children.

lib.python.jobserver.warn(*text*, *\*args*)
