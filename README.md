AIWC---a plugin for Oclgrind
============================

Oclgrind
--------

About
-----
This project implements a virtual OpenCL device simulator, including
an OpenCL runtime with ICD support. The goal is to provide a platform
for creating tools to aid OpenCL development. In particular, this
project currently implements utilities for debugging memory access
errors, detecting data-races and barrier divergence, collecting
instruction histograms, and for interactive OpenCL kernel debugging.
The simulator is built on an interpreter for LLVM IR. This project is
being developed by James Price and Simon McIntosh-Smith at the
University of Bristol.

Binary releases can be found on the GitHub releases page:

  https://github.com/jrprice/Oclgrind/releases


Build dependencies
------------------
To build this project, you will need LLVM and Clang 5.0 (or newer)
development libraries and headers. If you build LLVM from source, it
is recommended to enable optimizations to significantly improve the
performance of Oclgrind (set `CMAKE_BUILD_TYPE` to `Release` or
`RelWithDebInfo`).

You will need to use a compiler that supports C++11. Python should
also be available in order to run the test suite.

GNU readline (Debian package libreadline-dev) is required for command
history in the interactive debugger.


Building on Linux and OS X (CMake)
----------------------------------
The recommended method of building Oclgrind is via CMake.

When configuring the CMake build, you may be prompted to supply a
value for the `LLVM_DIR` parameter (this shouldn't be necessary if
LLVM is installed in a standard system location). This should be set
to the directory containing your LLVM installation's
`LLVMConfig.cmake` file (typically either
`${LLVM_ROOT}/lib/cmake/llvm` or `${LLVM_ROOT}/share/llvm/cmake/`).
If Clang is installed separately to LLVM, then you may also be
prompted to supply a path for the `CLANG_ROOT` parameter, which should
be the root of your Clang installation (containing the `bin/`, `lib/`
and `include/` directories).

A typical CMake command-line might look like this:

    cmake ${OCLGRIND_SOURCE} \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT} \
          -DLLVM_DIR=${LLVM_ROOT}/lib/cmake/llvm

where `${OCLGRIND_SOURCE}` is the path to the root directory
containing the Oclgrind source code, `${LLVM_ROOT}` is the path to the
LLVM installation, and `${INSTALL_ROOT}` is the desired installation
root directory (this can be omitted if installing to system
directories).

Next, build and install with make:

    make
    make test
    make install

If installing to a non-system location, you should add the `bin/`
directory to the `PATH` environment variable in order to make use of
the `oclgrind` command. If you wish to use Oclgrind via the OpenCL ICD
loader (optional), then you should create an ICD loading point by
copying the `oclgrind.icd` file from the build directory to
`/etc/OpenCL/vendors/`.


Building on Windows
-------------------
Building Oclgrind on Windows requires Visual Studio 2013 (or newer),
and Windows 7 (or newer). Compiling against recent versions of LLVM
may require Visual Studio 2015.

When configuring the CMake build, you may be prompted to supply a
value for the `LLVM_DIR` parameter. This should be set to the
directory containing your LLVM installation's `LLVMConfig.cmake` file
(for example `C:\Program Files\LLVM\lib\cmake\llvm`). If Clang is
installed separately to LLVM, then you may also be prompted to supply
a path in the `CLANG_ROOT` parameter, which should be the root of your
Clang installation (containing the `bin/`, `lib/` and `include/`
directories).

You should add the `bin` directory of the Oclgrind installation to the
`PATH` environment variable in order to make use of the `oclgrind`
command. If you wish to use Oclgrind via the OpenCL ICD loader
(optional), then you should also create an ICD loading point. To do
this, you should add a `REG_DWORD` value to the Windows Registry under
one or both of the registry keys below, with the name set to the
absolute path of the `oclgrind-rt-icd.dll` library and the value set
to 0.

Key for 32-bit machines or 64-bit apps on a 64-bit machine:
`HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\OpenCL\Vendors`

Key for 32-bit apps on a 64-bit machine:
`HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Khronos\OpenCL\Vendors`


Usage
-----
The recommended method of running an application with Oclgrind is to
use the `oclgrind` command, for example:

    oclgrind ./application

This command will make it such the only OpenCL platform and device
available to your application is Oclgrind. If you need more control
over platform selection then installing an ICD loading point for
Oclgrind will cause it to appear when an application calls
`clGetPlatformIDs()`, alongside any other OpenCL platforms installed
on your system.

If it encounters any invalid memory accesses, Oclgrind will
report the details to stderr, for example:

    Invalid write of size 4 at global memory address 0x1000000000040
        Kernel:  vecadd
        Entity:  Global(16,0,0) Local(0,0,0) Group(16,0,0)
        store i32 %tmp9, i32 addrspace(1)* %tmp15, align 4
        At line 4 of input.cl
          c[i] = a[i] + b[i]

Since it is interpreting an abstract intermediate representation and
bounds-checking each memory access, Oclgrind will run quite slowly
(typically a couple of orders of magnitude slower than a regular CPU
implementation). Therefore, it is recommended to run your application
with a small problem if possible.

To enable an interactive, GDB-style debugging session, supply the `-i`
flag to the oclgrind command, or export the environment variable
`OCLGRIND_INTERACTIVE=1`. This will cause Oclgrind to automatically
break at the beginning of each kernel invocation, and upon
encountering an invalid memory access. Type `help` for details of
available commands.

For more detailed information about using Oclgrind please visit the
GitHub Wiki:

  https://github.com/jrprice/Oclgrind/wiki/

AIWC
----

The Architecture Independent Workload Characterization (AIWC -- pronounced | \ 'air-wik) tool is a plugin for the Oclgrind OpenCL simulator that gathers metrics of OpenCL programs that can be used to understand and predict program performance on an arbitrary given hardware architecture.

## Usage

To use AIWC over the command line it is passed the appropriate `--aiwc` argument immediately after calling the oclgrind program.
An example of its usage on the kmeans application is shown below:

    oclgrind --aiwc ./kmeans <args>

The collected metrics are logged as text in the command line interface during execution and also in a csv file, stored separately for each kernel and invocation.
These files can be found in the working directory with the naming convention `aiwc_α_β.csv`, where `α` is the kernel name and `β` is the invocation count.

## Metrics

Metrics reported by AIWC should reflect important memory access patterns, control flow operations and available parallelism inherent to achieving efficiency across architectures.
The following are the metrics collected by the AIWC tool ordered by type.



Type                            | Metric                            | Description
:------------------------------ | :-------------------------------: | -------------------------------------------------------------------------:
Compute                         | Opcode                            | total # of unique opcodes required to cover 90% of dynamic instructions
                                | Total Instruction Count           | total # of instructions executed
Parallelism                     | Work-items                        | total # of work-items or threads executed
                                | Total Barriers Hit                | total # of barrier instructions
                                | Min ITB                           | minimum # of instructions executed until a barrier
                                | Max ITB                           | maximum # of instructions executed until a barrier
                                | Median ITB                        | median # of instructions executed until a barrier
                                | Min IPT                           | minimum # of instructions executed per thread
                                | Max IPT                           | maximum # of instructions executed per thread
                                | Median IPT                        | median # of instructions executed per thread
                                | Max SIMD Width                    | maximum # of data items operated on during an instruction
                                | Mean SIMD Width                   | mean # of data items operated on during an instruction
                                | SD SIMD Width                     | standard deviation across # of data items affected
Memory                          | Total Memory Footprint            | total # of unique memory addresses accessed
                                | 90\% Memory Footprint             | # of unique memory addresses that cover 90% of memory accesses
                                | Unique Reads                      | total # of unique memory addresses read
                                | Unique Writes                     | total # of unique memory addresses written
                                | Unique Read/Write Ratio           | indication of workload being (unique reads / unique writes) 
                                | Total Reads                       | total # of memory addresses read
                                | Total Writes                      | total # of memory addresses written
                                | Reread Ratio                      | indication of memory reuse for reads (unique reads/total reads)
                                | Rewrite Ratio                     | indication of memory reuse for writes (unique writes/total writes)
                                | Global Memory Address Entropy     | measure of the randomness of memory addresses
                                | Local Memory Address Entropy      | measure of the spatial locality of memory addresses
                                | Relative Local Memory Usage       | proportion of all memory accesses to memory allocated as `__local`
                                | Parallel Spatial Locality         | average of entropies of threads in a work group that share local memory
Control                         | Total Unique Branch Instructions  | total # of unique branch instructions
                                | 90\% Branch Instructions          | # of unique branch instructions that cover 90% of branch instructions
                                | Yokota Branch Entropy             | branch history entropy using Shannon's information entropy
                                | Average Linear Branch Entropy     | branch history entropy score using the average linear branch entropy



For each OpenCL kernel invocation, the Oclgrind simulator AIWC tool collects a set of 28 metrics, which are listed in the associated Table.
The Opcode, total memory footprint and 90% memory footprint measures are simple counts.
Likewise, total instruction count is the number of instructions achieved during a kernels execution.
The global memory address entropy (MAE) is a positive real number that corresponds to the randomness of memory addresses accessed.
The local memory address entropy is computed as 10 separate values according to an increasing number of Least Significant Bits (LSB), or low order bits, omitted in the calculation.

The number of bits skipped ranges from 1 to 10, and a steeper drop in entropy with an increasing number of bits indicates greater spatial locality in the address stream.
Both unique branch instructions and the associated 90% branch instructions are counts indicating the count of logical control flow branches encountered during kernel execution.
Yokota branch entropy ranges between 0 and 1, and offers an indication of a program’s predictability as a floating point entropy value.
The average linear branch entropy metric is proportional to the miss rate in program execution; p = 0 for branches always taken or not-taken but p = 0.5 for the most unpredictable control flow.
All branch-prediction metrics were computed using a fixed history of 16-element branch strings, each of which is composed of 1-bit branch results (taken/not-taken).

As the OpenCL programming model is targeted at parallel architectures, any workload characterization must consider exploitable parallelism and associated communication and synchronization costs.
We characterize thread-level parallelism (TLP) by the number of work- items executed by each kernel, which indicates the maximum number of threads that can be executed concurrently.

Work-item communication hinders TLP, and in the OpenCL setting, takes the form of either local communication (within a work-group) using local synchronization (barriers) or globally by dividing the kernel and invoking the smaller kernels on the command queue.
Both local and global synchronization can be measured in instructions to barrier (ITB) by performing a running tally of instructions executed per work-item until a barrier is encountered under which the count is saved and resets; this count will naturally include the final (implicit) barrier at the end of the kernel.
Min, max and median ITB are reported to understand synchronization overheads, as a large difference between min and max ITB may indicate an irregular workload.

Instructions per thread (IPT) based metrics are generated by performing a running tally of instructions executed per work-item until completion.
The count is saved and resets.
Min, max and median IPT are reported to understand load imbalance.

To characterize data parallelism, we examine the number and width of vector operands in the generated LLVM IR, reported as max SIMD width, mean SIMD width and standard deviation – SD SIMD width.
Further characterisation of parallelism is presented in the work-items and total barriers hit metrics.

Some of the other metrics are highly dependent on workload scale, so work-items may be used to normalize between different scales.
For example, total memory footprint can be divided by work-items to give the total memory footprint per work-item, which indicates the memory required per processing element.

Finally, unique verses absolute reads and writes can indicate shared and local memory reuse between work-items within a work-group, and globally, which shows the predictability of a workload.
To present these characteristics the unique reads, unique writes, unique read/write ratio, total reads, total writes, reread ratio, rewrite ratio metrics are proposed.
The unique read/write ratio shows that the workload is balanced, read intensive or write intensive.
They are computed by storing read and write memory accesses separately and are later combined, to compute the global memory address entropy and local memory address entropy scores.

On Relative Local Memory Usage (RLMU): This measures the proportion of all memory accesses from the symbolic execution of the kernel that occurred to memory allocated as `__local`.
On GPUs, this memory address space is mapped to fast on-chip shared memory.
Relative local memory usage is an example of a metric that is useful to measure performance-critical access patterns on some architectures such as GPUs, and not others, such as CPUs.
CPUs do not typically have a notion of user-controlled on-chip memory shared between hardware threads such as GPUs’ shared memory.
This is a natural consequence of programming for a heterogeneous system.
Specific code patterns may translate to performance improvements only on certain hardware.
RLMU performs the calculation of memory address entropy by using *virtual addresses* to calculate MAE values using an abstract ideal address space on which all memory accesses by the kernel occur.
This allows AIWC to accurately abstract over the hardware and ISA-specific differences in memory layouts across the diverse hardware targets.

On Parallel Spatial Locality (PSL): Aggregate metrics of the kind presented by AIWC necessarily present a simplified view of program behaviour, omitting many details.
Different ways of aggregating program measurements lead to different features of program execution being emphasised in the final metrics.
For example, the calculation of memory address entropy relies only on the frequency distribution of memory accesses to all addresses accessed by the kernel, and discards temporal information.
The order of sequential memory accesses performed by each thread, as well the relationship between work items in an OpenCL work work group, are both vital in accurately characterizing parallel codes.
To this end, PSL is a parallel computing analogue for MICA’s data stride metric that measures the distance between consecutive data accesses in a single-threaded environment.
In parallel programs, to accurately measure spatial locality of accesses, we must consider memory accesses performed by multiple threads in a close temporal scope.
PSL thus calculates the locality of accesses in each time step of the program's execution; The steeper reductions of `n`-bits-dropped in parallel spatial locality scores will be observed in programs that often access nearby memory addresses within the same timestamp.
Such programs will perform better on GPUs, as they will make better use of both global memory access coalescing and shared memory bank structures.
To a lesser extent, the proposed metric reflects performance-critical memory access patterns on CPUs, as pulling a single cache line from global memory into last-level cache may improve memory access times for all CPU cores.

## AIWC Examples

The following are examples of projects that have heavily used AIWC to perform analysis---either for performance predictions or workload characterization:

1) https://github.com/BeauJoh/aiwc-opencl-based-architecture-independent-workload-characterization-artefact
2) https://github.com/BeauJoh/opencl-predictions-with-aiwc

## Citing & Additional Information

If you use AIWC, please cite the most appropriate of the following papers:

* [Characterizing and Predicting Scientific Workloads for Heterogeneous Computing Systems](https://ieeexplore.ieee.org/abstract/document/863938://openresearch-repository.anu.edu.au/handle/1885/162792)
* [AIWC: OpenCL-Based Architecture-Independent Workload Characterization](https://ieeexplore.ieee.org/abstract/document/8639381)
* [OpenCL Performance Prediction using Architecture-Independent Features](https://ieeexplore.ieee.org/abstract/document/8514400)
* [Characterizing Optimizations to Memory Access Patterns using Architecture-Independent Program Features](https://dl.acm.org/doi/abs/10.1145/3388333.3388656)

Contact
-------
If you encounter any issues or have any questions, please use the
GitHub issues page:

  https://github.com/jrprice/Oclgrind/issues

For questions with AIWC please contact Beau Johnston <beau@inbeta.org> or over GitHub:

  https://github.com/beaujoh/Oclgrind/issues

