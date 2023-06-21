#!/usr/bin/env python3

"""
This script will print out a human readable overview of the features generated by AIWC
"""

import argparse
import os
import os.path as path
import sys
import matplotlib.pyplot as plt
import aiwctools


def plot(names, y_label, y_values, outdir="plots", scales=["linear"]):
    fig, ax = plt.subplots(figsize=(18, 6))
    x = list(range(len(y_values)))
    ax.bar(x, y_values)

    ax.set_xticks(list(range(len(y_values))))
    ax.set_xticklabels(names)

    ax.set_xlabel("Kernel")
    ax.set_ylabel(y_label)

    for scale in scales:
        ax.set_yscale(scale)
        os.makedirs(outdir, exist_ok=True)
        fig.savefig(
            path.join(outdir, f"{y_label.lower().replace(' ', '_')}_{scale}.pdf"), format="pdf", bbox_inches="tight"
        )
    plt.close(fig)


def to_percent(frac):
        return f"{frac * 100:.2f}%"


def compare_features(features, names, outdir="plots", diff_threshold=0.05, anchor="first"):
    """
    Prints a list of significant metric differences between a list of AiwcKernelMetrics
    instances. Also plots everything, putting plots in the given folder.

    @param features List of AiwcKernelMetrics
    @param outdir Directory to dump plots (will replace existing plots)
    @param diff_threshold Percent difference from baseline value to be considered significant
    @param anchor How to pick baseline for percent difference comparison (choice: first, mean)
    """

    def get_baseline(vals):
        if anchor == "mean":
            return sum(vals) / len(vals)
        elif anchor == "first":
            return vals[0]
        else:
            print(f"unknown anchor {anchor}")
            return vals[0]

    def gen_plot(name, getter):
        print(f"plotting {name}")
        vals = [getter(f) for f in features]
        plot(names, name, vals, outdir=outdir)

        baseline = get_baseline(vals)
        if baseline == 0:
            baseline = 0.000001
            vals = [v + baseline for v in vals]

        disps = [val - baseline for val in vals]
        pc_disps = [abs(disp) / baseline for disp in disps]

        if not any([pc_disp >= diff_threshold for pc_disp in pc_disps]):
            return

        print(f"{name}: {baseline}")
        for name, val, pc_disp in zip(names, vals, pc_disps):
            print(f" - {name}: {val} ({'+' if val >= baseline else '-'}{to_percent(pc_disp)})")

    gen_plot("Total instruction counts", lambda f: f.instructions_count_total())
    gen_plot("Freedom to reorder", lambda f: f.freedom_to_reorder())
    gen_plot("Resource pressure", lambda f: f.resource_pressure())
    gen_plot("Work items count", lambda f: f.work_items_count())
    gen_plot("Granularity", lambda f: f.granularity())
    gen_plot("Total barriers hit", lambda f: f.barriers_hit_total())
    gen_plot("Barriers per work item", lambda f: f.barriers_hit_per_work_item())
    gen_plot("Barriers per instruction", lambda f: f.barriers_hit_per_instruction())
    gen_plot("Instructions per work item (mean)", lambda f: f.instructions_count_per_work_item()["mean"])
    gen_plot("SIMD width (mean)", lambda f: f.simd_width()["mean"])
    gen_plot("SIMD sum", lambda f: f.simd_sum())
    gen_plot("Instructions per operand", lambda f: f.instructions_per_operand())
    gen_plot("Memory access count", lambda f: f.memory_access_count_total())
    gen_plot("Memory access count (unique)", lambda f: f.memory_access_count_unique())
    gen_plot("Memory read count (unique)", lambda f: f.memory_read_count_unique())
    gen_plot("Memory read count", lambda f: f.memory_read_count_total())
    gen_plot("Memory write count (unique)", lambda f: f.memory_write_count_unique())
    gen_plot("Memory write count", lambda f: f.memory_write_count_total())
    gen_plot("Memory read-write ratio (unique)", lambda f: f.memory_read_write_unique_ratio())
    gen_plot("Memory re-read ratio", lambda f: f.memory_reread_ratio())
    gen_plot("Memory re-write ratio", lambda f: f.memory_rewrite_ratio())
    gen_plot("Memory address entropy", lambda f: f.memory_address_entropy())
    gen_plot("NPSL sum", lambda f: f.normed_parallel_spacial_locality_sum())
    gen_plot("Global memory access %", lambda f: f.memory_global_access_count_pc())
    gen_plot("Local memory access %", lambda f: f.memory_local_access_count_pc())
    gen_plot("Constant memory access %", lambda f: f.memory_constant_access_count_pc())
    gen_plot("Yokota branch entropy", lambda f: f.branch_entropy_yokota())
    gen_plot("Average linear branch entropy", lambda f: f.branch_entropy_average_linear())


def print_features(akm):
    print(f"# Architecture-Independent Workload Characterization of kernel: {akm.kernel_name()}")

    def section(name):
        print()
        print()
        print(f"## {name}")

    def subsection(name):
        print()
        print(f"### {name}")

    def datapoint(name, value):
        print(f"- {name}: {value}")

    datapoint("Work-group size specified", akm.work_group_size_specified())
    datapoint("Work-groups", akm.work_groups())
    datapoint("Work-items per work-group", akm.work_items_per_work_group())

    section("Compute")

    operations = akm.instructions_by_count()
    append_pc_total(operations, key=lambda x: x["count"])
    append_pc_cumulative(operations, key=lambda x: x["count"])

    table(
        [["Opcode", "Count", "%total", "%cumulative"]]
        + [
            [
                x["opcode"],
                x["count"],
                to_percent(x["pc_total"]),
                to_percent(x["pc_cumulative"]),
            ]
            for x in operations
        ],
        [align_start, align_end, align_dot, align_dot],
    )
    datapoint("Total operation count", akm.instructions_count_total())

    section("Parallelism")

    subsection("Utilization")

    datapoint("Freedom to reorder", akm.freedom_to_reorder())
    datapoint("Resource pressure", akm.resource_pressure())

    subsection("Thread-Level Parallelism")

    datapoint("Work-items", akm.work_items_count())
    datapoint("Granularity", akm.granularity())
    datapoint("Total Barriers Hit", akm.barriers_hit_total())
    datapoint("Barriers hit per thread", akm.barriers_hit_per_work_item())

    i_to_barrier = akm.instructions_to_barrier()
    datapoint(
        "Instructions to Barrier (min/median/max)",
        f"{i_to_barrier['min']} / {i_to_barrier['median']} / {i_to_barrier['max']}",
    )
    datapoint("Barriers per Instruction", akm.barriers_hit_per_instruction())

    subsection("Work Distribution")

    i_per_thread = akm.instructions_count_per_work_item()
    datapoint(
        "Instructions per Thread (min/median/max)",
        f"{i_per_thread['min']} / {i_per_thread['median']} / {i_per_thread['max']}",
    )

    subsection("Data Parallelism")

    simd = akm.simd_width()
    datapoint(
        "SIMD width (min/mean/max/stdev)",
        f"{simd['min']} / {simd['mean']} / {simd['max']} / {simd['stdev']}",
    )
    datapoint("Instructions per operand", akm.instructions_per_operand())

    section("Memory")

    subsection("Memory Footprint")

    datapoint("Num memory accesses", akm.memory_access_count_total())
    datapoint(
        "Total Memory Footprint -- num unique memory addresses accessed",
        akm.memory_access_count_unique(),
    )
    datapoint(
        "Total Memory Footprint -- num unique memory addresses read",
        akm.memory_read_count_unique(),
    )
    datapoint(
        "Total Memory Footprint -- num unique memory addresses written",
        akm.memory_write_count_unique(),
    )
    datapoint(
        "Total Memory Footprint -- unique read/write ratio",
        akm.memory_read_write_unique_ratio(),
    )
    datapoint("Total Memory Footprint -- total reads", akm.memory_read_count_total())
    datapoint("Total Memory Footprint -- total writes", akm.memory_write_count_total())
    datapoint(
        "Total Memory Footprint -- ratio re-reads",
        akm.memory_reread_ratio(),
    )
    datapoint("Total Memory Footprint -- ratio re-writes", akm.memory_rewrite_ratio())
    datapoint(
        "90% Memory Footprint -- num unique memory addresses that cover 90% of memory accesses",
        akm.memory_address_count_significant(),
    )

    subsection("Memory Entropy")

    datapoint(
        "Global Memory Address Entropy -- measure of the randomness of memory addresses",
        akm.memory_address_entropy(),
    )

    table(
        [["LSBs skipped", "Entropy"]] + akm.memory_address_entropy_by_lsb(),
        [align_end, align_dot],
    )

    subsection("Parallel Spatial Locality")

    table(
        [["LSBs skipped", "Normed Parallel Spatial Locality"]]
        + akm.normed_parallel_spacial_locality_by_lsb(),
        [align_end, align_dot],
    )

    datapoint("Normed Locality Sum", akm.normed_parallel_spacial_locality_sum())

    subsection(
        "Memory Diversity -- Usage of local and constant memory relative to global memory"
    )

    datapoint(
        "Num local memory accesses",
        f"{akm.memory_local_access_count_total()} ({to_percent(akm.memory_local_access_count_pc())})",
    )
    datapoint(
        "Num global memory accesses",
        f"{akm.memory_global_access_count_total()} ({to_percent(akm.memory_global_access_count_pc())})",
    )
    datapoint(
        "Num constant memory accesses",
        f"{akm.memory_constant_access_count_total()} ({to_percent(akm.memory_constant_access_count_pc())})",
    )

    section("Control")

    branches = akm.branches_by_count()
    append_pc_total(branches, key=lambda x: x["count"])
    append_pc_cumulative(branches, key=lambda x: x["count"])
    table(
        [["Branch At Line", "Count (hit and miss)", "%total", "%cumulative"]]
        + [
            [
                x["line"],
                x["count"],
                to_percent(x["pc_total"]),
                to_percent(x["pc_cumulative"]),
            ]
            for x in branches
        ],
        [align_end, align_end, align_dot, align_dot],
    )

    subsection(
        "Branch Entropy -- measure of the randomness of branch behaviour, representing branch predictability"
    )

    datapoint("Using a branch history of", akm.branch_entropy_history_size())
    datapoint("Yokota Branch Entropy", akm.branch_entropy_yokota())
    datapoint("Average Linear Branch Entropy", akm.branch_entropy_average_linear())

    print()


def align_dot(c):
    return c.index(".")


def align_start(c):
    return 0


def align_end(c):
    return len(c)


def table(data, aligners, indent=""):
    data = [[str(c) for c in row] for row in data]

    columns = list(zip(*data))

    def adjust_column(column, aligner):
        header, *values = column

        def get_padding(c):
            i = aligner(c)
            return (i, len(c) - i)

        paddings = [get_padding(c) for c in values]

        pad_left = max([0] + [p[0] for p in paddings])
        pad_right = max([0] + [p[1] for p in paddings])

        if pad_left + pad_right < len(header):
            extra = len(header) - (pad_left + pad_right)
            pad_left += extra // 2
            pad_right += (extra + 1) // 2

        padded_col = [header.center(pad_left + pad_right + 2)] + [
            " " * (1 + pad_left - p[0]) + v + " " * (1 + pad_right - p[1])
            for v, p in zip(values, paddings)
        ]

        return padded_col

    adjusted_cols = [adjust_column(col, adj) for col, adj in zip(columns, aligners)]
    adjusted_rows = [[c for c in r] for r in zip(*adjusted_cols)]
    rule = "|".join(["-" * len(c) for c in adjusted_rows[0]])

    lines = [
        "|".join(adjusted_rows[0]),
        rule,
        *["|".join(r) for r in adjusted_rows[1:]],
    ]

    lines = [indent + "|" + r + "|" for r in lines]

    print()
    print("\n".join(lines))
    print()


def append_pc_total(vals, key=None, output="pc_total"):
    """
    Appends the field `output` to each dictionary in the list `vals`, calculated
    as the value extracted by `key` over the sum of all values extracted by `key`
    in the list

    @param vals List of dictionaries
    @param key Function taking list element and returning a number
    @param output Key to use for storing the result on each dictionary
    """

    total_count = sum([key(elem) for elem in vals])
    for elem in vals:
        elem[output] = key(elem) / total_count


def append_pc_cumulative(vals, key=None, output="pc_cumulative", reverse=False):
    """
    Appends the field `output` to each dictionary in the list `vals`, calculated
    as the cumulative sum of values extracted by `key` over the sum of all values
    extracted by `key` in the list

    @param vals List of dictionaries
    @param key Function taking list element and returning a number
    @param output Key to use for storing the result on each dictionary
    """

    total_count = sum([key(elem) for elem in vals])
    cumulative_count = 0

    if reverse:
        vals = reversed(vals)

    for elem in vals:
        count = key(elem)
        cumulative_count += count
        elem[output] = cumulative_count / total_count


def parse_argv():
    parser = argparse.ArgumentParser(
        description="Analyse and print a human readable overview of the data in an AIWC features file"
    )

    parser.add_argument(nargs='*', dest="input", help="features file to analyse")
    parser.add_argument("--compare", action="store_true", help="Compare features of given kernels")
    parser.add_argument("--names", help="Semicolon separated list of names for each metric in comparison. Order matches `input`. Defaults to kernel name of inputs.")
    return parser.parse_args()


def main():
    argv = parse_argv()

    filepaths = argv.input

    parsed = [aiwctools.read_aiwc_logfile(filepath) for filepath in filepaths]

    kernels = []
    for parsedfile in parsed:
        kernels += parsedfile["kernels"]

    features = [aiwctools.AiwcKernelMetrics(k) for k in kernels]

    if (argv.compare):
        names = [f.kernel_name() for f in features]
        if argv.names is not None:
            try_names = argv.names.split(";")
            if len(try_names) == len(features):
                names = try_names
            else:
                print("`names` is wrong length: using kernel names")

        compare_features(features, names)
    else:
        for aiwcKernelMetrics in features:
            print_features(aiwcKernelMetrics)

    return 0


if __name__ == "__main__":
    sys.exit(main())