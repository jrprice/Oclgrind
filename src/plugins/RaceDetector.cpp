// RaceDetector.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
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

THREAD_LOCAL RaceDetector::WorkerState RaceDetector::m_state = {NULL};

#define STATE(workgroup) (m_state.groups->at(workgroup))

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
  // Log races
  for (auto race : kernelRaces)
    logRace(race);
  kernelRaces.clear();

  // Clear all global memory accesses
  for (auto &buffer : m_globalAccesses)
  {
    size_t sz = buffer.second.size();
    buffer.second.clear();
    buffer.second.resize(sz);
  }

  m_kernelInvocation = NULL;
}

void RaceDetector::memoryAllocated(const Memory *memory, size_t address,
                                   size_t size, cl_mem_flags flags,
                                   const uint8_t *initData)
{
  size_t buffer = memory->extractBuffer(address);
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    m_globalAccesses[buffer].resize(size);
    m_globalMutexes[buffer] = new mutex[NUM_GLOBAL_MUTEXES];
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
  size_t buffer = memory->extractBuffer(address);
  if (memory->getAddressSpace() == AddrSpaceGlobal)
  {
    m_globalAccesses.erase(buffer);

    delete[] m_globalMutexes.at(buffer);
    m_globalMutexes.erase(buffer);
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

void RaceDetector::workGroupBarrier(const WorkGroup *workGroup, uint32_t flags)
{
  if (flags & CLK_LOCAL_MEM_FENCE)
  {
    syncWorkItems(workGroup->getLocalMemory(),
                  STATE(workGroup), STATE(workGroup).wiLocal);
  }
  if (flags & CLK_GLOBAL_MEM_FENCE)
  {
    syncWorkItems(m_context->getGlobalMemory(),
                  STATE(workGroup), STATE(workGroup).wiGlobal);
  }
}

void RaceDetector::workGroupBegin(const WorkGroup *workGroup)
{
  // Create worker state if haven't already
  if (!m_state.groups)
  {
    m_state.groups = new unordered_map<const WorkGroup*,WorkGroupState>;
  }

  // Initialize work-group state
  WorkGroupState& state = (*m_state.groups)[workGroup];
  Size3 wgsize = workGroup->getGroupSize();
  state.numWorkItems = wgsize.x*wgsize.y*wgsize.z;

  // Re-use pool allocator for all access maps
  AccessMap tmp(0, AccessMap::hasher(), AccessMap::key_equal(),
                state.wgGlobal.get_allocator());
  state.wiGlobal.resize(state.numWorkItems+1, tmp);
  state.wiLocal.resize(state.numWorkItems+1, tmp);
}

void RaceDetector::workGroupComplete(const WorkGroup *workGroup)
{
  WorkGroupState& state = STATE(workGroup);

  syncWorkItems(workGroup->getLocalMemory(), state, state.wiLocal);
  syncWorkItems(m_context->getGlobalMemory(), state, state.wiGlobal);

  // Merge global accesses across kernel invocation
  size_t group = workGroup->getGroupIndex();
  for (auto &record : state.wgGlobal)
  {
    size_t address = record.first;
    size_t buffer = m_context->getGlobalMemory()->extractBuffer(address);
    size_t offset = m_context->getGlobalMemory()->extractOffset(address);

    lock_guard<mutex> lock(GLOBAL_MUTEX(buffer, offset));

    AccessRecord& a = record.second;
    AccessRecord& b = m_globalAccesses.at(buffer)[offset];

    // Check for races with previous accesses
    if (check(a.load,  b.store) && getAccessWorkGroup(b.store) != group)
      insertKernelRace({AddrSpaceGlobal, address, a.load, b.store});
    if (check(a.store, b.load) && getAccessWorkGroup(b.load) != group)
      insertKernelRace({AddrSpaceGlobal, address, a.store, b.load});
    if (check(a.store, b.store) && getAccessWorkGroup(b.store) != group)
      insertKernelRace({AddrSpaceGlobal, address, a.store, b.store});

    // Insert accesses
    if (a.load.isSet())
      insert(b, a.load);
    if (a.store.isSet())
      insert(b, a.store);
  }
  state.wgGlobal.clear();

  // Clean-up work-group state
  m_state.groups->erase(workGroup);
  if (m_state.groups->empty())
  {
    delete m_state.groups;
    m_state.groups = NULL;
  }
}

bool RaceDetector::check(const MemoryAccess& a,
                         const MemoryAccess& b) const
{
  // Ensure both accesses are valid
  if (!a.isSet() || !b.isSet())
    return false;

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

size_t RaceDetector::getAccessWorkGroup(const MemoryAccess& access) const
{
  if (access.isWorkItem())
  {
    const Size3& wgsize = m_kernelInvocation->getLocalSize();
    return access.getEntity() / (wgsize.x*wgsize.y*wgsize.z);
  }
  else
    return access.getEntity();
}

void RaceDetector::insert(AccessRecord& record,
                          const MemoryAccess& access) const
{
  if (access.isLoad())
  {
    if (!record.load.isSet() || record.load.isAtomic())
      record.load = access;
  }
  else if (access.isStore())
  {
    if (!record.store.isSet() || record.store.isAtomic())
      record.store = access;
  }
}

void RaceDetector::insertKernelRace(const Race& race)
{
  lock_guard<mutex> lock(kernelRacesMutex);
  insertRace(kernelRaces, race);
}

void RaceDetector::insertRace(RaceList& races, const Race& race) const
{
  // Check list for duplicates
  for (auto x = races.begin(); x != races.end(); x++)
  {
    // Check if races are equal modulo address
    if ((race.a == x->a && race.b == x->b) ||
        (race.a == x->b && race.b == x->a))
    {
      // If they match, keep the one with the lowest address
      if (race.address < x->address)
      {
        races.erase(x);
        races.push_back(race);
        return;
      }
      else
        return;
    }
  }

  races.push_back(race);
}

void RaceDetector::logRace(const Race& race) const
{
  const char *raceType;
  if (race.a.isLoad() || race.b.isLoad())
    raceType = "Read-write";
  else
    raceType = "Write-write";

  Context::Message msg(ERROR, m_context);
  msg << raceType << " data race at "
      << getAddressSpaceName(race.addrspace)
      << " memory address 0x" << hex << race.address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << endl
      << "First entity:  ";

  if (race.a.isWorkItem())
  {
    Size3 wgsize = m_kernelInvocation->getLocalSize();
    Size3 global(race.a.getEntity(), m_kernelInvocation->getGlobalSize());
    Size3 local(global.x%wgsize.x, global.y%wgsize.y, global.z%wgsize.z);
    Size3 group(global.x/wgsize.x, global.y/wgsize.y, global.z/wgsize.z);
    msg << "Global" << global << " Local" << local << " Group" << group;
  }
  else
  {
    msg << "Group"
        << Size3(race.a.getEntity(), m_kernelInvocation->getLocalSize());
  }

  msg << endl << race.a.getInstruction() << endl
      << endl
      << "Second entity: ";

  // Show details of other entity involved in race
  if (race.b.isWorkItem())
  {
    Size3 wgsize = m_kernelInvocation->getLocalSize();
    Size3 global(race.b.getEntity(), m_kernelInvocation->getGlobalSize());
    Size3 local(global.x%wgsize.x, global.y%wgsize.y, global.z%wgsize.z);
    Size3 group(global.x/wgsize.x, global.y/wgsize.y, global.z/wgsize.z);
    msg << "Global" << global << " Local" << local << " Group" << group;
  }
  else
  {
    msg << "Group"
        << Size3(race.b.getEntity(), m_kernelInvocation->getLocalSize());
  }
  msg << endl << race.b.getInstruction() << endl;
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

  size_t index;
  if (workItem)
  {
    Size3 wgsize = workGroup->getGroupSize();
    Size3 lid = workItem->getLocalID();
    index = lid.x + (lid.y + lid.z*wgsize.y)*wgsize.x;
  }
  else
  {
    index = STATE(workGroup).wiLocal.size() - 1;
  }

  AccessMap& accesses = (addrSpace == AddrSpaceGlobal) ?
    STATE(workGroup).wiGlobal[index] :
    STATE(workGroup).wiLocal[index];

  for (size_t i = 0; i < size; i++)
  {
    if (storeData)
      access.setStoreData(storeData[i]);

    insert(accesses[address+i], access);
  }
}

void RaceDetector::syncWorkItems(const Memory *memory,
                                 WorkGroupState& state,
                                 vector<AccessMap>& accesses)
{
  AccessMap wgAccesses(0, AccessMap::hasher(), AccessMap::key_equal(),
                       state.wgGlobal.get_allocator());

  for (size_t i = 0; i < state.numWorkItems + 1; i++)
  {
    RaceList races;
    for (auto &record : accesses[i])
    {
      size_t address = record.first;

      AccessRecord& a = record.second;
      AccessRecord& b = wgAccesses[address];

      if (check(a.load,  b.store))
        insertRace(races, {memory->getAddressSpace(),address,a.load,b.store});
      if (check(a.store, b.load))
        insertRace(races, {memory->getAddressSpace(),address,a.store,b.load});
      if (check(a.store, b.store))
        insertRace(races, {memory->getAddressSpace(),address,a.store,b.store});

      if (a.load.isSet())
      {
        insert(b, a.load);
        if (memory->getAddressSpace() == AddrSpaceGlobal)
          insert(state.wgGlobal[address], a.load);
      }
      if (a.store.isSet())
      {
        insert(b, a.store);
        if (memory->getAddressSpace() == AddrSpaceGlobal)
          insert(state.wgGlobal[address], a.store);
      }
    }

    accesses[i].clear();

    // Log races
    for (auto race : races)
      logRace(race);
  }
}

RaceDetector::MemoryAccess::MemoryAccess()
{
  this->info = 0;
  this->instruction = NULL;
}

RaceDetector::MemoryAccess::MemoryAccess(const WorkGroup *workGroup,
                                         const WorkItem *workItem,
                                         bool store, bool atomic)
{
  this->info = 0;

  this->info |= 1 << SET_BIT;
  this->info |= store << STORE_BIT;
  this->info |= atomic << ATOMIC_BIT;

  if (workItem)
  {
    this->entity = workItem->getGlobalIndex();
    this->instruction = workItem->getCurrentInstruction();
  }
  else
  {
    this->info |= (1<<WG_BIT);
    this->entity = workGroup->getGroupIndex();
    this->instruction = NULL; // TODO?
  }
}

void RaceDetector::MemoryAccess::clear()
{
  this->info = 0;
  this->instruction = NULL;
}

bool RaceDetector::MemoryAccess::isSet() const
{
  return this->info & (1<<SET_BIT);
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

size_t RaceDetector::MemoryAccess::getEntity() const
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

bool RaceDetector::MemoryAccess::operator==(
  const RaceDetector::MemoryAccess& other) const
{
  return this->entity == other.entity &&
         this->instruction == other.instruction &&
         this->info == other.info;
}
