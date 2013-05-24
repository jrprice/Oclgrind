#include "config.h"

class GlobalMemory
{
public:
  GlobalMemory();

  size_t allocateBuffer(size_t size);
  void dump() const;

private:
  size_t m_allocated;
  std::vector<unsigned char> m_memory;
};
