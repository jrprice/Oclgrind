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
#include "WorkItem.h"

#define NUM_BUFFER_BITS 16
#define MAX_NUM_BUFFERS ((size_t)1 << NUM_BUFFER_BITS)
#define NUM_ADDRESS_BITS ((sizeof(size_t)<<3) - NUM_BUFFER_BITS)
#define MAX_BUFFER_SIZE ((size_t)1 << NUM_ADDRESS_BITS)

#define EXTRACT_BUFFER(address) \
  (address >> NUM_ADDRESS_BITS)
#define EXTRACT_OFFSET(address) \
  (address ^ (EXTRACT_BUFFER(address) << NUM_ADDRESS_BITS))

#define ENV_DATA_RACES "OCLGRIND_DATA_RACES"

using namespace spirsim;
using namespace std;

Memory::Memory(unsigned int addrSpace, Device *device)
{
  m_device = device;
  m_addressSpace = addrSpace;

  m_checkDataRaces = false;
  if (addrSpace != AddrSpacePrivate)
  {
    const char *dataRaces = getenv(ENV_DATA_RACES);
    m_checkDataRaces = (dataRaces && strcmp(dataRaces, "1") == 0);
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
  size_t src_buffer = EXTRACT_BUFFER(src);
  size_t src_offset = EXTRACT_OFFSET(src);
  size_t dest_buffer = EXTRACT_BUFFER(dest);
  size_t dest_offset = EXTRACT_OFFSET(dest);

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

  // Copy data
  Buffer srcBuffer = m_memory.at(src_buffer);
  Buffer destBuffer = m_memory.at(dest_buffer);
  memcpy(destBuffer.data + dest_offset,
         srcBuffer.data + src_offset,
         size);

  registerRead(src, size);
  registerWrite(dest, size);

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
  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);

  // Bounds check
  if (!isAddressValid(address, size))
  {
    m_device->notifyMemoryError(true, m_addressSpace, address, size);
    return false;
  }

  // Load data
  Buffer src = m_memory[buffer];
  memcpy(dest, src.data + offset, size);
  registerRead(address, size);

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
  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);

  // Bounds check
  if (!isAddressValid(address, size))
  {
    m_device->notifyMemoryError(false, m_addressSpace, address, size);
    return false;
  }

  // Store data
  Buffer dest = m_memory[buffer];
  memcpy(dest.data + offset, source, size);
  registerWrite(address, size);

  return true;
}

void Memory::registerRead(size_t address, size_t size) const
{
  if (!m_checkDataRaces)
  {
    return;
  }

  size_t workItemIndex = -1;
  const WorkItem *workItem = m_device->getCurrentWorkItem();
  if (workItem)
  {
    workItemIndex = workItem->getGlobalIndex();
  }

  size_t base = EXTRACT_OFFSET(address);
  Buffer buffer = m_memory[EXTRACT_BUFFER(address)];
  for (size_t offset = 0; offset < size; offset++)
  {
    if (!buffer.status[base+offset].canRead &&
        buffer.status[base+offset].workItem != workItemIndex)
    {
      m_device->notifyDataRace(m_addressSpace, address + offset);
      break; // TODO: remove (granualirty)
    }
    buffer.status[base+offset].canWrite = false;
    buffer.status[base+offset].workItem = workItemIndex;
  }
}

void Memory::registerWrite(size_t address, size_t size) const
{
  if (!m_checkDataRaces)
  {
    return;
  }

  size_t workItemIndex = -1;
  const WorkItem *workItem = m_device->getCurrentWorkItem();
  if (workItem)
  {
    workItemIndex = workItem->getGlobalIndex();
  }

  size_t base = EXTRACT_OFFSET(address);
  Buffer buffer = m_memory[EXTRACT_BUFFER(address)];
  for (size_t offset = 0; offset < size; offset++)
  {
    if (!buffer.status[base+offset].canWrite &&
        buffer.status[base+offset].workItem != workItemIndex)
    {
      m_device->notifyDataRace(m_addressSpace, address + offset);
      break; // TODO: remove (granularity)
    }
    buffer.status[base+offset].canRead = false;
    buffer.status[base+offset].canWrite = false;
    buffer.status[base+offset].workItem = workItemIndex;
  }
}

void Memory::synchronize()
{
  if (!m_checkDataRaces)
  {
    return;
  }

  vector<Buffer>::iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    if (itr->data)
    {
      for (size_t i = 0; i < itr->size; i++)
      {
        itr->status[i].canRead = true;
        itr->status[i].canWrite = true;
        itr->status[i].workItem = -1;
      }
    }
  }
}

Memory::Status::Status()
{
  canRead = true;
  canWrite = true;
  workItem = -1;
}
