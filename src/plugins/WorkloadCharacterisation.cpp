// WorkloadCharacterisation.cpp (Oclgrind)
// Copyright (c) 2017, Beau Johnston,
// The Australian National University. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"
#include "core/Kernel.h"
#include "core/KernelInvocation.h"
#include "core/Memory.h"
#include "core/WorkGroup.h"
#include "core/WorkItem.h"
#include "WorkloadCharacterisation.h"

#include <algorithm>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <numeric>
#include <sstream>

#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

using namespace oclgrind;
using namespace std;

#define COUNTED_LOAD_BASE (llvm::Instruction::OtherOpsEnd + 4)
#define COUNTED_STORE_BASE (COUNTED_LOAD_BASE + 8)
#define COUNTED_CALL_BASE (COUNTED_STORE_BASE + 8)

THREAD_LOCAL WorkloadCharacterisation::WorkerState
    WorkloadCharacterisation::m_state = {NULL};

WorkloadCharacterisation::WorkloadCharacterisation(const Context *context) : WorkloadCharacterisation::Plugin(context) {
  m_numberOfHostToDeviceCopiesBeforeKernelNamed = 0;
  m_last_kernel_name = "";
}

WorkloadCharacterisation::~WorkloadCharacterisation() {
  locale previousLocale = cout.getloc();
  locale defaultLocale("");
  cout.imbue(defaultLocale);

  // present memory transfer statistics -- only run once, since these are collected outside kernel invocations
  cout << "+-------------------------------------------------------------------------------------------------------+" << endl;
  cout << "|Memory Transfers -- statistics around host to device and device to host memory transfers               |" << endl;
  cout << "+=======================================================================================================+" << endl;
  // I can't imagine a scenario where data are copied from the device before a kernel is executed. So I use the deviceToHostCopy kernel names for the final statistics -- these names for the m_hostToDeviceCopy's are updated when the kernel is enqueued.
  std::vector<std::string> x = m_deviceToHostCopy;
  std::vector<std::string>::iterator unique_x = std::unique(x.begin(), x.end());
  x.resize(std::distance(x.begin(), unique_x));
  std::vector<std::string> unique_kernels_involved_with_device_to_host_copies = x;

  x = m_hostToDeviceCopy;
  unique_x = std::unique(x.begin(), x.end());
  x.resize(std::distance(x.begin(), unique_x));
  std::vector<std::string> unique_kernels_involved_with_host_to_device_copies = x;

  cout << "Total Host To Device Transfers (#) for kernel:" << endl;
  for (auto const &item : unique_kernels_involved_with_host_to_device_copies) {
    cout << "\t" << item << ": " << std::count(m_hostToDeviceCopy.begin(), m_hostToDeviceCopy.end(), item) << endl;
  }
  cout << "Total Device To Host Transfers (#) for kernel:" << endl;
  for (auto const &item : unique_kernels_involved_with_device_to_host_copies) {
    cout << "\t" << item << ": " << std::count(m_deviceToHostCopy.begin(), m_deviceToHostCopy.end(), item) << endl;
  }

  //write it out to special .csv file
  int logfile_count = 0;
  std::string logfile_name = "aiwc_memory_transfers_" + std::to_string(logfile_count) + ".csv";
  while (std::ifstream(logfile_name)) {
    logfile_count++;
    logfile_name = "aiwc_memory_transfers_" + std::to_string(logfile_count) + ".csv";
  }
  std::ofstream logfile;
  logfile.open(logfile_name);
  assert(logfile);
  logfile << "metric,kernel,count\n";

  for (auto const &item : unique_kernels_involved_with_host_to_device_copies) {
    logfile << "transfer: host to device," << item << "," << std::count(m_hostToDeviceCopy.begin(), m_hostToDeviceCopy.end(), item) << "\n";
  }
  for (auto const &item : unique_kernels_involved_with_device_to_host_copies) {
    logfile << "transfer: device to host," << item << "," << std::count(m_deviceToHostCopy.begin(), m_deviceToHostCopy.end(), item) << "\n";
  }
  logfile.close();

  // Restore locale
  cout.imbue(previousLocale);
}

void WorkloadCharacterisation::hostMemoryLoad(const Memory *memory, size_t address, size_t size) {
  //device to host copy -- synchronization
  m_deviceToHostCopy.push_back(m_last_kernel_name);
}

void WorkloadCharacterisation::hostMemoryStore(const Memory *memory, size_t address, size_t size, const uint8_t *storeData) {
  //host to device copy -- synchronization
  m_hostToDeviceCopy.push_back(m_last_kernel_name);
  m_numberOfHostToDeviceCopiesBeforeKernelNamed++;
}

void WorkloadCharacterisation::threadMemoryLedger(size_t address, uint32_t timestep, Size3 localID) {
  WorkloadCharacterisation::ledgerElement le;
  le.address = address;
  le.timestep = timestep;
  (m_state.ledger)//[groupID.x * m_group_num.x + groupID.y * m_group_num.y
        // + groupID.z * m_group_num.z]
        [localID.x * m_local_num.y * m_local_num.z + localID.y * m_local_num.z
         + localID.z].push_back(le);
}

void WorkloadCharacterisation::memoryLoad(const Memory *memory, const WorkItem *workItem, size_t address, size_t size) {
  if (memory->getAddressSpace() != AddrSpacePrivate) {
    //(*m_state.memoryOps)[pair(address, true)]++;
    (*m_state.loadOps)[address]++;
    threadMemoryLedger(address, 0, workItem->getLocalID());
  }
}

void WorkloadCharacterisation::memoryStore(const Memory *memory, const WorkItem *workItem, size_t address, size_t size, const uint8_t *storeData) {
  if (memory->getAddressSpace() != AddrSpacePrivate) {
    //(*m_state.memoryOps)[pair(address, false)]++;
    (*m_state.storeOps)[address]++;
    threadMemoryLedger(address, 0, workItem->getLocalID());
  }
}

void WorkloadCharacterisation::memoryAtomicLoad(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) {
  if (memory->getAddressSpace() != 0) {
    //(*m_state.memoryOps)[pair(address, true)]++;
    (*m_state.loadOps)[address]++;
    threadMemoryLedger(address, 0, workItem->getLocalID());
  }
}

void WorkloadCharacterisation::memoryAtomicStore(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size) {
  if (memory->getAddressSpace() != 0) {
    //(*m_state.memoryOps)[pair(address, false)]++;
    (*m_state.storeOps)[address]++;
    threadMemoryLedger(address, 0, workItem->getLocalID());
  }
}

void WorkloadCharacterisation::instructionExecuted(
    const WorkItem *workItem, const llvm::Instruction *instruction,
    const TypedValue &result) {

  unsigned opcode = instruction->getOpcode();
  std::string opcode_name = llvm::Instruction::getOpcodeName(opcode);
  (*m_state.computeOps)[opcode_name]++;

  bool isMemoryInst = false;
  unsigned addressSpace;

  //get all unique labels -- for register use -- and the # of instructions between loads and stores -- as the freedom to reorder
  m_state.ops_between_load_or_store++;
  if (auto inst = llvm::dyn_cast<llvm::LoadInst>(instruction)) {
    isMemoryInst = true;
    addressSpace = inst->getPointerAddressSpace();
    std::string name = inst->getPointerOperand()->getName().data();
    (*m_state.loadInstructionLabels)[name]++;
    m_state.instructionsBetweenLoadOrStore->push_back(m_state.ops_between_load_or_store);
    m_state.ops_between_load_or_store = 0;
  } else if (auto inst = llvm::dyn_cast<llvm::StoreInst>(instruction)) {
    isMemoryInst = true;
    addressSpace = inst->getPointerAddressSpace();
    std::string name = inst->getPointerOperand()->getName().data();
    (*m_state.storeInstructionLabels)[name]++;
    m_state.instructionsBetweenLoadOrStore->push_back(m_state.ops_between_load_or_store);
    m_state.ops_between_load_or_store = 0;
  }
  if (isMemoryInst) {
    switch (addressSpace) {
    case AddrSpaceLocal: {
      m_state.local_memory_access_count++;
      break;
    }
    case AddrSpaceGlobal: {
      m_state.global_memory_access_count++;
      break;
    }
    case AddrSpaceConstant: {
      m_state.constant_memory_access_count++;
      break;
    }
    case AddrSpacePrivate:
    default: {
      // we don't count these
    }
    }
  }

  //collect conditional branches and the associated trace to count which ones were taken and which weren't
  if (m_state.previous_instruction_is_branch == true) {
    auto* bb = instruction->getParent();

    if (bb == m_state.target1)
      (*m_state.branchOps)[m_state.branch_loc].push_back(true); //taken
    else if (bb == m_state.target2) {
      (*m_state.branchOps)[m_state.branch_loc].push_back(false); //not taken
    } else {
      cout << "Error in branching!" << endl;
      cout << "Basic block was: " << std::hex << bb << " but target was either: " << m_state.target1 << " or: " << m_state.target2 << std::dec << endl;
      std::raise(SIGINT);
    }
    m_state.previous_instruction_is_branch = false;
  }
  //if a conditional branch -- they have labels and thus temporarily store them and see where we jump to in the next instruction
  if (opcode == llvm::Instruction::Br && instruction->getNumOperands() == 3) {
    if (instruction->getOperand(1)->getType()->isLabelTy() && instruction->getOperand(2)->getType()->isLabelTy()) {
      m_state.previous_instruction_is_branch = true;
      m_state.target1 = (llvm::BasicBlock*) instruction->getOperand(1);
      m_state.target2 = (llvm::BasicBlock*) instruction->getOperand(2);
      m_state.branch_loc = instruction;
    }
  }

  //counter for instructions to barrier and other parallelism metrics
  m_state.instruction_count++;
  m_state.workitem_instruction_count++;

  //SIMD instruction width metrics use the following
  (*m_state.instructionWidth)[result.num]++;

  //TODO: add support for Phi, Switch and Select control operations
}

void WorkloadCharacterisation::workItemBarrier(const WorkItem *workItem) {
  m_state.barriers_hit++;
  m_state.instructionsBetweenBarriers->push_back(m_state.instruction_count);
  m_state.instruction_count = 0;
}

vector<double> parallelSpatialLocality(vector < vector < WorkloadCharacterisation::ledgerElement> > hist);

void WorkloadCharacterisation::workGroupBarrier(const WorkGroup *workGroup, uint32_t flags) {
  vector<double> psl = parallelSpatialLocality(m_state.ledger);
  size_t maxLength = 0;
  for (size_t i = 0; i < m_state.ledger.size(); i++) {
    maxLength = m_state.ledger[i].size() > maxLength ? m_state.ledger[i].size() : maxLength;
    m_state.ledger[i].clear();
  }
  m_state.psl_per_barrier->push_back(std::make_pair(psl, maxLength));
}

void WorkloadCharacterisation::workItemClearBarrier(const WorkItem *workItem) {
  m_state.instruction_count = 0;
}

void WorkloadCharacterisation::workItemBegin(const WorkItem *workItem) {
  m_state.threads_invoked++;
  m_state.instruction_count = 0;
  m_state.workitem_instruction_count = 0;
  m_state.ops_between_load_or_store = 0;
  //Size_3 local_ID = workItem->getLocalID;
  //m_state.work_item_no = localID.x * m_local_num.y * m_local_num.z + localID.y * m_local_num.z
  //       + localID.z;
  //m_state.work_group_no = 0;
}

void WorkloadCharacterisation::workItemComplete(const WorkItem *workItem) {
  m_state.instructionsBetweenBarriers->push_back(m_state.instruction_count);
  m_state.instructionsPerWorkitem->push_back(m_state.workitem_instruction_count);
}

void WorkloadCharacterisation::kernelBegin(const KernelInvocation *kernelInvocation) {
  m_kernelInvocation = kernelInvocation;

  //update the list of memory copies from host to device; since the only reason to write to the device is before an execution.
  m_last_kernel_name = kernelInvocation->getKernel()->getName();

  int end_of_list = m_hostToDeviceCopy.size() - 1;
  for (int i = 0; i < m_numberOfHostToDeviceCopiesBeforeKernelNamed; i++) {
    m_hostToDeviceCopy[end_of_list - i] = m_last_kernel_name;
  }
  m_numberOfHostToDeviceCopiesBeforeKernelNamed = 0;

  //m_memoryOps.clear();
  m_storeOps.clear();
  m_loadOps.clear();
  m_computeOps.clear();
  m_branchPatterns.clear();
  m_branchCounts.clear();
  m_instructionsToBarrier.clear();
  m_instructionWidth.clear();
  m_instructionsPerWorkitem.clear();
  m_instructionsBetweenLoadOrStore.clear();
  m_loadInstructionLabels.clear();
  m_storeInstructionLabels.clear();
  m_threads_invoked = 0;
  m_barriers_hit = 0;
  m_global_memory_access = 0;
  m_local_memory_access = 0;
  m_constant_memory_access = 0;

  m_group_num = kernelInvocation->getNumGroups();
  m_local_num = kernelInvocation->getLocalSize();
  m_psl_per_group = vector<vector<double>>();
}


vector<double> entropy(unordered_map<size_t, uint32_t> histogram) {
  std::vector<std::unordered_map<size_t, uint32_t>> local_address_count(11, unordered_map<size_t, uint32_t>());
  local_address_count[0] = histogram;
  uint64_t total_access_count = 0;
  vector<double> loc_entropy = vector<double>(11);

  for (const auto &m : histogram) {
    for (int nskip = 1; nskip <= 10; nskip++) {
      size_t local_addr = m.first >> nskip;
      local_address_count[nskip][local_addr] += m.second;
    }
    total_access_count += m.second;
  }

  if (total_access_count == 0) {
    loc_entropy = vector<double>(11, 0.0);
    return loc_entropy;
  }

  for (int nskip = 0; nskip < 11; nskip++) {
    double local_entropy = 0.0;
    for (const auto &it : local_address_count[nskip]) {
      double prob = (double)(it.second) * 1.0 / (double)(total_access_count+1);
      local_entropy = local_entropy - prob * std::log2(prob);
    }
    loc_entropy[nskip] = (float)local_entropy;
  }

  return loc_entropy;
}

vector<double> parallelSpatialLocality(vector < vector < WorkloadCharacterisation::ledgerElement> > hist) {
  size_t maxLength = 0;
  for (size_t i = 0; i < hist.size(); i++)
    maxLength = hist[i].size() > maxLength ? hist[i].size() : maxLength;

  unordered_map <size_t, uint32_t> histogram;
  vector<vector<double>> entropies = vector<vector<double>>(maxLength);

  for (size_t i = 0; i < maxLength; i++) { // for each timestep
    histogram.clear();
    for (size_t j = 0; j < hist.size(); j++) {
      if (i >= hist[j].size())
        continue;
      WorkloadCharacterisation::ledgerElement current = hist[j][i];
      histogram[current.address] = histogram[current.address] + 1;
    }

    entropies[i] = entropy(histogram);
  }

  vector<double> psl = vector<double>(11, 0.0);
  for (uint32_t i = 0; i < 11; i++) {
    for (size_t j = 0; j < entropies.size(); j++)
      psl[i] += entropies[j][i];
    psl[i] = psl[i] * 1.0/ ((double)entropies.size() + 1);
  }
  return psl;
}

void WorkloadCharacterisation::kernelEnd(const KernelInvocation *kernelInvocation) {
  // Load default locale
  locale previousLocale = cout.getloc();
  locale defaultLocale("");
  cout.imbue(defaultLocale);

  cout << endl
       << "# Architecture-Independent Workload Characterization of kernel: " << kernelInvocation->getKernel()->getName() << endl;

  cout << endl
       << "## Compute" << endl
       << endl;

  std::vector<std::pair<std::string, size_t>> sorted_ops(m_computeOps.size());
  std::partial_sort_copy(m_computeOps.begin(), m_computeOps.end(), sorted_ops.begin(), sorted_ops.end(), [](const std::pair<std::string, size_t> &left, const std::pair<std::string, size_t> &right) {
    return (left.second > right.second);
  });

  cout << "|" << setw(20) << left << "Opcode"
       << "|" << setw(12) << right << "count"
       << "|" << endl;
  cout << "|--------------------|-----------:|" << endl;
  for (auto const &item : sorted_ops)
    cout << "|" << setw(20) << left << item.first << "|" << setw(12) << right << item.second << "|" << endl;
  cout << endl;

  size_t operation_count = 0;
  for (auto const &item : sorted_ops)
    operation_count += item.second;
  size_t significant_operation_count = (size_t)ceil(operation_count * 0.9);
  size_t major_operations = 0;
  size_t total_instruction_count = operation_count;
  operation_count = 0;

  cout << "unique opcodes required to cover 90\% of dynamic instructions: ";
  while (operation_count < significant_operation_count) {
    if (major_operations > 0)
      cout << ", ";
    operation_count += sorted_ops[major_operations].second;
    cout << sorted_ops[major_operations].first;
    major_operations++;
  }
  cout << endl
       << endl;

  cout << "num unique opcodes required to cover 90\% of dynamic instructions: " << major_operations << endl
       << endl;
  cout << "Total Instruction Count: " << total_instruction_count << endl;

  cout << endl
       << "## Parallelism" << endl;

  cout << endl
       << "### Utilization" << endl
       << endl;

  double freedom_to_reorder = std::accumulate(m_instructionsBetweenLoadOrStore.begin(), m_instructionsBetweenLoadOrStore.end(), 0.0);
  freedom_to_reorder = freedom_to_reorder / m_instructionsBetweenLoadOrStore.size();
  cout << "Freedom to Reorder: " << setprecision(2) << freedom_to_reorder << endl
       << endl;

  double resource_pressure = 0;
  for (auto const &item : m_storeInstructionLabels)
    resource_pressure += item.second;
  for (auto const &item : m_loadInstructionLabels)
    resource_pressure += item.second;
  resource_pressure = resource_pressure / m_threads_invoked;
  cout << "Resource Pressure: " << setprecision(2) << resource_pressure << endl;

  cout << endl
       << "### Thread-Level Parallelism" << endl
       << endl;

  cout << "Work-items: " << m_threads_invoked << endl
       << endl;
  double granularity = 1.0 / static_cast<double>(m_threads_invoked);
  cout << "Granularity: " << granularity << endl
       << endl;

  cout << "Total Barriers Hit: " << m_barriers_hit << endl
       << endl;
  //cout << "num barriers hit per thread: " << m_instructionsToBarrier.size()/m_threads_invoked << endl;
  uint32_t itb_min = *std::min_element(m_instructionsToBarrier.begin(), m_instructionsToBarrier.end());
  uint32_t itb_max = *std::max_element(m_instructionsToBarrier.begin(), m_instructionsToBarrier.end());

  double itb_median;
  std::vector<uint32_t> itb = m_instructionsToBarrier;
  sort(itb.begin(), itb.end());

  size_t size = itb.size();
  if (size % 2 == 0) {
    itb_median = (itb[size / 2 - 1] + itb[size / 2]) / 2;
  } else {
    itb_median = itb[size / 2];
  }

  cout << "Instructions to Barrier (min/max/median): " << itb_min << "/" << itb_max << "/" << itb_median << endl
       << endl;
  double barriers_per_instruction = static_cast<double>(m_barriers_hit + m_threads_invoked) / static_cast<double>(total_instruction_count);
  cout << "Barriers per Instruction: " << barriers_per_instruction << endl
       << endl;

  cout << "### Work Distribution" << endl
       << endl;

  uint32_t ipt_min = *std::min_element(m_instructionsPerWorkitem.begin(), m_instructionsPerWorkitem.end());
  uint32_t ipt_max = *std::max_element(m_instructionsPerWorkitem.begin(), m_instructionsPerWorkitem.end());

  uint32_t ipt_median;
  std::vector<uint32_t> ipt = m_instructionsPerWorkitem;
  sort(ipt.begin(), ipt.end());

  size = ipt.size();
  if (size % 2 == 0) {
    ipt_median = (ipt[size / 2 - 1] + ipt[size / 2]) / 2;
  } else {
    ipt_median = ipt[size / 2];
  }
  cout << "Instructions per Thread (min/max/median): " << ipt_min << "/" << ipt_max << "/" << ipt_median << endl
       << endl;

  cout << "### Data Parallelism" << endl
       << endl;

  using pair_type = decltype(m_instructionWidth)::value_type;

  uint16_t simd_min = std::min_element(m_instructionWidth.begin(), m_instructionWidth.end(), [](const pair_type &a, const pair_type &b) { return a.first < b.first; })->first;
  uint16_t simd_max = std::max_element(m_instructionWidth.begin(), m_instructionWidth.end(), [](const pair_type &a, const pair_type &b) { return a.first < b.first; })->first;

  uint32_t simd_sum = 0;
  uint32_t simd_num = 0;
  for (const auto &item : m_instructionWidth) {
    simd_sum += item.second * item.first;
    simd_num += item.second;
  }
  double simd_mean = simd_sum / (double)simd_num;
  std::vector<double> diff(m_instructionWidth.size());
  std::transform(m_instructionWidth.begin(), m_instructionWidth.end(), diff.begin(), [simd_mean](const pair_type &x) { return (x.first - simd_mean) * (x.first - simd_mean) * x.second; });
  double simd_sq_sum = std::accumulate(diff.begin(), diff.end(), 0.0);
  double simd_stdev = std::sqrt(simd_sq_sum / (double)simd_num);
  cout << "SIMD Width (min/max/mean/stdev): " << simd_min << "/" << simd_max << "/" << simd_mean << "/" << simd_stdev << endl
       << endl;

  double instructions_per_operand = static_cast<double>(total_instruction_count) / simd_sum;
  cout << "Instructions per Operand: " << instructions_per_operand << endl
       << endl;

  cout << "## Memory" << endl
       << endl;

  cout << "### Memory Footprint" << endl
       << endl;

  // count accesses to memory addresses with different numbers of retained
  // significant bits
  std::vector<std::unordered_map<size_t, uint32_t>> local_address_count(11);
  // for (const auto &m : m_memoryOps) {
  //   for (int nskip = 0; nskip <= 10; nskip++) {
  //     size_t local_addr = m.first >> nskip;
  //     local_address_count[nskip][local_addr] += m.second;
  //   }
  // }
  size_t load_count = 0;
  size_t store_count = 0;

  for (const auto &m : m_storeOps) {
    for (int nskip = 0; nskip <= 10; nskip++) {
      size_t local_addr = m.first >> nskip;
      local_address_count[nskip][local_addr] += m.second;
    }
    store_count += m.second;
  }

  for (const auto &m : m_loadOps) {
    for (int nskip = 0; nskip <= 10; nskip++) {
      size_t local_addr = m.first >> nskip;
      local_address_count[nskip][local_addr] += m.second;
    }
    load_count += m.second;
  }

  std::vector<std::pair<size_t, uint32_t>> sorted_count(local_address_count[0].size());
  std::partial_sort_copy(local_address_count[0].begin(), local_address_count[0].end(), sorted_count.begin(), sorted_count.end(), [](const std::pair<size_t, uint32_t> &left, const std::pair<size_t, uint32_t> &right) {
    return (left.second > right.second);
  });

  size_t memory_access_count = 0;
  for (auto const &e : sorted_count) {
    memory_access_count += e.second;
    //cout << "address: "<< e.first << " accessed: " << e.second << " times!" << endl;
  }

  cout << "num memory accesses: " << memory_access_count << endl
       << endl;
  cout << "Total Memory Footprint -- num unique memory addresses accessed: " << local_address_count[0].size() << endl;
  cout << "                          num unique memory addresses read:     " << m_storeOps.size() << endl;
  cout << "                          num unique memory addresses written:  " << m_loadOps.size()  << endl;
  cout << "                          unique read/write ratio:              "
       << setprecision(4) << (float) (((double)m_loadOps.size()) / ((double)m_storeOps.size()))  << endl;
  cout << "                          total reads:                          " << load_count    << endl;
  cout << "                          total writes:                         " << store_count    << endl;
  cout << "                          re-reads:                             " << setprecision(4)
       << (float)((double)load_count / (double)m_loadOps.size()) << endl;
  cout << "                          re-writes:                            " << setprecision(4)
       << (float)((double)store_count / (double)m_storeOps.size()) << endl << endl;
  size_t significant_memory_access_count = (size_t)ceil(memory_access_count * 0.9);
  cout << "90\% of memory accesses: " << significant_memory_access_count << endl
       << endl;

  size_t unique_memory_addresses = 0;
  size_t access_count = 0;
  while (access_count < significant_memory_access_count) {
    access_count += sorted_count[unique_memory_addresses].second;
    unique_memory_addresses++;
  }
  cout << "90% Memory Footprint -- num unique memory addresses that cover 90\% of memory accesses: " << unique_memory_addresses << endl
       << endl;
  //cout << "the top 10:" << endl;
  //for (int i = 0; i < 10; i++){
  //    cout << "address: " << sorted_count[i].first << " contributed: " << sorted_count[i].second << " accesses!" << endl;
  //}

  cout << "### Memory Entropy" << endl
       << endl;

  double mem_entropy = 0.0;
  for (const auto &it : sorted_count) {
    uint32_t value = it.second;
    double prob = (double)value * 1.0 / (double)memory_access_count;
    mem_entropy = mem_entropy - prob * std::log2(prob);
  }
  cout << "Global Memory Address Entropy -- measure of the randomness of memory addresses: " << (float)mem_entropy << endl
       << endl;

  cout << "Local Memory Address Entropy -- measure of the spatial locality of memory addresses" << endl
       << endl;

  cout << "|" << setw(12) << right << "LSBs skipped"
       << "|" << setw(8) << right << "Entropy"
       << "|" << endl;
  cout << "|-----------:|-------:|" << endl;
  std::vector<float> loc_entropy;
  for (int nskip = 1; nskip < 11; nskip++) {
    double local_entropy = 0.0;
    for (const auto &it : local_address_count[nskip]) {
      double prob = (double)(it.second) * 1.0 / (double)memory_access_count;
      local_entropy = local_entropy - prob * std::log2(prob);
    }
    loc_entropy.push_back((float)local_entropy);
    cout << "|" << setw(12) << right << nskip << "|" << fixed << setw(8) << setprecision(4) << right << (float)local_entropy << "|" << endl;
  }

  cout << endl
       << "### Parallel Spatial Locality" << endl
       << endl;


  cout << "|" << setw(12) << right << "LSBs skipped"
       << "|" << setw(25) << right << "Normed Parallel Spatial Locality"
       << "|" << endl;
  cout << "|-----------:|------------------------:|" << endl;

  vector<double> avg_psl = vector<double>();
  double avg_psl_sum = 0.0;
  uint32_t items_per_group = (m_local_num[0] * m_local_num[1] * m_local_num[2]);
  for (size_t i = 0; i < m_psl_per_group[0].size(); i++) {
    double avg_psl_i = 0.0;
    for (size_t j = 0; j < m_psl_per_group.size(); j++){
      avg_psl_i += m_psl_per_group[j][i];
    }
    avg_psl_i = (avg_psl_i / double(m_psl_per_group.size())) / std::log2(double(items_per_group + 1));
    avg_psl.push_back(avg_psl_i);
    avg_psl_sum += avg_psl_i;
    cout << "|" << setw(12) << right << i << "|" << fixed << setw(25) << setprecision(4) << right << (float)avg_psl_i << "|" << endl;
  }

  cout << endl
       << "Normed Locality Sum: " << avg_psl_sum
       << endl;

  cout << endl
       << "### Memory Diversity -- Usage of local and constant memory relative to global memory" << endl
       << endl;

  cout << "num global memory accesses: " << m_global_memory_access << endl
       << endl;
  cout << "num local memory accesses: " << m_local_memory_access << endl
       << endl;
  cout << "num constant memory accesses: " << m_constant_memory_access << endl
       << endl;

  uint32_t m_total_memory_access = m_global_memory_access + m_local_memory_access + m_constant_memory_access;

  cout << "\% local memory accesses (local/total): " << setprecision(2) << (((float)m_local_memory_access / (float)m_total_memory_access) * 100) << endl
       << endl;
  cout << "\% constant memory accesses (constant/total): " << setprecision(2) << (((float)m_constant_memory_access / (float)m_total_memory_access) * 100) << endl
       << endl;

  cout << "## Control" << endl
       << endl;

  cout << "Unique Branch Instructions -- Total number of unique branch instructions to cover 90\% of the branches" << endl
       << endl;

  auto instruction_to_id = [](const llvm::Instruction* instruction) -> size_t {
    const llvm::DebugLoc& loc = instruction->getDebugLoc();
    if (loc) {
      return loc.getLine();
    }

    // TODO: Make a more useful ID than instruction address; perhaps function name + instruction offset?
    // Can get program with the following:
    // const Kernel* kernel = m_kernelInvocation->getKernel();
    // const Program* program = kernel->getProgram();
    return (size_t) instruction;
  };

  auto sorted_branch_ops = std::vector<std::pair<size_t, uint32_t>>();

  for (auto const &item : m_branchCounts) {
    size_t id = instruction_to_id(item.first);
    uint32_t count = item.second;
    sorted_branch_ops.push_back({id, count});
  }

  std::sort(sorted_branch_ops.begin(), sorted_branch_ops.end(), [](const std::pair<size_t, uint32_t> &left, const std::pair<size_t, uint32_t> &right) {
    if (left.second != right.second) {
      return left.second > right.second;
    }
    return left.first < right.first;
  });

  cout << "|" << setw(14) << left << "Branch At Line"
       << "|" << setw(23) << right << "Count (hit and miss)"
       << "|" << endl;
  cout << "|--------------|----------------------:|" << endl;
  size_t branch_op_count = 0;
  for (auto const &x : sorted_branch_ops) {
    branch_op_count += x.second;
    cout << "|" << setw(14) << left << x.first << "|" << setw(23) << right << x.second << "|" << endl;
  }
  cout << endl;

  size_t significant_branch_op_count = (size_t)ceil(branch_op_count * 0.9);

  size_t unique_branch_addresses = 0;
  size_t branch_count = 0;
  while (branch_count < significant_branch_op_count) {
    branch_count += sorted_branch_ops[unique_branch_addresses].second;
    unique_branch_addresses++;
  }
  cout << "Number of unique branches that cover 90\% of all branch instructions: " << unique_branch_addresses << endl;

  cout << endl
       << "### Branch Entropy -- measure of the randomness of branch behaviour, representing branch predictability" << endl
       << endl;

  //generate a history pattern
  //(arbitarily selected to a history of m=16 branches?)
  const unsigned m = 16;
  float average_entropy = 0.0f;
  float yokota_entropy = 0.0f;
  float yokota_entropy_per_workload = 0.0f;
  unsigned N = 0;

  for (auto const &branch : m_branchPatterns) {
    for (auto const &h : branch.second) {
      uint16_t pattern = h.first;
      uint32_t number_of_occurrences = h.second;
      //for each history pattern compute the probability of the taken branch
      unsigned taken = 0;
      uint16_t p = pattern;
      // count taken branches using Kernighan's algorithm
      while (p) {
        p &= p - 1;
        taken++;
      }
      unsigned not_taken = m - taken;
      float probability_of_taken = (float)taken / (float)(not_taken + taken);

      //compute Yokota branch entropy
      if (probability_of_taken != 0) {
        yokota_entropy -= number_of_occurrences * probability_of_taken * std::log2(probability_of_taken);
        yokota_entropy_per_workload -= probability_of_taken * std::log2(probability_of_taken);
      }
      //compute linear branch entropy
      float linear_branch_entropy = 2 * std::min(probability_of_taken, 1 - probability_of_taken);
      average_entropy += number_of_occurrences * linear_branch_entropy;
      N += number_of_occurrences;
    }
  }
  average_entropy = average_entropy / N;
  if (isnan(average_entropy)) {
    average_entropy = 0.0;
  }
  cout << "Using a branch history of " << m << endl
       << endl;
  cout << "Yokota Branch Entropy: " << yokota_entropy << endl
       << endl;
  cout << "Yokota Branch Entropy per Workload: " << yokota_entropy_per_workload << endl
       << endl;
  cout << "Average Linear Branch Entropy: " << average_entropy << endl
       << endl;

  std::string logfile_name;

  const char *result_path = getenv("OCLGRIND_WORKLOAD_CHARACTERISATION_OUTPUT_PATH");
  if (result_path != NULL){
    logfile_name = std::string(result_path);
  }else{
    int logfile_count = 0;
    logfile_name = "aiwc_" + kernelInvocation->getKernel()->getName() + "_" + std::to_string(logfile_count) + ".csv";
    while (std::ifstream(logfile_name)) {
      logfile_count++;
      logfile_name = "aiwc_" + kernelInvocation->getKernel()->getName() + "_" + std::to_string(logfile_count) + ".csv";
    }
  }
  std::ofstream logfile;
  logfile.open(logfile_name);
  assert(logfile);
  logfile << "metric,category,count\n";
  logfile << "Opcode,Compute," << major_operations << "\n";
  logfile << "Total Instruction Count,Compute," << total_instruction_count << "\n";
  logfile << "Freedom to Reorder,Compute," << freedom_to_reorder << "\n";
  logfile << "Resource Pressure,Compute," << resource_pressure << "\n";
  logfile << "Work-items,Parallelism," << m_threads_invoked << "\n";
  logfile << "Work-groups,Parallelism," << m_group_num[0] * m_group_num[1] * m_group_num[2] << "\n";
  logfile << "Work-items per Work-group,Parallelism," << m_local_num[0] * m_local_num[1] * m_local_num[2] << "\n";
  logfile << "SIMD Operand Sum,Parallelism," << simd_sum << "\n";
  logfile << "Total Barriers Hit,Parallelism," << m_barriers_hit << "\n";
  logfile << "Min ITB,Parallelism," << itb_min << "\n";
  logfile << "Max ITB,Parallelism," << itb_max << "\n";
  logfile << "Median ITB,Parallelism," << itb_median << "\n";
  logfile << "Min IPT,Parallelism," << ipt_min << "\n";
  logfile << "Max IPT,Parallelism," << ipt_max << "\n";
  logfile << "Median IPT,Parallelism," << ipt_median << "\n";
  logfile << "Max SIMD Width,Parallelism," << std::max_element(m_instructionWidth.begin(), m_instructionWidth.end(), [](const pair_type &a, const pair_type &b) { return a.first < b.first; })->first << "\n";
  logfile << "Mean SIMD Width,Parallelism," << simd_mean << "\n";
  logfile << "SD SIMD Width,Parallelism," << simd_stdev << "\n";
  logfile << "Granularity,Parallelism," << granularity << "\n";
  logfile << "Barriers Per Instruction,Parallelism," << barriers_per_instruction << "\n";
  logfile << "Instructions Per Operand,Parallelism," << instructions_per_operand << "\n";
  logfile << "Total Memory Footprint,Memory," << local_address_count[0].size() << "\n";
  logfile << "Unique Memory Accesses,Memory," << local_address_count[0].size() << "\n";
  logfile << "Unique Reads,Memory," << m_storeOps.size() << "\n";
  logfile << "Unique Writes,Memory," << m_loadOps.size()  << "\n";
  logfile << "Unique Read/Write Ratio,Memory,"
       << setprecision(4) << (float) (((double)m_loadOps.size()) / ((double)m_storeOps.size()))  << "\n";
  logfile << "Total Reads,Memory," << load_count    << "\n";
  logfile << "Total Writes,Memory," << store_count    << "\n";
  logfile << "Rereads,Memory," << setprecision(4) << (float)((double)load_count / (double)m_loadOps.size()) << "\n";
  logfile << "Rewrites,Memory," << setprecision(4) << (float)((double)store_count / (double)m_storeOps.size()) << "\n";

  logfile << "90\% Memory Footprint,Memory," << unique_memory_addresses << "\n";
  logfile << "Global Memory Address Entropy,Memory," << mem_entropy << "\n";
  for (int nskip = 1; nskip < 11; nskip++) {
    logfile << "LMAE -- Skipped " << nskip << " LSBs,Memory," << loc_entropy[nskip - 1] << "\n";
  }
  for (int nskip = 0; nskip < 11; nskip++) {
    logfile << "Normed PSL -- Skipped " << nskip << " LSBs,Memory," << avg_psl[nskip] << "\n";
  }
  logfile << "Normed PSL Sum,Memory," << avg_psl_sum << "\n";
  logfile << "Total Global Memory Accessed,Memory," << m_global_memory_access << "\n";
  logfile << "Total Local Memory Accessed,Memory," << m_local_memory_access << "\n";
  logfile << "Total Constant Memory Accessed,Memory," << m_constant_memory_access << "\n";
  logfile << "Relative Local Memory Usage,Memory," << (((float)m_local_memory_access / (float)m_total_memory_access) * 100) << "\n";
  logfile << "Relative Constant Memory Usage,Memory," << (((float)m_constant_memory_access / (float)m_total_memory_access) * 100) << "\n";
  logfile << "Total Unique Branch Instructions,Control," << sorted_branch_ops.size() << "\n";
  logfile << "90\% Branch Instructions,Control," << unique_branch_addresses << "\n";
  logfile << "Yokota Branch Entropy,Memory," << yokota_entropy_per_workload << "\n";
  logfile << "Average Linear Branch Entropy,Memory," << average_entropy << "\n";
  logfile.close();

  cout << endl
       << "The Architecture-Independent Workload Characterisation was written to file: " << logfile_name << endl;
  // Restore locale
  cout.imbue(previousLocale);

  // Reset kernel counts, ready to start anew
  //m_memoryOps.clear();
  m_kernelInvocation = nullptr;
  m_loadOps.clear();
  m_storeOps.clear();
  m_computeOps.clear();
  m_branchPatterns.clear();
  m_branchCounts.clear();
  m_instructionsToBarrier.clear();
  m_instructionsPerWorkitem.clear();
  m_threads_invoked = 0;
  m_instructionsBetweenLoadOrStore.clear();
  m_loadInstructionLabels.clear();
  m_storeInstructionLabels.clear();
}

void WorkloadCharacterisation::workGroupBegin(const WorkGroup *workGroup) {
  // Create worker state if haven't already
  //if (!m_state.memoryOps) {
  if (!m_state.storeOps) {
    //m_state.memoryOps = new unordered_map<pair<size_t, bool>, uint32_t>;
    m_state.storeOps = new unordered_map<size_t, uint32_t>;
    m_state.loadOps = new unordered_map<size_t, uint32_t>;
    m_state.computeOps = new unordered_map<std::string, size_t>;
    m_state.branchOps = new unordered_map<const llvm::Instruction*, std::vector<bool>>;
    m_state.instructionsBetweenBarriers = new vector<uint32_t>;
    m_state.instructionWidth = new unordered_map<uint16_t, size_t>;
    m_state.instructionsPerWorkitem = new vector<uint32_t>;
    m_state.instructionsBetweenLoadOrStore = new vector<uint32_t>;
    m_state.loadInstructionLabels = new unordered_map<std::string, size_t>;
    m_state.storeInstructionLabels = new unordered_map<std::string, size_t>;
    m_state.ledger = vector<vector<WorkloadCharacterisation::ledgerElement>>
      (m_local_num.x * m_local_num.y * m_local_num.z, vector<ledgerElement>());
    m_state.psl_per_barrier = new vector<pair<vector<double>,uint64_t>>;
  }

  //m_state.memoryOps->clear();
  m_state.storeOps->clear();
  m_state.loadOps->clear();
  m_state.computeOps->clear();
  m_state.branchOps->clear();
  m_state.instructionsBetweenBarriers->clear();
  m_state.instructionWidth->clear();
  m_state.instructionsPerWorkitem->clear();
  m_state.instructionsBetweenLoadOrStore->clear();
  m_state.loadInstructionLabels->clear();
  m_state.storeInstructionLabels->clear();

  m_state.threads_invoked = 0;
  m_state.instruction_count = 0;
  m_state.barriers_hit = 0;

  //memory type access count variables
  m_state.constant_memory_access_count = 0;
  m_state.local_memory_access_count = 0;
  m_state.global_memory_access_count = 0;

  //branch logic variables
  m_state.previous_instruction_is_branch = false;
  m_state.target1 = nullptr;
  m_state.target2 = nullptr;
  m_state.branch_loc = nullptr;

  for (size_t i = 0; i < (m_state.ledger).size(); i++)
    (m_state.ledger)[i].clear();
}

void WorkloadCharacterisation::workGroupComplete(const WorkGroup *workGroup) {

  lock_guard<mutex> lock(m_mtx);
  // merge operation counts back into global unordered map
  for (auto const &item : (*m_state.computeOps))
    m_computeOps[item.first] += item.second;

  // merge memory operations into global list
  // for (auto const &item : (*m_state.memoryOps))
  //   m_memoryOps[item.first] += item.second;

  for (auto const &item : (*m_state.storeOps))
    m_storeOps[item.first] += item.second;

  for (auto const &item : (*m_state.loadOps))
    m_loadOps[item.first] += item.second;

  // merge control operations into global unordered maps
  const unsigned m = 16;
  for (auto const &branch : (*m_state.branchOps)) {
    m_branchCounts[branch.first] += branch.second.size();

    // compute branch patterns
    //if we have fewer branches than the history window, skip it.
    if (branch.second.size() < m)
      continue;

    // generate the set of history patterns - one bit per branch encounter
    std::unordered_map<uint16_t, uint32_t> H;
    uint16_t current_pattern = 0;
    for (unsigned i = 0; i < branch.second.size(); i++) {
      bool b = branch.second[i];
      current_pattern = (current_pattern << 1) | (b ? 1 : 0);
      if (i >= m - 1) {
        // we now have m bits of pattern to compare
        m_branchPatterns[branch.first][current_pattern]++;
      }
    }
  }

  // add the current work-group item / thread counter to the global variable
  m_threads_invoked += m_state.threads_invoked;

  // add the instructions between barriers back to the global setting
  for (auto const &item : (*m_state.instructionsBetweenBarriers))
    m_instructionsToBarrier.push_back(item);

  m_barriers_hit += m_state.barriers_hit;

  // add the SIMD scores back to the global setting
  for (auto const &item : (*m_state.instructionWidth))
    m_instructionWidth[item.first] += item.second;

  // add the instructions executed per workitem scores back to the global setting
  for (auto const &item : (*m_state.instructionsPerWorkitem))
    m_instructionsPerWorkitem.push_back(item);

  // add the instruction reordering (flexability) metrics
  for (auto const &item : (*m_state.instructionsBetweenLoadOrStore))
    m_instructionsBetweenLoadOrStore.push_back(item);

  for (auto const &item : (*m_state.loadInstructionLabels))
    m_loadInstructionLabels[item.first] += item.second;

  for (auto const &item : (*m_state.storeInstructionLabels))
    m_storeInstructionLabels[item.first] += item.second;

  // merge memory type access count variables
  m_constant_memory_access += m_state.constant_memory_access_count;
  m_local_memory_access += m_state.local_memory_access_count;
  m_global_memory_access += m_state.global_memory_access_count;

  vector<double> psl = parallelSpatialLocality(m_state.ledger);
  size_t maxLength = 0;
  for (size_t i = 0; i < m_state.ledger.size(); i++) {
    maxLength = m_state.ledger[i].size() > maxLength ? m_state.ledger[i].size() : maxLength;
    m_state.ledger[i].clear();
  }

  m_state.psl_per_barrier->push_back(std::make_pair(psl, maxLength));

  maxLength = 0;
  vector<double> weighted_avg_psl = vector<double>(11, 0.0);
  for (const auto &elem : *m_state.psl_per_barrier) {
    maxLength += elem.second;
    for (size_t nskip = 0; nskip < 11; nskip++) {
      weighted_avg_psl[nskip] += elem.first[nskip] * elem.second;
    }
  }

  if (maxLength != 0) {
    for (size_t nskip = 0; nskip < 11; nskip++) {
      weighted_avg_psl[nskip] = weighted_avg_psl[nskip] / static_cast<float>(maxLength + 1);
    }
  }
  m_psl_per_group.push_back(weighted_avg_psl);
}
