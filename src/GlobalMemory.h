#include "config.h"

class GlobalMemory
{
public:
  GlobalMemory();

  size_t allocateBuffer(size_t size);
  void dump() const;
  unsigned char load(size_t address);
  void store(size_t address, unsigned char value);

private:
  size_t m_allocated;
  std::vector<unsigned char> m_memory;
};
