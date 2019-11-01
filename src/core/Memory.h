// Memory.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace oclgrind
{
  class Context;

  class Memory
  {
  public:
    struct Buffer
    {
      size_t size;
      cl_mem_flags flags;
      unsigned char *data;
    };

  public:
    Memory(unsigned addrSpace, unsigned bufferBits, const Context *context);
    virtual ~Memory();

    size_t allocateBuffer(size_t size, cl_mem_flags flags=0,
                          const uint8_t *initData = NULL);
    template<typename T> T atomic(AtomicOp op, size_t address, T value = 0);
    template<typename T> T atomicCmpxchg(size_t address, T cmp, T value);
    void clear();
    size_t createHostBuffer(size_t size, void *ptr, cl_mem_flags flags=0);
    bool copy(size_t dest, size_t src, size_t size);
    void deallocateBuffer(size_t address);
    void dump() const;
    unsigned int getAddressSpace() const;
    const Buffer* getBuffer(size_t address) const;
    void* getPointer(size_t address) const;
    size_t getTotalAllocated() const;
    bool isAddressValid(size_t address, size_t size=1) const;
    bool load(unsigned char *dst, size_t address, size_t size=1) const;
    void* mapBuffer(size_t address, size_t offset, size_t size);
    bool store(const unsigned char *source, size_t address, size_t size=1);

    size_t extractBuffer(size_t address) const;
    size_t extractOffset(size_t address) const;

    size_t getMaxAllocSize();

  private:
    const Context *m_context;
    std::queue<unsigned> m_freeBuffers;
    std::vector<Buffer*> m_memory;
    unsigned int m_addressSpace;
    size_t m_totalAllocated;

    unsigned m_numBitsBuffer;
    unsigned m_numBitsAddress;
    size_t m_maxNumBuffers;
    size_t m_maxBufferSize;

    unsigned getNextBuffer();
  };
}
