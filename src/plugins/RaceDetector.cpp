// RaceDetector.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include "core/Context.h"
#include "core/KernelInvocation.h"
#include "core/Memory.h"
#include "core/WorkGroup.h"
#include "core/WorkItem.h"

#include "RaceDetector.h"

using namespace oclgrind;
using namespace std;

#define KEY(memory,address) make_pair(memory, EXTRACT_BUFFER(address))

RaceDetector::RaceDetector(const Context *context)
 : Plugin(context)
{
  m_kernelInvocation = NULL;

  m_allowUniformWrites = !checkEnv("OCLGRIND_UNIFORM_WRITES");
}

bool RaceDetector::isThreadSafe() const
{
  // TODO: Improve DRD efficiency for multi-threaded case instead.
  return false;
}

void RaceDetector::kernelBegin(const KernelInvocation *kernelInvocation)
{
  m_kernelInvocation = kernelInvocation;
}

void RaceDetector::kernelEnd(const KernelInvocation *kernelInvocation)
{
  synchronize(m_context->getGlobalMemory(), false);

  m_kernelInvocation = NULL;
}

void RaceDetector::memoryAllocated(const Memory *memory, size_t address,
                                   size_t size, cl_mem_flags flags,
                                   const uint8_t *initData)
{
  if (memory->getAddressSpace() == AddrSpacePrivate ||
      memory->getAddressSpace() == AddrSpaceConstant)
    return;

  m_state[KEY(memory,address)] = make_pair(new State[size], size);
}

void RaceDetector::memoryAtomicLoad(const Memory *memory,
                                    const WorkItem *workItem,
                                    AtomicOp op, size_t address, size_t size)
{
  registerAtomic(memory, workItem, address, size, false);
}

void RaceDetector::memoryAtomicStore(const Memory *memory,
                                     const WorkItem *workItem,
                                     AtomicOp op, size_t address, size_t size)
{
  registerAtomic(memory, workItem, address, size, true);
}

void RaceDetector::memoryDeallocated(const Memory *memory, size_t address)
{
  if (memory->getAddressSpace() == AddrSpacePrivate ||
      memory->getAddressSpace() == AddrSpaceConstant)
    return;

  delete[] m_state[KEY(memory,address)].first;
  m_state.erase(KEY(memory,address));
}

void RaceDetector::memoryLoad(const Memory *memory, const WorkItem *workItem,
                              size_t address, size_t size)
{
  registerLoadStore(memory, workItem, workItem->getWorkGroup(),
                    address, size, NULL);
}

void RaceDetector::memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                              size_t address, size_t size)
{
  registerLoadStore(memory, NULL, workGroup, address, size, NULL);
}

void RaceDetector::memoryStore(const Memory *memory, const WorkItem *workItem,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  registerLoadStore(memory, workItem, workItem->getWorkGroup(),
                    address, size, storeData);
}

void RaceDetector::memoryStore(const Memory *memory, const WorkGroup *workGroup,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  registerLoadStore(memory, NULL, workGroup, address, size, storeData);
}

void RaceDetector::logRace(DataRaceType type,
                           unsigned int addrSpace,
                           size_t address,
                           size_t lastWorkGroup,
                           size_t lastWorkItem,
                           const llvm::Instruction *lastInstruction) const
{
  const char *raceType = NULL;
  switch (type)
  {
    case ReadWriteRace:
      raceType = "Read-write";
      break;
    case WriteWriteRace:
      raceType = "Write-write";
      break;
  }

  Context::Message msg(ERROR, m_context);
  msg << raceType << " data race at "
      << getAddressSpaceName(addrSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << endl
      << "First entity:  " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl
      << endl
      << "Second entity: ";

  // Show details of other entity involved in race
  if (lastWorkItem != -1)
  {
    Size3 global(lastWorkItem, m_kernelInvocation->getGlobalSize());
    Size3 local, group;
    local.x = global.x % m_kernelInvocation->getLocalSize().x;
    local.y = global.y % m_kernelInvocation->getLocalSize().y;
    local.z = global.z % m_kernelInvocation->getLocalSize().z;
    group.x = global.x / m_kernelInvocation->getLocalSize().x;
    group.y = global.y / m_kernelInvocation->getLocalSize().y;
    group.z = global.z / m_kernelInvocation->getLocalSize().z;
    msg << "Global" << global << " Local" << local << " Group" << group;
  }
  else if (lastWorkGroup != -1)
  {
    msg << "Group"
        << Size3(lastWorkGroup, m_kernelInvocation->getNumGroups());
  }
  else
  {
    msg << "(unknown)";
  }
  msg << endl
      << lastInstruction << endl;
  msg.send();
}

void RaceDetector::registerAtomic(const Memory *memory,
                                  const WorkItem *workItem,
                                  size_t address, size_t size,
                                  bool store)
{
  if (!memory->isAddressValid(address, size))
    return;

  State *state = m_state[KEY(memory,address)].first + EXTRACT_OFFSET(address);

  // Get work-item index
  size_t workItemIndex = workItem->getGlobalIndex();

  bool race = false;
  for (size_t offset = 0; offset < size; offset++, state++)
  {
    // Check for races with non-atomic operations
    bool conflict = store ? !state->canAtomicStore : !state->canAtomicLoad;
    if (!race && conflict && workItemIndex != state->workItem)
    {
      logRace(ReadWriteRace,
              memory->getAddressSpace(),
              address,
              state->workItem,
              state->workGroup,
              state->instruction);
      race = true;
    }

    // Update state
    if (store)
      state->canLoad = false;
    state->canStore = false;
    if (!state->wasWorkItem)
    {
      state->instruction = workItem->getCurrentInstruction();
      state->workItem = workItemIndex;
      state->wasWorkItem = true;
    }
  }
}

void RaceDetector::registerLoadStore(const Memory *memory,
                                     const WorkItem *workItem,
                                     const WorkGroup *workGroup,
                                     size_t address, size_t size,
                                     const uint8_t *storeData)
{
  if (!m_kernelInvocation)
    return;
  if (memory->getAddressSpace() == AddrSpacePrivate ||
      memory->getAddressSpace() == AddrSpaceConstant)
    return;
  if (!memory->isAddressValid(address, size))
    return;

  bool load = !storeData;
  bool store = storeData;

  // Get index of work-item and work-group performing access
  size_t workItemIndex = -1, workGroupIndex = -1;
  if (workItem)
  {
    workItemIndex = workItem->getGlobalIndex();
  }
  if (workGroup)
  {
    workGroupIndex = workGroup->getGroupIndex();
  }

  bool race = false;
  size_t base = EXTRACT_OFFSET(address);
  State *state = m_state[KEY(memory, address)].first + base;

  for (size_t offset = 0; offset < size; offset++, state++)
  {
    bool conflict = store ? !state->canStore : !state->canLoad;
    if (m_allowUniformWrites && storeData)
    {
      uint8_t *ptr = (uint8_t*)(memory->getPointer(address));
      conflict &= (ptr[offset] != storeData[offset]);
    }

    if (!race && conflict &&
        (state->wasWorkItem ?                // If state set by work-item,
         state->workItem != workItemIndex :  // must be same work-item,
         state->workGroup != workGroupIndex) // otherwise must be same group
        )
    {
      // Report data-race
      DataRaceType type = load|state->canLoad ? ReadWriteRace : WriteWriteRace;
      logRace(type, memory->getAddressSpace(),
              address + offset,
              state->workItem,
              state->workGroup,
              state->instruction);
      race = true;
    }
    else
    {
      // Only update WI info if this operation is stronger than previous one
      bool updateWI = store || (load && state->canStore);

      // Update state
      if (store)
        state->canAtomicLoad = false;
      state->canAtomicStore = false;
      state->canLoad &= load;
      state->canStore = false;
      if (updateWI)
      {
        state->workGroup = workGroupIndex;
        if (workItem)
        {
          state->instruction = workItem->getCurrentInstruction();
          state->workItem = workItemIndex;
          state->wasWorkItem = true;
        }
      }
    }
  }
}

void RaceDetector::synchronize(const Memory *memory, bool workGroup)
{
  StateMap::iterator itr;
  for (itr = m_state.begin(); itr != m_state.end(); itr++)
  {
    if (itr->first.first != memory)
      continue;

    pair<State*,size_t> obj = itr->second;
    for (State *state = obj.first; state < obj.first+obj.second; state++)
    {
      // TODO: atomic_intergroup_race test failure
      state->canAtomicLoad = true;
      state->canAtomicStore = true;
      state->workItem = -1;
      state->wasWorkItem = false;
      if (!workGroup)
      {
        state->workGroup = -1;
        state->canLoad = true;
        state->canStore = true;
      }
    }
  }
}

void RaceDetector::workGroupBarrier(const WorkGroup *workGroup, uint32_t flags)
{
  if (flags & CLK_LOCAL_MEM_FENCE)
    synchronize(workGroup->getLocalMemory(), false);
  if (flags & CLK_GLOBAL_MEM_FENCE)
    synchronize(m_context->getGlobalMemory(), true);
}

RaceDetector::State::State()
{
  instruction = NULL;
  workItem = -1;
  workGroup = -1;
  canAtomicLoad = true;
  canAtomicStore = true;
  canLoad = true;
  canStore = true;
  wasWorkItem = false;
}
