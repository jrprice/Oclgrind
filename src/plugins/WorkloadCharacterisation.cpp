// WorkloadCharacterisation.cpp (Oclgrind)
// Copyright (c) 2017, Beau Johnston,
// The Australian National University. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include <sstream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <fstream>

#include <csignal>
#include <iomanip>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/raw_ostream.h"

#include "WorkloadCharacterisation.h"

#include "core/Kernel.h"
#include "core/KernelInvocation.h"
#include "core/WorkGroup.h"
#include "core/WorkItem.h"

using namespace oclgrind;
using namespace std;

#define COUNTED_LOAD_BASE  (llvm::Instruction::OtherOpsEnd + 4)
#define COUNTED_STORE_BASE (COUNTED_LOAD_BASE + 8)
#define COUNTED_CALL_BASE  (COUNTED_STORE_BASE + 8)

THREAD_LOCAL WorkloadCharacterisation::WorkerState
WorkloadCharacterisation::m_state = {NULL};

void WorkloadCharacterisation::memoryLoad(const Memory *memory, const WorkItem *workItem,size_t address, size_t size){
    m_state.memoryOps->push_back(std::make_pair(address,//address
                size));//size (in bytes)
}

void WorkloadCharacterisation::memoryStore(const Memory *memory, const WorkItem *workItem,size_t address, size_t size, const uint8_t *storeData){
    m_state.memoryOps->push_back(std::make_pair(address,//address
                size));//size (in bytes)
}

void WorkloadCharacterisation::memoryAtomicLoad(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size)
{
    m_state.memoryOps->push_back(std::make_pair(address,//address
                size));//size (in bytes)
}

void WorkloadCharacterisation::memoryAtomicStore(const Memory *memory, const WorkItem *workItem, AtomicOp op, size_t address, size_t size)
{
    m_state.memoryOps->push_back(std::make_pair(address,//address
                size));//size (in bytes)
}

void WorkloadCharacterisation::instructionExecuted(
        const WorkItem *workItem, const llvm::Instruction *instruction,
        const TypedValue& result)
{

    unsigned opcode = instruction->getOpcode();
    std::string opcode_name = llvm::Instruction::getOpcodeName(opcode);
    (*m_state.computeOps)[opcode_name]++;

    //collect conditional branches and the associated trace to count which ones were taken and which weren't
    if (m_state.previous_instruction_is_branch == true){
        std::string Str;
        llvm::raw_string_ostream OS(Str);
        instruction->getParent()->printAsOperand(OS,false);
        OS.flush();
        if(Str == m_state.target1)
            (*m_state.branchOps)[m_state.branch_loc].push_back(true);//taken
        else if(Str == m_state.target2){
            (*m_state.branchOps)[m_state.branch_loc].push_back(false);//not taken
        }else{
            cout << "Error in branching!" << endl;
            cout << "Str was: " << Str << " but target was either: " << m_state.target1 << " or: " << m_state.target2 << endl;
            std::raise(SIGINT);
        }
        m_state.previous_instruction_is_branch = false;
    }
    //if a conditional branch -- they have labels
    if (opcode == llvm::Instruction::Br){
        if(instruction->getOperand(1)->getType()->isLabelTy() && instruction->getOperand(2)->getType()->isLabelTy()){
            m_state.previous_instruction_is_branch = true;
            std::string Str;
            llvm::raw_string_ostream OS(Str);
            instruction->getOperand(1)->printAsOperand(OS,false);
            OS.flush();
            m_state.target1 = Str;
            Str = "";
            instruction->getOperand(2)->printAsOperand(OS,false);
            OS.flush();
            m_state.target2 = Str;
            llvm::DebugLoc loc = instruction->getDebugLoc();
            m_state.branch_loc = loc.getLine();
        }
    }

    //counter for instructions to barrier and other parallelism metrics
    m_state.instruction_count++;

    //SIMD instruction width metrics use the following
    m_state.instructionWidth->push_back(result.num);

    //TODO: add support for Phi, Switch and Select control operations
}

void WorkloadCharacterisation::workItemBarrier(const WorkItem *workItem)
{
    m_state.barriers_hit++;
    m_state.instructionsBetweenBarriers->push_back(m_state.instruction_count);
    m_state.instruction_count = 0;
}

void WorkloadCharacterisation::workItemClearBarrier(const WorkItem *workItem){
    m_state.instruction_count = 0;
}

void WorkloadCharacterisation::workItemBegin(const WorkItem *workItem)
{
    m_state.threads_invoked++;
    m_state.instruction_count = 0;
}

void WorkloadCharacterisation::workItemComplete(const WorkItem *workItem)
{
    m_state.instructionsBetweenBarriers->push_back(m_state.instruction_count);
    m_state.instruction_count = 0;
}

void WorkloadCharacterisation::kernelBegin(const KernelInvocation *kernelInvocation)
{
    m_memoryOps.clear();
    m_computeOps.clear();
    m_branchOps.clear();
    m_instructionsToBarrier.clear();
    m_instructionWidth.clear();
    m_threads_invoked = 0;
    m_barriers_hit = 0;
}

void WorkloadCharacterisation::kernelEnd(const KernelInvocation *kernelInvocation)
{
    // Load default locale
    locale previousLocale = cout.getloc();
    locale defaultLocale("");
    cout.imbue(defaultLocale);
    cout << "Architecture-Independent Workload Characterization of kernel: " << kernelInvocation->getKernel()->getName() << endl;

    cout << "+----------------------------------------------------------------------------+" << endl;
    cout << "|Compute Opcode Instruction Histogram                                        |" << endl;
    cout << "+============================================================================+" << endl;

    for(auto const& item: m_computeOps)
        cout << "instruction: " << item.first << " count: " << item.second << endl;

    cout << "+----------------------------------------------------------------------------+" << endl;
    cout << "|Compute Opcode Unique Opcodes required to cover 90\% of Dynamic Instructions |" << endl;
    cout << "+============================================================================+" << endl;

    std::vector<std::pair<std::string,size_t> > sorted_ops;
    for(auto const& item: m_computeOps)
        sorted_ops.push_back(make_pair(item.first,item.second)); 

    std::sort(sorted_ops.begin(),sorted_ops.end(),[](const std::pair<std::string,size_t> &left, const std::pair<std::string,size_t> &right){
            return (left.second > right.second);
            });

    size_t operation_count = 0;

    for(auto const& item: sorted_ops)
        operation_count += item.second;

    size_t significant_operation_count = (unsigned)ceil(operation_count * 0.9);

    size_t major_operations = 0;
    size_t total_instruction_count = operation_count;
    operation_count = 0;

    cout << "Unique Op Codes comprising of 90\% of dynamic instructions:" << endl;
    while (operation_count < significant_operation_count){
        operation_count += sorted_ops[major_operations].second;
        cout << "\t" << sorted_ops[major_operations].first << endl;
        major_operations++;
    }

    cout << "Unique Opcodes required to cover 90\% of Dynamic Instructions: " << major_operations << endl;
    cout << "Total Instruction Count: " << total_instruction_count << endl;

    cout << "+--------------------------------------------------------------------------+" << endl;
    cout << "|Instruction Level Parallelism                                             |" << endl;
    cout << "+==========================================================================+" << endl;

    cout << "# of workitems invoked: " << m_threads_invoked << endl;
    cout << "total # of workitem barriers hit: " << m_barriers_hit << endl;
    //cout << "# of barriers hit per thread: " << m_instructionsToBarrier.size()/m_threads_invoked << endl;
    cout << "Min instructions to barrier: " << *std::min_element(m_instructionsToBarrier.begin(),m_instructionsToBarrier.end()) << endl;
    cout << "Max instructions to barrier: " << *std::max_element(m_instructionsToBarrier.begin(),m_instructionsToBarrier.end()) << endl;

    double median_itb;
    std::vector<float> itb = m_instructionsToBarrier;
    sort(itb.begin(), itb.end());

    size_t size = itb.size();
    if (size  % 2 == 0)
    {
        median_itb = (itb[size / 2 - 1] + itb[size / 2]) / 2;
    }
    else 
    {
        median_itb = itb[size / 2];
    }
    cout << "Median instructions to barrier: " << median_itb << endl;

    cout << "+--------------------------------------------------------------------------+" << endl;
    cout << "|Data Level Parallelism                                                    |" << endl;
    cout << "+==========================================================================+" << endl;
    cout << "Min data width: " << *std::min_element(m_instructionWidth.begin(),m_instructionWidth.end()) << endl;
    cout << "Max data width: " << *std::max_element(m_instructionWidth.begin(),m_instructionWidth.end()) << endl;

    double simd_sum = std::accumulate(m_instructionWidth.begin(), m_instructionWidth.end(), 0.0);
    double simd_mean = simd_sum / m_instructionWidth.size();
    std::vector<double> diff(m_instructionWidth.size());
    std::transform(m_instructionWidth.begin(), m_instructionWidth.end(), diff.begin(), [simd_mean](double x) { return x - simd_mean; });
    double simd_sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double simd_stdev = std::sqrt(simd_sq_sum / m_instructionWidth.size());
    cout << "Operand sum: " << simd_sum << endl;
    cout << "Mean data width: " << simd_mean << endl;
    cout << "stdev data width: "<< simd_stdev << endl;
    
    double granularity = 1.0/static_cast<double>(total_instruction_count);
    double barriers_per_instruction = static_cast<double>(m_barriers_hit+1)/static_cast<double>(total_instruction_count);
    double instructions_per_operand = static_cast<double>(total_instruction_count)/simd_sum;

    cout << "Granularity: " << granularity << endl;
    cout << "Barriers Per Instruction : " << barriers_per_instruction << endl;
    cout << "Instructions Per Operand : " << instructions_per_operand << endl;

    cout << "+--------------------------------------------------------------------------+" << endl;
    cout << "|Total Memory Footprint -- total number of unique memory addresses accessed|" << endl;
    cout << "+==========================================================================+" << endl;

    std::vector<unsigned> addresses;
    for(const auto& it : m_memoryOps){
        addresses.push_back(it.first);
    }

    std::vector<unsigned> sorted_addresses = addresses;

    std::sort(sorted_addresses.begin(),sorted_addresses.end());

    std::vector<unsigned> unique_sorted_addresses = sorted_addresses;
    std::vector<unsigned>::iterator unique_it;

    unique_it = std::unique(unique_sorted_addresses.begin(),unique_sorted_addresses.end());
    unique_sorted_addresses.resize(std::distance(unique_sorted_addresses.begin(),unique_it));

    cout << unique_sorted_addresses.size() << endl;

    cout << "+----------------------------------------------------------------------------------------------+" << endl;
    cout << "|90% Memory Footprint -- Number of unique memory addresses that cover 90\% of memory accesses   |" << endl;
    cout << "+==============================================================================================+" << endl;

    unordered_map<unsigned, size_t> count;
    for (unsigned i=0; i<addresses.size(); i++)        
        count[addresses[i]]++;

    std::vector<std::pair<unsigned,size_t> > sorted_count;
    for(auto const& umap: count)
        sorted_count.push_back(make_pair(umap.first,umap.second)); 

    std::sort(sorted_count.begin(),sorted_count.end(),[](const std::pair<unsigned,size_t> &left, const std::pair<unsigned,size_t> &right){
            return (left.second > right.second);
            });

    size_t memory_access_count = 0;
    for(auto const& e : sorted_count){
        memory_access_count += e.second;
        //cout << "address: "<< e.first << " accessed: " << e.second << " times!" << endl;
    }

    //cout << "total number of memory accesses = " << memory_access_count << endl;
    size_t significant_memory_access_count = (unsigned)ceil(memory_access_count * 0.9);
    //cout << "90\% of memory accesses:" << significant_memory_access_count << endl;

    size_t unique_memory_addresses = 0;
    size_t access_count = 0;
    while (access_count < significant_memory_access_count){
        access_count += sorted_count[unique_memory_addresses].second;
        unique_memory_addresses++;
    }
    cout << "Number of unique memory addresses that cover 90\% of memory accesses: " << unique_memory_addresses << endl;
    //cout << "the top 10:" << endl;
    //for (int i = 0; i < 10; i++){
    //    cout << "address: " << sorted_count[i].first << " contributed: " << sorted_count[i].second << " accesses!" << endl;
    //}

    cout << "+----------------------------------------------------------------------------------------------+" << endl;
    cout << "|Global Memory Address Entropy -- Measure of the randomness of memory addresses                |" << endl;
    cout << "+==============================================================================================+" << endl;
    float mem_entropy = 0.0f;
    for(const auto& it : sorted_count){
        int value = (int)it.second;
        float prob = (float)value * 1.0 / (float)memory_access_count;
        mem_entropy = mem_entropy - prob * std::log2(prob);
    }
    cout << mem_entropy << endl;

    cout << "+----------------------------------------------------------------------------------------------+" << endl;
    cout << "|Local Memory Address Entropy -- Measure of the spatial locality of memory addresses           |" << endl;
    cout << "+==============================================================================================+" << endl;

    cout << "LSBs skipped\tEntropy (bits)" << endl;
    for(int nskip = 1; nskip < 11; nskip++){
        float skip_value = pow(2,nskip);
        unordered_map<unsigned, size_t> local_address_count;
        for (unsigned i=0; i<addresses.size(); i++){
            int local_addr = int(int(addresses[i]) / skip_value);
            local_address_count[local_addr]++;
        } 

        float local_entropy = 0.0f;
        for(const auto& it : local_address_count){
            int value = (int)it.second;
            float prob = (float)value * 1.0 / (float)memory_access_count;
            local_entropy = local_entropy - prob * std::log2(prob);
        }
        cout << nskip << "\t\t" << local_entropy << endl;
    }

    cout << "+-------------------------------------------------------------------------------------------------------+" << endl;
    cout << "|Unique Branch Instructions -- Total number of unique branch instructions to cover 90\% of the branches|" << endl;
    cout << "+=======================================================================================================+" << endl;

    std::vector<std::pair<unsigned,std::vector<bool>> > sorted_branch_ops;
    for(auto const& umap: m_branchOps)
        sorted_branch_ops.push_back(make_pair(umap.first,umap.second)); 

    std::sort(sorted_branch_ops.begin(),sorted_branch_ops.end(),[](const std::pair<unsigned,std::vector<bool>> &left, const std::pair<unsigned,std::vector<bool>> &right){
            return (left.second.size() > right.second.size());
            });

    cout << "Branch At Line\tCount (hit and miss)" << endl;
    size_t branch_op_count = 0;
    for(auto const& x: sorted_branch_ops){
        branch_op_count += x.second.size();
        cout << x.first << "\t\t" << x.second.size() << endl;
    }

    size_t significant_branch_op_count = (unsigned)ceil(branch_op_count * 0.9);

    size_t unique_branch_addresses = 0;
    size_t branch_count = 0;
    while (branch_count < significant_branch_op_count){
        branch_count += sorted_branch_ops[unique_branch_addresses].second.size();
        unique_branch_addresses++;
    }
    cout << "Number of unique branches that cover 90\% of all branch instructions: " << unique_branch_addresses << endl;

    cout << "+-------------------------------------------------------------------------------------------------------+" << endl;
    cout << "|Branch Entropy -- Measure of the randomness of branch behaviour, representing branch predicability     |" << endl;
    cout << "+=======================================================================================================+" << endl;

    //generate a history pattern
    //(arbitarily selected to a history of m=16 branches?)
    const unsigned m = 16;
    float average_entropy = 0.0f;
    float yokota_entropy = 0.0f;
    float yokota_entropy_per_workload = 0.0f;
    unsigned N = 0;

    for(auto const& branch: m_branchOps){
        //generate the set of history patterns
        std::unordered_map<std::string,unsigned> H;
        for(unsigned i = 0; i < branch.second.size()-m-1; i++){
            std::string current_pattern = "";
            for(unsigned j = 0; j < m; j++){
                bool b = branch.second[i+j];
                current_pattern += b ? '1' : '0';
            }
            H[current_pattern]++;
        }

        for(auto const& h: H){
            std::string pattern = h.first;
            unsigned number_of_occurances = h.second;
            //for each history pattern compute the probability of the taken branch
            unsigned taken =  std::count(pattern.begin(),pattern.end(),'1');
            unsigned not_taken =  std::count(pattern.begin(),pattern.end(),'0');
            float probability_of_taken = (float)taken/(float)(not_taken+taken);

            //compute Yokota branch entropy
            if(probability_of_taken != 0){
                yokota_entropy -= number_of_occurances * probability_of_taken * std::log2(probability_of_taken);
                yokota_entropy_per_workload -= probability_of_taken * std::log2(probability_of_taken);
            }
            //compute linear branch entropy    
            float linear_branch_entropy = 2 * std::min(probability_of_taken, 1-probability_of_taken);
            average_entropy += number_of_occurances * linear_branch_entropy;
            N += number_of_occurances;
        }
    }
    average_entropy = average_entropy/N;
    cout << "Using a branch history of " << m << endl;
    cout << "Yokota Branch Entropy: " << yokota_entropy << endl;
    cout << "Yokota Branch Entropy Per Workload: " << yokota_entropy_per_workload << endl;
    cout << "Average Linear Branch Entropy: " << average_entropy << endl;

    int logfile_count = 0;
    std::string logfile_name = "aiwc_" + kernelInvocation->getKernel()->getName() + "_" + std::to_string(logfile_count) + ".csv";
    while(std::ifstream(logfile_name)){
        logfile_count ++;
        logfile_name = "aiwc_" + kernelInvocation->getKernel()->getName() + "_" + std::to_string(logfile_count) + ".csv";
    }
    std::ofstream logfile;
    logfile.open(logfile_name);
    assert(logfile);
    logfile << "metric,count\n";
    logfile << "opcode," << major_operations << "\n";
    logfile << "total instruction count," << total_instruction_count << "\n";
    logfile << "workitems," << m_threads_invoked << "\n";
    logfile << "operand sum," << simd_sum << "\n";
    logfile << "total # of barriers hit," << m_barriers_hit << "\n";
    logfile << "min instructions to barrier," << *std::min_element(m_instructionsToBarrier.begin(),m_instructionsToBarrier.end()) << "\n";
    logfile << "max instructions to barrier," << *std::max_element(m_instructionsToBarrier.begin(),m_instructionsToBarrier.end()) << "\n";
    logfile << "median instructions to barrier," << median_itb << "\n";
    logfile << "max simd width," << *std::max_element(m_instructionWidth.begin(),m_instructionWidth.end()) << "\n";
    logfile << "mean simd width," << simd_mean << "\n";
    logfile << "stdev simd width,"<< simd_stdev << "\n";
    logfile << "granularity," << granularity << "\n";
    logfile << "barriers per instruction," << barriers_per_instruction << "\n";
    logfile << "instructions per operand," << instructions_per_operand << "\n";
    logfile << "total memory footprint," << unique_sorted_addresses.size() << "\n";
    logfile << "90\% memory footprint," << unique_memory_addresses  << "\n";
    logfile << "global memory address entropy," << mem_entropy << "\n";
    for(int nskip = 1; nskip < 11; nskip++){
        float skip_value = pow(2,nskip);
        unordered_map<unsigned, size_t> local_address_count;
        for (unsigned i=0; i<addresses.size(); i++){
            int local_addr = int(int(addresses[i]) / skip_value);
            local_address_count[local_addr]++;
        } 

        float local_entropy = 0.0f;
        for(const auto& it : local_address_count){
            int value = (int)it.second;
            float prob = (float)value * 1.0 / (float)memory_access_count;
            local_entropy = local_entropy - prob * std::log2(prob);
        }
        logfile << "local memory address entropy -- " << nskip << " LSBs skipped," << local_entropy << "\n";
    }
    logfile << "total unique branch instructions," << sorted_branch_ops.size() << "\n";
    logfile << "90\% branch instructions," << unique_branch_addresses << "\n";
    logfile << "branch entropy (yokota)," << yokota_entropy_per_workload << "\n";
    logfile << "branch entropy (average linear)," << average_entropy << "\n";
    logfile.close();
    //ITP -- SIMT logfile
    std::string itb_logfile_name = logfile_name;
    itb_logfile_name.replace(itb_logfile_name.end()-4,itb_logfile_name.end(),"_itb.log");
    logfile.open(itb_logfile_name, std::ofstream::out | std::ofstream::app);
    for(const auto& it : m_instructionsToBarrier)
        logfile << it << "\n";
    logfile.close();
    cout << "The Architecture-Independent Workload Characterisation was written to file: " << logfile_name << endl;
    // Restore locale
    cout.imbue(previousLocale);

    // Reset kernel counts, ready to start anew
    m_memoryOps.clear();
    m_computeOps.clear();
    m_branchOps.clear();
    m_instructionsToBarrier.clear();
    m_threads_invoked = 0;
}

void WorkloadCharacterisation::workGroupBegin(const WorkGroup *workGroup)
{
    // Create worker state if haven't already
    if (!m_state.memoryOps)
    {
        m_state.memoryOps = new vector<std::pair<size_t,size_t>>;
        m_state.computeOps = new unordered_map<std::string,size_t>;
        m_state.branchOps = new unordered_map<unsigned,std::vector<bool>>;
        m_state.instructionsBetweenBarriers = new vector<unsigned>;
        m_state.instructionWidth = new vector<unsigned>;
    }

    m_state.memoryOps->clear();
    m_state.computeOps->clear();
    m_state.branchOps->clear();
    m_state.instructionsBetweenBarriers->clear();
    m_state.instructionWidth->clear();

    m_state.threads_invoked=0;
    m_state.instruction_count=0;
    m_state.barriers_hit = 0;

    //branch logic variables
    m_state.previous_instruction_is_branch=false;
    m_state.target1="";
    m_state.target2="";
    m_state.branch_loc=0;
    
}

void WorkloadCharacterisation::workGroupComplete(const WorkGroup *workGroup)
{

    lock_guard<mutex> lock(m_mtx);

    // merge operation counts back into global unordered map
    for(auto const& item: (*m_state.computeOps))
        m_computeOps[item.first]+=item.second;

    // merge memory operations into global list
    for(unsigned i = 0; i < m_state.memoryOps->size(); i++){
        m_memoryOps.push_back(m_state.memoryOps->back());
        m_state.memoryOps->pop_back();
    }

    // merge control operations into global unordered map
    for(auto const& item: (*m_state.branchOps))
        for(auto const& branch_taken:item.second)
            m_branchOps[item.first].push_back(branch_taken);

    // add the current work-group item / thread counter to the global variable
    m_threads_invoked += m_state.threads_invoked;

    // add the instructions between barriers back to the global setting
    for(auto const& item: (*m_state.instructionsBetweenBarriers))
        m_instructionsToBarrier.push_back(item);
    
    m_barriers_hit += m_state.barriers_hit;

    // add the SIMD scores back to the global setting
    for(auto const& item: (*m_state.instructionWidth))
        m_instructionWidth.push_back(item);

}

