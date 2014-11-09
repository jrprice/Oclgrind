// RaceDetector.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
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

  const char *uniformWrites = getenv("OCLGRIND_UNIFORM_WRITES");
  m_allowUniformWrites = !(uniformWrites && strcmp(uniformWrites, "1") == 0);
}

void RaceDetector::kernelBegin(KernelInvocation *kernelInvocation)
{
  m_kernelInvocation = kernelInvocation;
}

void RaceDetector::kernelEnd(KernelInvocation *kernelInvocation)
{
  synchronize(m_context->getGlobalMemory(), false);

  m_kernelInvocation = NULL;
}

void RaceDetector::memoryAllocated(const Memory *memory, size_t address,
                                   size_t size)
{
  if (memory->getAddressSpace() == AddrSpacePrivate)
    return;

  m_state[KEY(memory,address)] = make_pair(new State[size], size);
}

void RaceDetector::memoryAtomic(const Memory *memory, size_t address,
                                size_t size)
{
  State *state = m_state[KEY(memory,address)].first + EXTRACT_OFFSET(address);

  // Get work-item
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  size_t workItemIndex = workItem->getGlobalIndex();

  for (size_t offset = 0; offset < size; offset++, state++)
  {
    // Check for races with non-atomic operations
    if (!state->canAtomic && workItemIndex != state->workItem)
    {
      logRace(ReadWriteRace,
              memory->getAddressSpace(),
              address,
              state->workItem,
              state->workGroup,
              state->instruction);
    }

    // Update state
    state->canRead = false;
    state->canWrite = false;
    if (!state->wasWorkItem)
    {
      state->instruction = workItem->getCurrentInstruction();
      state->workItem = workItemIndex;
      state->wasWorkItem = true;
    }
  }
}

void RaceDetector::memoryDeallocated(const Memory *memory, size_t address)
{
  if (memory->getAddressSpace() == AddrSpacePrivate)
    return;

  delete[] m_state[KEY(memory,address)].first;
  m_state.erase(KEY(memory,address));
}

void RaceDetector::memoryLoad(const Memory *memory, size_t address, size_t size)
{
  registerLoadStore(memory, address, size, NULL);
}

void RaceDetector::memoryStore(const Memory *memory, size_t address,
                               size_t size, const uint8_t *storeData)
{
  registerLoadStore(memory, address, size, storeData);
}

void RaceDetector::logRace(DataRaceType type,
                           unsigned int addrSpace,
                           size_t address,
                           size_t lastWorkGroup,
                           size_t lastWorkItem,
                           const llvm::Instruction *lastInstruction) const
{
  const char *memType = NULL;
  switch (addrSpace)
  {
  case AddrSpacePrivate:
    memType = "private";
    break;
  case AddrSpaceGlobal:
    memType = "global";
    break;
  case AddrSpaceConstant:
    memType = "constant";
    break;
  case AddrSpaceLocal:
    memType = "local";
    break;
  default:
    assert(false && "Memory error in unsupported address space.");
    break;
  }

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
      << memType << " memory address 0x" << hex << address << endl
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

void RaceDetector::registerLoadStore(const Memory *memory, size_t address,
                                     size_t size, const uint8_t *storeData)
{
  if (!m_kernelInvocation)
    return;
  if (memory->getAddressSpace() == AddrSpacePrivate)
    return;

  bool load = !storeData;
  bool store = storeData;

  // Get index of work-item and work-group performing access
  size_t workItemIndex = -1, workGroupIndex = -1;
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  const WorkGroup *workGroup = m_kernelInvocation->getCurrentWorkGroup();
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
    bool conflict = store ? !state->canWrite : !state->canRead;
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
      DataRaceType type = load|state->canRead ? ReadWriteRace : WriteWriteRace;
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
      bool updateWI = store || (load && state->canWrite);

      // Update state
      state->canAtomic = false;
      state->canRead &= load;
      state->canWrite = false;
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
      state->canAtomic = true; // TODO: atomic_intergroup_race test failure
      state->workItem = -1;
      state->wasWorkItem = false;
      if (!workGroup)
      {
        state->workGroup = -1;
        state->canRead = true;
        state->canWrite = true;
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
  canAtomic = true;
  canRead = true;
  canWrite = true;
  wasWorkItem = false;
}
