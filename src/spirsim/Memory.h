// Memory.h (Oclgrind)
// Copyright (c) 2013-2014, University of Bristol
// All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace spirsim
{
  typedef struct
  {
    size_t address;
    cl_image_format format;
    cl_image_desc desc;
  } Image;

  class Memory
  {
  public:
    Memory();
    virtual ~Memory();

    size_t allocateBuffer(size_t size);
    void clear();
    Memory *clone() const;
    size_t createHostBuffer(size_t size, void *ptr);
    bool copy(size_t dest, size_t src, size_t size);
    void deallocateBuffer(size_t address);
    void dump() const;
    void* getPointer(size_t address) const;
    size_t getTotalAllocated() const;
    bool load(unsigned char *dest, size_t address, size_t size=1) const;
    unsigned char* mapBuffer(size_t address, size_t offset, size_t size);
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
