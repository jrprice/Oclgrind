// Memory.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <cassert>
#include <cmath>
#include <cstring>

#include "Context.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

Memory::Memory(unsigned int addrSpace, const Context *context)
{
  m_context = context;
  m_addressSpace = addrSpace;

  clear();
}

Memory::~Memory()
{
  clear();
}

size_t Memory::allocateBuffer(size_t size, cl_mem_flags flags)
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
  Buffer *buffer = new Buffer;
  buffer->size   = size;
  buffer->flags  = flags;
  buffer->data   = new unsigned char[size];

  // Initialize contents to 0
  memset(buffer->data, 0, size);

  if (b >= m_memory.size())
  {
    m_memory.push_back(buffer);
  }
  else
  {
    m_memory[b] = buffer;
  }

  m_totalAllocated += size;

  size_t address = ((size_t)b) << NUM_ADDRESS_BITS;

  m_context->notifyMemoryAllocated(this, address, size);

  return address;
}

uint32_t Memory::atomic(AtomicOp op, size_t address, uint32_t value)
{
  // Bounds check
  if (!isAddressValid(address, 4))
  {
    logError(true, address, 4);
    return 0;
  }

  // Get buffer and register access
  size_t offset = EXTRACT_OFFSET(address);
  Buffer *buffer = m_memory[EXTRACT_BUFFER(address)];
  uint32_t *ptr = (uint32_t*)(buffer->data + offset);
  m_context->notifyMemoryAtomic(this, op, address, 4);

  uint32_t old = *ptr;
  switch(op)
  {
  case AtomicAdd:
    *ptr = old + value;
    break;
  case AtomicAnd:
    *ptr = old & value;
    break;
  case AtomicCmpXchg:
    FATAL_ERROR("AtomicCmpXchg in generic atomic handler");
    break;
  case AtomicDec:
    *ptr = old - 1;
    break;
  case AtomicInc:
    *ptr = old + 1;
    break;
  case AtomicMax:
    *ptr = old > value ? old : value;
    break;
  case AtomicMin:
    *ptr = old < value ? old : value;
    break;
  case AtomicOr:
    *ptr = old | value;
    break;
  case AtomicSub:
    *ptr = old - value;
    break;
  case AtomicXchg:
    *ptr = value;
    break;
  case AtomicXor:
    *ptr = old ^ value;
    break;
  }
  return old;
}

uint32_t Memory::atomicCmpxchg(size_t address, uint32_t cmp, uint32_t value)
{
  // Bounds check
  if (!isAddressValid(address, 4))
  {
    logError(true, address, 4);
    return 0;
  }

  // Get buffer
  size_t offset = EXTRACT_OFFSET(address);
  Buffer *buffer = m_memory[EXTRACT_BUFFER(address)];

  // Perform cmpxchg
  uint32_t *ptr = (uint32_t*)(buffer->data + offset);
  uint32_t old = *ptr;
  if (old == cmp)
  {
    *ptr = value;

    m_context->notifyMemoryAtomic(this, AtomicCmpXchg, address, 4);
  }
  else
  {
    // TODO: Register read-only atomic access?
  }

  return old;
}

void Memory::clear()
{
  vector<Buffer*>::iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    if (*itr)
    {
      if (!((*itr)->flags & CL_MEM_USE_HOST_PTR))
      {
        delete[] (*itr)->data;
      }
      delete *itr;

      size_t address = (itr-m_memory.begin())<<NUM_ADDRESS_BITS;
      m_context->notifyMemoryDeallocated(this, address);
    }
  }
  m_memory.resize(1);
  m_memory[0] = NULL;
  m_freeBuffers = queue<int>();
  m_totalAllocated = 0;
}

Memory* Memory::clone() const
{
  Memory *mem = new Memory(m_addressSpace, m_context);

  // Clone buffers
  mem->m_memory.resize(m_memory.size());
  mem->m_memory[0] = NULL;
  for (int i = 1; i < m_memory.size(); i++)
  {
    Buffer *src = m_memory[i];
    Buffer *dst = new Buffer;
    dst->size   = src->size;
    dst->flags  = src->flags,
    dst->data   =
      (src->flags&CL_MEM_USE_HOST_PTR) ?
        src->data : new unsigned char[src->size],
    memcpy(dst->data, src->data, src->size);
    mem->m_memory[i] = dst;
    m_context->notifyMemoryAllocated(mem, ((size_t)i<<NUM_ADDRESS_BITS),
                                     src->size);
  }

  // Clone state
  mem->m_freeBuffers = m_freeBuffers;
  mem->m_totalAllocated = m_totalAllocated;

  return mem;
}

size_t Memory::createHostBuffer(size_t size, void *ptr, cl_mem_flags flags)
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
  Buffer *buffer = new Buffer;
  buffer->size   = size;
  buffer->flags  = flags;
  buffer->data   = (unsigned char*)ptr;

  if (b >= m_memory.size())
  {
    m_memory.push_back(buffer);
  }
  else
  {
    m_memory[b] = buffer;
  }

  m_totalAllocated += size;

  size_t address = ((size_t)b) << NUM_ADDRESS_BITS;

  m_context->notifyMemoryAllocated(this, address, size);

  return address;
}

bool Memory::copy(size_t dst, size_t src, size_t size)
{
  // Bounds checks
  if (!isAddressValid(src, size))
  {
    logError(true, src, size);
    return false;
  }
  if (!isAddressValid(dst, size))
  {
    logError(false, dst, size);
    return false;
  }

  // Get buffers and register accesses
  size_t src_offset = EXTRACT_OFFSET(src);
  size_t dst_offset = EXTRACT_OFFSET(dst);
  Buffer *src_buffer = m_memory.at(EXTRACT_BUFFER(src));
  Buffer *dst_buffer = m_memory.at(EXTRACT_BUFFER(dst));
  m_context->notifyMemoryLoad(this, src, size);
  m_context->notifyMemoryStore(this, dst, size, src_buffer->data + src_offset);

  // Copy data
  memcpy(dst_buffer->data + dst_offset,
         src_buffer->data + src_offset,
         size);

  return true;
}

void Memory::deallocateBuffer(size_t address)
{
  int buffer = EXTRACT_BUFFER(address);
  assert(buffer < m_memory.size() && m_memory[buffer]);

  if (!(m_memory[buffer]->flags & CL_MEM_USE_HOST_PTR))
  {
    delete[] m_memory[buffer]->data;
  }

  m_totalAllocated -= m_memory[buffer]->size;
  m_freeBuffers.push(buffer);

  delete m_memory[buffer];
  m_memory[buffer] = NULL;

  m_context->notifyMemoryDeallocated(this, address);
}

void Memory::dump() const
{
  for (int b = 0; b < m_memory.size(); b++)
  {
    if (!m_memory[b]->data)
    {
      continue;
    }

    for (int i = 0; i < m_memory[b]->size; i++)
    {
      if (i%4 == 0)
      {
        cout << endl << hex << uppercase
             << setw(16) << setfill(' ') << right
             << ((((size_t)b)<<NUM_ADDRESS_BITS) | i) << ":";
      }
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)m_memory[b]->data[i];
    }
  }
  cout << endl;
}

unsigned int Memory::getAddressSpace() const
{
  return m_addressSpace;
}

const Memory::Buffer* Memory::getBuffer(size_t address) const
{
  size_t buf = EXTRACT_BUFFER(address);
  if (buf == 0 || buf >= m_memory.size() || !m_memory[buf]->data)
  {
    return NULL;
  }

  return m_memory[buf];
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

  return m_memory[buffer]->data + EXTRACT_OFFSET(address);
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
      !m_memory[buffer] ||
      offset+size > m_memory[buffer]->size)
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
    logError(true, address, size);
    return false;
  }

  // Get buffer and register access
  size_t offset = EXTRACT_OFFSET(address);
  Buffer *src = m_memory[EXTRACT_BUFFER(address)];
  m_context->notifyMemoryLoad(this, address, size);

  // Load data
  memcpy(dest, src->data + offset, size);

  return true;
}

void Memory::logError(bool read, size_t address, size_t size) const
{
  // Error info
  Context::Message msg(ERROR, m_context);
  msg << "Invalid " << (read ? "read" : "write")
      << " of size " << size
      << " at " << getAddressSpaceName(m_addressSpace)
      << " memory address 0x" << hex << address << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

unsigned char* Memory::mapBuffer(size_t address, size_t offset, size_t size)
{
  size_t buffer = EXTRACT_BUFFER(address);

  // Bounds check
  if (!isAddressValid(address, size))
  {
    return NULL;
  }

  return m_memory[buffer]->data + offset + EXTRACT_OFFSET(address);
}

bool Memory::store(const unsigned char *source, size_t address, size_t size)
{
  // Bounds check
  if (!isAddressValid(address, size))
  {
    logError(false, address, size);
    return false;
  }

  // Get buffer and register access
  size_t offset = EXTRACT_OFFSET(address);
  Buffer *dst = m_memory[EXTRACT_BUFFER(address)];
  m_context->notifyMemoryStore(this, address, size, source);

  // Store data
  memcpy(dst->data + offset, source, size);

  return true;
}
