#include "common.h"
#include <cassert>
#include <cmath>

#include "Memory.h"

#define NUM_BUFFER_BITS 16
#define MAX_NUM_BUFFERS ((size_t)1 << NUM_BUFFER_BITS)
#define NUM_ADDRESS_BITS ((sizeof(size_t)<<3) - NUM_BUFFER_BITS)
#define MAX_BUFFER_SIZE ((size_t)1 << NUM_ADDRESS_BITS)

#define EXTRACT_BUFFER(address) \
  (address >> NUM_ADDRESS_BITS)
#define EXTRACT_OFFSET(address) \
  (address ^ (EXTRACT_BUFFER(address) << NUM_ADDRESS_BITS))

using namespace spirsim;
using namespace std;

Memory::Memory()
{
  m_totalAllocated = 0;

  // Reserve first buffer as a makeshift 'NULL'
  m_memory[0].data = new unsigned char[0];
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
    return NULL;
  }

  // Find first unallocated buffer slot
  int b = getNextBuffer();
  if (b < 0 || b >= MAX_NUM_BUFFERS)
  {
    return NULL;
  }

  // Create buffer
  Buffer buffer = {
    size,
    new unsigned char[size]
  };

  // Initialize contents to 0
  // TODO: Solution to catch use of uninitialised data
  memset(buffer.data, 0, size);

  m_memory[b] = buffer;
  m_totalAllocated += size;

  return ((size_t)b) << NUM_ADDRESS_BITS;
}

void Memory::clear()
{
  map<int,Buffer>::iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    delete[] itr->second.data;
  }
  m_memory.clear();
  m_freeBuffers = queue<int>();
  m_totalAllocated = 0;
}

Memory* Memory::clone() const
{
  Memory *mem = new Memory();

  // Clone buffers
  map<int,Buffer>::const_iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    Buffer src = itr->second;
    Buffer dest = {
      src.size,
      new unsigned char[src.size]
    };
    memcpy(dest.data, src.data, src.size);
    mem->m_memory[itr->first] = dest;
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
    return NULL;
  }

  // Find first unallocated buffer slot
  int b = getNextBuffer();
  if (b < 0 || b >= MAX_NUM_BUFFERS)
  {
    return NULL;
  }

  // Create buffer
  Buffer buffer = {
    size,
    (unsigned char*)ptr
  };

  m_memory[b] = buffer;
  m_totalAllocated += size;

  return ((size_t)b) << NUM_ADDRESS_BITS;
}

void Memory::deallocateBuffer(size_t address)
{
  int buffer = EXTRACT_BUFFER(address);
  assert(buffer < MAX_NUM_BUFFERS && m_memory.find(buffer) != m_memory.end());

  delete[] m_memory[buffer].data;
  m_totalAllocated -= m_memory[buffer].size;
  m_memory.erase(buffer);
  m_freeBuffers.push(buffer);
}

void Memory::dump() const
{
  map<int,Buffer>::const_iterator itr;
  for (itr = m_memory.begin(); itr != m_memory.end(); itr++)
  {
    for (int i = 0; i < itr->second.size; i++)
    {
      if (i%4 == 0)
      {
        cout << endl << hex << uppercase
             << setw(16) << setfill(' ') << right
             << ((((size_t)itr->first)<<NUM_ADDRESS_BITS) | i) << ":";
      }
      cout << " " << hex << uppercase << setw(2) << setfill('0')
           << (int)itr->second.data[i];
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

size_t Memory::getTotalAllocated() const
{
  return m_totalAllocated;
}

bool Memory::load(unsigned char *dest, size_t address, size_t size) const
{
  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);

  // Bounds check
  if (buffer >= MAX_NUM_BUFFERS ||
      m_memory.find(buffer) == m_memory.end() ||
      offset+size > m_memory.at(buffer).size)
  {
    return false;
  }

  // Load data
  memcpy(dest, m_memory.at(buffer).data + offset, size);

  return true;
}

bool Memory::store(const unsigned char *source, size_t address, size_t size)
{
  size_t buffer = EXTRACT_BUFFER(address);
  size_t offset = EXTRACT_OFFSET(address);

  // Bounds check
  if (buffer >= MAX_NUM_BUFFERS ||
      m_memory.find(buffer) == m_memory.end() ||
      offset+size > m_memory[buffer].size)
  {
    return false;
  }

  // Store data
  memcpy(m_memory[buffer].data + offset, source, size);

  return true;
}
