#include "common.h"

class Memory
{
public:
  Memory();

  size_t allocateBuffer(size_t size);
  void clear();
  void dump() const;
  bool load(size_t address, unsigned char *dest) const;
  bool load(size_t address, size_t size, unsigned char *dest) const;
  bool store(size_t address, unsigned char source);
  bool store(size_t address, size_t size, unsigned char *source);

private:
  size_t m_allocated;
  std::vector<unsigned char> m_memory;
};
