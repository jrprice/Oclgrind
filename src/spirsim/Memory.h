#include "common.h"

namespace spirsim
{
  class Memory
  {
  public:
    Memory();
    virtual ~Memory();

    size_t allocateBuffer(size_t size);
    void clear();
    Memory *clone() const;
    size_t createHostBuffer(size_t size, void *ptr);
    void deallocateBuffer(size_t address);
    void dump() const;
    size_t getTotalAllocated() const;
    bool load(unsigned char *dest, size_t address, size_t size=1) const;
    bool store(const unsigned char *source, size_t address, size_t size=1);

    static size_t getMaxAllocSize();

  private:
    typedef struct
    {
      bool hostPtr;
      size_t size;
      unsigned char *data;
    } Buffer;

    std::queue<int> m_freeBuffers;
    std::map<int,Buffer> m_memory;
    size_t m_totalAllocated;

    int getNextBuffer();
  };
}
