// Memory.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace spirsim
{
  class Device;

  typedef struct
  {
    size_t address;
    cl_image_format format;
    cl_image_desc desc;
  } Image;

  class Memory
  {
  public:
    Memory(unsigned int addrSpace, Device *device);
    virtual ~Memory();

    size_t allocateBuffer(size_t size);
    uint32_t atomicAdd(size_t address, uint32_t value);
    uint32_t atomicAnd(size_t address, uint32_t value);
    uint32_t atomicCmpxchg(size_t address, uint32_t cmp, uint32_t value);
    uint32_t atomicDec(size_t address);
    uint32_t atomicInc(size_t address);
    uint32_t atomicMax(size_t address, uint32_t value);
    uint32_t atomicMin(size_t address, uint32_t value);
    uint32_t atomicOr(size_t address, uint32_t value);
    uint32_t atomicSub(size_t address, uint32_t value);
    uint32_t atomicXchg(size_t address, uint32_t value);
    uint32_t atomicXor(size_t address, uint32_t value);
    void clear();
    Memory *clone() const;
    size_t createHostBuffer(size_t size, void *ptr);
    bool copy(size_t dest, size_t src, size_t size);
    void deallocateBuffer(size_t address);
    void dump() const;
    void* getPointer(size_t address) const;
    size_t getTotalAllocated() const;
    bool isAddressValid(size_t address, size_t size=1) const;
    bool load(unsigned char *dest, size_t address, size_t size=1) const;
    unsigned char* mapBuffer(size_t address, size_t offset, size_t size);
    void setDevice(Device *device);
    bool store(const unsigned char *source, size_t address, size_t size=1);
    void synchronize(bool workGroup = false);

    static size_t getMaxAllocSize();

  private:
    struct Status
    {
      const llvm::Instruction *instruction;
      size_t workItem;
      size_t workGroup;
      bool canAtomic;
      bool canRead;
      bool canWrite;
      bool wasWorkItem;

      Status();
    };

    typedef struct
    {
      bool hostPtr;
      size_t size;
      unsigned char *data;
      Status *status;
    } Buffer;

    Device *m_device;
    std::queue<int> m_freeBuffers;
    std::vector<Buffer> m_memory;
    unsigned int m_addressSpace;
    size_t m_totalAllocated;
    bool m_checkDataRaces;

    uint32_t* atomic(size_t address);
    int getNextBuffer();
    void registerAccess(size_t address, size_t size,
                        const uint8_t *data = NULL) const;
  };
}
