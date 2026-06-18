# Using Propeller with the Linux kernel

> 출처(원문): https://docs.kernel.org/dev-tools/propeller.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Using Propeller with the Linux kernel

This enables Propeller build support for the kernel when using Clang
compiler. Propeller is a profile-guided optimization (PGO) method used
to optimize binary executables. Like AutoFDO, it utilizes hardware
sampling to gather information about the frequency of execution of
different code paths within a binary. Unlike AutoFDO, this information
is then used right before linking phase to optimize (among others)
block layout within and across functions.

A few important notes about adopting Propeller optimization:

1. Although it can be used as a standalone optimization step, it is
   strongly recommended to apply Propeller on top of AutoFDO,
   AutoFDO+ThinLTO or Instrument FDO. The rest of this document
   assumes this paradigm.
2. Propeller uses another round of profiling on top of
   AutoFDO/AutoFDO+ThinLTO/iFDO. The whole build process involves
   “build-afdo - train-afdo - build-propeller - train-propeller -
   build-optimized”.
3. Propeller requires LLVM 19 release or later for Clang/Clang++
   and the linker(ld.lld).
4. In addition to LLVM toolchain, Propeller requires a profiling
   conversion tool: <https://github.com/google/autofdo> with a release
   after v0.30.1: <https://github.com/google/autofdo/releases/tag/v0.30.1>.

The Propeller optimization process involves the following steps:

1. Initial building: Build the AutoFDO or AutoFDO+ThinLTO binary as
   you would normally do, but with a set of compile-time / link-time
   flags, so that a special metadata section is created within the
   kernel binary. The special section is only intend to be used by the
   profiling tool, it is not part of the runtime image, nor does it
   change kernel run time text sections.
2. Profiling: The above kernel is then run with a representative
   workload to gather execution frequency data. This data is collected
   using hardware sampling, via perf. Propeller is most effective on
   platforms supporting advanced PMU features like LBR on Intel
   machines. This step is the same as profiling the kernel for AutoFDO
   (the exact perf parameters can be different).
3. Propeller profile generation: Perf output file is converted to a
   pair of Propeller profiles via an offline tool.
4. Optimized build: Build the AutoFDO or AutoFDO+ThinLTO optimized
   binary as you would normally do, but with a compile-time /
   link-time flag to pick up the Propeller compile time and link time
   profiles. This build step uses 3 profiles - the AutoFDO profile,
   the Propeller compile-time profile and the Propeller link-time
   profile.
5. Deployment: The optimized kernel binary is deployed and used
   in production environments, providing improved performance
   and reduced latency.

## Preparation

Configure the kernel with:

```
CONFIG_AUTOFDO_CLANG=y
CONFIG_PROPELLER_CLANG=y
```

## Customization

The default CONFIG\_PROPELLER\_CLANG setting covers kernel space objects
for Propeller builds. One can, however, enable or disable Propeller build
for individual files and directories by adding a line similar to the
following to the respective kernel Makefile:

* For enabling a single file (e.g. foo.o):

  ```
  PROPELLER_PROFILE_foo.o := y
  ```
* For enabling all files in one directory:

  ```
  PROPELLER_PROFILE := y
  ```
* For disabling one file:

  ```
  PROPELLER_PROFILE_foo.o := n
  ```
* For disabling all files in one directory:

  ```
  PROPELLER__PROFILE := n
  ```

## Workflow

Here is an example workflow for building an AutoFDO+Propeller kernel:

1. Assuming an AutoFDO profile is already collected following
   instructions in the AutoFDO document, build the kernel on the host
   machine, with AutoFDO and Propeller build configs

   ```
   CONFIG_AUTOFDO_CLANG=y
   CONFIG_PROPELLER_CLANG=y
   ```

   and

   ```
   $ make LLVM=1 CLANG_AUTOFDO_PROFILE=<autofdo-profile-name>
   ```
2. Install the kernel on the test machine.
3. Run the load tests. The ‘-c’ option in perf specifies the sample
   event period. We suggest using a suitable prime number, like 500009,
   for this purpose.

   * For Intel platforms:

     ```
     $ perf record -e BR_INST_RETIRED.NEAR_TAKEN:k -a -N -b -c <count> -o <perf_file> -- <loadtest>
     ```
   * For AMD platforms:

     ```
     $ perf record --pfm-event RETIRED_TAKEN_BRANCH_INSTRUCTIONS:k -a -N -b -c <count> -o <perf_file> -- <loadtest>
     ```

   Note you can repeat the above steps to collect multiple <perf\_file>s.
4. (Optional) Download the raw perf file(s) to the host machine.
5. Use the create\_llvm\_prof tool (<https://github.com/google/autofdo>) to
   generate Propeller profile.

   ```
   $ create_llvm_prof --binary=<vmlinux> --profile=<perf_file>
                      --format=propeller --propeller_output_module_name
                      --out=<propeller_profile_prefix>_cc_profile.txt
                      --propeller_symorder=<propeller_profile_prefix>_ld_profile.txt
   ```

   “<propeller\_profile\_prefix>” can be something like “/home/user/dir/any\_string”.

   This command generates a pair of Propeller profiles:
   “<propeller\_profile\_prefix>\_cc\_profile.txt” and
   “<propeller\_profile\_prefix>\_ld\_profile.txt”.

   If there are more than 1 perf\_file collected in the previous step,
   you can create a temp list file “<perf\_file\_list>” with each line
   containing one perf file name and run:

   ```
   $ create_llvm_prof --binary=<vmlinux> --profile=@<perf_file_list>
                      --format=propeller --propeller_output_module_name
                      --out=<propeller_profile_prefix>_cc_profile.txt
                      --propeller_symorder=<propeller_profile_prefix>_ld_profile.txt
   ```
6. Rebuild the kernel using the AutoFDO and Propeller
   profiles.

   ```
   CONFIG_AUTOFDO_CLANG=y
   CONFIG_PROPELLER_CLANG=y
   ```

   and

   ```
   $ make LLVM=1 CLANG_AUTOFDO_PROFILE=<profile_file> CLANG_PROPELLER_PROFILE_PREFIX=<propeller_profile_prefix>
   ```
