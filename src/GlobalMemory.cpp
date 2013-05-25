#include "common.h"

#include "GlobalMemory.h"

using namespace std;

GlobalMemory::GlobalMemory()
{
  m_allocated = 0;
}

size_t GlobalMemory::allocateBuffer(size_t size)
{
  size_t address = m_allocated;
  for (int i = 0; i < size; i++)
  {
    // TODO: Solution to catch use of uninitialised data
    m_memory.push_back(0);
  }
  m_allocated += size;
  return address;
}

void GlobalMemory::dump() const
{
  cout << endl << "Global Memory:";

  for (int i = 0; i < m_allocated; i++)
  {
    if (i%4 == 0)
    {
      cout << endl << hex << uppercase
           << setw(16) << setfill(' ') << right
           << i << ":";
    }
    cout << " " << hex << uppercase << setw(2) << setfill('0')
         << (int)m_memory[i];
  }
  cout << endl;
}


bool GlobalMemory::load(size_t address, unsigned char *dest)
{
  // Bounds check
  if (address > m_allocated)
  {
    return false;
  }

  // Load data
  *dest = m_memory[address];

  return true;
}


bool GlobalMemory::load(size_t address, size_t size, unsigned char *dest)
{
  // Bounds check
  if (address+size > m_allocated)
  {
    return false;
  }

  // Load data
  for (int i = 0; i < size; i++)
  {
    dest[i] = m_memory[address + i];
  }

  return true;
}

bool GlobalMemory::store(size_t address, unsigned char source)
{
  // Bounds check
  if (address > m_allocated)
  {
    return false;
  }

  // Store byte
  m_memory[address] = source;

  return true;
}

bool GlobalMemory::store(size_t address, size_t size, unsigned char *source)
{
  // Bounds check
  if (address+size > m_allocated)
  {
    return false;
  }

  // Store data
  for (int i = 0; i < size; i++)
  {
    m_memory[address + i] = source[i];
  }

  return true;
}
