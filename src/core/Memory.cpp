// Memory.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <cassert>
#include <cmath>
#include <cstring>

#include "Device.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

#define NUM_BUFFER_BITS 16
#define MAX_NUM_BUFFERS ((size_t)1 << NUM_BUFFER_BITS)
#define NUM_ADDRESS_BITS ((sizeof(size_t)<<3) - NUM_BUFFER_BITS)
#define MAX_BUFFER_SIZE ((size_t)1 << NUM_ADDRESS_BITS)

#define EXTRACT_BUFFER(address) \
  (address >> NUM_ADDRESS_BITS)
#define EXTRACT_OFFSET(address) \
  (address & (((size_t)-1) >> NUM_BUFFER_BITS))

#define ENV_DATA_RACES "OCLGRIND_DATA_RACES"
#define ENV_UNIFORM_WRITES "OCLGRIND_UNIFORM_WRITES"

using namespace oclgrind;
using namespace std;

Memory::Memory(unsigned int addrSpace, Device *device)
{
  m_device = device;
  m_addressSpace = addrSpace;

  m_checkDataRaces = false;
  m_allowUniformWrites = true;
  if (addrSpace != AddrSpacePrivate)
  {
    const char *dataRaces = getenv(ENV_DATA_RACES);
    m_checkDataRaces = (dataRaces && strcmp(dataRaces, "1") == 0);
    const char *uniformWrites = getenv(ENV_UNIFORM_WRITES);
    m_allowUniformWrites = !(uniformWrites && strcmp(uniformWrites, "1") == 0);
  }

  clear();
}

Memory::~Memory()
{
  clear();
}

size_t Memory::allocateBuffer(size_t size)
{
  // Check requested size doesn't exceed maximum
  if (size > MAX_BUFFER_SIZE)
  {
    return 0;
  }

  // Find first unallocated buffer slot
  int b = getNextBuffer();
  if (b < 0 || b >= MAX_NUM_BUFFERS)
  {
    return 0;
  }

  // Create buffer
  Buffer buffer = {
    false,
    size,
    new unsigned char[size],
    m_checkDataRaces ? new Status[size] : NULL
  };

  // Initialize contents to 0
  memset(buffer.data, 0, size);

  if (b >= m_memory.size())
  {
    m_memory.push_back(buffer);
  }
  else
  {
    m_memory[b] = buffer;
  }

  m_totalAllocated += size;

  return ((size_t)b) << NUM_ADDRESS_BITS;
}

uint32_t* Memory::atomic(size_t address)
{
  // Bounds check
  if (!isAddressValid(address, 4))
  {
    m_device->notifyMemoryError(true, m_addressSpace, address, 4);
    return NULL;
  }

  // Get buffer
  size_t offset = EXTRACT_OFFSET(address);
  Buffer buffer = m_memory[EXTRACT_BUFFER(address)];

  if (m_checkDataRaces)
  {
    Status *status = buffer.status + offset;

    // Get work-item
    size_t workItemIndex = status->workItem;
    const WorkItem *workItem = m_device->getCurrentWorkItem();
    if (workItem)
    {
      workItemIndex = workItem->getGlobalIndex();
    }

    // Check for races with non-atomic operations
    if (!status->canAtomic && workItemIndex != status->workItem)
    {
      m_device->notifyDataRace(ReadWriteRace, m_addressSpace, address,
                               status->workItem,
                               status->workGroup,
                               status->instruction);
    }

    // Update status
    status->canRead = false;
    status->canWrite = false;
    if (workItem)
    {
      if (!status->wasWorkItem)
      {
        status->instruction = workItem->getCurrentInstruction();
        status->workItem = workItemIndex;
        status->wasWorkItem = true;
      }
    }
  }

  return (uint32_t*)(buffer.data + offset);
}

uint32_t Memory::atomicAdd(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old + value;
  return old;
}

uint32_t Memory::atomicAnd(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old & value;
  return old;
}

uint32_t Memory::atomicCmpxchg(size_t address, uint32_t cmp, uint32_t value)
{
  // Bounds check
  if (!isAddressValid(address, 4))
  {
    m_device->notifyMemoryError(true, m_addressSpace, address, 4);
    return 0;
  }

  // Get buffer
  size_t offset = EXTRACT_OFFSET(address);
  Buffer buffer = m_memory[EXTRACT_BUFFER(address)];

  // Perform cmpxchg
  uint32_t *ptr = (uint32_t*)(buffer.data + offset);
  uint32_t old = *ptr;
  bool xchg = (old == cmp);
  *ptr = xchg ? value : old;

  // TODO: Check for data-races

  return old;
}

uint32_t Memory::atomicDec(size_t address)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old - 1;
  return old;
}

uint32_t Memory::atomicInc(size_t address)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old + 1;
  return old;
}

uint32_t Memory::atomicMax(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old > value ? old : value;
  return old;
}

uint32_t Memory::atomicMin(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old < value ? old : value;
  return old;
}

uint32_t Memory::atomicOr(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old | value;
  return old;
}

uint32_t Memory::atomicSub(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old - value;
  return old;
}

uint32_t Memory::atomicXchg(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = value;
  return old;
}

uint32_t Memory::atomicXor(size_t address, uint32_t value)
{
  uint32_t *ptr = atomic(address);
  if (!ptr)
  {
    return 0;
  }

  uint32_t old = *ptr;
  *ptr = old ^ value;
  return old;
}

void Memory::clear()
{
  vector<Buffer>::iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    if (itr->data)
    {
      if (!itr->hostPtr)
      {
        delete[] itr->data;
      }
      if (itr->status)
      {
        delete[] itr->status;
      }
    }
  }
  m_memory.resize(1);
  m_freeBuffers = queue<int>();
  m_totalAllocated = 0;
}

Memory* Memory::clone() const
{
  Memory *mem = new Memory(m_addressSpace, m_device);

  // Clone buffers
  mem->m_memory.resize(m_memory.size());
  for (int i = 1; i < m_memory.size(); i++)
  {
    Buffer src = m_memory[i];
    Buffer dest = {
      src.hostPtr,
      src.size,
      src.hostPtr ? src.data : new unsigned char[src.size],
      m_checkDataRaces ? new Status[src.size] : NULL
    };
    memcpy(dest.data, src.data, src.size);
    mem->m_memory[i] = dest;
  }

  // Clone state
  mem->m_freeBuffers = m_freeBuffers;
  mem->m_totalAllocated = m_totalAllocated;

  return mem;
}

size_t Memory::createHostBuffer(size_t size, void *ptr)
{
  // Check requested size doesn't exceed maximum
  if (size > MAX_BUFFER_SIZE)
  {
    return 0;
  }

  // Find first unallocated buffer slot
  int b = getNextBuffer();
  if (b < 0 || b >= MAX_NUM_BUFFERS)
  {
    return 0;
  }

  // Create buffer
  Buffer buffer = {
    true,
    size,
    (unsigned char*)ptr,
    m_checkDataRaces ? new Status[size] : NULL
  };

  if (b >= m_memory.size())
  {
    m_memory.push_back(buffer);
  }
  else
  {
    m_memory[b] = buffer;
  }

  m_totalAllocated += size;

  return ((size_t)b) << NUM_ADDRESS_BITS;
}

bool Memory::copy(size_t dest, size_t src, size_t size)
{
  // Bounds checks
  if (!isAddressValid(src, size))
  {
    m_device->notifyMemoryError(true, m_addressSpace, src, size);
    return false;
  }
  if (!isAddressValid(dest, size))
  {
    m_device->notifyMemoryError(false, m_addressSpace, dest, size);
    return false;
  }

  // Get buffers and register accesses
  size_t src_offset = EXTRACT_OFFSET(src);
  size_t dest_offset = EXTRACT_OFFSET(dest);
  Buffer src_buffer = m_memory.at(EXTRACT_BUFFER(src));
  Buffer dest_buffer = m_memory.at(EXTRACT_BUFFER(dest));
  registerAccess(src, size);
  registerAccess(dest, size, src_buffer.data + src_offset);

  // Copy data
  memcpy(dest_buffer.data + dest_offset,
         src_buffer.data + src_offset,
         size);

  return true;
}

void Memory::deallocateBuffer(size_t address)
{
  int buffer = EXTRACT_BUFFER(address);
  assert(buffer < m_memory.size() && m_memory[buffer].data);

  if (!m_memory[buffer].hostPtr)
  {
    delete[] m_memory[buffer].data;
  }
  if (m_memory[buffer].status)
  {
    delete[] m_memory[buffer].status;
  }

  m_memory[buffer].data = NULL;
  m_memory[buffer].status = NULL;
  m_totalAllocated -= m_memory[buffer].size;
  m_freeBuffers.push(buffer);
}

void Memory::dump() const
{
  for (int b = 0; b < m_memory.size(); b++)
  {
    if (!m_memory[b].data)
    {
      continue;
    }

    for (int i = 0; i < m_memory[b].size; i++)
    {
      if (i%4 == 0)
      {
        cout << endl << hex << uppercase
             << setw(16) << setfill(' ') << right
             << ((((size_t)b)<<NUM_ADDRESS_BITS) | i) << ":";
      }
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)m_memory[b].data[i];
    }
  }
  cout << endl;
}

size_t Memory::getMaxAllocSize()
{
  return MAX_BUFFER_SIZE;
}

int Memory::getNextBuffer()
{
  if (m_freeBuffers.empty())
  {
    return m_memory.size();
  }
  else
  {
    int b = m_freeBuffers.front();
    m_freeBuffers.pop();
    return b;
  }
}

void* Memory::getPointer(size_t address) const
{
  size_t buffer = EXTRACT_BUFFER(address);

  // Bounds check
  if (!isAddressValid(address))
  {
    return NULL;
  }

  return m_memory.at(buffer).data;
}

size_t Memory::getTotalAllocated() const
{
  return m_totalAllocated;
}

bool Memory::isAddressValid(size_t address, size_t size) const
{
  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);
  if (buffer == 0 ||
      buffer >= m_memory.size() ||
      !m_memory[buffer].data ||
      offset+size > m_memory.at(buffer).size)
  {
    return false;
  }
  return true;
}

bool Memory::load(unsigned char *dest, size_t address, size_t size) const
{
  // Bounds check
  if (!isAddressValid(address, size))
  {
    m_device->notifyMemoryError(true, m_addressSpace, address, size);
    return false;
  }

  // Get buffer and register access
  size_t offset = EXTRACT_OFFSET(address);
  Buffer src = m_memory[EXTRACT_BUFFER(address)];
  registerAccess(address, size);

  // Load data
  memcpy(dest, src.data + offset, size);

  return true;
}

unsigned char* Memory::mapBuffer(size_t address, size_t offset, size_t size)
{
  size_t buffer = EXTRACT_BUFFER(address);

  // Bounds check
  if (!isAddressValid(address, size))
  {
    return NULL;
  }

  return m_memory[buffer].data + offset + EXTRACT_OFFSET(address);
}

void Memory::setDevice(Device *device)
{
  m_device = device;
}

bool Memory::store(const unsigned char *source, size_t address, size_t size)
{
  // Bounds check
  if (!isAddressValid(address, size))
  {
    m_device->notifyMemoryError(false, m_addressSpace, address, size);
    return false;
  }

  // Get buffer and register access
  size_t offset = EXTRACT_OFFSET(address);
  Buffer dest = m_memory[EXTRACT_BUFFER(address)];
  registerAccess(address, size, source);

  // Store data
  memcpy(dest.data + offset, source, size);

  return true;
}

void Memory::registerAccess(size_t address, size_t size,
                            const uint8_t *data) const
{
  if (!m_checkDataRaces)
  {
    return;
  }

  bool load = !data;
  bool store = data;

  // Get index of work-item and work-group performing access
  size_t workItemIndex = -1, workGroupIndex = -1;
  const WorkItem *workItem = m_device->getCurrentWorkItem();;
  const WorkGroup *workGroup = m_device->getCurrentWorkGroup();
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
  Buffer buffer = m_memory[EXTRACT_BUFFER(address)];
  Status *status = buffer.status + base;
  for (size_t offset = 0; offset < size; offset++, status++)
  {
    bool conflict = store ? !status->canWrite : !status->canRead;
    if (m_allowUniformWrites && data)
    {
      conflict &= (buffer.data[base + offset] != data[offset]);
    }

    if (!race && conflict &&
        (status->wasWorkItem ?                // If status set by work-item,
         status->workItem != workItemIndex :  // must be same work-item,
         status->workGroup != workGroupIndex) // otherwise must be same group
        )
    {
      // Report data-race
      DataRaceType type = load|status->canRead ? ReadWriteRace : WriteWriteRace;
      m_device->notifyDataRace(type, m_addressSpace, address + offset,
                               status->workItem,
                               status->workGroup,
                               status->instruction);
      race = true;
    }
    else
    {
      // Only update WI info if this operation is stronger than previous one
      bool updateWI = store || (load && status->canWrite);

      // Update status
      status->canAtomic = false;
      status->canRead &= load;
      status->canWrite = false;
      if (updateWI)
      {
        status->workGroup = workGroupIndex;
        if (workItem)
        {
          status->instruction = workItem->getCurrentInstruction();
          status->workItem = workItemIndex;
          status->wasWorkItem = true;
        }
      }
    }
  }
}

void Memory::synchronize(bool workGroup)
{
  if (!m_checkDataRaces)
  {
    return;
  }

  vector<Buffer>::iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    if (!itr->status)
    {
      continue;
    }
    for (Status *status = itr->status; status < itr->status+itr->size; status++)
    {
      status->canAtomic = true; // TODO: atomic_intergroup_race test failure
      status->workItem = -1;
      status->wasWorkItem = false;
      if (!workGroup)
      {
        status->workGroup = -1;
        status->canRead = true;
        status->canWrite = true;
      }
    }
  }
}

Memory::Status::Status()
{
  instruction = NULL;
  workItem = -1;
  workGroup = -1;
  canAtomic = true;
  canRead = true;
  canWrite = true;
  wasWorkItem = false;
}
