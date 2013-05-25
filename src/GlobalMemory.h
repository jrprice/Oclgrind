#include "common.h"

class GlobalMemory
{
public:
  GlobalMemory();

  size_t allocateBuffer(size_t size);
  void clear() {m_memory.clear(); m_allocated = 0;};
  void dump() const;
  bool load(size_t address, unsigned char *dest);
  bool load(size_t address, size_t size, unsigned char *dest);
  bool store(size_t address, unsigned char source);
  bool store(size_t address, size_t size, unsigned char *source);

private:
  size_t m_allocated;
  std::vector<unsigned char> m_memory;
};
