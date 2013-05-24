#include "config.h"
#include <iomanip>
#include <iostream>
#include <vector>

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
      cout << endl << hex << uppercase << setw(16) << setfill(' ')
           << i << ":";
    }
    cout << " " << hex << uppercase << setw(2) << setfill('0')
         << (int)m_memory[i];
  }
  cout << endl;
}

unsigned char GlobalMemory::load(size_t address)
{
  // Bounds check
  // TODO: Improve reporting
  if (address >= m_allocated)
  {
    cout << "Memory access out of bounds" << endl;
    return 0;
  }

  return m_memory[address];
}

void GlobalMemory::store(size_t address, unsigned char value)
{
  // Bounds check
  // TODO: Improve reporting
  if (address >= m_allocated)
  {
    cout << "Memory access out of bounds" << endl;
    return;
  }

  m_memory[address] = value;
}
