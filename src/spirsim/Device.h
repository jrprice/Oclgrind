#include "common.h"

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
  void run(const Kernel& kernel,
           const size_t ndrange[3], const size_t wgsize[3]);
  void setOutputMask(unsigned char mask);

private:
  Memory *m_globalMemory;
  unsigned char m_outputMask;
};
