#include "common.h"

#include "Memory.h"

using namespace std;

Memory::Memory()
{
  m_allocated = 0;
}

size_t Memory::allocateBuffer(size_t size)
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

void Memory::clear()
{
  m_memory.clear();
  m_allocated = 0;
}

void Memory::dump() const
{
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


bool Memory::load(size_t address, unsigned char *dest) const
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


bool Memory::load(size_t address, size_t size, unsigned char *dest) const
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

bool Memory::store(size_t address, unsigned char source)
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

bool Memory::store(size_t address, size_t size, unsigned char *source)
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
