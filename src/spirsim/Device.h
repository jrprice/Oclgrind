#include "common.h"

namespace spirsim
{
  class Kernel;
  class Memory;

  class Device
  {
  public:
    static const unsigned char OUTPUT_GLOBAL_MEM = 0x01;
    static const unsigned char OUTPUT_LOCAL_MEM = 0x02;
    static const unsigned char OUTPUT_PRIVATE_MEM = 0x04;
    static const unsigned char OUTPUT_INSTRUCTIONS = 0x08;

  public:
    Device();
    virtual ~Device();

    Memory *getGlobalMemory() const;
    void run(Kernel& kernel, unsigned int workDim,
             const size_t *globalOffset,
             const size_t *globalSize,
             const size_t *localSize);
    void setOutputMask(unsigned char mask);

  private:
    Memory *m_globalMemory;
    unsigned char m_outputMask;
  };
}
