#!/usr/bin/env python3

import csv


KEYVAL_SEP = "="
LIST_DELIM = ";"


def read_aiwc_logfile(filepath):
    """
    Reads the contents of an AIWC log file. Each set of kernel
    metrics are represented as a dictionary of metric name to
    value. All values are automatically coerced into lists, ints,
    floats, etc.

    @param string filepath: Path to features file
    @return {
        kernels: List of dictionaries, representing metrics for kernel run
        categories: mapping of metric name -> category
    }
    """
    kernels = []
    categories = {}

    def is_new_kernel(row):
        return row[0] == "metric" and row[1] == "category" and row[2] == "count"

    def coerce(val):
        try:
            return int(val)
        except ValueError:
            try:
                return float(val)
            except ValueError:
                return val

    with open(filepath, newline="") as csvfile:
        features_reader = csv.reader(csvfile)

        kernel = None
        for row in features_reader:
            if len(row) != 3:
                print(f"unexpected row length {len(row)}, skipping: {row}")
                continue

            if is_new_kernel(row):
                if kernel is not None:
                    kernels.append(kernel)
                kernel = {}
                continue

            metric = row[0].strip()
            category = row[1].strip()
            count = row[2].strip()

            if LIST_DELIM in count:
                items = [item.strip() for item in count.split(LIST_DELIM) if item.strip() != ""]
                if len(items) > 0:
                    if KEYVAL_SEP in items[0]:
                        items = [item.split(KEYVAL_SEP) for item in items]
                        for item in items:
                            if len(item) != 2:
                                print("ERROR! Unexpected count of key val members")
                                return []
                            item[1] = coerce(item[1])
                    else:
                        items = [coerce(item) for item in items]

                count = items
            else:
                count = coerce(count)

            if metric in kernel:
                print("ERROR! Duplicate metric entries")
                return []

            kernel[metric] = count
            categories[metric] = category

        if kernel is not None:
            kernels.append(kernel)

    return {
        "kernels": kernels,
        "categories": categories,
    }


class AiwcKernelMetrics:
    """
    Represents the metrics for a single kernel run. Methods offer
    access to a variety of metrics derived from the log file
    """
    def __init__(self, features):
        """
        @param features dictionary as return in the list by `read_aiwc_logfile`
        """
        self.features = features

    def kernel_name(self):
        return self.features["kernel_name"]

    def work_group_size_specified(self):
        return self.features["work_group_size_specified"] == 1

    def work_groups(self):
        return self.features["work_groups"]

    def work_items_per_work_group(self):
        return self.features["work_items_per_work_group"];

    def instructions_count_total(self):
        return sum([count for _, count in self.features["opcode_counts"]])

    def instructions_by_count(self, most_first=True):
        result = sorted(
            [
                {"opcode": opcode, "count": count}
                for opcode, count in self.features["opcode_counts"]
            ],
            key=lambda x: x["count"],
            reverse=most_first,
        )

        total_count = self.instructions_count_total()
        cumulative_count = 0
        for elem in result:
            cumulative_count += elem["count"]
            elem["pc_cumulative"] = cumulative_count / total_count
            elem["pc_total"] = elem["count"] / total_count

        return result

    def freedom_to_reorder(self):
        return self.features["freedom_to_reorder"]

    def resource_pressure(self):
        return self.features["resource_pressure"]

    def work_items_count(self):
        return self.features["work_items"]

    def granularity(self):
        return 1.0 / self.work_items_count()

    def barriers_hit_total(self):
        return self.features["total_barriers_hit"]

    def barriers_hit_per_work_item(self):
        return self.barriers_hit_total() / self.work_items_count()

    def barriers_hit_per_instruction(self):
        return (
            self.barriers_hit_total() + self.work_items_count()
        ) / self.instructions_count_total()

    def instructions_to_barrier(self):
        return {
            "min": self.features["min_ITB"],
            "median": self.features["median_ITB"],
            "max": self.features["max_ITB"],
        }

    def instructions_count_per_work_item(self):
        return {
            "min": self.features["min_IPT"],
            "mean": self.instructions_count_total() / self.work_items_count(),
            "median": self.features["median_IPT"],
            "max": self.features["max_IPT"],
        }

    def simd_width(self):
        return {
            "min": self.features["min_SIMD_width"],
            "mean": self.features["mean_SIMD_width"],
            "max": self.features["max_SIMD_width"],
            "stdev": self.features["sd_SIMD_width"],
        }

    def simd_sum(self):
        return self.features["SIMD_operand_sum"]

    def instructions_per_operand(self):
        return self.instructions_count_total() / self.simd_sum()

    def memory_access_count_total(self):
        return self.features["num_memory_accesses"]

    def memory_access_count_unique(self):
        return self.features["total_memory_footprint"]

    def memory_read_count_unique(self):
        return self.features["unique_reads"]

    def memory_write_count_unique(self):
        return self.features["unique_writes"]

    def memory_read_write_unique_ratio(self):
        return self.memory_read_count_unique() / self.memory_write_count_unique()

    def memory_read_count_total(self):
        return self.features["total_reads"]

    def memory_write_count_total(self):
        return self.features["total_writes"]

    def memory_reread_ratio(self):
        return self.memory_read_count_total() / self.memory_read_count_unique()

    def memory_rewrite_ratio(self):
        return self.memory_write_count_total() / self.memory_write_count_unique()

    def memory_address_count_significant(self):
        return self.features["memory_footprint_90pc"]

    def memory_address_entropy(self):
        return self.features["global_memory_address_entropy"]

    def memory_address_entropy_by_lsb(self):
        return self.features["LMAE"]

    def normed_parallel_spacial_locality_by_lsb(self):
        return self.features["normed_PSL"]

    def normed_parallel_spacial_locality_sum(self):
        return sum([npsl for _, npsl in self.normed_parallel_spacial_locality_by_lsb()])

    def memory_global_access_count_total(self):
        return self.features["total_global_memory_accessed"]

    def memory_global_access_count_pc(self):
        return (
            self.memory_global_access_count_total()
            / self.memory_all_access_count_total()
        )

    def memory_local_access_count_total(self):
        return self.features["total_local_memory_accessed"]

    def memory_local_access_count_pc(self):
        return (
            self.memory_local_access_count_total()
            / self.memory_all_access_count_total()
        )

    def memory_constant_access_count_total(self):
        return self.features["total_constant_memory_accessed"]

    def memory_constant_access_count_pc(self):
        return (
            self.memory_constant_access_count_total()
            / self.memory_all_access_count_total()
        )

    def memory_all_access_count_total(self):
        return (
            self.memory_global_access_count_total()
            + self.memory_local_access_count_total()
            + self.memory_constant_access_count_total()
        )

    def branches_by_count(self, most_first=True):
        return sorted(
            [
                {"line": line, "count": count}
                for line, count in self.features["branch_counts"]
            ],
            key=lambda x: x["count"],
            reverse=most_first,
        )

    def branches_count_total(self):
        return sum([count for _, count in self.features["branch_counts"]])

    def branch_entropy_history_size(self):
        return self.features["branch_history_size"]

    def branch_entropy_yokota(self):
        return self.features["yokota_branch_entropy"]

    def branch_entropy_average_linear(self):
        return self.features["average_linear_branch_entropy"]
