AIWC - Built on Oclgrind
========================

The Architecture Independent Workload Characterization (AIWC -- pronounced | \ 'air-wik) tool is a plugin for the Oclgrind OpenCL simulator that gathers metrics of OpenCL programs that can be used to understand and predict program performance on an arbitrary given hardware architecture.

## Building & Installing

### Docker

The tool has a docker file provided for rapid evaluation, a prebuilt image can be run directly with:

    docker run beaujoh/aiwc:v1.1

Alternatively, to build your own image based on the provided Dockerfile, run:

    docker build -t <yourname>/aiwc:v1.1 .
    docker run <yourname>/aiwc:v1.1 .

Both images will launch a demonstration of using AIWC to profile LU Decomposition (an OpenCL implementations from the Extended OpenDwarfs Benchmark suite), and should support all OpenCL codes.
To test on your own codes, run docker with an interactive session:

    docker run -it beaujoh/aiwc:v1.1 bash

### Manual

Set the following environment variables as desired

    export OCLGRIND=/oclgrind
    export OCLGRIND_SRC=/oclgrind-source
    export OCLGRIND_BIN=/oclgrind/bin/oclgrind

The rest can be built with the following commands (tested on Ubuntu 18.04)

    apt-get update && apt-get install --no-install-recommends -y libreadline-dev
    git clone https://github.com/BeauJoh/AIWC.git $OCLGRIND_SRC
    mkdir $OCLGRIND_SRC/build
    cd $OCLGRIND_SRC/build
    CC /llvm-11.0.1/bin/clang
    CXX /llvm-11.0.1/bin/clang++
    cmake $OCLGRIND_SRC -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_DIR=/llvm-11.0.1/lib/cmake/llvm -DCLANG_ROOT=/llvm-11.0.1 -DCMAKE_INSTALL_PREFIX=$OCLGRIND -DBUILD_SHARED_LIBS=On
    make
    make install
    mkdir -p /etc/OpenCL/vendors && echo $OCLGRIND/lib/liboclgrind-rt-icd.so > /etc/OpenCL/vendors/oclgrind.icd

## Usage

To use AIWC over the command line it is passed the appropriate `--aiwc` argument immediately after calling the oclgrind program.
An example of its usage on the OpenCL kmeans application is shown below:

    oclgrind --aiwc ./kmeans <args>

The collected metrics are stored in a csv file, stored separately for each kernel and invocation. These files can be found in the working directory with the naming convention `aiwc_α_β.csv`, where `α` is the kernel name and `β` is the invocation count.

Alternatively, Oclgrind can be used as a regular OpenCL device but AIWC flags can be used with the following environment variables:

* `OCLGRIND_WORKLOAD_CHARACTERISATION`, as an int/boolean to enable AIWC as the plugin used within Oclgrind, and,
* `OCLGRIND_WORKLOAD_CHARACTERISATION_OUTPUT_PATH`, is a string used to denote the path where the AIWC metrics should be logged (as a csv).

For example:

    OCLGRIND_WORKLOAD_CHARACTERISATION=1 OCLGRIND_WORKLOAD_CHARACTERISATION_OUTPUT_PATH=~/aiwc_metrics.csv ./kmeans <args>


To generate a markdown report of the metrics, run `script/aiwc_report.py ./path/to/logfile`. This will print a variety of metrics, with some derived from others. To compare metrics between logs, run `script/aiwc_report.py --compare ./logfile1 ./logfile2 ...`. This will print out values that are significantly different (threshold can be configured by modifying the script), and also generate a plot for each metric in a created `plots` directory. To set the name of each log file, pass `--names "name1;name2;..."` as an additional argument. Names default to the kernel name if not passed.


## Metrics

Metrics reported by AIWC should reflect important memory access patterns, control flow operations and available parallelism inherent to achieving efficiency across architectures.
The following are the metrics collected by the AIWC tool ordered by type.

<!--
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
Or rendered by GitHub Markdown:-->


Type                            | Metric                            | Description
:------------------------------ | :-------------------------------: | -------------------------------------------------------------------------:
Compute                         | Opcode<br/>Total Instruction Count| total # of unique opcodes required to cover 90% of dynamic instructions<br/>total # of instructions executed
Parallelism                     | Work-items<br/>Total Barriers Hit<br/>Min ITB<br/>Max ITB<br/>Median ITB<br/>Min IPT<br/>Max IPT<br/>Median IPT<br/>Max SIMD Width<br/>Mean SIMD Width<br/>SD SIMD Width | total # of work-items or threads executed<br/>total # of barrier instructions<br/>minimum # of instructions executed until a barrier<br/>maximum # of instructions executed until a barrier<br/>median # of instructions executed until a barrier<br/>minimum # of instructions executed per thread<br/>maximum # of instructions executed per thread<br/>median # of instructions executed per thread<br/>maximum # of data items operated on during an instruction<br/>mean # of data items operated on during an instruction<br/>standard deviation across # of data items affected
Memory                          | Total Memory Footprint<br/>90\% Memory Footprint<br/>Unique Reads<br/>Unique Writes<br/>Unique Read/Write Ratio<br/>Total Reads<br/>Total Writes<br/>Reread Ratio<br/>Rewrite Ratio<br/>Global Memory Address Entropy<br/>Local Memory Address Entropy<br/>Relative Local Memory Usage<br/>Parallel Spatial Locality | total # of unique memory addresses accessed<br/># of unique memory addresses that cover 90% of memory accesses<br/>total # of unique memory addresses read<br/>total # of unique memory addresses written<br/>indication of workload being (unique reads / unique writes)<br/>total # of memory addresses read<br/>total # of memory addresses written<br/>indication of memory reuse for reads (unique reads/total reads)<br/>indication of memory reuse for writes (unique writes/total writes)<br/>measure of the randomness of memory addresses<br/>measure of the spatial locality of memory addresses<br/>proportion of all memory accesses to memory allocated as `__local`<br/>average of entropies of threads in a work group that share local memory
Control                         | Total Unique Branch Instructions<br/>90% Branch Instructions<br/>Yokota Branch Entropy <br/> Average Linear Branch Entropy | total # of unique branch instructions<br/> # of unique branch instructions that cover 90% of branch instructions<br/>branch history entropy using Shannon's information entropy<br/>branch history entropy score using the average linear branch entropy



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

Finally, unique verses absolute reads and writes can indicate shared and local memory reuse between work-items within a work-group, and globally, which shows the predictability of a workload.
To present these characteristics the unique reads, unique writes, unique read/write ratio, total reads, total writes, reread ratio, rewrite ratio metrics are proposed.
The unique read/write ratio shows that the workload is balanced, read intensive or write intensive.
They are computed by storing read and write memory accesses separately and are later combined, to compute the global memory address entropy and local memory address entropy scores.

## AIWC Examples

The following are examples of projects that have heavily used AIWC to perform analysis---either for performance predictions or workload characterization:

1) https://github.com/BeauJoh/aiwc-opencl-based-architecture-independent-workload-characterization-artefact
2) https://github.com/BeauJoh/opencl-predictions-with-aiwc

## Citing & Additional Information

If you use AIWC, please cite [Oclgrind](https://github.com/jrprice/Oclgrind) and the most appropriate of the following papers:

* [Characterizing and Predicting Scientific Workloads for Heterogeneous Computing Systems](https://openresearch-repository.anu.edu.au/handle/1885/162792)
* [AIWC: OpenCL-Based Architecture-Independent Workload Characterization](https://ieeexplore.ieee.org/abstract/document/8639381)
* [OpenCL Performance Prediction using Architecture-Independent Features](https://ieeexplore.ieee.org/abstract/document/8514400)
* [Characterizing Optimizations to Memory Access Patterns using Architecture-Independent Program Features](https://dl.acm.org/doi/abs/10.1145/3388333.3388656)

Contact
-------

For issues and questions with AIWC please contact Beau Johnston <beau@inbeta.org> or over GitHub:

  https://github.com/beaujoh/AIWC/issues

For additional information on Oclgrind--on which this plugin is built--please check out https://github.com/jrprice/Oclgrind
