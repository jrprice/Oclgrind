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

THREAD_LOCAL RaceDetector::LocalState RaceDetector::m_localAccesses = {NULL};

// Use a bank of mutexes to reduce unnecessary synchronisation
#define NUM_GLOBAL_MUTEXES 4096 // Must be power of two
#define GLOBAL_MUTEX(buffer,offset) \
  m_globalMutexes[buffer][offset & (NUM_GLOBAL_MUTEXES-1)]

RaceDetector::RaceDetector(const Context *context)
 : Plugin(context)
{
  m_kernelInvocation = NULL;

  m_allowUniformWrites = !checkEnv("OCLGRIND_UNIFORM_WRITES");
}

void RaceDetector::kernelBegin(const KernelInvocation *kernelInvocation)
{
  m_kernelInvocation = kernelInvocation;
}

void RaceDetector::kernelEnd(const KernelInvocation *kernelInvocation)
{
  // Clear all global memory accesses
  AccessMap& accessMap = m_globalAccesses;
  for (auto addr = accessMap.begin(); addr != accessMap.end(); addr++)
    for (auto al = addr->second.begin(); al != addr->second.end(); al++)
      al->clear();

  m_kernelInvocation = NULL;
}

void RaceDetector::memoryAllocated(const Memory *memory, size_t address,
                                   size_t size, cl_mem_flags flags,
                                   const uint8_t *initData)
{
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    m_globalAccesses[EXTRACT_BUFFER(address)].resize(size);
    m_globalMutexes[EXTRACT_BUFFER(address)] = new mutex[NUM_GLOBAL_MUTEXES];
  }
  else if (memory->getAddressSpace() == AddrSpaceLocal)
  {
    if (!m_localAccesses.map)
      m_localAccesses.map = new map<const Memory*,AccessMap>;
    (*m_localAccesses.map)[memory][EXTRACT_BUFFER(address)].resize(size);
  }
}

void RaceDetector::memoryAtomicLoad(const Memory *memory,
                                    const WorkItem *workItem,
                                    AtomicOp op, size_t address, size_t size)
{
  registerAccess(memory, workItem->getWorkGroup(), workItem,
                 address, size, true);
}

void RaceDetector::memoryAtomicStore(const Memory *memory,
                                     const WorkItem *workItem,
                                     AtomicOp op, size_t address, size_t size)
{
  registerAccess(memory, workItem->getWorkGroup(), workItem,
                 address, size, true,
                 (const uint8_t*)memory->getPointer(address));
}

void RaceDetector::memoryDeallocated(const Memory *memory, size_t address)
{
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    m_globalAccesses.erase(EXTRACT_BUFFER(address));

    delete[] m_globalMutexes[EXTRACT_BUFFER(address)];
    m_globalMutexes.erase(EXTRACT_BUFFER(address));
  }
  else if (memory->getAddressSpace() == AddrSpaceLocal)
  {
    m_localAccesses.map->at(memory).erase(EXTRACT_BUFFER(address));
    if (!m_localAccesses.map->at(memory).size())
      m_localAccesses.map->erase(memory);
  }
}

void RaceDetector::memoryLoad(const Memory *memory, const WorkItem *workItem,
                              size_t address, size_t size)
{
  registerAccess(memory, workItem->getWorkGroup(), workItem,
                 address, size, false, NULL);
}

void RaceDetector::memoryLoad(const Memory *memory, const WorkGroup *workGroup,
                              size_t address, size_t size)
{
  registerAccess(memory, workGroup, NULL, address, size, false);
}

void RaceDetector::memoryStore(const Memory *memory, const WorkItem *workItem,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  registerAccess(memory, workItem->getWorkGroup(), workItem,
                 address, size, false, storeData);
}

void RaceDetector::memoryStore(const Memory *memory, const WorkGroup *workGroup,
                               size_t address, size_t size,
                               const uint8_t *storeData)
{
  registerAccess(memory, workGroup, NULL,
                 address, size, false, storeData);
}

Size3 RaceDetector::getAccessWorkGroup(const MemoryAccess& access) const
{
  Size3 wg = access.getEntity();
  if (access.isWorkItem())
  {
    Size3 wgsize = m_kernelInvocation->getLocalSize();
    wg = Size3(wg.x/wgsize.x, wg.y/wgsize.y, wg.z/wgsize.z);
  }
  return wg;
}

bool RaceDetector::check(const MemoryAccess& a,
                         const MemoryAccess& b) const
{
  // No race if same work-item
  if (a.isWorkItem() && b.isWorkItem() && (a.getEntity() == b.getEntity()))
    return false;

  // No race if both operations are atomics
  if (a.isAtomic() && b.isAtomic())
    return false;

  // Potential race if at least one store
  if (a.isStore() || b.isStore())
  {
    // Read-write race if one is a load
    if (a.isLoad() || b.isLoad())
      return true;

    // Write-write race if not uniform
    if (!m_allowUniformWrites || (a.getStoreData() != b.getStoreData()))
      return true;
  }

  return false;
}

void RaceDetector::logRace(const Memory *memory, size_t address,
                           const MemoryAccess& first,
                           const MemoryAccess& second) const
{
  const char *raceType;
  if (first.isLoad() || second.isLoad())
    raceType = "Read-write";
  else
    raceType = "Write-write";

  Context::Message msg(ERROR, m_context);
  msg << raceType << " data race at "
      << getAddressSpaceName(memory->getAddressSpace())
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << endl
      << "First entity:  ";

  if (first.isWorkItem())
  {
    Size3 wgsize = m_kernelInvocation->getLocalSize();
    Size3 global = first.getEntity();
    Size3 local(global.x%wgsize.x, global.y%wgsize.y, global.z%wgsize.z);
    Size3 group(global.x/wgsize.x, global.y/wgsize.y, global.z/wgsize.z);
    msg << "Global" << global << " Local" << local << " Group" << group;
  }
  else
  {
    msg << "Group" << first.getEntity();
  }

  msg << endl << first.getInstruction() << endl
      << endl
      << "Second entity: ";

  // Show details of other entity involved in race
  if (second.isWorkItem())
  {
    Size3 wgsize = m_kernelInvocation->getLocalSize();
    Size3 global = second.getEntity();
    Size3 local(global.x%wgsize.x, global.y%wgsize.y, global.z%wgsize.z);
    Size3 group(global.x/wgsize.x, global.y/wgsize.y, global.z/wgsize.z);
    msg << "Global" << global << " Local" << local << " Group" << group;
  }
  else
  {
    msg << "Group" << second.getEntity();
  }
  msg << endl << second.getInstruction() << endl;
  msg.send();
}

void RaceDetector::registerAccess(const Memory *memory,
                                  const WorkGroup *workGroup,
                                  const WorkItem *workItem,
                                  size_t address, size_t size, bool atomic,
                                  const uint8_t *storeData)
{
  unsigned addrSpace = memory->getAddressSpace();
  if (addrSpace == AddrSpacePrivate ||
      addrSpace == AddrSpaceConstant)
    return;
  if (!memory->isAddressValid(address, size))
    return;

  // Construct access
  MemoryAccess access(workGroup, workItem, storeData != NULL, atomic);

  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);

  AccessList *accessList = (addrSpace == AddrSpaceGlobal) ?
    m_globalAccesses[buffer].data() :
    m_localAccesses.map->at(memory)[buffer].data();
  accessList += offset;

  bool race = false;
  for (unsigned i = 0; i < size; i++, accessList++)
  {
    if (storeData)
      access.setStoreData(storeData[i]);

    if (addrSpace == AddrSpaceGlobal)
      GLOBAL_MUTEX(buffer,offset+i).lock();

    for (auto a = accessList->begin(); a != accessList->end(); a++)
    {
      if (addrSpace == AddrSpaceGlobal)
      {
        // Check if access was from same work-group and before a fence
        if (a->hasWorkGroupSync() &&
            getAccessWorkGroup(*a) == workGroup->getGroupID())
          continue;
      }


      if (!race)
      {
        if (check(access, *a))
        {
          logRace(memory, address+i, access, *a);
          race = true;
          break;
        }
      }
    }

    accessList->push_back(access);

    if (addrSpace == AddrSpaceGlobal)
      GLOBAL_MUTEX(buffer,offset+i).unlock();
  }
}

void RaceDetector::workGroupBarrier(const WorkGroup *workGroup, uint32_t flags)
{
  if (flags & CLK_LOCAL_MEM_FENCE && m_localAccesses.map)
  {
    // Clear all local memory accesses
    AccessMap& accessMap = m_localAccesses.map->at(workGroup->getLocalMemory());
    for (auto addr = accessMap.begin(); addr != accessMap.end(); addr++)
      for (auto al = addr->second.begin(); al != addr->second.end(); al++)
        al->clear();
  }
  if (flags & CLK_GLOBAL_MEM_FENCE)
  {
    // Set sync bits for all accesses from this work-group
    AccessMap& accessMap = m_globalAccesses;
    for (auto addr = accessMap.begin(); addr != accessMap.end(); addr++)
    {
      //for (auto al = addr->second.begin(); al != addr->second.end(); al++)
      for (unsigned i = 0; i < addr->second.size(); i++)
      {
        //for (auto a = al->begin(); a != al->end(); a++)
        lock_guard<mutex> lock(GLOBAL_MUTEX(addr->first,i));
        for (auto a = addr->second[i].begin(); a != addr->second[i].end(); a++)
          if (getAccessWorkGroup(*a) == workGroup->getGroupID())
            a->setWorkGroupSync();
      }
    }
  }
}

RaceDetector::MemoryAccess::MemoryAccess(const WorkGroup *workGroup,
                                         const WorkItem *workItem,
                                         bool store, bool atomic)
{
  this->info = 0;

  this->info |= store << STORE_BIT;
  this->info |= atomic << ATOMIC_BIT;

  if (workItem)
  {
    this->entity = workItem->getGlobalID();
    this->instruction = workItem->getCurrentInstruction();
  }
  else
  {
    this->info |= (1<<WG_BIT);
    this->entity = workGroup->getGroupID();
    this->instruction = NULL; // TODO?
  }
}

bool RaceDetector::MemoryAccess::isAtomic() const
{
  return this->info & (1<<ATOMIC_BIT);
}

bool RaceDetector::MemoryAccess::isLoad() const
{
  return !isStore();
}

bool RaceDetector::MemoryAccess::isStore() const
{
  return this->info & (1<<STORE_BIT);
}

bool RaceDetector::MemoryAccess::isWorkGroup() const
{
  return this->info & (1<<WG_BIT);
}

bool RaceDetector::MemoryAccess::isWorkItem() const
{
  return !isWorkGroup();
}

bool RaceDetector::MemoryAccess::hasWorkGroupSync() const
{
  return this->info & (1<<WG_SYNC_BIT);
}

void RaceDetector::MemoryAccess::setWorkGroupSync()
{
  this->info |= (1<<WG_SYNC_BIT);
}

Size3 RaceDetector::MemoryAccess::getEntity() const
{
  return this->entity;
}

const llvm::Instruction* RaceDetector::MemoryAccess::getInstruction() const
{
  return this->instruction;
}

uint8_t RaceDetector::MemoryAccess::getStoreData() const
{
  return this->storeData;
}

void RaceDetector::MemoryAccess::setStoreData(uint8_t data)
{
  this->storeData = data;
}
